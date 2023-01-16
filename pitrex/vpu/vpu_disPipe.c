/* GPU Driver for display of Vectrex Interface Library pipeline via PiTrex
 * For Videocore IV VPU. Build with vc4-toolchain.
 * - Builds for CPU instead, using standard GCC, if FAKE_VPU is defined.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include <pitrex/pitrexio-gpio.h>
#include <pitrex/bcm2835.h>
#include <vectrex/vectrexInterface.h>

#ifdef FAKE_VPU
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "loader/mailbox.h"
#include "loader/gpuMem.h"

GpuMemory pipeline0Mem;
GpuMemory pipeline1Mem;
GpuMemory pipelineInfoMem;
void *pipelineVirtualAddr;
int mbox;
#endif

volatile uint32_t *pipelineInfoPtr;

#define LINE_DEBUG_OUT(...)
/* #define LINE_DEBUG_OUT(...) printf(__VA_ARGS__) */

// resolution "about" 4 nanoseconds
// but not less than OFFSET_CYCLE_OVERHEAD
void delayNano (uint32_t n) {
uint32_t nWait;

  nWait = n - OFFSET_CYCLE_OVERHEAD;
  if (nWait > ((uint32_t) - 20))
    return;
  WAIT_CYCLE_NANO (nWait);
}

// delay in vectrex cycles
void v_delayCycles (uint32_t n) {
  uint32_t nWait;

  nWait = (n * DELAY_PI_CYCLE_EQUIVALENT) - (OFFSET_CYCLE_OVERHEAD - 3);
  if (nWait > ((uint32_t) - 20))
    return;
  WAIT_CYCLE_NANO (nWait);
}

void v_delayCyclesQuarter (void) {
  uint32_t nWait;

  nWait = (DELAY_PI_CYCLE_EQUIVALENT / 4) - (OFFSET_CYCLE_OVERHEAD - 3);
  if (nWait > ((uint32_t) - 20))
    return;
  WAIT_CYCLE_NANO (nWait);
}

void v_delayCyclesEighth (void) {
  uint32_t nWait;

  nWait = (DELAY_PI_CYCLE_EQUIVALENT / 8) - (OFFSET_CYCLE_OVERHEAD - 3);
  if (nWait > ((uint32_t) - 20))
    return;
  WAIT_CYCLE_NANO (nWait);
}

/***********************************************************************/

/* Set current brightness of the Vectorbeam (provided BLANK is inactive).
 If bit 7 is set, vectrex does not show any brightness!
 It is checked if brightness is already set to the
 given value, if so - the function returns immediately.

 Ends without a delay, last set: VIA port B!
 */
void v_setBrightness (uint8_t brightness, PipelineInfo *pipelineInfo) {
  if (brightness == currentZSH)
    return;                     // beware -> this might cause brightness drift to zero on completely same intensities all the time!
  if (brightness != currentPortA) {
    SET (VIA_port_a, brightness);
    currentPortA = (int8_t) brightness;
  }
  SET (VIA_port_b, 0x84);       // MUX to intensity
  currentZSH = brightness;
  DELAY_ZSH ();
  SET (VIA_port_b, 0x81);       // SET port b to no Mux
}

/***********************************************************************/

/*      Sets the current scale value absolute to given value.
 Does NOT change VIA!
 */
void v_setScale (uint16_t s, PipelineInfo *pipelineInfo) {
  lastScale = currentScale;
  currentScale = s;
}

/**********************************************************************/

/* This routine moves the vector beam to the extrem edges of the screen and zeros afterwards
 * this prevents "vector collaps".
 * If the "application" uses vectors which are far from the center anyway... this is not needed.
 */
void v_deflok (PipelineInfo *pipelineInfo) {
  ZERO_AND_WAIT ();
  UNZERO ();
  v_setScale (255, pipelineInfo);
  SET_YSH_IMMEDIATE_8 (127);
  SET_XSH_IMMEDIATE_8 (127);
  START_WAIT_T1 ();
  ZERO_AND_WAIT ();
  UNZERO ();
  SET_YSH_IMMEDIATE_8 (-127);
  SET_XSH_IMMEDIATE_8 (-127);
  START_WAIT_T1 ();
  ZERO_AND_WAIT ();
}

/***********************************************************************/

