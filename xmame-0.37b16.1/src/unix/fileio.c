#include "xmame.h"
#include <stdarg.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <zlib.h>

#ifdef BSD43 /* old style directory handling */
#include <sys/types.h>
#include <sys/dir.h>
#define dirent direct
#endif

/* #define FILEIO_DEBUG */
#define MAXPATHC 20 /* at most 20 path entries */
#define MAXPATHL BUF_SIZE /* at most BUF_SIZE-1 character path length */
 
int load_zipped_file (const char *zipfile,const char *filename, unsigned char **buf, unsigned int *length);
int checksum_zipped_file (const char *zipfile, const char *filename, unsigned int *length, unsigned int *sum);
static int config_handle_inputfile(struct rc_option *option, const char *arg,
   int priority);

/* from ... */
extern char *cheatfile;
extern char *db_filename;
extern char *history_filename;
extern char *mameinfo_filename;

/* local vars */
static char *rompathv[MAXPATHC];
static int   rompathc = 0;
static char *rompath = NULL;
static char *spooldir = NULL; /* directory to store high scores */
static char *screenshot_dir = NULL;
static FILE *errorlog = NULL;
#ifdef MESS
static char *cheatdir = NULL;
#endif

/* struct definitions */
typedef enum
{
	kPlainFile,
	kRamFile
} eFileType;

typedef struct
{
	FILE		*file;
	unsigned char	*data;
	unsigned int	offset;
	unsigned int	length;
	unsigned int	crc;
 	eFileType	type;
} FakeFileHandle;

struct rc_option fileio_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "Fileio Related",	NULL,			rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "rompath",		"rp",			rc_string,	&rompath,
     XMAMEROOT"/roms",		0,			0,		NULL,
     "Set the rom search path" },
   { "spooldir",	"sd",			rc_string,	&spooldir,
     "hi",		0,			0,		NULL,
     "Set highscore spooldir" },
   { "screenshotdir",	"ssd",			rc_string,	&screenshot_dir,
     "snap",		0,			0,		NULL,
     "Set dir to store screenshots in" },
#ifdef MESS
   { "cheatdir",	NULL,			rc_string,	&cheatdir,
     XMAMEROOT"/cheat",	0,			0,		NULL,
     "Set dir to look for cheat files in" },
   { "crcdir",		NULL,			rc_string,	&crcdir,
     XMAMEROOT"/crc",	0,			0,		NULL,
     "Set dir to look for crc files in" },
   { "cheatfile",	"cf",			rc_string,	&cheatfile,
     "cheat.cdb",	 0,			0,		NULL,
     "Set the file to use as cheat database" },
#else
   { "cheatfile",	"cf",			rc_string,	&cheatfile,
     XMAMEROOT"/cheat.dat", 0,			0,		NULL,
     "Set the file to use as cheat database" },
#endif
   { "hiscorefile",	"hif",			rc_string,	&db_filename,
     XMAMEROOT"/hiscore.dat",	0,		0,		NULL,
     "Set the file to use as high score database" }, 
   { "historyfile",	"hf",			rc_string,	&history_filename,
     XMAMEROOT"/history.dat", 0,		0,		NULL,
     "Set the file to use as history database" },
   { "mameinfofile",	"mf",			rc_string,	&mameinfo_filename,
     XMAMEROOT"/mameinfo.dat", 0,		0,		NULL,
     "Set the file to use as mameinfo database" },
   { "record",		"rec",			rc_use_function, &options.record,
     NULL,		1,			0,		config_handle_inputfile,
     "Set a file to record keypresses into" },
   { "playback",	"pb",			rc_use_function, &options.playback,
     NULL,		0,			0,		config_handle_inputfile,
     "Set a file to playback keypresses from" },
   { "stdout-file",	"out",			rc_file,	&stdout_file,
     NULL,		1,			0,		NULL,
     "Set a file to redirect stderr to" },
   { "stderr-file",	"err",			rc_file,	&stderr_file,
     NULL,		1,			0,		NULL,
     "Set a file to redirect stdout to" },
   { "log",		"L",			rc_file,	&errorlog,
     NULL,		1,			0,		NULL,
     "Set a file to log debug info to" },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

