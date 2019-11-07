///////////////////
// Main Function //
///////////////////

// Compile: gcc -Wall main.c pager.c fifo.c sc.c lru.c file_data.c -o pager

#include "general.h"
#include "pager.h"
#include "fifo.h"
#include "sc.h"
#include "lru.h"
#include "file_data.h"

#include <stdio.h>
#include <string.h>

int main(int argc, const char** argv)
{
	// Basic argument checks
    if (argc == 1)
    {
        printf("usage: %s input_file [FIFO|SC|LRU]\n", argv[0]);
        return 0;
    }

    if (argc != 3)
    {
        fprintf(stderr, "wrong number of arguments (expected 2, got %d)\n", argc-1);
        return 1;
    }

    // Open the data file
    file_data* fd = file_data_open(argv[1]);
    if (!fd)
    {
        fprintf(stderr, "the file %s could not be opened\n", argv[1]);
        return 1;
    }

    // Setup which page replacement algorithm we will be using
    f_select_victim_frame select_victim_frame;
    if      (!strcmp(argv[2], "FIFO")) { select_victim_frame = fifo_select_victim_frame; }
    else if (!strcmp(argv[2], "SC"))   { select_victim_frame = sc_select_victim_frame;   }
    else if (!strcmp(argv[2], "LRU"))  { select_victim_frame = lru_select_victim_frame;  }
    else
    {
        fprintf(stderr, "%s is not a valid page replacement algorithm\n", argv[2]);
        file_data_close(fd);
        return 1;
    }

    // Read in the basic information about the memory system
    uint64 log_mem_sz, phy_mem_sz, page_sz, num_procs;
    if (!file_data_read_basic_info(fd, &log_mem_sz, &phy_mem_sz, &page_sz, &num_procs))
    {
        fprintf(stderr, "invalid first line of data, must be 4 base-10 unsigned integers separated by whitespace\n");
        file_data_close(fd);
        return 1;
    }

    // Initialize the pager
    void* pager = pager_data_init(log_mem_sz, phy_mem_sz, page_sz, num_procs);
    if (!pager)
    {
        fprintf(stderr, "unable to initlize the pager\n");
        file_data_close(fd);
        return 1;
    }

    // Loop through all lines in the paging data file
    while (true)
    {
        char type;
        uint64 pid, val;
        byte access;
        // Read data line
        if (!file_data_read_data_line(fd, &type, &pid, &val, &access))
        {
            const char *s = file_data_get_last_line_read(fd);
            if (!s[0]) { break; } // EOF reached
			
            // Otherwise it is bad data
            fprintf(stderr, "invalid data: %s\n", s);
			pager_data_dealloc(pager);
            file_data_close(fd);
            return 1;
        }
        // Run page allocation (val is the page number)
        if (type == 'a') { alloc_page(pager, pid, val, access); }
        else
		{
			// Run memory reference (val is the logical address)
			int status = check_log_addr(pager, pid, val, access);
			if (status == PAGE_FAULT)
			{
				// We had a page fault so handle it
				uint64 f = select_victim_frame(pager);
				claim_frame(pager, pid, val, f);
			}
		}
    }

	// Print out the summary and cleanup
    print_summary(pager);
	pager_data_dealloc(pager);
    return 0;
}
