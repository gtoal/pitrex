#pragma once

#ifdef FREESTANDING
/*
done: FILE *fopen(const char *filename, const char *mode);
done: int fclose(FILE *stream);
done: size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
done: size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
done: int fseek(FILE *stream, long int offset, int whence);
done: int _chdir(char *_dir)

int fflush(FILE *stream);
int fprintf(FILE *stream, const char *format, ...);
int fgetc(FILE *stream);
char *fgets(char *str, int n, FILE *stream);
int fputc(int char, FILE *stream);
int fputs(const char *str, FILE *stream);
*/
#pragma once
#include <stdio.h>
#include <errno.h>
#undef errno
//extern int errno;
extern int errno;
#include <ff.h>

#define MAX_FILE_OPEN 10
#define PATH_MAX 80
#define PATH_MAXDEPTH 10
#define MODE_LEN 10

#define SEPERATOR '/'

#ifndef SEEK_SET
#define	SEEK_SET	0	/* set file offset to offset */
#define	SEEK_CUR	1	/* set file offset to current plus offset */
#define	SEEK_END	2	/* set file offset to EOF plus offset */
#endif

#define NAME_MAX 128
#undef dirent
struct _dirent {
   // long            d_ino;  /* Inode Nummer */
   //  off_t           d_off;  /* Offset zum nächsten dirent */
   //  unsigned short  d_reclen;/* Länge dieses Eintrags */
    char            d_name[NAME_MAX+1]; /* Dateiname */
  };
typedef struct _dirent dirent;



typedef enum {
    E_OK = 0,        ///<
    E_DIR_NOT_FOUND,       ///<
    E_SYSTEM_BOUNDS,
    E_FILE_NOT_FOUND,
    E_FILE_NOT_OPEN,
    E_READ_ERROR,
    E_WRITE_ERROR,
    E_SEEK_ERROR,
    E_OTHER
} ErrnoErrors;



typedef struct {
    FRESULT result;
    int pathCounter;
    char path[PATH_MAXDEPTH][PATH_MAX]; // path part of the file (without the name)
    char name[PATH_MAX]; // only the name part
    char mode[MODE_LEN];
    unsigned char modeTranslation;
    FIL file;
    int used;
    int pos;
} FF_WRAP;

#define fopen __fopen
#define fclose __fclose
#define fread __fread
#define fwrite __fwrite
#define fseek __fseek
#define ftell __ftell
#define fflush __fflush
#define chdir __chdir
#define getcwd __getcwd
#define opendir __opendir
#define readdir __readdir
#define closedir __closedir


FILE *__fopen(const char *filename, const char *mode);
int __fclose(FILE *stream);
size_t __fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t __fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int __fseek(FILE *stream, long int offset, int whence);
int __ftell(FILE *stream);
int __chdir(char *_dir);
int __fflush(FILE *stream);
char *__getcwd(char *buf, size_t size);
DIR *__opendir(char *name);
dirent *__readdir(DIR *cd);
int __closedir(DIR *cd);



void initFileSystem();
#define exit(a) v_errori("ERROR!", a);
static int (*old_fprintf)(FILE *stream, const char *format, ...) = fprintf;

#define fprintf(fd,...) \
do  \
{ \
  if ((((int) fd) == (int)stdout) || (((int) fd) == (int)stderr)) \
  { \
    old_fprintf(fd, __VA_ARGS__); \
  } \
  else \
  { \
    char buf[512]; \
    sprintf(buf, __VA_ARGS__); \
    f_printf(&((FF_WRAP *)fd)->file, "%s", buf ); \
  } \
} while (0)

#define fputs(message, fd) \
do  \
{ \
  fprintf(fd, "%s", message); \
} while (0)

#define fputc(message, fd) \
do  \
{ \
  fprintf(fd, "%c", message); \
} while (0)

extern char *getLoadParameter();
extern void v_errori(char *message, int i);

static inline int bm_isupper(int c) {
  return (c >= (int) 'A' && c <= (int) 'Z') ? 1 : 0;
}

static inline int bm_tolower(int c) {
  return ((bm_isupper(c) != 0) ? (c + 32) : c);
}
static inline int stricmp(const char *s1, const char *s2) {
  unsigned char *p1 = (unsigned char *) s1;
  unsigned char *p2 = (unsigned char *) s2;

  for (; bm_tolower((int) *p1) == bm_tolower((int) *p2); p1++, p2++) {
    if (*p1 == (unsigned char) '\0') {
      return 0;
    }
  }

  return (int) (bm_tolower((int) *p1) - bm_tolower((int) *p2));
}
#else // Not FREESTANDING

// Most of what this is for is already supplied under linux
#include <dirent.h>
typedef struct dirent dirent;
void initFileSystem();

#endif // Not FREESTANDING

// This is for my WIP in reintegrating some of Malban's fork with the dual system base.
// I strongly recommend against anyone else enabling this for now.

#ifdef USE_MALBAN_BAREMETAL_CALLS // These are from Malban's fork of the bare metal environment.
                                  // They might be needed from linux while backporting apps like 'sim'.
                                  // or maybe in linux I should supply dummy versions that do nothing?

#define EnableInterrupts()  __asm volatile ("cpsie i")
#define DisableInterrupts() __asm volatile ("cpsid i")

//
// Cache control
//
#define InvalidateInstructionCache()    \
                __asm volatile ("mcr p15, 0, %0, c7, c5,  0" : : "r" (0) : "memory")
#define FlushPrefetchBuffer()   __asm volatile ("mcr p15, 0, %0, c7, c5,  4" : : "r" (0) : "memory")
#define FlushBranchTargetCache()    \
                __asm volatile ("mcr p15, 0, %0, c7, c5,  6" : : "r" (0) : "memory")
#define InvalidateDataCache()   __asm volatile ("mcr p15, 0, %0, c7, c6,  0" : : "r" (0) : "memory")
#define CleanDataCache()    __asm volatile ("mcr p15, 0, %0, c7, c10, 0" : : "r" (0) : "memory")

//
// Barriers
//
#define DataSyncBarrier()   __asm volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (0) : "memory")
#define DataMemBarrier()    __asm volatile ("mcr p15, 0, %0, c7, c10, 5" : : "r" (0) : "memory")

#define InstructionSyncBarrier() FlushPrefetchBuffer()
#define InstructionMemBarrier() FlushPrefetchBuffer()

#define CompilerBarrier()   __asm volatile ("" ::: "memory")

void EnterCritical (void);
void LeaveCritical (void);

void MsDelay (unsigned nMilliSeconds);
void usDelay (unsigned nMicroSeconds);

#endif // USE_MALBAN_BAREMETAL_CALLS