/* This routine resets the integrator offsets to zero. This might be neccessary once in a while.
 *
 * The "opposite" (apart from natural degradation of the offsets) is Kristof Tuts like calibration.
 * Which actively sets a integrator offset to a very small value to compensate vectrex "drift".
 */
inline static void v_resetIntegratorOffsets (PipelineInfo *pipelineInfo) {
  SET (VIA_port_b, 0x81);
  DELAY_PORT_B_BEFORE_PORT_A ();
  if (calibrationValue == 0) {
    SET (VIA_port_a, 0x00);
    DELAY_CYCLES (4);
    // reset integrators
    SET (VIA_port_b, 0x82);     // mux=1, enable mux - integrator offset = 0
    DELAY_CYCLES (6);
    SET (VIA_port_b, 0x81);     // disable mux
  } else {
    SET (VIA_port_b, 0x81);
    DELAY_PORT_B_BEFORE_PORT_A ();
    SET (VIA_port_a, calibrationValue);
    DELAY_CYCLES (6);
    SET (VIA_port_b, 0x82);
    DELAY_PORT_B_BEFORE_PORT_A ();
    SET (VIA_port_a, 0xff);
    DELAY_CYCLES (2);
    SET (VIA_port_b, 0x81);
  }
  DELAY_CYCLES (4);
  currentPortA = 0x100;         // non regular value!
}