static int config_handle_inputfile(struct rc_option *option, const char *arg,
   int priority)
{
   if (*(void **)option->dest)
      osd_fclose(*(void **)option->dest);
   
   *(void **)option->dest = osd_fopen(NULL, arg, OSD_FILETYPE_INPUTLOG,
      option->min);
   if (*(void **)option->dest == NULL)
   {
      fprintf(stderr, "error: couldn't open %s\n", arg);
      return -1;
   }
   
   option->priority = priority;
   
   return 0;
}


/* unix helper functions */

/* helper function which decomposes a path list into a vector of paths */
void init_rom_path(void)
{
	char *token = strtok(rompath, ":");
	while ((rompathc < MAXPATHC) && token)
	{
		rompathv[rompathc] = token;
		rompathc++;
		token = strtok (NULL, ":");
	}
}

/*
 * Search file caseinsensitively
 *
 * Arguments: 
 *	char * path - complete pathname to the desired file. The string will
 *	              be modified during search (and contains the final output).
 *
 * Return TRUE if found, FALSE otherwise.
 */
static int filesearch(char *path)
{
    DIR *dirp;
    struct dirent *de = NULL;
    char *ep, *dp, *fp;

    ep = strrchr(path, '/');
    if (ep) {
	*ep = '\0';	/* I guess root directory is not supported */
	dp = path;
	fp = ep + 1;
    } else {
	dp = "."; /* well, what should be the correct name for "current dir" */
	fp = path;
    }

    if (*fp == '\0') {
	return FALSE;
    }

    /* jamc: code to perform a upper/lowercase rom search */
    /* try to open directory */
    if ((dirp = opendir(dp)) == (DIR *) 0) { 
	return FALSE;
    }

    /* search entry and upcasecompare to desired file name */
    for (de = readdir(dirp); de; de = readdir(dirp))
	if (!strcasecmp(de->d_name, fp)) break;
    if (de) strcpy(fp, de->d_name);
    closedir(dirp);
    
    if (ep) *ep = '/';

    if (de) return TRUE;
    return FALSE;
}

#define GZIP_BLOCK_SIZE 8192

/* Try to load the normal or gzipped file "name" into FakeFileHandle "f",
   converting it into a ram-file allowing it to be crc'd.
   return 1 on success 0 otherwise. */
static int open_gzip_file (FakeFileHandle *f, char *name)
{
    int read;
    
#ifdef FILEIO_DEBUG
    fprintf(stderr_file, "Trying to open: %s\n", name);
#endif

    if(!filesearch(name))
       return 0;
       
    if((f->file = gzopen(name, "r")) == NULL)
       return 0;
    
    f->data   = malloc(GZIP_BLOCK_SIZE);
    if (!f->data) goto gzip_error;
    
    while ((read = gzread(f->file, f->data+f->length, GZIP_BLOCK_SIZE)) == GZIP_BLOCK_SIZE)
    {
        unsigned char *tmp;
        f->length += GZIP_BLOCK_SIZE;
        if (!(tmp = realloc(f->data, f->length + GZIP_BLOCK_SIZE)))
            goto gzip_error;
        f->data = tmp;
    }
    if (read == -1) goto gzip_error;
    f->length += read;
    f->crc  = crc32(0L, f->data, f->length);
    f->type = kRamFile;
    gzclose(f->file);
    return 1;
    
gzip_error:
    if (f->data) free(f->data);
    gzclose(f->file);
    memset(f,0,sizeof(FakeFileHandle));
    return 0;
}

/* Try to load "name" from the zipfile "zipname" into FakeFileHandle "f".
   return 1 on success 0 otherwise. */
static int open_zip_file (FakeFileHandle *f, char *zipname, const char *name)
{
    const char *my_name;
    
#ifdef FILEIO_DEBUG
    fprintf(stderr_file, "Trying to open: %s, in %s\n", name, zipname);
#endif

    if(!filesearch(zipname))
       return 0;
       
    if ( (my_name=strrchr(name, '/')) )
       my_name = my_name + 1;
    else
       my_name = name;
       
    if(load_zipped_file(zipname, my_name, &f->data, &f->length) == 0)
    {
	f->crc  = crc32(0L, f->data, f->length);
	f->type = kRamFile;
	f->file = (FILE *)-1;
	return 1;
    }
    
    memset(f,0,sizeof(FakeFileHandle));
    return 0;
}

