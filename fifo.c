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
	// Select the next victim by selecting the next frame. This works since the frames are populated
	// in order (frame 0, 1,..) and thus we will loop through them in order when selecting a victim.
	return (pager->FIFO_victim = (pager->FIFO_victim + 1) % pager->num_frames);
}
