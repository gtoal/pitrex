#ifdef FREESTANDING
/* Required include for fstat() */
#include <sys/stat.h>
#include <string.h>

#include "vectrexInterface.h"
#include "osWrapper.h"

static FATFS fat_fs;            /* File system object */
static int fsInit = 0;
int errno;

FF_WRAP ff_allFiles[MAX_FILE_OPEN];

// assuming for now only ONE directory open at one TIME
DIR currentDir;

void initFileSystem()
{
    if (fsInit!=0) return;
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);

	for (int i=0;i<MAX_FILE_OPEN; i++)
	{
		ff_allFiles[i].used = 0;
	}
    FRESULT result = f_mount(&fat_fs, (const TCHAR *) "", (unsigned char) 1);
    if (result != FR_OK)
    {
      printf("NO filesystem...!\r\n");
      printf("f_mount failed! %d\r\n", (int) result);
      v_error("FAT NOT FOUND");
    }
    else
    {
      printf("FAT filesystem found!\r\n");
    }
    fsInit=1;
}
// for file accesss
char * ff_getErrorText(int errNo)
{
  switch (errNo)
  {
    case FR_OK:             /* (0) Succeeded */
      return "OK";
    case FR_DISK_ERR:           /* (1) A hard error occurred in the low level disk I/O layer */
      return "FR_DISK_ERR";
    case FR_INT_ERR:                /* (2) Assertion failed */
      return "FR_INT_ERR";
    case FR_NOT_READY:          /* (3) The physical drive cannot work */
      return "FR_NOT_READY";
    case FR_NO_FILE:                /* (4) Could not find the file */
      return "FR_NO_FILE";
    case FR_NO_PATH:                /* (5) Could not find the path */
      return "FR_NO_PATH";
    case FR_INVALID_NAME:       /* (6) The path name format is invalid */
      return "FR_INVALID_NAME";
    case FR_DENIED:             /* (7) Access denied due to prohibited access or directory full */
      return "FR_DENIED";
    case FR_EXIST:              /* (8) Access denied due to prohibited access */
      return "FR_EXIST";
    case FR_INVALID_OBJECT:     /* (9) The file/directory object is invalid */
      return "FR_INVALID_OBJECT";
    case FR_WRITE_PROTECTED:        /* (10) The physical drive is write protected */
      return "FR_WRITE_PROTECTED";
    case FR_INVALID_DRIVE:      /* (11) The logical drive number is invalid */
      return "FR_INVALID_DRIVE";
    case FR_NOT_ENABLED:            /* (12) The volume has no work area */
      return "FR_NOT_ENABLED";
    case FR_NO_FILESYSTEM:      /* (13) There is no valid FAT volume */
      return "FR_NO_FILESYSTEM";
    case FR_MKFS_ABORTED:       /* (14) The f_mkfs() aborted due to any problem */
      return "FR_MKFS_ABORTED";
    case FR_TIMEOUT:                /* (15) Could not get a grant to access the volume within defined period */
      return "FR_TIMEOUT";
    case FR_LOCKED:             /* (16) The operation is rejected according to the file sharing policy */
      return "FR_LOCKED";
    case FR_NOT_ENOUGH_CORE:        /* (17) LFN working buffer could not be allocated */
      return "FR_NOT_ENOUGH_CORE";
    case FR_TOO_MANY_OPEN_FILES:    /* (18) Number of open files > _FS_LOCK */
      return "FR_TOO_MANY_OPEN_FILES";
    case FR_INVALID_PARAMETER:  /* (19) Given parameter is invalid */
      return "FR_INVALID_PARAMETER";
  }
  char *t = "UNKOWN ERROR:      \r\n";
  sprintf(t, "UNKOWN ERROR: %i", errNo);
  return t;
}

// -1 on error
// number of file ide else
static int getFreeFilePos()
{
	for (int i=0;i<MAX_FILE_OPEN; i++)
	{
		if (ff_allFiles[i].used == 0) return i;
	}
	return -1;
}

