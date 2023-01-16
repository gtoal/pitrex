#ifndef GPUMEM_H
#define GPUMEM_H

#include <inttypes.h>

#define ALIGN_UP(ptr, alignment) (((ptr) + ((alignment)-1)) & ~((alignment)-1))

// Bus address is what's used by the VPU (0xC0000000 prefix for L2 cache disabled)
#define BUS_TO_PHYS(x) ((x) & ~0xC0000000)
#define PHYS_TO_BUS(x) ((x) |  0xC0000000)
#define VIRT_TO_BUS(block, x) ((uintptr_t)(x) - (uintptr_t)((block).virtualAddr) + (block).busAddress)

typedef struct
{
  uint32_t allocationHandle;
  void *virtualAddr;
  uintptr_t busAddress;
  uint32_t sizeBytes;
} GpuMemory;

#ifdef FAKE_VPU
GpuMemory AllocateUncachedGpuMemory(int mbox, uint32_t numBytes, int mem_fd);
#else
GpuMemory AllocateUncachedGpuMemory(int mbox, uint32_t numBytes);
#endif
void FreeUncachedGpuMemory(int mbox, GpuMemory mem);

#endif /* GPUMEM_H */
