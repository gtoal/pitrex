/**************************************
Gamelist.c
Chad Gray, 15 Nov 2009
Reads input file, creates a linked list
Provides all the functions required
to operate on the list
**************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gamelist.h>


/**************************************
      Print out the linked list
**************************************/
int printlist(m_node *list)
{
   int      m_total = 0, g_total = 0, c_total = 0;
   g_node   *game, *clone;
   while (list)
   {
      m_total ++;
      printf("Manufacturer: %s\n", list->name);
      game = list->firstgame;
      while(game)
      {
         g_total ++;
         printf("   * game: %s [%s]", game->name, game->clone);
         // if (game->prev) printf(" P:%s", game->prev->clone);
         // if (game->next) printf(" N:%s", game->next->clone);
         printf("\n");
         clone = game->nclone;
         while (clone)
         {
            c_total ++;
            printf("      \\ clone: %s [%s]\n", clone->name, clone->clone);
            clone = clone->nclone;
         }
         game = game->next;
      }
      list = list->nmanuf;
      printf("\n");
   }
   printf("There are %d manufacturers, %d original games and %d clones.\n", m_total, g_total, c_total);
   return(g_total+c_total);
}


/**************************************
    Link the last game to the first
**************************************/
void linklist(m_node *list)
{
   g_node   *firstgame, *lastgame;
   m_node   *firstman, *lastman;
   firstman = list;
   lastman  = gotolastmanuf(list);
   while (list)
   {
      firstgame = list->firstgame;
      lastgame = gotolastgame(firstgame);
      lastgame->next = firstgame;
      firstgame->prev = lastgame;
      list = list->nmanuf;
   }
   firstman->pmanuf = lastman;
   lastman->nmanuf = firstman;
}


/**************************************
   Create a new manufacturer record
**************************************/
m_node* add_manuf(char *manuf)
{
   m_node   *newrec;
   newrec = (m_node *)malloc(sizeof(m_node));
   strcpy(newrec->name, manuf);
   newrec->nmanuf = NULL;
   newrec->pmanuf = NULL;
   newrec->firstgame = NULL;
   return newrec;
}


/**************************************
   Create a new game or clone record
**************************************/
g_node* add_game(char *gamename, char *mamename, char *clonename)
{
   g_node   *newrec;
   newrec = (g_node *)malloc(sizeof(g_node));   // create a new game record
   strcpy(newrec->name, gamename);
   strcpy(newrec->parent, mamename);
   strcpy(newrec->clone, clonename);
   newrec->next = NULL;
   newrec->prev = NULL;
   newrec->nclone = NULL;
   newrec->pclone = NULL;
   return newrec;
}


/**************************************
Go to the last game in the list
**************************************/
g_node* gotolastgame(g_node *gamelist)
{
   if (gamelist)
   {
      while (gamelist->next != NULL)
         gamelist = gamelist->next;
   }
   else
      gamelist = NULL;
   return gamelist;
}

/**************************************
Find the given game in the list
**************************************/
g_node* findparentgame(g_node *gamelist, char *parent)
{
   int      found = 0;
   g_node   *here = NULL;
   while (gamelist)
   {
      if ((strcmp(gamelist->parent, parent) == 0)) //  && (strcmp(gamelist->parent, gamelist->clone) == 0)
      {
         found = 1;
         here = gamelist;
      }
      gamelist = gamelist->next;
   }
   if (found == 0)
      here = NULL;
   return here;
}


/**************************************
Go to a given manufacturer in the list
**************************************/
m_node* findmanuf(m_node *m_list, char *manufname)
{
   int      found = 0;
   m_node   *here = NULL;
   while (m_list)
   {
      if (strcmp(m_list->name, manufname) == 0)
      {
         found = 1;
         here = m_list;
      }
      m_list = m_list->nmanuf;
   }
   if (found == 0)
      here = NULL;
   return here;
}


/**************************************
Go to the last manufacturer in the list
**************************************/
m_node* gotolastmanuf(m_node *mlist)
{
   if (mlist)
   {
      while (mlist->nmanuf != NULL)
         mlist = mlist->nmanuf;
   }
   else
      mlist = NULL;
   return mlist;
}


