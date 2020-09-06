/**************************************************************

Vectrex game list creator for USB-DVG VMMenu
Code by Mario Montminy, Aug 2020

Modifications by Chad Gray, Aug 2020

Usage: cartlist <vectrex rom directory> >> vmmenu.ini

You *WILL* have to edit the resulting file and clean up some of
the names, as the game name is not always presented in the ROM,
and for homebrew free text is often used instead.

***************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>
#include <dirent.h>
#include <stdbool.h>

#define MAX_FILENAME      128
#define MAX_COPYRIGHT_STR 32
#define MAX_GAME_STR      32
#define MAX_HEADER_SIZE   0x100

#define ROMDIR  "/usr/local/share/advance/image/vectrex"

typedef struct
{
    char    filename[MAX_FILENAME];
    char    copyright[MAX_COPYRIGHT_STR];
    char    game[MAX_GAME_STR];
    char    game_clone[MAX_GAME_STR + 4];
} vectrex_rom_info_t;

static char s_buf[MAX_HEADER_SIZE];


/**********************************************************
 Convert string to (Upper) Camel Case
***********************************************************/
void CamelCase(char line[])
{
   bool active = true;
   for (int i = 0; line[i] != '\0'; i++)
   {
      if (isalpha(line[i]))
      {
         if(active)
         {
            line[i] = toupper(line[i]);
            active = false;
         }
         else line[i] = tolower(line[i]);
      }
      else if(line[i] == ' ') active = true;
   }
}


/**********************************************************
 Open ROM file and attempt to read the game name
 Name should start at offset 17
 0x80,0xF8 represents a line break
 0x80,0x00 marks the end of string
***********************************************************/
static int get_vectrex_rom_info(uint8_t *buf, uint32_t size, vectrex_rom_info_t *p_rom_info)
{
   int      result = -1, state, str_size, ended, check_end;
   uint32_t  skip_cnt;
   char    *p_str;

   if (size < 9)
   {
      // printf( "parse_rom_header: rom size too small.\n");
      return result;
   }
   ended     = 0;
   state     = 0;
   skip_cnt  = 0;
   str_size  = MAX_GAME_STR;
   p_str     = p_rom_info->game;
   check_end = 0;

   buf+=17;                               // Skip forward to game name offset

   while (str_size > 1 && !ended)
   {
      if (*buf == 0x00 && check_end)      // End of string has been detected (0x80,0x00)
      {
         ended = 1;
      }
      if (*buf == 0xf8 && check_end)      // Line break has been found (0x80,0xF8)
      {
         buf +=3;                         // Skip ctrl chars
         *buf = 0x20;                     // Insert a space
      }
      if (*buf == 0x80)                   // Potential end of string detected...
      {
         check_end = 1;                   // ..perform checks on next byte next time around
      }
      else
      {
         check_end = 0;
      }

      if (str_size)
      {
         *p_str = 0;
         if (isprint(*buf))
         {
            *p_str++ = *buf;
            str_size--;
            buf++;
         }
         else
         {
            buf++;                        // Skip over non-printable characters
         }
      }
   }
   //if (str_size == 1) printf("Name truncated...\n");

   CamelCase(p_rom_info->game);
   char name[MAX_GAME_STR];
   int first = 1, i, len, cnt, ch;
   memset(p_rom_info->game_clone, 0, sizeof(p_rom_info->game_clone));
   len = strlen(p_rom_info->game);
   cnt = 0;
   for (i = 0 ; i < len ; i++)
   {
      ch = tolower(p_rom_info->game[i]);
      if ((ch == ' ') || ((ch >= 'a') && (ch <= 'z')))
      {
         name[cnt++] = ch;
      }
   }
   name[cnt] = 0;
   strcpy(p_rom_info->game_clone, "v");
   p_str = strtok(name, " ");
   while (p_str)
   {
      if (!first)
      {
         p_str[1] = 0;
      }
      strcat(p_rom_info->game_clone, p_str);
      p_str = strtok(NULL, " ");
      first = 0;
   }
   result = 0;

   return result;
}



/**********************************************************
 Read ROM directory and parse results
***********************************************************/
int main(int argc, char **argv)
{
   int             result;
   char           *dir_str = ROMDIR;
   FILE           *fp = NULL;
   DIR            *d = NULL;
   struct dirent  *dir = NULL;
   char            file_name[2 * MAX_FILENAME];
   vectrex_rom_info_t rom;

   argc--;
   argv++;
   if (argc)
   {
      dir_str = *argv;
   }
   else
   {
      dir_str = ROMDIR;
      printf("Using default ROM DIR: %s\n", dir_str);
   }
   d = opendir(dir_str);
   if (d == NULL)
   {
      printf("Unable to open ROM Directory: %s\nUsage: cartlist romdir\n", ROMDIR);
      goto END;
   }
   dir = readdir(d);
   while (dir)
   {
      memset(&rom, 0, sizeof(rom));
      memset(file_name, 0, sizeof(file_name));
      if (dir->d_type == DT_REG) // We only want to look at regular files
      {
         strncpy(rom.filename, dir->d_name, sizeof(rom.filename) - 1);
         strncpy(file_name, dir_str, sizeof(file_name) - 1);
         strncat(file_name, "/", sizeof(file_name) - strlen(file_name) - 1);
         strncat(file_name, dir->d_name, sizeof(file_name) - strlen(file_name) - 1);
         fp = fopen(file_name, "rb");
         if (fp == NULL)
         {
            // printf("fopen(\"%s\"): %s\n", file_name, strerror(errno));
            goto SKIP;
         }
         if (fread(s_buf, 1, MAX_HEADER_SIZE, fp) != MAX_HEADER_SIZE)
         {
            // printf("fread() != %d : %s\n", MAX_HEADER_SIZE, strerror(errno));
            goto SKIP;
         }
         result = get_vectrex_rom_info(s_buf, MAX_HEADER_SIZE, &rom);
         result = 0;
         if (result == 0)
         {
            if (rom.game[0] && rom.game[0] != ' ')
            {
               printf("Vectrex|%s|%.8s|%s\n", rom.game, rom.game_clone, rom.filename);
            }
         }
         else
         {
            //printf( "Invalid ROM found: \"%s\". Skipping to next.\n", dir->d_name);
         }
      }
SKIP:
      if (fp)
      {
         fclose(fp);
         fp = NULL;
      }
      dir = readdir(d);
   }

   if (d)
   {
      closedir(d);
      d = NULL;
   }

END:
   if (fp)
   {
      fclose(fp);
   }
   if (d)
   {
      closedir(d);
   }
   return 0;
}

