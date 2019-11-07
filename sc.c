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
	// TODO

	// Select the first empty frame available, if any
	for (int i = 0; i < pager->num_frames; i++) { if (!pager->frames[i].occupied) { return i; } }
	return 0;
}
