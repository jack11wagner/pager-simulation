//////////////////////////////////////////////
// Second Change Page Replacement Algorithm //
//////////////////////////////////////////////

#include "general.h"
#include "pager.h"
#include "sc.h"

// Function that is unique to the SC page replacement pager system: selecting victim frame.
// If there is a free frame, it returns it. Otherwise it has to select a frame that is currently in
// use based on the SC algorithm. It returns the frame number of the selected frame but does not
// update any pager data.
uint64 sc_select_victim_frame(pager_data* pager)
{
	// Select the first empty frame available, if any
	if (pager->num_free_frames > 0) { return pager->num_frames - pager->num_free_frames; }

	// Loop through the pages in the frames and select the first frame whose page has a 
	// REFERENCED bit of 0.
	uint64 frame_number = pager->SC_head_frame;
	page_table_entry* page = get_page_from_frame(pager, frame_number);
	while (page->flags & REFERENCED) {
		page->flags ^= REFERENCED; // Set bit to 0
		frame_number = (frame_number + 1) % pager->num_frames;
		page = get_page_from_frame(pager, frame_number);
	}
	pager->SC_head_frame = (frame_number + 1) % pager->num_frames;
	return frame_number;
}
