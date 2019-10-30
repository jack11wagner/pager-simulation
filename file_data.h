///////////////////////////////////////
// Paging File Data Reader Functions //
///////////////////////////////////////

#ifndef _FILE_DATA_H_
#define _FILE_DATA_H_

#include "general.h"
#include <stdbool.h>

typedef struct _file_data file_data;

// Open the given file and set it up for reading the paging data from it. Will return NULL if the
// file cannot be opened.
file_data* file_data_open(const char* filename);

// Close the given file data object, freeing all associated memory.
void file_data_close(file_data* fd);

// Read the first line of the paging data file which has 4 unsigned integer on it. Returns false for
// an invalid line and true otherwise. If true is returned then the arguments are filled in with the
// read values.
bool file_data_read_basic_info(file_data* fd, uint64* log_mem_sz, uint64* phy_mem_sz, uint64* page_sz, uint64* num_procs);

// Read a line of data from the paging data file which is either a page allocation or a memory
// reference. The arguments are filled in with the type ('a' or 'r'), the PID of the process,
// the associated value (either a page number for type == 'a' or a logical address for type == 'r'),
// and the access being allowed/requestd. Returns false for an invalid line and true otherwise.
bool file_data_read_data_line(file_data* fd, char* type, uint64* pid, uint64* val, byte* access);

// Gets the last line read by either file_data_read_basic_info or file_data_read_data_line
const char* file_data_get_last_line_read(file_data* fd);

#endif