#ifdef MESS
/* Try to load the normal file "name" into FakeFileHandle "f".
   return 1 on success 0 otherwise. */
static int open_normal_file (FakeFileHandle *f, char *name, int write)
{
#ifdef FILEIO_DEBUG
    fprintf(stderr_file, "Trying to open: %s\n", name);
#endif

    switch(write)
    {
       case OSD_FOPEN_READ:
          if (filesearch(name))
             f->file = fopen(name, "r");
          break;
       case OSD_FOPEN_WRITE:
          /* try to overwrite the same name with a different case,
             if we're faking case insensitivity, fake it all the way ;) */
          filesearch(name);
          f->file = fopen(name, "w");
          break;
       case OSD_FOPEN_RW:
          if (filesearch(name))
             f->file = fopen(name, "r+");
          break;
       case OSD_FOPEN_RW_CREATE:
          if (filesearch(name))
             f->file = fopen(name, "r+");
          else
             f->file = fopen(name, "w+");
          break;
    }

    if(f->file)
       return 1;
    
    return 0;
}
#endif


/* osd functions */

/*
 * check if roms/samples for a game exist at all
 * return 1 on success, otherwise 0
 */
int osd_faccess(const char *filename, int filetype)
{
	char name[MAXPATHL];
	int i;
	
	switch (filetype)
	{
		case OSD_FILETYPE_ROM:
		case OSD_FILETYPE_SAMPLE:
		    for(i=0;i<rompathc;i++)
		    {
			/* try filename.zip */
			snprintf(name, MAXPATHL, "%s/%s.zip", rompathv[i], filename);
			if (access(name, F_OK) == 0) return 1;
			
			/* try filename dir */
			snprintf(name, MAXPATHL, "%s/%s",rompathv[i],filename);
			if (access(name, F_OK) == 0) return 1;
		    }
		    break;
		case OSD_FILETYPE_SCREENSHOT:
		    snprintf(name, MAXPATHL, "%s/.%s%s/%s/%s.png", home_dir, NAME, VERSION, screenshot_dir, filename);
		    if (access(name, F_OK) == 0) return 1;
		    break;
	}
	
	return 0;
}

/*
 * file handling routines
 *
 * gamename holds the driver name, filename is only used for ROMs and samples.
 * if 'write' is not 0, the file is opened for write. Otherwise it is opened
 * for read.
 */
