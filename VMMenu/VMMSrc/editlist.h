/**************************************
Editlist.h
Chad Gray, 8 Sep 2019
Function declarations
**************************************/

#ifndef _EDITLIST_H_
#define _EDITLIST_H_

typedef struct listnode
{
   struct listnode   *next, *prev;
   char              manuf[20];
   char              desc[50];
   char              parent[128];
   char              clone[128];
   int               hidden;
} list_node;

list_node*  build_games_list(void);
list_node*  add_list_game(int, char*, char*, char*, char*);
void        dump_list(list_node*);
void        write_list(list_node*);

#endif

