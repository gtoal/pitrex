/******************************************************************
* Vector Mame Menu - Edit game List Functions
*
* Author:  Chad Gray
* Created: 08 Sep 19
*
* History: Initial version
*
*
*******************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <editlist.h>


/******************************************************************
   Build Games List from ini file, include all games
*******************************************************************/
list_node* build_games_list(void)
{
   char        temp[260];
   char        *nl;
   char        smanuf[30], sdesc[60], smame[128], sclone[128];
   char        *manuf=smanuf, *desc=sdesc, *mame=smame, *clone=sclone;
   int         hidden, total=0, i=0, len=0;
   FILE        *fp;
   list_node   *list_root = NULL, *list_cursor = NULL, *list_new = NULL;

   fp = fopen ("vmmenu.ini","rt" );
   if (fp == NULL)
   {
      printf("* Fatal Error - Unable to open menu file vmmenu.ini\n");
      printf("Please run makeini.exe >vmmenu.ini\n");
      exit(1) ;
   }
#ifdef DEBUG
   printf("Building games list...\n");
#endif
   while (fgets(temp, 250, fp) != NULL)
   {
      total++;
      if ((strlen(temp) > 1) && ((strchr(temp, '#') - temp) != 0 ))
//      if ((strlen(temp) > 1) && ((strchr(temp, '#') - temp) != 0 ) && (strchr(temp, '|') != 0))
      {
         hidden=0;      // Game is not commented out
      }
      else
      {
         len = strlen(temp);
         for (i=0; i<len; i++)
         {
            temp[i] = temp[i+1];
         }
         hidden=1;
      }
      if ((strlen(temp) > 1) && (strchr(temp, '|') != 0))
      {
         manuf = strtok(temp, "|");
         desc = strtok(NULL, "|");
         mame = strtok(NULL, "|");
         clone = strtok(NULL, "|");

         nl = strrchr(clone, '\r');
         if (nl) *nl = '\0';
         nl = strrchr(clone, '\n');
         if (nl) *nl = '\0';

         //printf("Hidden:%s Name:%s\n", (hidden==1 ? "Y": "N"), desc);

         list_new=add_list_game(hidden, manuf, desc, mame, clone);
         if (list_root == NULL)              // if the list is null
         {
            list_root = list_new;            // this is the first item so point root to it
            list_cursor = list_new;          // also initialise the cursor here
         }
         else
         {
            list_cursor->next = list_new;    // link the current game to the newly added game
            list_new->prev = list_cursor;    // link the newly added game to the previous game
         }
         list_cursor = list_new;             // update the cursor to the new record
      }
   }
   fclose(fp);
   list_root->prev=list_cursor;              // link the last item to the first as the previous game
   list_cursor->next=list_root;              // link the first item to the last as the next game
#ifdef DEBUG
   printf("Show/Hide list: there are %i games.\n", total);
#endif
   //dump_list(list_root);
   return list_root;
}


/**************************************
   Create a new game record
**************************************/
list_node* add_list_game(int hidden, char *manuf, char *gamename, char *mamename, char *clonename)
{
   list_node   *newrec;
   newrec = (list_node *)malloc(sizeof(list_node));   // create a new game record
   strcpy(newrec->manuf, manuf);
   strcpy(newrec->desc, gamename);
   strcpy(newrec->parent, mamename);
   strcpy(newrec->clone, clonename);
   newrec->next = NULL;
   newrec->prev = NULL;
   newrec->hidden = hidden;
   return newrec;
}


/**************************************
   Print linked list to console
**************************************/
void dump_list(list_node *list_root)
{
   int i=1;
   list_node *list_cursor=list_root;
   do
   {
      printf("%i: [%s] %s %s\n", i, (list_cursor->hidden ==1 ? "Hide":"Show"), list_cursor->manuf, list_cursor->desc);
      list_cursor=list_cursor->next;
      i++;
   }
   while (list_cursor != list_root);
}


/**************************************
   Write linked list to vmmenu.ini
**************************************/
void write_list(list_node *list_root)
{
   FILE   *fp;
   int    i=1, enabled=0;
   list_node *list_cursor=list_root;
   // Check we haven't disabled ALL games...
   do
   {
      if (list_cursor->hidden==0)
         enabled=1;
      list_cursor=list_cursor->next;
      i++;
   }
   while (list_cursor != list_root);
   if (enabled==0)
      list_root->hidden=0;

   fp = fopen ("vmmenu.ini","w" );
   if (fp == NULL)
   {
      printf("* Fatal Error - Unable to open menu file vmmenu.ini\n");
      printf("Please run makeini.exe >vmmenu.ini\n");
      exit(1) ;
   }
   printf("Writing vmmenu.ini file...\n");

   i=1;
   do
   {
      //printf("%i: [%s] %s %s\n", i, (list_cursor->hidden ==1 ? "Hide":"Show"), list_cursor->manuf, list_cursor->desc);
      fputs((list_cursor->hidden==1 ? "#" : ""), fp);
      fputs(list_cursor->manuf, fp);
      fputs("|", fp);
      fputs(list_cursor->desc, fp);
      fputs("|", fp);
      fputs(list_cursor->parent, fp);
      fputs("|", fp);
      fputs(list_cursor->clone, fp);
      fputs("\n", fp);

      list_cursor=list_cursor->next;
      i++;
   }
   while (list_cursor != list_root);
   fclose(fp);
}

