//////////////////////////////////////////////
// First-In-First_out Replacement Algorithm //
//////////////////////////////////////////////

#ifndef _FIFO_H_
#define _FIFO_H_

#include "general.h"
#include "pager.h"

// Function that is unique to the FIFO page replacement pager system: selecting victim frame.
// If there is a free frame, it returns it. Otherwise it has to select a frame that is currently in
// use based on the FIFO algorithm. It returns the frame number of the selected frame but does not
// update any pager data.
uint64 fifo_select_victim_frame(pager_data* pager);

#endif
