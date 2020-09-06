/**************************************
Gamelist.h
Chad Gray, 15 Nov 2009
Function declarations
**************************************/

#ifndef _GAMELIST_H_
#define _GAMELIST_H_

typedef struct gamenode
{
   struct gamenode   *next, *prev;
   struct gamenode   *nclone, *pclone;
   char              name[60];
   char              parent[128];
   char              clone[128];
} g_node;

typedef struct manufnode
{
   struct manufnode  *nmanuf;
   struct manufnode  *pmanuf;
   struct gamenode   *firstgame;
   char              name[30];
} m_node;

m_node*  createlist();

int      printlist(m_node*);
void     linklist(m_node*);
m_node*  add_manuf(char*);
m_node*  findmanuf(m_node*, char*);
m_node*  gotolastmanuf(m_node*);

g_node*  add_game(char*, char*, char*);
g_node*  gotolastgame(g_node*);
g_node*  gotolastclone(g_node*);
g_node*  findparentgame(g_node*, char*);

#endif
