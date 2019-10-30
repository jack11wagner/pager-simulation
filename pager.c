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
	if (pid >= pager->num_procs) { fprintf(stderr, "Invalid PID: %d\n", pid); return; }
	if (p >= pager->num_pages) { fprintf(stderr, "Invalid page: %d\n", p); return; }

	// TODO
}

// This checks that the referenced page is a valid page for the given pocess and access request.
//
// If it is not valid then a descriptive message is printed out and INVALID_PAGE is returned.
// If it is valid then it updates the REFERENCED and possibly the DIRTY flag of the page. If memory
// resident then VALID_PAGE is returned, otherwise PAGE_FAULT is returned.
int check_log_addr(pager_data* pager, uint64 pid, uint64 logical_addr, byte access)
{
	// TODO
	return INVALID_PAGE;
}

// Have page p of process pid claim the frame f. If the frame is not free than its contents are
// evicted. This updates the frame and page table along with printing out status messages.
void claim_frame(pager_data* pager, uint64 pid, uint64 logical_addr, uint64 f)
{
	// TODO
}

// Prints out the summary information for the simulation run including a divider.
void print_summary(pager_data* pager)
{
	// TODO
}
