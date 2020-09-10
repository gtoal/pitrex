/*
 * ROIDSUPP - hyperoid support functions
 *
 * Version: 1.1  Copyright (C) 1991, Hutchins Software
 *      This software is licenced under the GNU General Public Licence
 *      Please read the associated legal documentation
 * Author: Edward Hutchins
 * Revisions:
 * 11/01/91	added GNU General Public License - Ed.
 * 2000 Mar 17	Unix port -rjm
 */

#include <stdio.h>
#include <string.h>
#include "misc.h"
#include "hyperoid.h"

#include "roidsupp.h"


/* these parts map to "abcdefghijklm" */
POINT LetterPart[] =
{
	{83, 572}, {64, 512}, {45, 572}, {96, 362}, {32, 362},
	{128, 256}, {0, 0}, {0, 256},
	{160, 362}, {224, 362}, {173, 572}, {192, 512}, {211, 572}
};

/* here's the vector font */
char *NumberDesc[] =
{
	"cakmck",       /* 0 */
	"dbl",          /* 1 */
	"abekm",        /* 2 */
	"abegjlk",      /* 3 */
	"mcfh",         /* 4 */
	"cbfgjlk",      /* 5 */
	"bdiljgi",      /* 6 */
	"acgl",         /* 7 */
	"bdjlieb",      /* 8 */
	"ljebdge"       /* 9 */
};

char *LetterDesc[] =
{
	"kdbemhf",      /* A */
	"kabegjlk",     /* B */
	"cbflm",        /* C */
	"kabejlk",      /* D */
	"cafgfkm",      /* E */
	"cafgfk",       /* F */
	"bdiljhg",      /* G */
	"kafhcm",       /* H */
	"bl",           /* I */
	"cjli",         /* J */
	"akcgm",        /* K */
	"akm",          /* L */
	"kagcm",        /* M */
	"kamc",         /* N */
	"bdiljeb",      /* O */
	"kabegf",       /* P */
	"mlidbejl",     /* Q */
	"kabegfgm",     /* R */
	"ebdjli",       /* S */
	"lbac",         /* T */
	"ailjc",        /* U */
	"alc",          /* V */
	"akgmc",        /* W */
	"amgkc",        /* X */
	"aglgc",        /* Y */
	"ackm"          /* Z */
};



/* PrintLetters - create letter objects from a string */

void PrintLetters( char *npszText, POINT Pos, POINT Vel,
					BYTE byColor, int nSize )
{
	int             nLen = strlen( npszText );
	int             nCnt = nLen;
	int             nSpace = nSize + nSize / 2;
	int             nBase = (nLen - 1) * nSpace;
	int             nBaseStart = Pos.x + nBase / 2;

	while (nCnt--)
	{
		OBJ *npLtr = CreateLetter( npszText[nCnt], nSize / 2 );
		if (npLtr)
		{
			npLtr->Pos.x = nBaseStart;
			npLtr->Pos.y = Pos.y;
			npLtr->Vel = Vel;
			npLtr->byColor = byColor;
		}
		nBaseStart -= nSpace;
	}
}


/* SpinLetters - spin letter objects away from center for effect */

void SpinLetters( char *npszText, POINT Pos, POINT Vel,
					BYTE byColor, int nSize )
{
	int             nLen = strlen( npszText );
	int             nCnt = nLen;
	int             nSpace = nSize + nSize / 2;
	int             nBase = (nLen - 1) * nSpace;
	int             nBaseStart = Pos.x + nBase / 2;

	while (nCnt--)
	{
		OBJ *npLtr = CreateLetter( npszText[nCnt], nSize / 2 );
		if (npLtr)
		{
			int nSpin = (nCnt - nLen / 2) * 2;
			npLtr->Pos.x = nBaseStart;
			npLtr->Pos.y = Pos.y;
			npLtr->Vel = Vel;
			npLtr->Vel.x += nSpin * 16;
			npLtr->nSpin = -nSpin;
			npLtr->byColor = byColor;
		}
		nBaseStart -= nSpace;
	}
}
