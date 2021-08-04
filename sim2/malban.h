#ifndef _MALBAN_H_
#define _MALBAN_H_ 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <vectrex/vectrexInterface.h>
#include "ini.h"

typedef struct
{
  char *name;
  int value;
} NameValuePair;

typedef struct
{
  char *replace;
  char *with;
} NameReplacePair;

typedef struct
{
  char *name;
  unsigned char *destination;
  NameValuePair values[];
} DipSwitchSetting;

void v_message(char *mess);
int loadFromZip(char *zipFile, char *name, unsigned char* block);
int crc32(const void *data, size_t n_bytes, uint32_t* crc);
int iniHandler(char *user, const char *section, const char *name, const char *value);
//int ini_parse(char *inifile, int (simIniHandler)(void* user, const char* section, const char* name, const char* value), int n);
void v_stopSamplePlaying(void);
int v_getSamplePlaying(void);
#define PLAY_ONCE 1
int v_playIRQSample(void *fire1Sample, int fire1Size, int fire1Rate, int PLAY);

void v_setClientHz(int hz);
void v_setRefresh(int hz);
int v_getRefresh();
void v_removeIRQHandling();
void v_setupIRQHandling();

void v_enableExtendedSoundOut(int yesNo);
void v_enableSoundOut(int yesNo);
void v_enableButtons(int yesNo);
void v_enableJoystickDigital(int yesNoX1,int yesNoY1,int yesNoX2,int yesNoY2); // resets analog
void v_enableJoystickAnalog(int yesNoX1,int yesNoY1,int yesNoX2,int yesNoY2);  // resets digital
void v_enable50HzSync(int yesNo);
void v_disableReturnToLoader();
int v_loadRAW(const char *filename, unsigned char *largeEnoughBuffer);


extern unsigned char useDoubleTimer;
extern unsigned char keepDotsTogether;

extern int (*checkExternal)(VectorPipelineBase **, VectorPipeline **, int *, int *, int *);

typedef struct
{
  char *name;
  signed char *destination;
  int mapping;
} InputMapping;

void v_doInputMapping(InputMapping inputMapping[]);
void v_getDipSettings(DipSwitchSetting *dips[], char * title);

#endif // _MALBAN_H_
