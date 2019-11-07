////////////////////////////////////////////
// Pager System Functions and Definitions //
////////////////////////////////////////////

#ifndef _PAGERS_H_
#define _PAGERS_H_

#include "general.h"

#include <stdbool.h>

// Access bit-masks for reading, writing, and executing privileges
#define READ        0x01
#define WRITE       0x02
#define EXECUTE     0x04

// Additional flags used for pages (in addition READ, WRITE, and EXECUTE)
#define DIRTY   	0x10
#define VALID   	0x20
#define REFERENCED  0x40

// Constants returned by check_log_addr
#define INVALID_PAGE -1
#define VALID_PAGE	  0
#define PAGE_FAULT 	  1

// Constant for empty head/next_frame
#define EMPTY (uint64) -1

// Each frame needs to know which process/page is currently resident in it
// next_frame is the frame after the current frame in the FIFO queue
typedef struct _frame
{
	bool occupied; // false if free, true otherwise
	uint64 pid, page_number;
	// TODO: may need to add additional fields here for use with the paging algorithm(s)
} frame;

// Each page needs to have a set of flags (some combination of VALID, DIRTY, REFERENCED, READ,
// WRITE, and EXECUTE) along with which frame it is in (only if the VALID flag is set).
//
// This uses a special C struct called a bit field. This will be a 64-bit integer in memory but in
// your code you can access the fields and it will automatically bit-shift and mask the data.
typedef struct _page_table_entry
{
	uint64 flags  : 12; // Lowest 12 bits are for flags
	uint64 frame  : 40; // The next 40 bits are for the frame
	uint64 valid  : 1;  // The 53rd bit is for whether or not the page table is valid, i.e., whether or not the page table exists
	uint64 unused : 11; // TODO: last 11 bits are unused for now, you may use them for use with the paging algorithm(s)
} page_table_entry;

// Structure for common fields used by all pagers
typedef struct _pager_data
{
	// This is the basic information given to the init function that we need to keep around
	uint64 num_pages, num_frames, page_sz, num_procs;

	// Since we never deallocate frames we can just keep track of the number of free frames instead
	// of keeping a list of the free frames or a flag on each frame indicating if it is free.
	uint64 num_free_frames;
	frame* frames; // array to lookup pid/page number resident in each frame

	// The page tables, a 2D array of pages indexed by process number then by the page number.
	page_table_entry** page_tables;

	// Page fault statistics
	uint64 memory_reference_count, pf_total, pf_discarded_frames, pf_written_frames;

	// Next victim of FIFO queue
	uint64 FIFO_victim;

	// TODO: may need to add additional fields here for use with the paging algorithm(s)
} pager_data;

// Utility function to get the page currently resident in a frame
static inline page_table_entry* get_page_from_frame(pager_data* pager, uint64 f)
{
	frame* frm = &pager->frames[f];
	return &pager->page_tables[frm->pid][frm->page_number];
}

// Initialize the pager with the given logical memory size (in number of pages), the physical
// memory size (in number of frames), the size of an individual page/frame (in bits), and the
// maximum number of processes on the system.
pager_data* pager_data_init(uint64 log_mem_sz, uint64 phy_mem_sz, uint64 page_sz, uint64 num_procs);

// Deallocate any memory that was allocated for the pager (including the pager itself). After
// this is called the pager can no longer be used.
void pager_data_dealloc(pager_data* pager);

// A request to allocate a page for a process is being made. The given PID is the process
// identifier, the page number is the page number being allocated for the process, and the
// access is the allowed access (a combination of READ, WRITE, and EXECUTE flags) for future
// memory reference requests. If the page is already allocated then its access flags are updated
// but nothing else is changed. This function does not bring a page into memory and does not print
// anything out.
void alloc_page(pager_data* pager, uint64 pid, uint64 p, byte access);

// This checks that the referenced page is a valid page for the given pocess and access request.
//
// If it is not valid then a descriptive message is printed out and INVALID_PAGE is returned.
// If it is valid then it updates the REFERENCED and possibly the DIRTY flag of the page. If memory
// resident then VALID_PAGE is returned, otherwise PAGE_FAULT is returned.
int check_log_addr(pager_data* pager, uint64 pid, uint64 logical_addr, byte access);

// Have page p of process pid claim the frame f. If the frame is not free than its contents are
// evicted. This updates the frame and page table along with printing out status messages.
void claim_frame(pager_data* pager, uint64 pid, uint64 logical_addr, uint64 f);

// Prints out the summary information for the simulation run including a divider.
void print_summary(pager_data* pager);

// Function that is unique for each replacement page replacement algorithm: selecting victim frame.
// If there is a free frame, it returns it. Otherwise it has to select a frame that is currently in
// use based on the its algorithm. It returns the frame number of the selected frame but does not
// update any pager data.
typedef uint64 (*f_select_victim_frame)(pager_data* pager);

#endif
