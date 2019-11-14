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
	pager->FIFO_victim = page_sz - 2;
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

	// Set the flags in the page table entry; include the flag for allocation
	pager->page_tables[pid][p].flags = access | ALLOCATED;
}

// Update DIRTY and REFERENCE flags, and increment reference count
void update_flags_and_count(pager_data* pager, byte access, uint64 pid, uint64 page_number) {
	pager->memory_reference_count++;
	byte dirty_or_not = 0;
	if (access & WRITE) { dirty_or_not = DIRTY; }
	pager->page_tables[pid][page_number].flags = pager->page_tables[pid][page_number].flags | REFERENCED | dirty_or_not;
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
	// Check if not VALID (not memory resident)
	if (!(entry.flags & VALID)) {
		// If page table entry allocated, then increment both memory reference count and page fault total,
		// and return a page fault
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

	// Process has incompatible privileges
	if (!(entry.flags & access)) {
		printf("Process %lu attempted to", pid);

		// Print the attempted access
		if (access & READ) { printf(" read from "); }
		else if (access & WRITE) { printf(" write to "); }
		else if (access & EXECUTE) { printf(" execute "); }

		printf("page %lu but that page can only be ", page_number);

		int need_or = 0;
		// Print the actual access
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

		return INVALID_PAGE;
	}

	// Otherwise, the page is memory resident and allocated; update flags and reference count
	update_flags_and_count(pager, access, pid, page_number);
	return VALID_PAGE;
}

// Have page page_number of process pid claim the frame f. If the frame is not free, then its contents are
// evicted. This updates the frame and page table along with printing out status messages.
void claim_frame(pager_data* pager, uint64 pid, uint64 logical_addr, uint64 f)
{
	// Get page number
	uint64 page_number = logical_addr >> pager->page_sz;

	// If frame is occupied, evict the contents
	frame frame_f = pager->frames[f];
	if (pager->frames[f].occupied) {
		printf("Page %lu of process %lu is selected to be paged out of frame %lu\n",
				frame_f.page_number, frame_f.pid, f);
		page_table_entry* evicted_page = get_page_from_frame(pager, f);
		if (evicted_page->flags & DIRTY) {
			printf("It has been modified so it will be written to the swap space\n");
			pager->pf_written_frames++;
		}
		else {
			printf("It has not been modified so it will be discarded\n");
			pager->pf_discarded_frames++;
		}
		evicted_page->flags = evicted_page->flags ^ VALID;
	} else { pager->num_free_frames--; }

	printf("Page %lu of process %lu was paged into frame %lu\n", page_number, pid, f);

	// Update the contents of the frame and page table
	pager->frames[f].occupied = true;
	pager->frames[f].pid = pid;
	pager->frames[f].page_number = page_number;
	pager->page_tables[pid][page_number].frame = f;
	get_page_from_frame(pager, f)->flags = get_page_from_frame(pager, f)->flags | VALID;
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
