/////////////////////
// Pager Functions //
/////////////////////

#include "general.h"
#include "pager.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Initialize the pager with the given logical memory size (in number of pages), the physical
// memory size (in number of frames), the size of an individual page/frame (in bits), and the
// maximum number of processes on the system.
pager_data* pager_data_init(uint64 log_mem_sz, uint64 phy_mem_sz, uint64 page_sz, uint64 num_procs)
{
	pager_data* pager = malloc(sizeof(pager_data));
	if (!pager) { return NULL; }
	memset(pager, 0, sizeof(pager_data));
	
	// Basic settings
	pager->FIFO_victim = -1;
	pager->SC_head_frame = 0;
	pager->memory_reference_count = pager->pf_total = 0;
	pager->pf_discarded_frames = pager->pf_written_frames = 0;
	pager->num_pages = log_mem_sz;
	pager->num_frames = pager->num_free_frames = phy_mem_sz;
	pager->page_sz = page_sz;
	pager->num_procs = num_procs;
	
	// Allocate the frames array
	pager->frames = malloc(phy_mem_sz*sizeof(frame));
	if (!pager->frames) { pager_data_dealloc(pager); return NULL; }
	memset(pager->frames, 0, phy_mem_sz*sizeof(frame));
	
	// Allocate the page_tables array
	pager->page_tables = malloc(num_procs*sizeof(page_table_entry*));
	if (!pager->page_tables) { pager_data_dealloc(pager); return NULL; }
	memset(pager->page_tables, 0, num_procs*sizeof(page_table_entry*));
	
	// Allocate each page table (all entries initialized to all-0)
	for (int i = 0; i < pager->num_procs; ++i)
	{
		pager->page_tables[i] = malloc(log_mem_sz*sizeof(page_table_entry));
		if (!pager->page_tables[i]) { pager_data_dealloc(pager); return NULL; }
		memset(pager->page_tables[i], 0, log_mem_sz*sizeof(page_table_entry));
	}

    return pager;
}

// Deallocate any memory that was allocated for the pager (including the pager itself). After
// this is called the pager can no longer be used.
void pager_data_dealloc(pager_data* pager)
{
	if (pager)
	{
		// Free frames array
		free(pager->frames);
		
		if (pager->page_tables)
		{
			// Free each page table
			for (int i = 0; i < pager->num_procs; ++i)
			{
				free(pager->page_tables[i]);
			}
			
			// Free page_tables array
			free(pager->page_tables);
		}
		
		free(pager);
	}
}

// A request to allocate a page for a process is being made. The given PID is the process
// identifier, the page number is the page number being allocated for the process, and the
// access is the allowed access (a combination of READ, WRITE, and EXECUTE flags) for future
// memory reference requests. If the page is already allocated then its access flags are updated
// but nothing else is changed. This function does not bring a page into memory and does not print
// anything out.
void alloc_page(pager_data* pager, uint64 pid, uint64 p, byte access)
{
	// Argument checking
	if (pid >= pager->num_procs) { fprintf(stderr, "Invalid PID: %lu\n", pid); return; }
	if (p >= pager->num_pages) { fprintf(stderr, "Invalid page: %lu\n", p); return; }

	// Set the flags in the page table entry (including the flag for allocation)
	pager->page_tables[pid][p].flags = access | ALLOCATED;
}

// Helper function: Updates DIRTY and REFERENCE flags. Also increments the reference count.
void update_flags_and_count(pager_data* pager, byte access, uint64 pid, uint64 page_number) {
	pager->memory_reference_count++;
	pager->page_tables[pid][page_number].flags |= REFERENCED | ((access & WRITE) ? DIRTY : 0);
}

// Helper function: processes incompatible privileges
void print_incompatible_privileges(page_table_entry entry, uint64 pid, uint64 page_number, byte access) {
	printf("Process %lu attempted to", pid);

	// Print the attempted access
	if (access & READ) { printf(" read from "); }
	else if (access & WRITE) { printf(" write to "); }
	else if (access & EXECUTE) { printf(" execute "); }

	printf("page %lu but that page can only be ", page_number);

	// Print the actual access
	int need_or = 0;
	if (entry.flags & READ) { printf("read"); need_or++; }
	if (entry.flags & WRITE) {
		if (need_or++) { printf(" or "); }
		printf("written");
	}
	if (entry.flags & EXECUTE) { 
		if (need_or) { printf(" or "); }
		printf("executed");
	}
	printf("\n");
}

