/*
 * X-mame config-file and commandline parsing
 * We don't use stderr_file resp stdout_file in here since we don't know if 
 * it's valid yet.
 */

#define __CONFIG_C_
#include <time.h>
#include "xmame.h"
#include "driver.h"
#include "audit.h"
#include "sysdep/sysdep_dsp.h"
#include "sysdep/sysdep_mixer.h"
#include "sysdep/misc.h"

/* from ... */
extern char *cheatfile;
extern char *db_filename;
extern char *history_filename;
extern char *mameinfo_filename;
#if (HAS_CYCLONE)
int use_cyclone;
#endif
#if (HAS_DRZ80)
int use_drz80;
int use_drz80_audio;
#endif
int skip_warnings;
int skip_disclaimer;

#ifdef SPEEDHACK
static int speedhack = 0;
#endif

/* some local vars */
static int showconfig = 0;
static int showmanusage = 0;
static int showversion = 0;
static int showusage  = 0;
static int use_fuzzycmp = 1;
static int loadconfig = 1;
static char *language = NULL;
static char *gamename = NULL;
#ifndef MESS
static char *defaultgamename;
#else
static int iodevice_type = 0;
static char crcfilename[BUF_SIZE] = "";
const char *crcfile = crcfilename;
static char pcrcfilename[BUF_SIZE] = "";
const char *pcrcfile = pcrcfilename;
#endif
static struct rc_struct *rc;

static int config_handle_arg(char *arg);
static int config_handle_debug_size(struct rc_option *option, const char *arg,
   int priority);
void show_usage(void);