/**************************************
   Go to the last clone in the list
**************************************/
g_node* gotolastclone(g_node *gamelist)
{
   if (gamelist)
   {
      while (gamelist->nclone != NULL)
         gamelist = gamelist->nclone;
   }
   else
      gamelist = NULL;
   return gamelist;
}


/**************************************
 Create games list from the input file
**************************************/
m_node* createlist()
{
   char     temp[260];
   char     *nl;
   char     smanuf[30], sdesc[60], smame[128], sclone[128];
   char     *manuf=smanuf, *desc=sdesc, *mame=smame, *clone=sclone;
   FILE     *fp;
   m_node   *man_root = NULL, *man_cursor = NULL, *man_last = NULL;
   g_node   *game_root = NULL, *game_cursor = NULL, *game_last = NULL;

   fp = fopen ("vmmenu.ini","rt" );
   if (fp == NULL)
   {
      printf("* Fatal Error - Unable to open menu file vmmenu.ini\n");
      printf("Please run makeini.exe >vmmenu.ini\n");
      exit(1) ;
   }
   while (fgets(temp, 250, fp) != NULL)
   {
      if ((strlen(temp) > 1) && ((strchr(temp, '#') - temp) != 0 ) && (strchr(temp, '|') != 0))
      {
         manuf = strtok(temp, "|");
         desc = strtok(NULL, "|");
         mame = strtok(NULL, "|");
         clone = strtok(NULL, "|");
         //printf("manuf: %s desc: %s mame: %s clone %s \n ",manuf, desc, mame, clone);
         nl = strrchr(clone, '\r');
         if (nl) *nl = '\0';
         nl = strrchr(clone, '\n');
         if (nl) *nl = '\0';

         man_cursor = findmanuf(man_root, manuf);
         if (man_cursor == NULL)
         {
            man_cursor = add_manuf(manuf);
            man_last = gotolastmanuf(man_root);
            if (man_last)
            {
               man_last->nmanuf = man_cursor;            // pevious last->next = this record
               man_cursor->pmanuf = man_last;
            }
            if (man_root == NULL)
               man_root = man_cursor;                    // this is the first item
         }

         // when we get to here, we're guaranteed the manufacturer has been added
         // man_cursor points to our manufacturer so now we need to add the game
         if  (strcmp(clone, mame) == 0)                  // original game to add
         {
            game_cursor = add_game(desc, mame, clone);
            // check a clone hasn't been added as a parent
            game_root = findparentgame(man_cursor->firstgame, mame);
            if (game_root)
            {
               game_cursor->nclone = game_root;          // point new record's clone field to the clone
               game_cursor->next = game_root->next;

               if (man_cursor->firstgame == game_root)   // we are at top of list...
                  man_cursor->firstgame = game_cursor;   // ...so make this the firstgame
               else
                  game_root->prev->next = game_cursor;   // else make prev game point to us as next in list

               if (game_root->next)
                  game_root->next->prev = game_cursor;
               game_root->next = NULL;
               game_cursor->prev = game_root->prev;
               game_root->prev = NULL;
            }
            else
            {
               game_root = man_cursor->firstgame;
               if (game_root == NULL)
                  man_cursor->firstgame = game_cursor;   // this is the first item
               else
               {
                  game_last = gotolastgame(game_root);
                  game_last->next = game_cursor;         // previous last->next = this record
                  game_cursor->prev = game_last;         // prev points to former last record
               }
            }
         }
         else  // add a clone
         {
            game_cursor = add_game(desc, mame, clone);
            game_root = findparentgame(man_cursor->firstgame, mame);
            if (game_root)                               // check if parent is already added
            {
               // game_cursor now points to the parent game of this clone
               game_last = gotolastclone(game_root);
               if (game_last)
               {
                  game_last->nclone = game_cursor;       // former last rec now points to this as next
                  game_cursor->pclone = game_last;       // new game previous clone points to former last entry
               }
            }
            else
            {
               game_root = man_cursor->firstgame;
               if (game_root == NULL)
               {
                  man_cursor->firstgame = game_cursor;   // this is the first item
               }
               else
               {
                  game_last = gotolastgame(game_root);
                  game_last->next = game_cursor;         // previous last->next = this record
                  game_cursor->prev = game_last;         // prev points to former last record
               }
            }
         }
      }
   }
   fclose(fp);
   return man_root;
}

