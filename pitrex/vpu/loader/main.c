/* Allocates GPU memory for PiTrex code and pipeline data, then execute pipeline
 * code on spare VPU processor in GPU. Addresses of GPU memory allocated for pipeline
 * data are written to file for later reading by programs using the Vectrex Interface
 * Library.
 *
 * VPU binary filename is read from the command line, or defaults to "vpu_disPipe.bin"
 * in the current directory.
 *
 * Kevin Koster, 2022.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <pitrex/bcm2835.h>
#include <vectrex/vectrexInterface.h>
#include "mailbox.h"
#include "gpuMem.h"

int mbox;
unsigned handle;
unsigned ptr;
uint8_t *program;

void setup(char *binfile) {
    mbox = mbox_open();

    int fd = 0;
    fd = open(binfile, O_RDONLY);
    if (fd < 1) {
      printf("Failed loading %s\n", binfile);
      exit(1);
    } else {
        int size = (int)lseek(fd, 0, SEEK_END);
        handle = mem_alloc(mbox, ALIGN_UP(size, PAGE_SIZE), PAGE_SIZE, 4);
        ptr = mem_lock(mbox, handle);
        program = mapmem(BUS_TO_PHYS(ptr), size);
        lseek(fd, 0, SEEK_SET);
        read(fd, program, size);
        printf("Loaded %s with size %d byte\n", binfile, size);
     }
}

int main(int argc, char *argv[]) {
     volatile uint32_t *pipelineInfoPtr;
     uint32_t physAddr_pipeline0;
     uint32_t physAddr_pipeline1;
     uint32_t physAddr_pipelineInfo;
     GpuMemory gpuMem;
     unsigned int lenSaved = 0;
     FILE *fd = NULL;

     if (!bcm2835_init())
       return 1;
      pipelineInfoPtr = (uint32_t *)PIPELINE_INFO_ADDR;
      /* 0xFFFFFFFF = "cleared" (disable VPU display update) */
      *pipelineInfoPtr = (uint32_t)0xFFFFFFFF;

      /* Load VPU binary into GPU memory from file*/
      if (argc > 1)
       setup(argv[1]);
      else
       setup("../vpu_disPipe.bin");

          /* Allocate GPU memory for pipeline data and write physical starting addresses to file */
	  if (mbox != -1) {
	    fd = fopen("/tmp/pitrex_gpu_mem", "w");
	    if (fd == NULL) {
	      printf ("Failed to open /tmp/pitrex_gpu_mem for writing\n");
	      return 1;
	    }
	    else
	    {
	      gpuMem = AllocateUncachedGpuMemory(mbox, sizeof(VectorPipeline)*MAX_PIPELINE);
	      physAddr_pipeline0 = BUS_TO_PHYS(gpuMem.busAddress);
	      lenSaved = fwrite (&physAddr_pipeline0, 4, 1, fd);
	      if (1 != lenSaved) {
                printf ("GPU pipeline 0 address write failed (len saved: %i) (Error: %m)\n", lenSaved);
                fclose (fd);
		unlink("/tmp/pitrex_gpu_mem");
		return 1;
	      }
	      else
	      {
	        gpuMem = AllocateUncachedGpuMemory(mbox, sizeof(VectorPipeline)*MAX_PIPELINE);
	        physAddr_pipeline1 = BUS_TO_PHYS(gpuMem.busAddress);
	        lenSaved = fwrite (&physAddr_pipeline1, 4, 1, fd);
	        if (1 != lenSaved) {
                  printf ("GPU pipeline 1 address write failed (len saved: %i) (Error: %m)\n", lenSaved);
                  fclose (fd);
		  unlink("/tmp/pitrex_gpu_mem");
		  return 1;
	        }
	        else {
	          gpuMem = AllocateUncachedGpuMemory(mbox, sizeof(PipelineInfo));
	          physAddr_pipelineInfo = BUS_TO_PHYS(gpuMem.busAddress);
	          lenSaved = fwrite (&physAddr_pipelineInfo, 4, 1, fd);
		  fclose(fd);
		  if (1 != lenSaved) {
                    printf ("GPU pipeline info address write failed (len saved: %i) (Error: %m)\n", lenSaved);
		    unlink("/tmp/pitrex_gpu_mem");
		    return 1;
	          }
		  /* When GPU video output turned off, mailbox routines can return same address for both
		   * pipeline allocations, so check for that to avoid memory conflicts. */
		  if (physAddr_pipeline0 == physAddr_pipeline1 ||
		      physAddr_pipeline0 == physAddr_pipelineInfo ||
		      physAddr_pipeline1 == physAddr_pipelineInfo) {
	             printf ("GPU allocated same memory twice! Aborting.\n");
	             unlink("/tmp/pitrex_gpu_mem");
		     return 1;
	          }
	        }
	      }
	    }
	  }
	  else
	    return 1;
    /* Start the VPU code running, then exit */
    printf("GPU memory allocated. Starting VPU code.\n");
    execute_code(mbox, ptr, 0, 0, 0, 0, 0, 0);
    mbox_close(mbox);
    bcm2835_close();
    return 0;
}