void *osd_fopen(const char *gamename, const char *filename, int filetype, 
     int write)
{
	char name[MAXPATHL];
	FakeFileHandle *f;
	int i;
	char *pt;

	f = (FakeFileHandle *)calloc(1, sizeof(FakeFileHandle));
	if (f == NULL) return f;
	
	switch (filetype)
	{
#ifdef MESS
		case OSD_FILETYPE_IMAGE_RW:
		    /* writable images are only supported as normal files */
		    if (write)
		    {
			/* try filename.ext */
			snprintf(name, MAXPATHL, "%s", filename);
			if (open_normal_file(f, name, write))
			    break;
			for(i=0; i < rompathc; i++)
			{
			    /* try <rompath>/filename.ext */
			    snprintf(name, MAXPATHL, "%s/%s", rompathv[i], filename);
			    if (open_normal_file(f, name, write))
				break;
			    
			    /* try <rompath>/<systemname>/filename.ext */
			    snprintf(name, MAXPATHL, "%s/%s/%s", rompathv[i], gamename,
			        filename);
			    if (open_normal_file(f, name, write))
				break;;
			}
		        break;
		    }
		    /* fall through for non writable images */
		case OSD_FILETYPE_IMAGE_R:
		    if (write)
		    {
			logerror("Error trying to open ro image %s in write mode\n", filename);
			break;
		    }
		    /* try relative and absolute filenames */
		    
		    /* try filename.ext */
		    snprintf(name, MAXPATHL, "%s", filename);
		    if (open_gzip_file(f, name))
		        break;
		    
		    /* try filename.ext.gz */
		    snprintf(name, MAXPATHL, "%s.gz", filename);
		    if (open_gzip_file(f, name))
		        break;
		    
		    /* try filename.zip */
		    snprintf(name, MAXPATHL, "%s", filename);
		    if ( (pt=strrchr(name, '.')) ) *pt = 0;
		    strncat(name, ".zip", (MAXPATHL - 1) - strlen(name));
		    if (open_zip_file(f, name, filename))
		        break;
		    /* fall through */
#endif
		case OSD_FILETYPE_ARTWORK:
		    if (write)
		    {
			logerror("Error trying to open ro image %s in write mode\n", filename);
			break;
		    }
		    for(i=0; i < rompathc; i++)
		    {
		        /* try <rompath>/filename.ext */
			snprintf(name, MAXPATHL, "%s/%s", rompathv[i], filename);
			if (open_gzip_file(f, name))
			    break;
			
			/* try <rompath>/filename.ext.gz */
			snprintf(name, MAXPATHL, "%s/%s.gz", rompathv[i], filename);
			if (open_gzip_file(f, name))
			    break;
			
		        /* try <rompath>/filename.zip */
		        snprintf(name, MAXPATHL, "%s/%s", rompathv[i], filename);
		        if ( (pt=strrchr(name, '.')) ) *pt = 0;
			strncat(name, ".zip", (MAXPATHL - 1) - strlen(name));
		        if (open_zip_file(f, name, filename))
		            break;
		    }
		    if(f->file)
			break;
		    /* fall through */
		case OSD_FILETYPE_ROM:
		case OSD_FILETYPE_SAMPLE:
		    if (write)
		    {
			logerror("Error trying to open rom/sample %s in write mode\n", filename);
			break;
		    }
		    
		    for(i=0; i < rompathc; i++)
		    {
			/* try <rompath>/gamename.zip */
			snprintf(name, MAXPATHL, "%s/%s.zip", rompathv[i], gamename);
			if (open_zip_file(f, name, filename))
			    break;
			
		        /* try <rompath>/<gamename>/filename.ext */
			snprintf(name, MAXPATHL, "%s/%s/%s", rompathv[i], gamename,
			    filename);
			if (open_gzip_file(f, name))
			    break;
			
			/* try <rompath>/<gamename>/filename.ext.gz */
			snprintf(name, MAXPATHL, "%s/%s/%s.gz", rompathv[i], gamename,
			    filename);
			if (open_gzip_file(f, name))
			    break;
			
			/* really only usefull for mess, but to keep
			   both the src and the docs clean we try it always */
		        /* try <rompath>/<gamename>/filename.zip */
		        snprintf(name, MAXPATHL, "%s/%s/%s", rompathv[i], gamename,
		            filename);
		        if ( (pt=strrchr(name, '.')) ) *pt = 0;
			strncat(name, ".zip", (MAXPATHL - 1) - strlen(name));
		        if (open_zip_file(f, name, filename))
		            break;
		    }
		    break;
		case OSD_FILETYPE_CONFIG:
		    snprintf(name, MAXPATHL, "%s/.%s%s/cfg/%s.cfg", home_dir, NAME, VERSION, gamename);
		    f->file = fopen(name,write ? "w" : "r");
		    break;
		case OSD_FILETYPE_STATE:
		    snprintf(name, MAXPATHL, "%s/.%s%s/sta/%s-%s.sta", home_dir, NAME, VERSION, gamename, filename);
		    f->file = fopen(name,write ? "w" : "r");
		    break;
		case OSD_FILETYPE_NVRAM:
		    snprintf(name, MAXPATHL, "%s/.%s%s/nvram/%s.nv", home_dir, NAME, VERSION, gamename);
		    f->file = fopen(name,write ? "w" : "r");
		    break;
		case OSD_FILETYPE_MEMCARD:
		    snprintf(name, MAXPATHL, "%s/.%s%s/mem/%s.mem", home_dir, NAME, VERSION, filename);
		    f->file = fopen(name,write ? "w" : "r");
		    break;
		case OSD_FILETYPE_HIGHSCORE:
		    if (mame_highscore_enabled())
		    {
			snprintf(name, MAXPATHL, "%s/.%s%s/%s/%s.hi", home_dir, NAME, VERSION, spooldir, gamename);
			f->file = fopen(name,write ? "w" : "r");
		    }
		    break;
		case OSD_FILETYPE_SCREENSHOT:
		    /* only for writing */
		    if (!write) break;
		    
		    snprintf(name, MAXPATHL, "%s/.%s%s/%s/%s.png", home_dir, NAME, VERSION, screenshot_dir, filename);
		    f->file = fopen(name, "w");
		    break;
		case OSD_FILETYPE_INPUTLOG:
		    f->file = fopen(filename,write ? "w" : "r");
		    break;
		case OSD_FILETYPE_HIGHSCORE_DB:
		case OSD_FILETYPE_HISTORY:
		    /* only for reading */
		    if (write) break;

		    f->file = fopen (filename, "r");
		    break;
		case OSD_FILETYPE_CHEAT:
		    /* only for reading */
		    if (write) break;
#ifdef MESS
		    snprintf(name, MAXPATHL, "%s/%s", cheatdir, filename);
		    f->file = fopen (name, "r");
#else
		    f->file = fopen (filename, "r");
#endif
		    break;
		case OSD_FILETYPE_LANGUAGE:
		    /* only for reading */
		    if (write) break;
		    
		    snprintf (name, MAXPATHL, "%s.lng", filename);
		    f->file = fopen (name, "r");
	}

	if (f->file == NULL)
	{
		free(f); 
		return NULL;
	}
	
	return f;
}

