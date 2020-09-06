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
#else
#include <dirent.h>
typedef struct dirent dirent;
void initFileSystem();
#endif
