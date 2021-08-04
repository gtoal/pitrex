// Some new procedures that only exist in Malban's new baremetal environment.
// Eventually these will be put in the appropriate system directories and files,
// but for now I'm lumping everything together, at least until I work out what's
// all missing.

// In the short term I could probably put all this in pitrex/vectrex/vectrexInterface.[hc]
// and avoid the need to add #include "malban.h" to Malban's sources.  Might make it
// easier to keep up with his changes that way.

#ifndef _MALBAN_C_
#define _MALBAN_C_ 1

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <error.h>
#include <errno.h>

#include "malban.h"

unsigned char useDoubleTimer = 0;
unsigned char keepDotsTogether = 0;

// I had to modify one of the VectorPipeline structures to add new fields, but am not doing
// anything with the new fields yet.
int (*checkExternal)(VectorPipelineBase **, VectorPipeline **, int *, int *, int *) = NULL;

int loadFromZip(char *zipFile, char *name, unsigned char* block) { // needs extra param of blocksize :-(
  static char unzip[512];
  FILE *contents;
  int c;
  // We'll do this the cheap way.  One of the benefits of having Linux available.
  sprintf(unzip, "unzip -p %s %s", zipFile, name); // We'll let stderr go to the tty...
  fprintf(stderr, "$ %s\n", unzip);
  contents = popen(unzip, "r"); // "rb" not supported - "Invalid argument"...
  if (!contents) {
    fprintf(stderr, "Cannot open %s/%s - %s\n", zipFile, name, strerror(errno));
    return 1;
  }
  for (;;) {
    c = fgetc(contents);
    if (c == EOF || feof(contents) || ferror(contents)) break; // catch the error on the CRC32 call.
    *block++ = c;
  }
  pclose(contents);
  return 0;
}

int iniHandler(char *user, const char *section, const char *name, const char *value) {
  // not sure where this is from. Not ini.[hc]?
  return 1;
}

// int (simIniHandler)(void* user, const char* section, const char* name, const char* value)

// no longer needed here now that I have added ini.c to pitrex/vectrex/ directory.
//int ini_parse(const char* filename, ini_handler handler, void* user);
//  return 1;
//}

void v_setClientHz(int hz) {
  // needs support for separate display rate vs execute rate
}

void v_enableSoundOut(int yesNo) {
}

int v_getSamplePlaying(void) {
  // don't have sample support yet in Linux.
  return 0;
}

void v_stopSamplePlaying(void) {
}

void v_message(char *mess) {
  // Need a neat pop-up with dismiss or exit button. Kevin's "vecho" should use this when it's ready.
  // Would be nice to extend with "..." parameters.  Or maybe add a v_messagef procedure for that.
  fprintf(stderr, "%s\n", mess);
  exit(1);
}

void v_enableButtons(int yesNo) {
}

void v_enableJoystickAnalog(int yesNoX1,int yesNoY1,int yesNoX2,int yesNoY2) {  // resets digital
}

void v_setupIRQHandling() {
}

int v_playIRQSample(void *fire1Sample, int fire1Size, int fire1Rate, int PLAY) {
  return 0;
}

void v_getDipSettings(DipSwitchSetting *dips[], char *title)
{
  // I don't think dip switches should be in the system - this should be in
  // emulator applications code.  The vectrex/pitrex has no dip switches.
}

int v_loadRAW(const char *filename, unsigned char *largeEnoughBuffer) { // needs a buffsize parameter
  FILE *sample;
  int c, i=0;
  sample = fopen(filename, "rb");
  if (!sample) {
    fprintf(stderr, "v_loadRAW: cannot open %s - %s\n", filename, strerror(errno)); // v_message when implemented
    return 0;
  }
  for (;;) {
    c = fgetc(sample);
    if (c == EOF || feof(sample)) {
      fclose(sample);
      return i;
    }
    if (ferror(sample)) {
      break; // catch the error on the CRC32 call.
    }
    largeEnoughBuffer[i++] = c;
  }
  fclose(sample);
  fprintf(stderr, "v_loadRAW: error reading from %s - %s\n", filename, strerror(errno));
  return 0;
}

//http://home.thep.lu.se/~bjorn/crc/

/* Simple public domain implementation of the standard CRC32 checksum.
 * Outputs the checksum for each file given as a command line argument.
 * Invalid file names and files that cause errors are silently skipped.
 * The program reads from stdin if it is called with no arguments. */

uint32_t crc32_for_byte(uint32_t r)
{
  for(int j = 0; j < 8; ++j)
    r = (r & 1? 0: (uint32_t)0xEDB88320L) ^ r >> 1;
  return r ^ (uint32_t)0xFF000000L;
}

int crc32(const void *data, size_t n_bytes, uint32_t* crc)
{
  static uint32_t table[0x100];
  if(!*table)
    for(size_t i = 0; i < 0x100; ++i)
      table[i] = crc32_for_byte(i);

  for(size_t i = 0; i < n_bytes; ++i)
    *crc = table[(uint8_t)*crc ^ ((uint8_t*)data)[i]] ^ *crc >> 8;
  return *crc;
}

#endif // _MALBAN_C_