void displayPipeline (PipelineInfo *pipelineInfo) {
  int c = 0;
#ifdef FAKE_VPU
  volatile VectorPipeline *dpl = pipelineVirtualAddr;
#else
  volatile VectorPipeline *dpl = pipelineInfo->dpl;
#endif
  int delayedBeamOff = 0;

  if (browseMode) {
    v_setBrightness (50,pipelineInfo);
  }
  int lineNo = 0;
//  printf ("start type = %d\n", dpl[c].type);
  while (dpl[c].type != PL_END) {
    if (browseMode) {
      if (lineNo == currentBrowsline) {
        v_setBrightness (127,pipelineInfo);
      }
      if (lineNo == currentBrowsline + 1) {
        v_setBrightness (50,pipelineInfo);
      }
    }

    switch (dpl[c].type) {
    case PL_DEFLOK:
      {
        LINE_DEBUG_OUT ("PL DEFLOK\r\n");
        // loop ensures that there are no pending draws/moves
        if (dpl[c].flags & PL_SWITCH_BEAM_OFF)
          SWITCH_BEAM_OFF ();
        v_deflok (pipelineInfo);
        LINE_DEBUG_OUT ("PL DEFLOK CALIB \r\n");
        v_resetIntegratorOffsets (pipelineInfo);
        break;
      }
    case PL_ZERO:
      {
        LINE_DEBUG_OUT ("PL ZERO: %i, %i\r\n", dpl[c + 1].last_timing, dpl[c].this_timing);
#ifndef BEAM_LIGHT_BY_CNTL
        if (dpl[c].flags & PL_SWITCH_BEAM_OFF)
          SWITCH_BEAM_OFF ();
#endif
        ZERO_AND_CONTINUE ();
        {
          int timeDone = 24;

          // TODO enable calib again when zeroing!
          v_resetIntegratorOffsets (pipelineInfo);
          dpl[c + 1].last_timing = (dpl[c].this_timing - timeDone);
          if (dpl[c + 1].last_timing < 0)
            dpl[c + 1].last_timing = 0;
        }
        break;
      }
    case PL_SET_BRIGHTNESS:
      {
        LINE_DEBUG_OUT ("PL Brightness  A = %x\r\n", dpl[c].intensity);
        if (browseMode)
          break;
        if (dpl[c].flags & PL_I_A_MUST_BE_SET) {
          SET_WORD_ORDERED (VIA_port_b, 0x084, dpl[c].intensity);
        } else {
          SET (VIA_port_b, 0x84);       // MUX to intensity
        }
        DELAY_ZSH ();
        break;
      }
    case PL_DRAW_DOT:
      {
        LINE_DEBUG_OUT ("PL DOT  %i, %i :%i\r\n    %s\r\n", dpl[c].x * dpl[c].this_timing, dpl[c].y * dpl[c].this_timing, dpl[c].this_timing,
                        ((dpl[c].base != 0) ? dpl[c].base->debug : ""));
        if (dpl[c].flags & PL_SWITCH_BEAM_ON) {
          SWITCH_BEAM_ON ();
        }
        break;
      }
    case PL_MOVE:
      {
        LINE_DEBUG_OUT ("PL MOVE %i, %i :%i\r\n", dpl[c].x * dpl[c].this_timing, dpl[c].y * dpl[c].this_timing, dpl[c].this_timing);
      }
    case PL_DRAW_PATTERN:
      {
        if (dpl[c].type == PL_DRAW_PATTERN) {
          LINE_DEBUG_OUT ("PL DRAW PATTERN \r\n    %s\r\n", ((dpl[c].base != 0) ? dpl[c].base->debug : ""));
        }
      }
    case PL_DRAW:
      {
        int delayed = 1;

        if (dpl[c].type == PL_DRAW) {
          LINE_DEBUG_OUT ("PL DRAW \r\n    %s\r\n", ((dpl[c].base != 0) ? dpl[c].base->debug : ""));
        }

        if (dpl[c].flags & PL_CALIBRATE_0) {
          LINE_DEBUG_OUT ("    PL D CALIB 0\r\n");
          SET (VIA_port_b, 0x81);
          DELAY_PORT_B_BEFORE_PORT_A ();
          SET (VIA_port_a, 0x00);
          DELAY_CYCLES (2);
          SET (VIA_port_b, 0x82);       // mux=1, enable mux - integrator offset = 0
          DELAY_CYCLES (2);
          SET (VIA_port_b, 0x81);
          DELAY_CYCLES (2);
          delayed += 14;
        } else if (dpl[c].flags & PL_CALIBRATE) {
          LINE_DEBUG_OUT ("    PL D CALIB \r\n");
          v_resetIntegratorOffsets (pipelineInfo);
          delayed += 24;
        }

        int afterYDelay = 0;

        if (dpl[c].flags & PL_Y_MUST_BE_SET) {
          LINE_DEBUG_OUT ("     Y MUST BE SET\r\n");
          afterYDelay = 2;      // cranky dependend
          if (crankyFlag & CRANKY_BETWEEN_VIA_B) {
            afterYDelay += crankyFlag & 0x0f;
          }

          delayed += 2;

          if (dpl[c].flags & PL_Y_A_MUST_BE_SET) {
            LINE_DEBUG_OUT ("     YA MUST BE SET\r\n");
            if (dpl[c].flags & PL_MUX_Y_MUST_BE_SET) {
              LINE_DEBUG_OUT ("     YMUX MUST BE SET\r\n");
              SET (VIA_port_b, 0x80);
              DELAY_PORT_B_BEFORE_PORT_A ();
              SET (VIA_port_a, dpl[c].y);
              delayed += 2 + DELAY_PORT_B_BEFORE_PORT_A_VALUE;
            } else {
              SET (VIA_port_a, dpl[c].y);
            }
          } else {
            if (dpl[c].flags & PL_MUX_Y_MUST_BE_SET) {
              SET (VIA_port_b, 0x80);   // MUX to y integrator
            }
          }
        } else
          LINE_DEBUG_OUT ("     Y NEED NOT BE SET\r\n");

        if (dpl[c].flags & PL_DEACTIVATE_ZERO) {
          // attention!
          // UNZERO is also a BEAM_OFF!
          UNZERO ();
          delayed += 2;
          afterYDelay -= 2;
        }

        if (dpl[c].flags & PL_Y_DELAY_TO_NULL) {
          if (crankyFlag & CRANKY_NULLING_WAIT) {
            // some crankies need additional waits here!
            afterYDelay += CRANKY_DELAY_Y_TO_NULL_VALUE;
          }
        }

        // after the last set Y value
        // We have to wait for "afterYDelay" cycles
        // so Y can settly

        // for the beam to be switched off from last drawing we have
        // to wait alltogether for "DELAY_AFTER_T1_END_VALUE" cycles
        // now we do the wait / wait we still need
        if (afterYDelay < 0)
          afterYDelay = 0;
        if (delayedBeamOff == 3) {
          int toDelayOff = DELAY_AFTER_T1_END_VALUE - delayed;

          if (dpl[c].type == PL_DRAW)
            toDelayOff -= 5;    // consecutive draws wait less ;-)

          if (toDelayOff >= afterYDelay) {
            // todo
            // theoretically we could check the difference
            // do a part delay here
            // and set the X value as a "delayer"

            if (toDelayOff > 0)
              DELAY_CYCLES (toDelayOff);
            SWITCH_BEAM_OFF ();
          } else {
            if (toDelayOff > 0)
              DELAY_CYCLES (toDelayOff);
            else
              toDelayOff = 0;
            SWITCH_BEAM_OFF ();
            DELAY_CYCLES (afterYDelay - toDelayOff);
          }
        } else {
          DELAY_CYCLES (afterYDelay);   // EQ DELAY_AFTER_YSH_VALUE this is cranky dependend!
        }

        // not checking - since we also do ramp with B
        // if (dpl[c].flags & PL_MUX_X_MUST_BE_SET;
        SET (VIA_port_b, 0x81);
        if (dpl[c].flags & PL_X_A_MUST_BE_SET) {
          // to test only if cranky?dpl[c].x
          DELAY_PORT_B_BEFORE_PORT_A ();
          SET (VIA_port_a, dpl[c].x);
        }
        // sync()
        // setMarkStart ();
        // start T1 timer
        if (dpl[c].flags & PL_T1_LO_EQUALS) {
          SET (VIA_t1_cnt_hi, (dpl[c].this_timing) >> 8);
        } else {
          SETW_inverse (VIA_t1, dpl[c].this_timing);    /* scale due to "enlargement" is 16 bit! */
        }

        if (dpl[c].type == PL_DRAW) {
          LINE_DEBUG_OUT ("     %i, %i :%i \r\n", dpl[c].x * dpl[c].this_timing, dpl[c].y * dpl[c].this_timing, dpl[c].this_timing);
          if (dpl[c].flags & PL_SWITCH_BEAM_ON) {
            SWITCH_BEAM_ON ();
          }
        } else if (dpl[c].type == PL_DRAW_PATTERN)      // this must be (dpl[c].type == PL_DRAW_PATTERN))
        {
          LINE_DEBUG_OUT ("     %i, %i :%i - $%02x\r\n", dpl[c].x * dpl[c].this_timing, dpl[c].y * dpl[c].this_timing, dpl[c].this_timing, dpl[c].pattern);
          int patternAnds[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
          int pCount = 0;
          while ((GET (VIA_int_flags) & 0x40) == 0) {
#ifdef BEAM_LIGHT_BY_CNTL
            if (dpl[c].pattern & patternAnds[pCount])
              SWITCH_BEAM_ON ();
            else
              SWITCH_BEAM_OFF ();
            pCount = pCount + 1;
            if (pCount == 8)
              pCount = 0;
#endif
#ifdef BEAM_LIGHT_BY_SHIFT
            if (pCount == 0) {
              SET_SHIFT_REG (dpl[c].pattern);
              pCount = 18;
            } else
              pCount -= 2;
#endif
          }
          int delayT1 = DELAY_AFTER_T1_END_VALUE;

          while (delayT1 > 0) {
#ifdef BEAM_LIGHT_BY_CNTL
            if (dpl[c].pattern & patternAnds[pCount])
              SWITCH_BEAM_ON ();
            else
              SWITCH_BEAM_OFF ();
            pCount = pCount + 1;
            if (pCount == 8)
              pCount = 0;
#endif
#ifdef BEAM_LIGHT_BY_SHIFT
            if (pCount == 0) {
              SET_SHIFT_REG (dpl[c].pattern);
              pCount = 18;
            } else
              pCount -= 2;
#endif
            delayT1 -= 2;
          }
          SWITCH_BEAM_OFF ();
          if ((browseMode) && (dpl[c].type == PL_DRAW_PATTERN)) {
            if (lineNo == currentBrowsline)
              currentDisplayedBrowseLine = currentBrowsline;
            lineNo++;
          }
        }
        break;
      }
    default:
      break;
    }
    delayedBeamOff = 0;
    c++;

    if (dpl[c].flags & PL_LAST_MUST_FINISH) {
      if (dpl[c].flags & PL_LAST_IS_RAMPING) {
        while ((GET (VIA_int_flags) & 0x40) == 0) ;

        if ((dpl[c].flags & PL_SWITCH_BEAM_OFF) /* && (dpl[c].type != PL_ZERO) */ ) {
          // is drawing
          if ((dpl[c].type == PL_MOVE) || (dpl[c].type == PL_DRAW)) {
            delayedBeamOff = 1;
          } else {
            DELAY_T1_OFF ();
          }
        } else {
          // is Moving
          // finish move - is not as bad is BEAM
          // DELAY_T1_OFF();
          DELAY_CYCLES (8);
          if (dpl[c].type == PL_DRAW_DOT) {
            // since we switch the light DIRECTLY ON...
            // better wait some cycles more!
            DELAY_CYCLES (4);
          }
        }
      } else {
        // e.g. zeroing
        // dots
        DELAY_CYCLES (dpl[c].last_timing);
      }
      if ((browseMode) && ((dpl[c - 1].type == PL_DRAW) || (dpl[c - 1].type == PL_DRAW_DOT) || (dpl[c - 1].type == PL_DRAW_PATTERN))) {
        if (lineNo == currentBrowsline)
          currentDisplayedBrowseLine = currentBrowsline;
        lineNo++;
      }
    }

    if ((dpl[c].flags & PL_SWITCH_BEAM_OFF) /* && (dpl[c].type != PL_ZERO) */ ) {
      if (delayedBeamOff == 0) {
        SWITCH_BEAM_OFF ();
      } else {
        delayedBeamOff += 2;
      }

    }
  }

  // safety only
  SWITCH_BEAM_OFF ();
  ZERO_AND_CONTINUE ();
}

#ifdef FAKE_VPU
void sig_handler(int signo) {
  if (signo == SIGINT) {
    vectrexclose();
    printf("FAKE_VPU: received SIGINT, exiting.\n");
    FreeUncachedGpuMemory(mbox, pipeline0Mem);
    FreeUncachedGpuMemory(mbox, pipeline1Mem);
    FreeUncachedGpuMemory(mbox, pipelineInfoMem);
    mbox_close(mbox);
    if (unlink("/tmp/pitrex_gpu_mem") < 0)
      printf("Couldn't remove /tmp/pitrex_gpu_mem (Error: %m)\n");
    exit (0);
  }
}
#endif

int main (void)
{

#ifdef FAKE_VPU
  uint32_t physAddr_pipeline0;
  uint32_t physAddr_pipeline1;
  uint32_t physAddr_pipelineInfo;
  unsigned int lenSaved = 0;
  FILE *fd = NULL;
  int memfd = 0;

 if ((memfd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
   printf("can't open /dev/mem, unable to map GPU pipeline memory.\nThis program should be run as root.\n");
   return 1;
 }
 else
 {
  mbox = mbox_open();
  /* Allocate GPU memory for pipeline data and write physical starting addresses to file */
  if (mbox != -1) {
    fd = fopen("/tmp/pitrex_gpu_mem", "w");
    if (fd == NULL) {
      printf ("Failed to open /tmp/pitrex_gpu_mem for writing\n");
      close(memfd);
      return 1;
    }
    else
    {
      pipeline0Mem = AllocateUncachedGpuMemory(mbox, sizeof(VectorPipeline)*MAX_PIPELINE, memfd);
      physAddr_pipeline0 = BUS_TO_PHYS(pipeline0Mem.busAddress);
      lenSaved = fwrite (&physAddr_pipeline0, 4, 1, fd);
      if (1 != lenSaved) {
        printf ("GPU pipeline 0 address write failed (len saved: %i) (Error: %m)\n", lenSaved);
        fclose (fd);
	unlink("/tmp/pitrex_gpu_mem");
	close(memfd);
    	FreeUncachedGpuMemory(mbox, pipeline0Mem);
	return 1;
      }
      else
      {
        pipeline1Mem = AllocateUncachedGpuMemory(mbox, sizeof(VectorPipeline)*MAX_PIPELINE, memfd);
        physAddr_pipeline1 = BUS_TO_PHYS(pipeline1Mem.busAddress);
        lenSaved = fwrite (&physAddr_pipeline1, 4, 1, fd);
        if (1 != lenSaved) {
          printf ("GPU pipeline 1 address write failed (len saved: %i) (Error: %m)\n", lenSaved);
          fclose (fd);
	  unlink("/tmp/pitrex_gpu_mem");
	  close(memfd);
	  FreeUncachedGpuMemory(mbox, pipeline0Mem);
    	  FreeUncachedGpuMemory(mbox, pipeline1Mem);
	  return 1;
        }
        else {
	  pipelineInfoMem = AllocateUncachedGpuMemory(mbox, sizeof(PipelineInfo), memfd);
	  physAddr_pipelineInfo = BUS_TO_PHYS(pipelineInfoMem.busAddress);
	  lenSaved = fwrite (&physAddr_pipelineInfo, 4, 1, fd);
	  fclose(fd);
	  close(memfd);
	  if (1 != lenSaved) {
            printf ("GPU pipeline info address write failed (len saved: %i) (Error: %m)\n", lenSaved);
	    unlink("/tmp/pitrex_gpu_mem");
    	    FreeUncachedGpuMemory(mbox, pipeline0Mem);
	    FreeUncachedGpuMemory(mbox, pipeline1Mem);
   	    FreeUncachedGpuMemory(mbox, pipelineInfoMem);
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
 }

  if (signal(SIGINT, sig_handler) == SIG_ERR)
    printf("Can't catch SIGINT\n");
#endif/* FAKE_VPU */
  if (vectrexinit(0) == 1) {
   pipelineInfoPtr = (uint32_t *)PIPELINE_INFO_ADDR;
   *pipelineInfoPtr = (uint32_t)0xFFFFFFFF; // Disables pipeline display
   while (1) {
     /* Should use Mailbox instead of polling? Apparantly MAILBOX1 and MAILBOX2 are unused:
       https://forums.raspberrypi.com/viewtopic.php?t=192069#p1204538 */
     if ((uint32_t)*pipelineInfoPtr != 0xFFFFFFFF) {
       /* Access the CPU's PipelineInfo structure in GPU RAM, via pointer in "Multicore Sync Block" register */
#ifdef FAKE_VPU
       /* Check that addresses set by Vectrex Interface Library are correct */
       if ((uint32_t)*pipelineInfoPtr != (uint32_t)physAddr_pipelineInfo) {
         printf("Pipeline info pointer register value wrong! pipelineInfoPtr = %p, allocated address = %p\n",
	        (void *)*pipelineInfoPtr, (void *)physAddr_pipelineInfo);
       }
       else { /* Work out which pipeline we're using by matching physical address */
         if ((uint32_t)(((PipelineInfo *)pipelineInfoMem.virtualAddr)->dpl) == (uint32_t)physAddr_pipeline0) {
	   pipelineVirtualAddr = pipeline0Mem.virtualAddr;
	 }
	 else {
	   pipelineVirtualAddr = pipeline1Mem.virtualAddr;
	   if ((uint32_t)(((PipelineInfo *)pipelineInfoMem.virtualAddr)->dpl) != (uint32_t)physAddr_pipeline1) {
	     printf("Pipeline pointer value wrong! pipeline pointer = %p, allocated addresses = %p & %p\n",
	        (void *)(((PipelineInfo *)pipelineInfoMem.virtualAddr)->dpl), (void *)physAddr_pipeline0, (void *)physAddr_pipeline1);
	   }
         }
//       printf ("launching pipeline\n");
       displayPipeline ((PipelineInfo *)pipelineInfoMem.virtualAddr);
       }
#else
       /* TODO: Need to do PHYS_TO_BUS on VPU? Or is this done already by compiler/assembler? */
       displayPipeline ((PipelineInfo *)*pipelineInfoPtr);
#endif/* FAKE_VPU */
       *pipelineInfoPtr = (uint32_t)0xFFFFFFFF; // Indicates pipeline display complete
//       printf ("pipeline ptr reset to: %p\n", (void *)*pipelineInfoPtr);
     }
   }
  }
}

/* In case you're lost:
 *  pipelineInfoPtr is a pointer to a register, which is used as a pointer 
 *  to the pipelineInfo structure in allocated GPU memory. The pipelineInfo
 *  structure has a member dpl, which is a pointer to the vector pipeline in
 *  allocated GPU memory. Then in Linux, the virtual address space means that
 *  even more pointers need to get involved.
 */
