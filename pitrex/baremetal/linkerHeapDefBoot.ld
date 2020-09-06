/* linker "add" file, so that symbols needed for malloc are defined! */
/* NOT used, progs from bootcode are always directly loaded to LOADER_START (atm 0x4000000 !!!*/

heap_low =  0x5000000;
heap_top =  0xfffffff;/* MAX 0x1fffffff; */
 