int osd_fread(void *file,void *buffer,int length)
{
	FakeFileHandle *f = (FakeFileHandle *)file;

	switch (f->type)
	{
		case kPlainFile:
			return fread(buffer,1,length,f->file);
			break;
		case kRamFile:
			/* reading from the uncompressed image of a zipped file */
			if (f->data)
			{
				if (length + f->offset > f->length)
					length = f->length - f->offset;
				memcpy(buffer, f->offset + f->data, length);
				f->offset += length;
				return length;
			}
			break;
	}

	return 0;
}

int osd_fread_swap(void *file,void *buffer,int length)
{
	int i;
	unsigned char *buf;
	unsigned char temp;
	int res;

	res = osd_fread(file,buffer,length);

	buf = buffer;
	for (i = 0;i < length;i+=2)
	{
		temp = buf[i];
		buf[i] = buf[i+1];
		buf[i+1] = temp;
	}

	return res;
}

int osd_fread_scatter(void *file,void *buffer,int length,int increment)
{
	unsigned char *buf = buffer;
	FakeFileHandle *f = (FakeFileHandle *)file;
	unsigned char tempbuf[4096];
	int totread,r,i;

	switch (f->type)
	{
		case kPlainFile:
			totread = 0;
			while (length)
			{
				r = length;
				if (r > 4096) r = 4096;
				r = fread(tempbuf,1,r,f->file);
				if (r == 0) return totread;	/* error */
				for (i = 0;i < r;i++)
				{
					*buf = tempbuf[i];
					buf += increment;
				}
				totread += r;
				length -= r;
			}
			return totread;
			break;
		case kRamFile:
			/* reading from the RAM image of a file */
			if (f->data)
			{
				if (length + f->offset > f->length)
					length = f->length - f->offset;
				for (i = 0;i < length;i++)
				{
					*buf = f->data[f->offset + i];
					buf += increment;
				}
				f->offset += length;
				return length;
			}
			break;
	}

	return 0;
}

int osd_fwrite(void *file,const void *buffer,int length)
{
	FakeFileHandle *f = (FakeFileHandle *)file;

	switch (f->type)
	{
		case kPlainFile:
			return fwrite(buffer,1,length,f->file);
		default:
			return -1; /* note dos returns 0, but this is incorrect */
	}
}