// This checks that the referenced page is a valid page for the given process and access request.
//
// If it is not valid then a descriptive message is printed out and INVALID_PAGE is returned.
// If it is valid then it updates the REFERENCED and possibly the DIRTY flag of the page. If memory
// resident then VALID_PAGE is returned, otherwise PAGE_FAULT is returned.
int check_log_addr(pager_data* pager, uint64 pid, uint64 logical_addr, byte access)
{	
	// Get the page table entry
	uint64 page_number = logical_addr >> pager->page_sz;
	page_table_entry entry = pager->page_tables[pid][page_number];

	// Process has incompatible privileges
	if (!(entry.flags & access)) {
		print_incompatible_privileges(entry, pid, page_number, access);
		return INVALID_PAGE;
	}

	// Check if not VALID (not memory resident)
	if (!(entry.flags & VALID)) {
		// If the page table entry is allocated, then increment both memory reference count and
		// page fault total. Finally, return a page fault.
		if (entry.flags & ALLOCATED)
		{
			pager->pf_total++;
			update_flags_and_count(pager, access, pid, page_number);
			return PAGE_FAULT;
		}

		// Attempted to access unallocated page
		printf("Process %lu attempted to access page %lu which has not been allocated\n", pid, page_number);
		return INVALID_PAGE;
	}

	// Otherwise, the page is memory resident and allocated.
	// Update flags and reference count. Return valid page.
	if (entry.flags & ALLOCATED) {
		update_flags_and_count(pager, access, pid, page_number);

		// The memory reference count increases during page faults and therefore will always
		// give a strict ordering to the frames for the LRU victim selection algorithm.
		uint64 f = pager->page_tables[pid][page_number].frame; // Frame number
		pager->frames[f].LRU_value = pager->memory_reference_count;
		return VALID_PAGE;
	}

	// This is the case where the page is memory resident, but not allocated.
	// This should never happen, but is included for completeness.
	return INVALID_PAGE;
}

// Have page page_number of process pid claim the frame f. If the frame is not free, then its contents are
// evicted. This updates the frame and page table along with printing out status messages.
void claim_frame(pager_data* pager, uint64 pid, uint64 logical_addr, uint64 f)
{
	// Get the page number and the frame being claimed.
	uint64 page_number = logical_addr >> pager->page_sz;
	frame* claimed_frame = &pager->frames[f];

	// If frame is occupied, evict the contents. Otherwise decrease the count of free frames.
	if (claimed_frame->occupied) {
		printf("Page %lu of process %lu ", claimed_frame->page_number, claimed_frame->pid);
		printf("is selected to be paged out of frame %lu\n", f);
		page_table_entry* evicted_page = get_page_from_frame(pager, f);
		if (evicted_page->flags & DIRTY) {
			printf("It has been modified so it will be written to the swap space\n");
			pager->pf_written_frames++;
		}
		else {
			printf("It has not been modified so it will be discarded\n");
			pager->pf_discarded_frames++;
		}
		evicted_page->flags &= ~(VALID | REFERENCED | DIRTY);
		
		// The memory reference count increases during page faults and therefore will always
		// give a strict ordering to the frames for the LRU victim selection algorithm.
		pager->frames[f].LRU_value = pager->memory_reference_count;
	} else { pager->num_free_frames--; }

	printf("Page %lu of process %lu was paged into frame %lu\n", page_number, pid, f);

	// Update the contents of the claimed frame and page table
	claimed_frame->occupied = true;
	claimed_frame->pid = pid;
	claimed_frame->page_number = page_number;
	pager->page_tables[pid][page_number].frame = f;
	get_page_from_frame(pager, f)->flags |= VALID;
}

// Prints out the summary information for the simulation run including a divider.
void print_summary(pager_data* pager)
{
	printf("----------------------------------------\n");
	printf("Page Fault Rate: %f\n", (double) pager->pf_total / (pager->memory_reference_count));
	printf("Total Page Faults: %lu\n", pager->pf_total);
	printf("Total Page Faults Evicting and Discarding a Frame: %lu\n", pager->pf_discarded_frames);
	printf("Total Page Faults Evicting and Writing a Frame: %lu\n", pager->pf_written_frames);
}