static FF_WRAP *getFileWrapper(const char *filename, const char *mode)
{
	FF_WRAP * f = 0;
	int no = getFreeFilePos();
	if (no < 0)
    {
      return 0;
    }
	f = &ff_allFiles[no];

	strncpy(f->mode, mode,MODE_LEN-1);
	char *modePointer=f->mode;
	while (!modePointer!=0)
	{
		*modePointer = bm_tolower(*modePointer);
		modePointer++;
	}

	char sep[2];
	sep[0] =SEPERATOR;
	sep[1] =0;

	char tokenBuffer[PATH_MAX];
	strncpy(tokenBuffer, filename, PATH_MAX);

	f->pathCounter = 0;
	char *token = strtok(tokenBuffer, sep);
	strcpy(f->name, token);
	do
	{
		token = strtok(0, sep);
		if ((token == 0) ||(token[0]==0))
		{
			break;
		}
		strcpy(f->path[f->pathCounter], f->name);
		strcpy(f->name, token);
		f->pathCounter++;
        if (f->pathCounter>=PATH_MAX)
        {
          return 0;
        }
	} while(1); 
	f->modeTranslation = FA_OPEN_EXISTING;
	if (strstr(f->mode, "r") != 0) 
    {
      f->modeTranslation=FA_READ;
    }
	if (strstr(f->mode, "w") != 0) 
    {
      f->modeTranslation=FA_CREATE_ALWAYS|FA_WRITE;//FA_WRITE|FA_CREATE_ALWAYS|FA_CREATE_NEW;
    }
	if (strstr(f->mode, "a") != 0) 
    {
      f->modeTranslation=FA_OPEN_APPEND;
    }
	if (strstr(f->mode, "+") != 0) 
    {
      f->modeTranslation=FA_READ|FA_WRITE|FA_OPEN_ALWAYS;
    }

/*
"r" Opens a file for reading. The file must exist.
"w" Creates an empty file for writing. If a file with the same name already exists, its content is erased and the file is considered as a new empty file.
"a" Appends to a file. Writing operations, append data at the end of the file. The file is created if it does not exist.
"r+" Opens a file to update both reading and writing. The file must exist.
"w+" Creates an empty file for both reading and writing.
"a+" Opens a file for reading and appending.

File access mode and open method flags (3rd argument of f_open)
#define	FA_READ				0x01
#define	FA_WRITE			0x02
#define	FA_OPEN_EXISTING	0x00
#define	FA_CREATE_NEW		0x04
#define	FA_CREATE_ALWAYS	0x08
#define	FA_OPEN_ALWAYS		0x10
#define	FA_OPEN_APPEND		0x30
*/
	f->used = 1;
	f->pos = 0;
	return f;
}

int errno = 0;
int changeDirsTo(FF_WRAP *f)
{
	int pc = 0;
	while (pc<f->pathCounter)
	{
	  FRESULT rc_rd = f_chdir (f->path[pc]);
	  if (rc_rd != FR_OK)
	  {
		printf("NO directory found... (%s)!\r\n", f->path[pc]);
		return errno = 1;
	  }
	  pc++;
	}
	return errno = 0;
}

int changeDirsBack(FF_WRAP *f)
{
	int pc = 0;
	while (pc<f->pathCounter)
	{
	  f_chdir ("..");
	  pc++;
	}
	return errno = 0;
}
void outputCurrentDirectory()
{
  char buf[256];
  FRESULT result = f_getcwd (buf,256);
  if (result != FR_OK)
  {
    printf("f_getcwd failed (%i) \r\n", result);
    return ;
  }
  printf("Current directory: %s \r\n", buf);
  DIR dp;
  result = f_opendir (&dp, buf);
  if (result != FR_OK)
  {
    printf("f_opendir failed (%i) \r\n", result);
    return ;
  }
  printf("Contents: \r\n");
  printf("--------- \r\n");
  FILINFO finfo;
  while (result == FR_OK)
  {
    result =  f_readdir (&dp,&finfo);
    if (result == FR_OK)
    {
      /*
       *       typedef struct {
       *               FSIZE_t fsize;                  / * File size * /
       *               WORD    fdate;                  / * Modified date * /
       *               WORD    ftime;                  / * Modified time * /
       *               BYTE    fattrib;                / * File attribute * /
       *               TCHAR   altname[13];            / * Altenative file name * /
       *               TCHAR   fname[_MAX_LFN + 1];    / * Primary file name * /
    } FILINFO;
    */
    printf("\t %s\t%i\r\n",finfo.fname, finfo.fsize);
    }
  }
  printf("\r\n");
  f_closedir (&dp);
}
char * ff_getErrorText(int errNo);
FILE *__fopen(const char *filename, const char *mode)
{
  initFileSystem();
	FF_WRAP *f = getFileWrapper(filename, mode);
	if (f == 0)
    {
      errno = E_SYSTEM_BOUNDS;
      printf("_fopen - wrapper error \r\n");
      return 0;
    }
	f->result = FR_DISK_ERR; // default is error :-)
	changeDirsTo(f);
	f->result = f_open(&f->file, f->name,  f->modeTranslation);
	changeDirsBack(f);
	if (f->result != FR_OK)
	{
		f->used = 0;
		errno = E_FILE_NOT_FOUND;
		printf("Could not open file %s (%s) \r\n", filename, ff_getErrorText(f->result));
		return 0;
	}
	errno = E_OK;
	return (FILE *) f;
}
int __fclose(FILE *_f)
{
	FF_WRAP *f = (FF_WRAP *)_f;
	if (f->used==0)
    {
      errno = E_FILE_NOT_OPEN;
      return 0;
    }

	// close flushes
    f_close(&f->file);
	f->used = 0;
    errno = E_OK;
	return 0;
}

