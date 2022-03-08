This project simulates page management systems using the C programming language.
The simulator is given a sequence of page allocations and memory references by processes and must keep each process's page-table up-to-date.
If/when the physical memory becomes full then a page currently resident in memory will need to be selected to be paged out so the new page can be placed into memory.
Since the project is simulating a page management system it does not require any actual memory for the pages or 
copy of data into and out of actual memory.
Instead the simulator will simply print out what action is being performed and update its internal page-table.
In all cases the frames will be allocated globally with no concern for a minimum number of frames per process, a page pool, or anything like that.

The page replacement algorithms implemented in this project are: 
* FIFO 
* SC (Second-Chance) 
* LRU 

