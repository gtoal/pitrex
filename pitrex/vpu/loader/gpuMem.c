/* GPU memory allocation code based on the fbcp-ili9341 project (2022-12-29).
 * https://github.com/juj/fbcp-ili9341|fbcp-ili9341
 * https://github.com/juj/fbcp-ili9341/blob/master/dma.cpp
 * https://github.com/juj/fbcp-ili9341/blob/master/mailbox.cpp
 * https://github.com/raspberrypi/userland/issues/463#issuecomment-386059929
 *
 * Modified to use Broadcom mailbox.c routines.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "mailbox.h"
#include "gpuMem.h"
#ifdef FAKE_VPU
#include <sys/mman.h>
#endif

// Memory allocation flags
#define MEM_ALLOC_FLAG_DIRECT (1 << 2) // Allocate uncached memory that bypasses L1 and L2 cache on loads and stores
#define MEM_ALLOC_FLAG_COHERENT (1 << 3) // Non-allocating in L2 but coherent
#define MEM_ALLOC_FLAG_NO_INIT (1 << 5) // Don't initialise (default is initialise to all ones)

// Allocates the given number of bytes in GPU side memory, and returns the virtual address and physical bus address of the allocated memory block.
// The virtual address holds an uncached view to the allocated memory, so writes and reads to that memory address bypass the L1 and L2 caches. Use
// this kind of memory to pass data blocks over to the DMA controller to process.
#ifdef FAKE_VPU
GpuMemory AllocateUncachedGpuMemory(int mbox, uint32_t numBytes, int mem_fd)
#else
GpuMemory AllocateUncachedGpuMemory(int mbox, uint32_t numBytes);
#endif
{
  GpuMemory mem;
  mem.sizeBytes = ALIGN_UP(numBytes, PAGE_SIZE);
  uint32_t allocationFlags = MEM_ALLOC_FLAG_DIRECT | MEM_ALLOC_FLAG_COHERENT | MEM_ALLOC_FLAG_NO_INIT;
  mem.allocationHandle = mem_alloc(mbox, /*size=*/mem.sizeBytes, /*alignment=*/PAGE_SIZE, /*flags=*/allocationFlags);
  if (!mem.allocationHandle) {
    printf ("Failed to allocate GPU memory! Try increasing gpu_mem allocation in /boot/config.txt. See https://www.raspberrypi.com/documentation/computers/config_txt.html#gpu_mem|gpu_mem\n");
    exit(1);
  }
  mem.busAddress = mem_lock(mbox, mem.allocationHandle);
  if (!mem.busAddress) {
    printf ("Failed to lock GPU memory!\n");
    exit(1);
  }
#ifdef FAKE_VPU
  mem.virtualAddr = mmap(0, mem.sizeBytes, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, BUS_TO_PHYS(mem.busAddress));
  if (mem.virtualAddr == MAP_FAILED)
    printf("Failed to mmap GPU memory!");
  printf("Allocated %u bytes of GPU memory (bus address=%p)\n", mem.sizeBytes, (void*)mem.busAddress);
#endif
  return mem;
}

void FreeUncachedGpuMemory(int mbox, GpuMemory mem)
{
  /* unmapmem(mem.virtualAddr, mem.sizeBytes); */
  mem_unlock(mbox, mem.allocationHandle);
  mem_free(mbox, mem.allocationHandle);
}
