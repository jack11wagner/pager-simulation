///////////////////////////////////////
// Paging File Data Reader Functions //
///////////////////////////////////////

#include "general.h"
#include "pager.h"
#include "file_data.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct _file_data {
    FILE* f;
    char* buf;
	size_t capacity;
} file_data;

// Removes leading and trailing whitespace like Python's str.strip() or Java's String.trim() methods
// Modifies the string in-place, returning the new start of the string
char* strip(char* s)
{
	while (isspace(*s)) { s++; } // find first non-whitespace character
	char* end = s + strlen(s) - 1;
	while (end > s && isspace(*end)) { end--; } // find last non-whitespace character
	end[1] = '\0';
	return s;
}

// This reads a single line from the given file similar to fgets. Unlike fgets the buffer and size
// are pointers and will be updated by this method so that if a line longer than *sz is found then
// the buffer is reallocated and the size is increased. If *buf is NULL or *sz is 0 then a buffer
// is allocated as well. For the dynamic allocation part of this the *buf must be from malloc
// (especially it cannot be a static array).
//
// This method returns NULL if the end-of-file was reached before this reads a single character
// otherwise it returns *buf, just like fgets. If there is an issue with memory allocation then an
// error is printed out and the program is killed with the error code -1.
char* readline(char **buf, size_t *sz, FILE* f)
{
    if (!*buf || *sz == 0) // no buffer yet, allocate a starting buffer now
    {
        *sz = 1024;
        *buf = (char*)malloc(1024);
        if (!*buf) { perror("failed to allocate memory"); exit(-1); }
    }
    // Get a line of text from the file
    if (!fgets(*buf, *sz, f)) { return NULL; }
    size_t len = strlen(*buf);
    while (len == 0 || (*buf)[len-1] != '\n') // if we are not at the end-of-line then grow and repeat
    {
        *sz += 1024; // increase size
        *buf = (char*)realloc(*buf, *sz); // grow buffer
        if (!*buf) { perror("failed to allocate memory"); exit(-1); }
        if (fgets(*buf + len, *sz - len, f) == NULL) { return *buf; } // get the next chunk of the line
        len += strlen(*buf + len);
    }
    return *buf;
}

// Open the given file and set it up for reading the paging data from it. Will return NULL if the
// file cannot be opened.
file_data* file_data_open(const char* filename)
{
	// Allocate memory for the structure
	file_data* fd = (file_data*)malloc(sizeof(file_data));
	if (!fd) { perror("failed to allocate memory"); exit(-1); }
	// Set all fields to NULL/0
    memset(fd, 0, sizeof(file_data));
	// Open the file
    fd->f = fopen(filename, "r");
    if (!fd->f) { free(fd); return NULL; }
    return fd;
}

// Close the given file data object, freeing all associated memory.
void file_data_close(file_data* fd)
{
    if (fd->f) { fclose(fd->f); }
    if (fd->buf) { free(fd->buf); }
    memset(fd, 0, sizeof(file_data));
    free(fd);
}

// Read the first line of the paging data file which has 4 unsigned integer on it. Returns false for
// an invalid line and true otherwise. If true is returned then the arguments are filled in with the
// read values.
bool file_data_read_basic_info(file_data* fd, uint64* log_mem_sz, uint64* phy_mem_sz,
                                              uint64* page_sz, uint64* num_procs)
{
    char dummy = 0;
    int count;
    do
    {
        if (!readline(&fd->buf, &fd->capacity, fd->f)) { fd->buf[0] = 0; return false; } // EOF
        count = sscanf(fd->buf, "%lu %lu %lu %lu %c", log_mem_sz, phy_mem_sz, page_sz, num_procs, &dummy);
    } while (count == -1); // skip blank lines
	// Make sure we read the correct number of values
    return count == 4 && !dummy;
}

// Convert a string of r, w, and x characters into a bit-mask of READ | WRITE | EXECUTE. At most one
// of r, w, and x are allowed. This is case-sensitive. Returns 0 in case of errors.
byte convert_rwx(const char* s)
{
    byte mask = 0;
    while (*s)
    {
		// Convert character to the flag READ, WRITE, or EXECUTE
        byte flag = (*s == 'r') ? READ : ((*s == 'w') ? WRITE : ((*s == 'x') ? EXECUTE : 0));
		// Make sure that the flag is legitimate and not already set
        if (!flag || mask & flag) { return 0; }
		// Set the flag
        mask |= flag;
		// Move to the next character
        s++;
    }
    return mask;
}

// Read a line of data from the paging data file which is either a page allocation or a memory
// reference. The arguments are filled in with the type ('a' or 'r'), the PID of the process,
// the associated value (either a page number for type == 'a' or a logical address for type == 'r'),
// and the access being allowed/requestd. Returns false for an invalid line and true otherwise.
bool file_data_read_data_line(file_data* fd, char* type, uint64* pid, uint64* val, byte* access)
{
    // Get the next line of data
    char* s;
    do
    {
        if (!readline(&fd->buf, &fd->capacity, fd->f)) { fd->buf[0] = 0; return false; } // EOF
        s = strip(fd->buf);
    } while (!s[0]); // skip blank lines
    *type = s[0];

	// Parse the data line
    char dummy = 0, _access[4] = {0, 0, 0, 0};
    int count = (s[0] == 'a') ? sscanf(s, "a %lu %lu %3s %c", pid, val, _access, &dummy) : // parse a memory allocation line of data
                ((s[0] == 'r') ? sscanf(s, "r %lu %lx %c %c", pid, val, _access, &dummy) : 0); // parse a memory reference line of data
    // Check that the data was parsed correctly and convert the rwx flags
    return count == 3 && !dummy && (*access = convert_rwx(_access));
}

// Gets the last line read by either file_data_read_basic_info or file_data_read_data_line
const char* file_data_get_last_line_read(file_data* fd)
{
	// Simply return our buffer
    return fd->buf;
}