size_t __fread(void *ptr, size_t size, size_t nmemb, FILE *_f)
{
	FF_WRAP *f = (FF_WRAP *)_f;
	if (f->used==0)
	{
		errno = E_FILE_NOT_OPEN;
		return 0;
	}

	unsigned int lenLoaded=0;
	f->result = f_read(&f->file, ptr, size*nmemb, &lenLoaded);
	f->pos += lenLoaded;
	if (( f->result!= FR_OK) || (size*nmemb != lenLoaded))
	{
		printf("fread() of %s fails (len loaded: %i/%i) (Error: %s)\r\n", f->name, lenLoaded,(size*nmemb), ff_getErrorText(f->result));
		errno = E_READ_ERROR;
		return 0;
	}
    errno = E_OK;
	return lenLoaded/size; // return in number of "objects" read
}
size_t __fwrite(const void *ptr, size_t size, size_t nmemb, FILE *_f)
{
	FF_WRAP *f = (FF_WRAP *)_f;
	if (f->used==0)
	{
		errno = E_FILE_NOT_OPEN;
		return 0;
	}

	unsigned int lenSaved=0;
	f->result = f_write(&f->file, ptr, size*nmemb, &lenSaved);
	f->pos += lenSaved;
	if (( f->result!= FR_OK) || (size*nmemb != lenSaved))
	{
		printf("fwrite() of '%s' fails (len saved: %i/%i) (Error: %s)\r\n", f->name, lenSaved,(size*nmemb), ff_getErrorText(f->result));
		errno = E_WRITE_ERROR;
		return 0;
	}
    errno = E_OK;
	return lenSaved/size; // return in number of "objects" read
}

// different docomentation
// some say 0 on success and other is failure
// some say 0 is failure and the position is returned otherwise...
int __fseek(FILE *_f, long int offset, int whence)
{
	FF_WRAP *f = (FF_WRAP *)_f;
	if (f->used==0)
	{
		errno = E_FILE_NOT_OPEN;
		return -1;
	}
	if (whence==SEEK_SET) // SEEK_SET
	{
		// do nothing
	}
	else if (whence==SEEK_CUR) // SEEK_CUR
	{
		offset+=f->pos;
	}
	else if (whence==SEEK_END) // SEEK_END
	{
      changeDirsTo(f);
	  FILINFO finfo;
	  f->result = f_stat (f->name, &finfo);	/* Get file status */
      changeDirsBack(f);
	  if (f->result != FR_OK)
	  {
	      printf ("fseek(): could not open file %s (%s) \r\n", f->name, ff_getErrorText (f->result));
	      errno = E_SEEK_ERROR;
	  }
	  offset = finfo.fsize-offset;
	}

    changeDirsTo(f);
	f->result =  f_lseek (&f->file, offset);
    changeDirsBack(f);
	if ( f->result!= FR_OK)
	{
		printf("Can't seek to %d in %s (error: %s)\r\n", offset, f->name, ff_getErrorText(f->result));
		errno = E_SEEK_ERROR;
		return -1;
	}

	f->pos = offset;
	errno = E_OK;
	return 0;
}
int __ftell(FILE *_f)
{
    FF_WRAP *f = (FF_WRAP *)_f;
    return f->pos;
}

int __chdir(char *_dir)
{
  initFileSystem();
  FRESULT _result = f_chdir (_dir);
  if (_result != FR_OK)
  {
    printf("NO %s directory found...!\r\n", _dir);
    v_error("NO DIRECTORY");
  }
  errno = E_OK;
  return 0;
}

int __fflush(FILE *_f)
{
	FF_WRAP *f = (FF_WRAP *)_f;
	if (((int) f) == 1) // stdout
	{
	  // i assume the buffer is null
	  return 0;
	}
	if (((int) f) == 2) // stderr
	{
	  // i assume the buffer is null
	  return 0;
	}
	// file is expected
	f_sync (&f->file);
}
char *__getcwd(char *buf, size_t size)
{
    FRESULT result = f_getcwd (buf,256);
    if (result != FR_OK)
    {
      printf("f_getcwd failed (%i) \r\n", result);
      errno = E_OTHER;
      return 0;
    }
    errno = E_OK;
    return buf;
}
// assuming for now only ONE directory open at one TIME
// !!!
DIR *__opendir(char *name)
{
    FRESULT result = f_opendir (&currentDir, name);
    if (result != FR_OK)
    {
      printf("f_opendir failed '%s' (%i) \r\n",name, result);
      return 0;
    }
    return &currentDir;
}
// only 1 also!!!
FILINFO finfo;
dirent oneDirEntry;
dirent *__readdir(DIR *cd)
{
      FRESULT result  =  f_readdir (cd, &finfo);
      if (result != FR_OK)
      {
        return 0;
      }
      strncpy(oneDirEntry.d_name, finfo.fname, NAME_MAX);
    return &oneDirEntry;
}
int __closedir(DIR *cd)
{
    FRESULT result = f_closedir (cd);
    if (result != FR_OK)
    {
      printf("f_closedir failed (%i) \r\n", result);
      return 0;
    }
    return 0;
}

#endif // FREESTANDING
