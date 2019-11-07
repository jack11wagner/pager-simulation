//////////////////////////////////////////////
// First-In-First_out Replacement Algorithm //
//////////////////////////////////////////////

#include "general.h"
#include "pager.h"
#include "fifo.h"

// Function that is unique to the FIFO page replacement pager system: selecting victim frame.
// If there is a free frame, it returns it. Otherwise it has to select a frame that is currently in
// use based on the FIFO algorithm. It returns the frame number of the selected frame but does not
// update any pager data.
uint64 fifo_select_victim_frame(pager_data* pager) {
	// Select the first empty frame available, if any
	if (pager->num_free_frames > 0) { return pager->num_frames - pager->num_free_frames; }
	
	// Remove next victim, add a new victim, and return the next victim (all from/into the FIFO queue)
	uint64 victim_frame = pager->FIFO_victim;
	pager->FIFO_victim = (pager->FIFO_victim + 1) % pager->num_frames;
	return victim_frame;
}