int osd_fwrite_swap(void *file,const void *buffer,int length)
{
	int i;
	unsigned char *buf;
	unsigned char temp;
	int res;

	buf = (unsigned char *)buffer;
	for (i = 0;i < length;i+=2)
	{
		temp = buf[i];
		buf[i] = buf[i+1];
		buf[i+1] = temp;
	}

	res = osd_fwrite(file,buffer,length);

	for (i = 0;i < length;i+=2)
	{
		temp = buf[i];
		buf[i] = buf[i+1];
		buf[i+1] = temp;
	}

	return res;
}

int osd_fseek(void *file,int offset,int whence)
{
	FakeFileHandle *f = (FakeFileHandle *)file;

	switch (f->type)
	{
		case kPlainFile:
			return fseek(((FakeFileHandle *)file)->file,offset,whence);
			break;
		case kRamFile:
			/* seeking within the uncompressed image of a zipped file */
			switch (whence)
			{
				case SEEK_SET:
					f->offset = offset;
					return 0;
					break;
				case SEEK_CUR:
					f->offset += offset;
					return 0;
					break;
				case SEEK_END:
					f->offset = f->length + offset;
					return 0;
					break;
			}
			break;
	}

	return -1;
}

int osd_fgetc(void *file)
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	if (f->type == kPlainFile && f->file)
		return fgetc(f->file);
	else
		return EOF;
}

int osd_ungetc(int c, void *file)
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	if (f->type == kPlainFile && f->file)
		return ungetc(c,f->file);
	else
		return EOF;
}

char *osd_fgets(char *s, int n, void *file)
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	if (f->type == kPlainFile && f->file)
		return fgets(s,n,f->file);
	else
		return NULL;
}

int osd_feof(void *file)
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	if (f->type == kPlainFile && f->file)
		return feof(f->file);
	else
		return 1;
}

int osd_ftell(void *file)
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	if (f->type == kPlainFile && f->file)
		return ftell(f->file);
	else
		return -1L;
}

void osd_fclose(void *file)
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	switch(f->type)
	{
		case kPlainFile:
			fclose(f->file);
			break;
		case kRamFile:
			if (f->data)
				free(f->data);
			break;
	}
	free(f);
}

int osd_fsize(void *file)
{
	int position, end;
	FakeFileHandle *f = (FakeFileHandle *) file;

	switch(f->type)
	{
		case kPlainFile:
			position = ftell(f->file);
			fseek(f->file, 0, SEEK_END);
			end = ftell(f->file);
			fseek(f->file, position, SEEK_SET);
			return end;
			break;
		case kRamFile:
			return f->length;
			break;
	}
	
	return 0;
}

unsigned int osd_fcrc (void *file)
{
	FakeFileHandle *f = (FakeFileHandle *)file;
	return f->crc;
}

int osd_fchecksum (const char *game, const char *filename, unsigned int *length, unsigned int *sum)
{
  char name[MAXPATHL];
  FakeFileHandle *f;
  int i;
  
  for(i=0;i<rompathc;i++)
  {
    snprintf(name, MAXPATHL, "%s/%s.zip",rompathv[i], game);

    if (access(name, R_OK)==0)
    {
      if (checksum_zipped_file(name, filename, length, sum) == 0)
        return 0;
    }
  }

  f = osd_fopen(game, filename, OSD_FILETYPE_ROM, 0);
  if (f==NULL) return -1;
  *sum    = osd_fcrc(f);
  *length = osd_fsize(f);
  osd_fclose(f);
  return 0;
}

/* called while loading ROMs. It is called a last time with name == 0 to signal */
/* that the ROM loading process is finished. */
/* return non-zero to abort loading */
int osd_display_loading_rom_message(const char *name,int current,int total)
{
	static int count = 0;
	
	if (name)
		fprintf(stderr_file,"loading rom %d: %-12s\n", count, name);
	else
		fprintf(stderr_file,"done\n");
	
	fflush(stderr_file);
	count++;

	return 0;
}

void logerror(const char *text, ...)
{
	va_list arg;
	
	if (errorlog)
	{
		va_start(arg, text);
		vfprintf(errorlog, text, arg);
		va_end(arg);
		fflush(errorlog);
	}
}

#ifdef MESS
int osd_select_file(int sel, char *filename)
{
	return 0;
}
#endif