/* struct definitions */
static struct rc_option opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { NULL,		NULL,			rc_link,	video_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { NULL,		NULL,			rc_link,	sound_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { NULL,		NULL,			rc_link,	input_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { NULL,		NULL,			rc_link,	network_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { NULL,		NULL,			rc_link,	fileio_opts,
     NULL,		0,			0,		NULL,
     NULL },
#ifdef MESS
   { "Mess Related",	NULL,			rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "cartridge",	"cart",			rc_set_int,	&iodevice_type,
     NULL,		IO_CARTSLOT,		0,		NULL,
     "All following images are seen as cartridges/roms" },
   { "floppydisk",	"flop",			rc_set_int,	&iodevice_type,
     NULL,		IO_FLOPPY,		0,		NULL,
     "All following images are seen as floppies" },
   { "harddisk",	"hard",			rc_set_int,	&iodevice_type,
     NULL,		IO_HARDDISK,		0,		NULL,
     "All following images are seen as hard disks" },
   { "cylinder",	"cyln",			rc_set_int,	&iodevice_type,
     NULL,		IO_CYLINDER,		0,		NULL,
     "All following images are seen as cylinders" },
   { "cassette",	"cass",			rc_set_int,	&iodevice_type,
     NULL,		IO_CASSETTE,		0,		NULL,
     "All following images are seen as cassettes" },
   { "punchcard",   "pcrd",			rc_set_int,     &iodevice_type,
     NULL,		IO_PUNCHCARD,		0,		NULL,
     "All following images are seen as punchcards" },
   { "punchtape",   "ptap",			rc_set_int,     &iodevice_type,
     NULL,		IO_PUNCHTAPE,		0,		NULL,
     "All following images are seen as punchtapes" },
   { "printer",		"prin",			rc_set_int,	&iodevice_type,
     NULL,		IO_PRINTER,		0,		NULL,
     "All following images are seen as printers" },
   { "serial",		"serl",			rc_set_int,	&iodevice_type,
     NULL,		IO_SERIAL,		0,		NULL,
     "All following images are seen as serial ports" },
   { "parallel",    "parl",			rc_set_int,     &iodevice_type,
     NULL,		IO_PARALLEL,		0,		NULL,
     "All following images are seen as parallel ports" },
   { "snapshot",    "dump",			rc_set_int,     &iodevice_type,
     NULL,		IO_SNAPSHOT,		0,		NULL,
     "All following images are seen as snapshots" },
   { "quickload",   "quik",			rc_set_int,     &iodevice_type,
     NULL,		IO_QUICKLOAD,		0,		NULL,
     "All following images are seen as quickload images" },
#else
   { "Mame Related",	NULL,			rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "defaultgame",	"def",			rc_string,	&defaultgamename,
     "pacman",		0,			0,		NULL,
     "Set the default game started when no game is given on the commandline, only useful for in the configfiles" },
#endif
   { "language",	"lang",			rc_string,	&language,
     "english",		0,			0,		NULL,
     "Select the language for the menus and osd" },
   { "fuzzycmp",	"fc",			rc_bool,	&use_fuzzycmp,
     "1",		0,			0,		NULL,
     "Enable/disable use of fuzzy gamename matching when there is no exact match" },
   { "cheat",		"c",			rc_bool,	&options.cheat,
     "0",		0,			0,		NULL,
     "Enable/disable cheat subsystem" },
#ifdef SPEEDHACK
   { "speedhack",	"sh",			rc_int,		&speedhack,
     "0",		0,			3,		NULL,
     "Enable speedhacks 1=90% CPU, 2=80% CPU, 3=70% CPU underclock" },
#endif
#if (HAS_CYCLONE)
   { "cyclone",		NULL,			rc_bool,	&use_cyclone,
     "0",		0,			0,		NULL,
     "Use Cyclone in lieu of the regular MAME MC68000/MC68010 core" },
#endif
#if (HAS_DRZ80)
   { "drz80",		NULL,			rc_bool,	&use_drz80,
     "0",		0,			0,		NULL,
     "Use DRZ80 in lieu of the regular MAME Z80 core" },
   { "drz80_audio",	NULL,			rc_bool,	&use_drz80_audio,
     "0",		0,			0,		NULL,
     "Use DRZ80 in lieu of the regular MAME Z80 core (AUDIO CPU only)" },
#endif
   { "skip_disclaimer",	NULL,			rc_bool,	&skip_disclaimer,
     "0",		0,			0,		NULL,
     "Skip displaying the disclaimer screen" },
   { "skip_warnings",	NULL,			rc_bool,	&skip_warnings,
     "0",		0,			0,		NULL,
     "Skip displaying the warnings and game info screen" },
#ifdef MAME_DEBUG     
   { "debug",		"d",			rc_bool,	&options.mame_debug,
     NULL,		0,			0,		NULL,
     "Enable/disable debugger" },
   { "debug-size",	"ds",			rc_use_function, NULL,
     "640x480",		0,			0,		config_handle_debug_size,
     "Specify the resolution/windowsize to use for the debugger(window) in the form of XRESxYRES (minimum size = 640x480)" },
#endif
   { NULL,		NULL,			rc_link,	frontend_list_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { NULL,		NULL,			rc_link,	frontend_ident_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { "General Options",	NULL,			rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "loadconfig",	"lc",			rc_bool,	&loadconfig,
     "1",		0,			0,		NULL,
     "Load (don't load) configfiles" },
   { "showconfig",	"sc",			rc_set_int,	&showconfig,
     NULL,		1,			0,		NULL,
     "Display running parameters in rc style" },
   { "manhelp",		"mh",			rc_set_int,	&showmanusage,
     NULL,		1,			0,		NULL,
     "Print commandline help in man format, useful for manpage creation" },
   { "version",		"V",			rc_set_int,	&showversion,
     NULL,		1,			0,		NULL,
     "Display version" },
   { "help",		"?",			rc_set_int,	&showusage,
     NULL,		1,			0,		NULL,
     "Show this help" },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

/* fuzzy string compare, compare short string against long string        */
/* e.g. astdel == "Asteroids Deluxe". The return code is the fuzz index, */
/* we simply count the gaps between maching chars.                       */
static int fuzzycmp (const char *s, const char *l)
{
   int gaps = 0;
   int match = 0;
   int last = 1;

   for (; *s && *l; l++)
   {
       if (*s == *l)
           match = 1;
       else if (*s >= 'a' && *s <= 'z' && (*s - 'a') == (*l - 'A'))
           match = 1;
       else if (*s >= 'A' && *s <= 'Z' && (*s - 'A') == (*l - 'a'))
           match = 1;
       else
           match = 0;

       if (match)
           s++;

       if (match != last)
       {
           last = match;
           if (!match)
               gaps++;
       }
   }

   /* penalty if short string does not completely fit in */
   for (; *s; s++)
       gaps++;

   return gaps;
}

/* for verify roms which is used for the random game selection */
static int config_printf(const char *fmt, ...)
{
   return 0;
}

static int config_handle_arg(char *arg)
{
   static int got_gamename = 0;
   
   if (!got_gamename) /* notice: for MESS game means system */
   {
      gamename     = arg;
      got_gamename = 1;
   }
   else
#ifdef MESS
   {
      if( options.image_count >= MAX_IMAGES )
      {
         fprintf(stderr, "error: too many image names specified!\n");
         return -1;
      }
      options.image_files[options.image_count].type = iodevice_type;
      options.image_files[options.image_count].name = arg;
      options.image_count++;
   }
#else
   {
      fprintf(stderr,"error: duplicate gamename: %s\n", arg);
      return -1;
   }
#endif

   return 0;
}

static int config_handle_debug_size(struct rc_option *option, const char *arg,
   int priority)
{
  int width, height;
  
  if (sscanf(arg, "%dx%d", &width, &height) == 2)
  {
     if((width >= 640) && (height >= 480))
     {
        options.debug_width  = width;
        options.debug_height = height;
        return 0;
     }
  }
  fprintf(stderr,
      "error: invalid debugger size or too small (minimum size = 640x480): \"%s\".\n",
      arg);
  return -1;
}


/*
 * get configuration from configfile and env.
 */
int config_init (int argc, char *argv[])
{
   char buffer[BUF_SIZE];
   unsigned char lsb_test[2]={0,1};
   INP_HEADER inp_header;
   int i;
   
   memset(&options,0,sizeof(options));
   
   /* Lett's see of the endians of this arch is correct otherwise
      YELL about it and bail out. */
#ifdef LSB_FIRST
   if(*((unsigned short*)lsb_test) != 0x0100)
#else	
   if(*((unsigned short*)lsb_test) != 0x0001)
#endif
   {
      fprintf(stderr, "error: compiled byte ordering doesn't match machine byte ordering\n"
         "are you sure you choose the right arch?\n"
#ifdef LSB_FIRST
         "compiled for lsb-first, are you sure you choose the right cpu in makefile.unix\n");
#else
         "compiled for msb-first, are you sure you choose the right cpu in makefile.unix\n");
#endif
      return OSD_NOT_OK;
   }

   /* some settings which are static for xmame and thus aren't controled
      by options */
#ifndef RASPI
   options.use_emulated_ym3812 = TRUE;
#endif
   options.gui_host = 1;
   cheatfile = NULL;
   db_filename = NULL;
   history_filename = NULL;
   mameinfo_filename = NULL;

   /* create the rc object */
   if (!(rc = rc_create()))
      return OSD_NOT_OK;
      
   if(sysdep_dsp_init(rc, NULL))
      return OSD_NOT_OK;
      
   if(sysdep_mixer_init(rc, NULL))
      return OSD_NOT_OK;
      
   if(rc_register(rc, opts))
      return OSD_NOT_OK;
   
   /* get the homedir */
   if(!(home_dir = rc_get_home_dir()))
      return OSD_NOT_OK;
   
   /* check the nescesarry dirs exist, and create them if nescesarry */
   snprintf(buffer, BUF_SIZE, "%s/.%s%s", home_dir, NAME, VERSION);
   if (rc_check_and_create_dir(buffer))
      return OSD_NOT_OK;
   snprintf(buffer, BUF_SIZE, "%s/.%s%s/%s", home_dir, NAME, VERSION, "cfg");
   if (rc_check_and_create_dir(buffer))
      return OSD_NOT_OK;
   snprintf(buffer, BUF_SIZE, "%s/.%s%s/%s", home_dir, NAME, VERSION, "mem");
   if (rc_check_and_create_dir(buffer))
      return OSD_NOT_OK;
   snprintf(buffer, BUF_SIZE, "%s/.%s%s/%s", home_dir, NAME, VERSION, "sta");
   if (rc_check_and_create_dir(buffer))
      return OSD_NOT_OK;
   snprintf(buffer, BUF_SIZE, "%s/.%s%s/%s", home_dir, NAME, VERSION, "nvram");
   if (rc_check_and_create_dir(buffer))
      return OSD_NOT_OK;
#ifdef RASPI
   snprintf(buffer, BUF_SIZE, "%s/.%s%s/%s", home_dir, NAME, VERSION, "snap");
   if (rc_check_and_create_dir(buffer))
      return OSD_NOT_OK;
   snprintf(buffer, BUF_SIZE, "%s/.%s%s/%s", home_dir, NAME, VERSION, "hi");
   if (rc_check_and_create_dir(buffer))
      return OSD_NOT_OK;
#endif
   snprintf(buffer, BUF_SIZE, "%s/.%s%s/%s", home_dir, NAME, VERSION, "rc");
   
   /* parse the commandline */
   if (rc_parse_commandline(rc, argc, argv, 2, config_handle_arg))
      return OSD_NOT_OK;
      
   /* parse the various configfiles, starting with the one with the
      lowest priority */
   if(loadconfig)
   {
      snprintf(buffer, BUF_SIZE, "%s/%s%src", XMAMEROOT, NAME, VERSION);
      if(rc_load(rc, buffer, 1, 1))
         return OSD_NOT_OK;
      snprintf(buffer, BUF_SIZE, "%s/.%s%s/%src", home_dir, NAME, VERSION, NAME);
      if(rc_load(rc, buffer, 1, 1))
         return OSD_NOT_OK;
      snprintf(buffer, BUF_SIZE, "%s/%s%s-%src", XMAMEROOT, NAME, VERSION, DISPLAY_METHOD);
      if(rc_load(rc, buffer, 1, 1))
         return OSD_NOT_OK;
      snprintf(buffer, BUF_SIZE, "%s/.%s%s/%s-%src", home_dir, NAME, VERSION, NAME,
         DISPLAY_METHOD);
      if(rc_load(rc, buffer, 1, 1))
         return OSD_NOT_OK;
   }
   
   /* setup stderr_file and stdout_file */
   if (!stderr_file) stderr_file = stderr;
   if (!stdout_file) stdout_file = stdout;
   
   if (showconfig)
   {
      rc_write(rc, stdout_file, NAME" running parameters");
      return OSD_OK;
   }
   
   if (showmanusage)
   {
      rc_print_man_options(rc, stdout_file);
      return OSD_OK;
   }
   
   if (showversion)
   {
      fprintf(stdout_file, "%s\n", title);
      return OSD_OK;
   }

   if (showusage)
   {
      show_usage();
      return OSD_OK;
   }
   
   /* must be done after showconfig, since this modifies the rompath rc_string,
      but before any of the frontend options are handled */
   init_rom_path();

   /* handle frontend options */
   if ( (i=frontend_list(gamename)) != 1234)
      return i;
   
   if ( (i=frontend_ident(gamename)) != 1234)
      return i;
   
   if (options.playback)
   {
      /* read the playback header */
      osd_fread(options.playback, &inp_header, sizeof(INP_HEADER));
      if (!isalnum(inp_header.name[0]))
      {
         /* old .inp file - no header */
         osd_fseek(options.playback, 0, SEEK_SET); 
         if(!gamename)
         {
            fprintf(stderr_file, "error: old type .inp file and no game specified\n");
            return OSD_NOT_OK;
         }
      }
      else
      {
         if(gamename)
         {
            if(strcmp(gamename, inp_header.name))
            {
               fprintf(stderr_file, "Error: Input file is for a different game as specified\n");
               return OSD_NOT_OK;
            }
            else
               fprintf(stderr_file, "Hint: with new .inp files you don't have to specify a game anymore\n");
         }
         gamename = inp_header.name;
      }
   }
   
   /* handle the game selection */
   game_index = -1;

   if (!gamename)
#ifdef MESS
   {
      show_usage();
      return OSD_NOT_OK;
   }
#else
      gamename = defaultgamename;

   /* random game? */
   if (strcasecmp(gamename, "random") == 0)
   {
      for (i=0; drivers[i]; i++) ; /* count available drivers */

      srand(time(NULL));
      
      for(;;)
      {
         game_index = (float)rand()*i/RAND_MAX;
      
         fprintf(stdout_file, "Random game selected: %s (%s)\n  verifying roms... ",drivers[game_index]->name,drivers[game_index]->description);
         if(VerifyRomSet (game_index, (verify_printf_proc)config_printf) == CORRECT)
         {
            fprintf(stdout_file, "OK\n");
            break;
         }
         else
            fprintf(stdout_file, "FAILED\n");
      }
   }
   else
#endif
   /* do we have a driver for this? */
   for (i = 0; drivers[i]; i++)
      if (strcasecmp(gamename,drivers[i]->name) == 0)
      {
         game_index = i;
         break;
      }

   /* educated guess on what the user wants to play */
   if ( (game_index == -1) && use_fuzzycmp)
   {
       int fuzz = 9999; /*best fuzz factor so far*/

       for (i = 0; (drivers[i] != 0); i++)
       {
           int tmp;
           tmp = fuzzycmp(gamename, drivers[i]->description);
           /* continue if the fuzz index is worse */
           if (tmp > fuzz)
               continue;
           /* on equal fuzz index, we prefear working, original games */
           if (tmp == fuzz)
           {
               /* game is a clone */
               if (drivers[i]->clone_of != 0 && !(drivers[i]->clone_of->flags & NOT_A_DRIVER))
               {
                   if ((!drivers[game_index]->flags & GAME_NOT_WORKING) || (drivers[i]->flags & GAME_NOT_WORKING))
                       continue;
               }
               else continue;
           }


           /* we found a better match */
           game_index = i;
           fuzz = tmp;
       }

       if (game_index != -1)
           fprintf(stdout_file,
              "fuzzy name compare, running %s\n", drivers[game_index]->name);
   }

   if (game_index == -1)
   {
      fprintf(stderr_file, "\"%s\" not supported\n", gamename);
      return OSD_NOT_OK;
   }
   
   /* now that we've got the gamename parse the game specific configfile */
   if (loadconfig)
   {
      snprintf(buffer, BUF_SIZE, "%s/rc/%src", XMAMEROOT,
         drivers[game_index]->name);
      if(rc_load(rc, buffer, 1, 1))
         return OSD_NOT_OK;
      snprintf(buffer, BUF_SIZE, "%s/.%s%s/rc/%src", home_dir, NAME, VERSION,
         drivers[game_index]->name);
      if(rc_load(rc, buffer, 1, 1))
         return OSD_NOT_OK;
   }
   
#ifdef MESS
   /* Build the CRC database filename */
   snprintf(crcfilename, BUF_SIZE, "%s/%s.crc", crcdir, drivers[game_index]->name);
   if(drivers[game_index]->clone_of &&
      !(drivers[game_index]->clone_of->flags & NOT_A_DRIVER) &&
      drivers[game_index]->clone_of->name)
   {
      snprintf(pcrcfilename, BUF_SIZE, "%s/%s.crc", crcdir,
         drivers[game_index]->clone_of->name);
   }
   
   /* set the image type if nescesarry */
   for(i=0; i<options.image_count; i++)
   {
      if(options.image_files[i].type)
      {
         logerror("User specified %s for %s\n",
               device_typename(options.image_files[i].type),
               options.image_files[i].name);
      }
      else
      {
         char *ext;
         char name[BUF_SIZE];
         
         /* make a copy of the name */
         strncpy(name, options.image_files[i].name, BUF_SIZE);
         /* strncpy is buggy */
         name[BUF_SIZE-1]=0;
         
         /* get ext, skip .gz */
         ext = strrchr(name, '.');
         if( ext && !strcmp(ext, ".gz") )
         {
            *ext = 0;
            ext = strrchr(name, '.');
         }
         
         /* Look up the filename extension in the drivers device list */
         if(ext && drivers[game_index]->dev)
         {
            const struct IODevice *dev = drivers[game_index]->dev;
            
            ext++; /* skip the "." */
            
            while (dev->type != IO_END)
            {
               const char *dst = dev->file_extensions;
               /* scan supported extensions for this device */
               while (dst && *dst)
               {
                  if (strcasecmp(dst,ext) == 0)
                  {
                     logerror("Extension match %s [%s] for %s\n",
                           device_typename(dev->type), dst,
                           options.image_files[i].name);
                           
                     options.image_files[i].type = dev->type;
                  }
                  /* skip '\0' once in the list of extensions */
                  dst += strlen(dst) + 1;
               }
               dev++;
            }
         }
         if(!options.image_files[i].type)
            options.image_files[i].type = IO_CARTSLOT;
      }
   }
#endif
   
   if (options.record)
   {
      memset(&inp_header, '\0', sizeof(INP_HEADER));
      strcpy(inp_header.name, drivers[game_index]->name);
      /* MAME32 stores the MAME version numbers at bytes 9 - 11
       * MAME DOS and xmame keeps this information in a rc_string, the
       * Windows code defines them in the Makefile.
      inp_header.version[0] = 0;
      inp_header.version[1] = VERSION;
      inp_header.version[2] = BETA_VERSION;
      */
      osd_fwrite(options.record, &inp_header, sizeof(INP_HEADER));
   }
   
   if(language)
      options.language_file = osd_fopen(0,language,OSD_FILETYPE_LANGUAGE,0);

#ifdef SPEEDHACK
    /* Underclock CPUs */
    if (speedhack)
    {

	float cpu_underclock;
	unsigned int number_of_cpus = cpu_gettotalcpu();

	switch (speedhack)
	{
	    case 1: cpu_underclock = 0.90; break;
	    case 2: cpu_underclock = 0.80; break;
	    case 3: cpu_underclock = 0.70; break;
	}
	

	for (i=0; i<number_of_cpus; i++)
	{
	    timer_set_overclock(i, cpu_underclock);
	}
    }
#endif

   return 1234;
}

void config_exit(void)
{
   if(rc)
   {
      sysdep_mixer_exit();
      sysdep_dsp_exit();
      rc_destroy(rc);
   }
   
   if(home_dir)
      free(home_dir);
      
   /* close open files */
   if (options.playback) osd_fclose (options.playback);
   if (options.record)   osd_fclose (options.record);
   if (options.language_file) osd_fclose (options.language_file);
}

/* 
 * show help and exit
 */
void show_usage(void) 
{
  /* header */
  fprintf(stdout_file, 
#ifdef MESS
     "Usage: xmess <system> [game] [options]\n"
#else
     "Usage: xmame [game] [options]\n"
#endif 
     "Options:\n");
  
  /* actual help message */
  rc_print_help(rc, stdout_file);
  
  /* footer */
  fprintf(stdout_file, "\nFiles:\n\n");
  fprintf(stdout_file, "Config Files are parsed in the following order:\n");
  fprint_colums(stdout_file, XMAMEROOT"/"NAME"rc",
     "Global configuration config file");
  fprint_colums(stdout_file, "${HOME}/."NAME"/"NAME"rc",
     "User configuration config file");
  fprint_colums(stdout_file, XMAMEROOT"/"NAME"-"DISPLAY_METHOD"rc",
     "Global per display method config file");
  fprint_colums(stdout_file, "${HOME}/."NAME"/"NAME"-"DISPLAY_METHOD"rc",
     "User per display method config file");
  fprint_colums(stdout_file, XMAMEROOT"/rc/<game>rc",
     "Global per game config file");
  fprint_colums(stdout_file, "${HOME}/."NAME"/rc/<game>rc",
     "User per game config file");
/*  fprintf(stdout_file, "\nEnvironment variables:\n\n");
  fprint_colums(stdout_file, "ROMPATH", "Rom search path"); */
  fprintf(stdout_file, "\n"
#ifdef MESS
     "M.E.S.S. - Multi-Emulator Super System\n"
     "Copyright (C) 1998-2001 by the MESS team\n"
#else
     "M.A.M.E. - Multiple Arcade Machine Emulator\n"
     "Copyright (C) 1997-2001 by Nicola Salmoria and the MAME Team\n"
#endif
     "%s port maintained by Lawrence Gold\n"
     "Raspberry Pi port by Slaanesh\n", NAME);

}

#ifdef MESS
/* Function to handle Aliases in the MESS.CFG file */
char* get_alias(const char *driver_name, char *argv)
{
	/* we don't do aliases, feel free to write support for it if you want
	   it */
	return NULL;
}
#endif
