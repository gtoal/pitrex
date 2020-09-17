/*
 * HYPEROID - a neato game
 *
 * Version: 1.1  Copyright (C) 1990,91 Hutchins Software
 *      This software is licenced under the GNU General Public Licence
 *      Please read the associated legal documentation
 * Author: Edward Hutchins
 * Internet: eah1@cec1.wustl.edu
 * USNail: c/o Edward Hutchins, 63 Ridgemoor Dr., Clayton, MO, 63105
 * Revisions:
 * 10/31/91 made game better/harder - Ed.
 *
 * Music: R.E.M./The Cure/Ministry/Front 242/The Smiths/New Order/Hendrix...
 * Beers: Bass Ale, Augsberger Dark
 */

/* Unix/Linux conversion by Russell Marks, 2000
 *
 * I think I'll add Rush to the Music list :-)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>

#include "misc.h"
#include "roidsupp.h"
#include "sound.h"
#include "graphics.h"

#include "hyperoid.h"
#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>

#include "lib/svgalib-vectrex/vectrextokeyboard.h"

static int palrgb[16*3]=
  {
  0,0,0,	128,128,128,
  192,192,192,	255,255,255,
  128,0,0,	255,0,0,
  0,128,0,	0,255,0,
  0,0,128,	0,0,255,
  128,128,0,	255,255,0,
  0,128,128,	0,255,255,
  128,0,128,	255,0,255
  };



volatile int timer_flag=0;

static int restart_timer_count=0;


void sighandler(int foo)
{
timer_flag=1;
}



/* globals */

int nDrawDelay,nLevel,nSafe,nShield,nBomb,nBadGuys;
int lScore,lLastLife,lHighScore;
int bRestart,bPaused;

OBJ *npPlayer;
LIST FreeList,RoidList,ShotList,FlameList,SpinnerList;
LIST HunterList,HunterShotList,SwarmerList,LetterList,BonusList;
int nCos[DEGREE_SIZE],nSin[DEGREE_SIZE];
OBJ Obj[MAX_OBJS];


/* locals */

int dwSeed;
static RECT          rectShotClip;
static POINT         Player[] =
	{ {0, 0}, {160, 150}, {0, 250}, {96, 150}, {0, 0} };
static POINT         Spinner[] =
	{ {160, 150}, {224, 100}, {96, 100}, {32, 150}, {160, 150} };
static POINT         Swarmer[] =
	{ {0, 100}, {64, 100}, {128, 100}, {192, 100}, {0, 100} };
static POINT         Hunter[] =
{
	{160, 150}, {0, 250}, {192, 30}, {64, 30},
	{0, 250}, {96, 150}, {128, 150}, {160, 150}
};
static POINT         Bonus[] =
	{ {0, 150}, {102, 150}, {205, 150}, {51, 150}, {154, 150}, {0, 150} };


/* KillBadGuy - kill off a badguy (made into a macro) */

#define KillBadGuy() \
	((--nBadGuys <= 0)?(SetRestart( RESTART_NEXTLEVEL ),TRUE):FALSE)


/* arand - pseudorandom number from 0 to x-1 (thanks antman!) */

/* XXX replace? - it's probably v. poor */

int arand( int x )
{
	dwSeed = dwSeed * 0x343fd + 0x269ec3;
	return( (int)(((dwSeed >> 16) & 0x7fff) * x >> 15) );
}


/* AddHead - add an object to the head of a list */

void AddHead( LIST *npList, NODE *npNode )
{
	if (npList->npHead)
	{
		npNode->npNext = npList->npHead;
		npNode->npPrev = NULL;
		npList->npHead = (npList->npHead->npPrev = npNode);
	}
	else /* add to an empty list */
	{
		npList->npHead = npList->npTail = npNode;
		npNode->npNext = npNode->npPrev = NULL;
	}
}


/* RemHead - remove the first element in a list */

NODE *RemHead( LIST *npList )
{
	if (npList->npHead)
	{
		NODE *npNode = npList->npHead;
		if (npList->npTail != npNode)
		{
			npList->npHead = npNode->npNext;
			npNode->npNext->npPrev = NULL;
		}
		else npList->npHead = npList->npTail = NULL;
		return( npNode );
	}
	else return( NULL );
}


/* Remove - remove an arbitrary element from a list */

void Remove( LIST *npList, NODE *npNode )
{
	if (npNode->npPrev) npNode->npPrev->npNext = npNode->npNext;
	else npList->npHead = npNode->npNext;
	if (npNode->npNext) npNode->npNext->npPrev = npNode->npPrev;
	else npList->npTail = npNode->npPrev;
}


/* DrawObject - draw a single object */

void DrawObject( OBJ *npObj )
{
	int             nCnt;
	int             nDir = (npObj->nDir += npObj->nSpin);
	int             x = (npObj->Pos.x += npObj->Vel.x);
	int             y = (npObj->Pos.y += npObj->Vel.y);
	POINT           Pts[MAX_PTS];

	if (x < -CLIP_COORD) npObj->Pos.x = x = CLIP_COORD;
	else if (x > CLIP_COORD) npObj->Pos.x = x = -CLIP_COORD;
	if (y < -CLIP_COORD) npObj->Pos.y = y = CLIP_COORD;
	else if (y > CLIP_COORD) npObj->Pos.y = y = -CLIP_COORD;

	for (nCnt = npObj->byPts - 1; nCnt >= 0; --nCnt)
	{
		int wDeg = DEG( npObj->Pts[nCnt].x + nDir );
		int nLen = npObj->Pts[nCnt].y;
		Pts[nCnt].x = x + MULDEG( nLen, nCos[wDeg] );
		Pts[nCnt].y = y + MULDEG( nLen, nSin[wDeg] );
	}

	if (npObj->byPts > 1)
	{
		set_colour(BLACK);
		Polyline( npObj->Old, npObj->byPts );
		if (npObj->nCount > 0)
		{
			set_colour(npObj->byColor);
			Polyline( Pts, npObj->byPts );
			for (nCnt = npObj->byPts - 1; nCnt >= 0; --nCnt)
				npObj->Old[nCnt] = Pts[nCnt];
		}
	}
	else /* just a point */
	{
		SetPixel( npObj->Old[0].x, npObj->Old[0].y, BLACK );
		if (npObj->nCount > 0)
		{
			SetPixel( Pts[0].x, Pts[0].y, npObj->byColor );
			npObj->Old[0] = Pts[0];
		}
	}
}


/* SetRestart - set the restart timer */

void SetRestart( RESTART_MODE Restart )
{
	POINT           Pt;
	char            szBuff[32];

	if (bRestart) return;
        restart_timer_count=RESTART_DELAY_FRAMES;
	bRestart = TRUE;

	Pt.x = Pt.y = 0;
	switch (Restart)
	{
	case RESTART_GAME:
		SpinLetters( "GAME OVER", Pt, Pt, RED, 400 );
		break;
	case RESTART_LEVEL:
		PrintLetters( "GET READY", Pt, Pt, BLUE, 300 );
		break;
	case RESTART_NEXTLEVEL:
		sprintf( szBuff, "LEVEL %d", nLevel + 1 );
		PrintLetters( szBuff, Pt, Pt, BLUE, 300 );
		break;
	}
}


/* PrintPlayerMessage - show the player a status message */

void PrintPlayerMessage( char * npszText )
{
	POINT Pos, Vel;

	Pos = npPlayer->Pos;
	Pos.y -= 400;
	Vel.x = 0;
	Vel.y = -50;
	PrintLetters( npszText, Pos, Vel, GREEN, 150 );
}


/* AddExtraLife - give the player another life */

void AddExtraLife( void )
{
	PrintPlayerMessage( "EXTRA LIFE" );
	queuesam(EFFECT_CHANNEL,EXTRALIFE_SAMPLE);
	++npPlayer->nCount;
	npPlayer->byColor = (BYTE)(BLACK + npPlayer->nCount);
	if (npPlayer->byColor > WHITE) npPlayer->byColor = WHITE;
}


/* Hit - something hit an object, do fireworks */

void Hit( OBJ *npObj )
{
	int             nCnt;

	for (nCnt = 0; nCnt < 6; ++nCnt)
	{
		OBJ *npFlame = RemHeadObj( &FreeList );
		if (!npFlame) return;
		npFlame->Pos.x = npObj->Pos.x;
		npFlame->Pos.y = npObj->Pos.y;
		npFlame->Vel.x = npObj->Vel.x;
		npFlame->Vel.y = npObj->Vel.y;
		npFlame->nDir = npObj->nDir + (nCnt * DEGREE_SIZE) / 6;
		npFlame->nSpin = 0;
		npFlame->nCount = 10 + arand( 8 );
		npFlame->byColor = YELLOW;
		npFlame->byPts = 1;
		npFlame->Pts[0].x = npFlame->Pts[0].y = 0;
		ACCEL( npFlame, npFlame->nDir, 50 - npFlame->nCount );
		AddHeadObj( &FlameList, npFlame );
	}
}


/* Explode - explode an object */

void Explode( OBJ *npObj )
{
	int             nCnt, nSize = npObj->byPts;

	DrawObject( npObj );
	for (nCnt = 0; nCnt < nSize; ++nCnt)
	{
		OBJ *npFlame;
		if (arand( 2 )) continue;
		if (!(npFlame = RemHeadObj( &FreeList ))) return;
		npFlame->Pos.x = npObj->Pos.x;
		npFlame->Pos.y = npObj->Pos.y;
		npFlame->Vel.x = npObj->Vel.x;
		npFlame->Vel.y = npObj->Vel.y;
		npFlame->nDir = npObj->nDir + nCnt * DEGREE_SIZE / nSize + arand( 32 );
		npFlame->nSpin = arand( 31 ) - 15;
		npFlame->nCount = 25 + arand( 16 );
		npFlame->byColor = npObj->byColor;
		npFlame->byPts = 2;
		npFlame->Pts[0] = npObj->Pts[nCnt];
		if (nCnt == nSize - 1) npFlame->Pts[1] = npObj->Pts[0];
		else npFlame->Pts[1] = npObj->Pts[nCnt + 1];
		ACCEL( npFlame, npFlame->nDir, 60 - npFlame->nCount );
		AddHeadObj( &FlameList, npFlame );
	}
	Hit( npObj );
}


/* HitPlayer - blow up the player */

int HitPlayer( OBJ *npObj )
{
	POINT           Vel;
	int             nMass, nSpin;

	if (nSafe || (npPlayer->nCount <= 0)) return( FALSE );

	/* rumble and shake both objects */
	nMass = npPlayer->nMass + npObj->nMass;

	nSpin = npPlayer->nSpin + npObj->nSpin;
	npObj->nSpin -= MulDiv( nSpin, npPlayer->nMass, nMass );
	npPlayer->nSpin -= MulDiv( nSpin, npObj->nMass, nMass );

	Vel.x = npPlayer->Vel.x - npObj->Vel.x;
	Vel.y = npPlayer->Vel.y - npObj->Vel.y;
	npObj->Vel.x += MulDiv( Vel.x, npPlayer->nMass, nMass );
	npObj->Vel.y += MulDiv( Vel.y, npPlayer->nMass, nMass );
	npPlayer->Vel.x -= MulDiv( Vel.x, npObj->nMass, nMass );
	npPlayer->Vel.y -= MulDiv( Vel.y, npObj->nMass, nMass );

	if (--npPlayer->nCount)
	{
		npPlayer->byColor = (BYTE)(BLACK + npPlayer->nCount);
		if (npPlayer->byColor > WHITE) npPlayer->byColor = WHITE;
		Hit( npPlayer );
		queuesam(EFFECT_CHANNEL,PHIT_SAMPLE);
		return( TRUE );
	}

	/* final death */
	npPlayer->byColor = WHITE;
	Explode( npPlayer );
	queuesam(EFFECT_CHANNEL,EXPLODE2_SAMPLE);
	SetRestart( RESTART_GAME );
	return( FALSE );
}


/* CreateLetter - make a new letter object */

OBJ *CreateLetter( int cLetter, int nSize )
{
	OBJ *npLtr;
	int nCnt;
	char *npDesc;

	if (cLetter >= '0' && cLetter <= '9') npDesc = NumberDesc[cLetter - '0'];
	else if (cLetter >= 'A' && cLetter <= 'Z') npDesc = LetterDesc[cLetter - 'A'];
	else if (cLetter >= 'a' && cLetter <= 'z') npDesc = LetterDesc[cLetter - 'a'];
	else if (cLetter == '.') npDesc = "l";
	else return( NULL );

	if ((npLtr = RemHeadObj( &FreeList )))
	{
		npLtr->nMass = 1;
		npLtr->nDir = 0;
		npLtr->nSpin = 0;
		npLtr->nCount = 40;
		npLtr->byColor = WHITE;
		npLtr->byPts = (BYTE)(nCnt = strlen( npDesc ));
		while (nCnt--)
		{
			npLtr->Pts[nCnt] = LetterPart[npDesc[nCnt] - 'a'];
			npLtr->Pts[nCnt].y = MulDiv( npLtr->Pts[nCnt].y, nSize, LETTER_MAX );
		}
		AddHeadObj( &LetterList, npLtr );
	}
	return( npLtr );
}


/* DrawLetters - draw letters and such */

void DrawLetters( void )
{
	OBJ *npLtr, *npNext;

	for (npLtr = HeadObj( &LetterList ); npLtr; npLtr = npNext)
	{
		npNext = NextObj( npLtr );
		switch (--npLtr->nCount)
		{
		case 3:
			--npLtr->byColor;
			break;
		case 0:
			RemoveObj( &LetterList, npLtr );
			AddHeadObj( &FreeList, npLtr );
			break;
		}
		DrawObject( npLtr );
	}
}


/* CreateBonus - make a new bonus object */

void CreateBonus( void )
{
	OBJ *          npBonus;
	int             nCnt;

	if ((npBonus = RemHeadObj( &FreeList )))
	{
		queuesam(EFFECT_CHANNEL,NEWBONUS_SAMPLE);
		npBonus->Pos.x = arand( CLIP_COORD * 2 ) - CLIP_COORD;
		npBonus->Pos.y = -CLIP_COORD;
		npBonus->Vel.x = npBonus->Vel.y = 0;
		npBonus->nDir = arand( DEGREE_SIZE );
		npBonus->nSpin = (arand( 2 ) ? 12 : -12);
		npBonus->nCount = arand( 4 ) + 1;
		npBonus->nDelay = 64 + arand( 128 );
		npBonus->nMass = 1;
		npBonus->byColor = (BYTE)(WHITE + (npBonus->nCount * 2));
		npBonus->byPts = DIM(Bonus);
		for (nCnt = 0; nCnt < DIM(Bonus); ++nCnt)
			npBonus->Pts[nCnt] = Bonus[nCnt];
		ACCEL( npBonus, npBonus->nDir, 30 + nLevel * 2 );
		AddHeadObj( &BonusList, npBonus );
	}
}


/* DrawBonuses - process and draw the bonus list */

void DrawBonuses( void )
{
	OBJ *npBonus, *npNext;
	static int       nNextBonus = 1000;

	if (nBadGuys && (--nNextBonus < 0))
	{
		CreateBonus();
		nNextBonus = 1000;
	}

	for (npBonus = HeadObj( &BonusList ); npBonus; npBonus = npNext)
	{
		OBJ *          npShot;
		int             nDelta;
		RECT            rect;

		npNext = NextObj( npBonus );

		MKRECT( &rect, npBonus->Pos, 150 );

		if (PTINRECT( &rect, npPlayer->Pos ))
		{
			if (npPlayer->nCount > 0) switch (npBonus->nCount)
			{
			case 1:
				{
					char szBuff[32];
					int lBonus = 1000L * nLevel;
					if (lBonus == 0) lBonus = 500;
					lScore += lBonus;
					sprintf( szBuff, "%d", lBonus );
					PrintPlayerMessage( szBuff );
				}
				break;
			case 2:
				nSafe = 15;
				++nShield;
				npPlayer->byColor = GREEN;
				PrintPlayerMessage( "EXTRA SHIELD" );
				break;
			case 3:
				++nBomb;
				PrintPlayerMessage( "EXTRA BOMB" );
				break;
			case 4:
				AddExtraLife();
				break;
			}
			npBonus->nCount = 0;
			Explode( npBonus );
			queuesam(BADDIE_CHANNEL,BONUSGOT_SAMPLE);
			RemoveObj( &BonusList, npBonus );
			AddHeadObj( &FreeList, npBonus );
		}
		else if (INTRECT(&rect, &rectShotClip))
		{
			for (npShot = HeadObj( &ShotList ); npShot; npShot = NextObj( npShot ))
			{
				if (!PTINRECT( &rect, npShot->Pos )) continue;
				npShot->nCount = 1;
				npBonus->nCount = 0;
				Explode( npBonus );
				queuesam(BADDIE_CHANNEL,BONUSSHOT_SAMPLE);
				RemoveObj( &BonusList, npBonus );
				AddHeadObj( &FreeList, npBonus );
			}
		}
		if (npBonus->nCount && --npBonus->nDelay <= 0)
		{
			--npBonus->nCount;
			npBonus->nDelay = 64 + arand( 128 );
			npBonus->byColor = (BYTE)(WHITE + (npBonus->nCount * 2));
			if (npBonus->nCount == 0)
			{
				Explode( npBonus );
				queuesam(BADDIE_CHANNEL,BONUSTIMEOUT_SAMPLE);
				RemoveObj( &BonusList, npBonus );
				AddHeadObj( &FreeList, npBonus );
			}
		}
		nDelta = npPlayer->Pos.x - npBonus->Pos.x;
		while (nDelta < -16 || nDelta > 16) nDelta /= 2;
		npBonus->Vel.x += nDelta - npBonus->Vel.x / 16;
		nDelta = npPlayer->Pos.y - npBonus->Pos.y;
		while (nDelta < -16 || nDelta > 16) nDelta /= 2;
		npBonus->Vel.y += nDelta - npBonus->Vel.y / 16;
		DrawObject( npBonus );
	}
}


/* DrawHunterShots - process and draw the hunter shot list */

void DrawHunterShots( void )
{
	OBJ *npShot, *npNext;

	for (npShot = HeadObj( &HunterShotList ); npShot; npShot = npNext)
	{
		RECT            rect;

		npNext = NextObj( npShot );

		MKRECT( &rect, npShot->Pos, 200 );

		if (PTINRECT( &rect, npPlayer->Pos ))
		{
			HitPlayer( npShot );
			npShot->nCount = 1;
		}
		switch (--npShot->nCount)
		{
		case 7:
			npShot->byColor = DKGREEN;
			break;
		case 0:
			RemoveObj( &HunterShotList, npShot );
			AddHeadObj( &FreeList, npShot );
			break;
		}
		DrawObject( npShot );
	}
}


/* FireHunterShot - fire a hunter bullet */

void FireHunterShot( OBJ *npHunt )
{
	OBJ *          npShot;

	if ((npShot = RemHeadObj( &FreeList )))
	{
		queuesam(BSHOT_CHANNEL,BSHOT_SAMPLE);
		npShot->Pos.x = npHunt->Pos.x;
		npShot->Pos.y = npHunt->Pos.y;
		npShot->Vel.x = npHunt->Vel.x;
		npShot->Vel.y = npHunt->Vel.y;
		npShot->nMass = 8;
		npShot->nDir = npHunt->nDir + arand( 5 ) - 2;
		npShot->nSpin = (arand( 2 ) ? 10 : -10);
		npShot->nCount = 16 + arand( 8 );
		npShot->byColor = GREEN;
		npShot->byPts = 2;
		npShot->Pts[0].x = 128;
		npShot->Pts[0].y = 50;
		npShot->Pts[1].x = 0;
		npShot->Pts[1].y = 50;
		ACCEL( npShot, npShot->nDir, 200 + npShot->nCount );
		AddHeadObj( &HunterShotList, npShot );
	}
}


/* CreateHunter - make a new hunter */

void CreateHunter( void )
{
	OBJ *          npHunt;
	int             nCnt;

	if ((npHunt = RemHeadObj( &FreeList )))
	{
		queuesam(EFFECT_CHANNEL,NEWHUNT_SAMPLE);
		npHunt->Pos.x = arand( CLIP_COORD * 2 ) - CLIP_COORD;
		npHunt->Pos.y = -CLIP_COORD;
		npHunt->Vel.x = npHunt->Vel.y = 0;
		npHunt->nMass = 256;
		npHunt->nDir = arand( DEGREE_SIZE );
		npHunt->nSpin = 0;
		npHunt->nCount = 1 + arand( nLevel );
		npHunt->nDelay = 2 + arand( 10 );
		npHunt->byColor = CYAN;
		npHunt->byPts = DIM(Hunter);
		for (nCnt = 0; nCnt < DIM(Hunter); ++nCnt)
			npHunt->Pts[nCnt] = Hunter[nCnt];
		ACCEL( npHunt, npHunt->nDir, 30 + nLevel * 2 );
		AddHeadObj( &HunterList, npHunt );
		++nBadGuys;
	}
}


/* DrawHunters - process and draw the hunter list */

void DrawHunters( void )
{
	OBJ *npHunt, *npNext;
	static int       nNextHunter = 200;

	if (nBadGuys && (--nNextHunter < 0))
	{
		CreateHunter();
		nNextHunter = 1000 + arand( 1000 ) - nLevel * 8;
	}

	for (npHunt = HeadObj( &HunterList ); npHunt; npHunt = npNext)
	{
		OBJ *          npShot;
		RECT            rect;

		npNext = NextObj( npHunt );

		MKRECT( &rect, npHunt->Pos, 200 );

		if (PTINRECT( &rect, npPlayer->Pos ))
		{
			HitPlayer( npHunt );
			--npHunt->nCount;
			if (npHunt->nCount < 1)
			{
				KillBadGuy();
				npHunt->byColor = CYAN;
				Explode( npHunt );
				queuesam(BADDIE_CHANNEL,HUNTEXPLODE_SAMPLE);
				RemoveObj( &HunterList, npHunt );
				AddHeadObj( &FreeList, npHunt );
			}
			else if (npHunt->nCount == 1)
			{
				npHunt->byColor = DKCYAN;
				queuesam(BADDIE_CHANNEL,BADDIEWOUND_SAMPLE);
			}
		}
		else if (INTRECT(&rect, &rectShotClip))
		{
			for (npShot = HeadObj( &ShotList ); npShot; npShot = NextObj( npShot ))
			{
				if (!PTINRECT( &rect, npShot->Pos )) continue;
				npShot->nCount = 1;
				lScore += npHunt->nCount * 1000;
				if (--npHunt->nCount < 1)
				{
					KillBadGuy();
					npHunt->byColor = CYAN;
					Explode( npHunt );
					queuesam(BADDIE_CHANNEL,HUNTEXPLODE_SAMPLE);
					RemoveObj( &HunterList, npHunt );
					AddHeadObj( &FreeList, npHunt );
				}
				else
				{
					if (npHunt->nCount == 1) npHunt->byColor = DKCYAN;
					Hit( npHunt );
					queuesam(BADDIE_CHANNEL,BADDIEWOUND_SAMPLE);
				}
				break;
			}
		}
		ACCEL( npHunt, npHunt->nDir, 8 );
		npHunt->Vel.x -= npHunt->Vel.x / 16;
		npHunt->Vel.y -= npHunt->Vel.y / 16;
		if (--npHunt->nDelay <= 0)
		{
			npHunt->nDelay = arand( 10 );
			npHunt->nSpin = arand( 11 ) - 5;
			FireHunterShot( npHunt );
		}
		DrawObject( npHunt );
	}
}


/* CreateSwarmer - make a new swarmer */

void CreateSwarmer( POINT Pos, int nDir, int nCount )
{
	OBJ *          npSwarm;
	int             nCnt;

	if ((npSwarm = RemHeadObj( &FreeList )))
	{
		queuesam(EFFECT_CHANNEL,NEWSWARM_SAMPLE);
		npSwarm->Pos = Pos;
		npSwarm->Vel.x = npSwarm->Vel.y = 0;
		npSwarm->nDir = nDir;
		npSwarm->nSpin = arand( 31 ) - 15;
		npSwarm->nCount = nCount;
		npSwarm->nDelay = 64 + arand( 64 );
		npSwarm->nMass = 32;
		npSwarm->byColor = DKGREEN;
		npSwarm->byPts = DIM(Swarmer);
		for (nCnt = 0; nCnt < DIM(Swarmer); ++nCnt)
		{
			npSwarm->Pts[nCnt] = Swarmer[nCnt];
			npSwarm->Pts[nCnt].y += nCount * 10;
		}
		ACCEL( npSwarm, npSwarm->nDir, 30 + nLevel * 2 );
		AddHeadObj( &SwarmerList, npSwarm );
		++nBadGuys;
	}
}


/* DrawSwarmers - process and draw the swarmer list */

void DrawSwarmers( void )
{
	OBJ *npSwarm, *npNext;
	static int       nNextSwarmer = 1000;

	if (nBadGuys && (--nNextSwarmer < 0))
	{
		POINT Pos;
		Pos.x = arand( CLIP_COORD * 2 ) - CLIP_COORD;
		Pos.y = -CLIP_COORD;
		CreateSwarmer( Pos, arand( DEGREE_SIZE ), 8 + nLevel * 2 );
		nNextSwarmer = 1000 + arand( 500 ) - nLevel * 4;
	}

	for (npSwarm = HeadObj( &SwarmerList ); npSwarm; npSwarm = npNext)
	{
		OBJ *          npShot;
		RECT            rect;

		npNext = NextObj( npSwarm );

		MKRECT( &rect, npSwarm->Pos, 150 + npSwarm->nCount * 10 );

		if (PTINRECT( &rect, npPlayer->Pos ))
		{
			HitPlayer( npSwarm );
			npSwarm->nCount = 0;
		}
		else if (INTRECT(&rect, &rectShotClip))
		{
			for (npShot = HeadObj( &ShotList ); npShot; npShot = NextObj( npShot ))
			{
				if (!PTINRECT( &rect, npShot->Pos )) continue;
				npShot->nCount = 1;
				lScore += npSwarm->nCount * 25;
				npSwarm->nCount = 0;
				break;
			}
		}
		if (npSwarm->nCount <= 0)
		{
			npSwarm->byColor = GREEN;
			KillBadGuy();
			Explode( npSwarm );
			queuesam(BADDIE_CHANNEL,SWARMSPLIT_SAMPLE);
			RemoveObj( &SwarmerList, npSwarm );
			AddHeadObj( &FreeList, npSwarm );
		}
		else
		{
			if ((npSwarm->nCount > 1) && (--npSwarm->nDelay <= 0))
			{
				int nDir = arand( DEGREE_SIZE );
				int nCount = npSwarm->nCount / 2;
				CreateSwarmer( npSwarm->Pos, nDir, nCount );
				nCount = npSwarm->nCount - nCount;
				CreateSwarmer( npSwarm->Pos, nDir + 128, nCount );
				npSwarm->nCount = 0;
			}
			DrawObject( npSwarm );
		}
	}
}


/* CreateSpinner - make a new spinner */

void CreateSpinner( void )
{
	OBJ *          npSpin;
	int             nCnt;

	if ((npSpin = RemHeadObj( &FreeList )))
	{
		queuesam(EFFECT_CHANNEL,NEWSPIN_SAMPLE);
		npSpin->Pos.x = arand( CLIP_COORD * 2 ) - CLIP_COORD;
		npSpin->Pos.y = -CLIP_COORD;
		npSpin->Vel.x = npSpin->Vel.y = 0;
		npSpin->nDir = arand( DEGREE_SIZE );
		npSpin->nSpin = -12;
		npSpin->nCount = 1 + arand( nLevel );
		npSpin->nMass = 64 + npSpin->nCount * 32;
		npSpin->byColor = (BYTE)(MAGENTA - npSpin->nCount);
		npSpin->byPts = DIM(Spinner);
		for (nCnt = 0; nCnt < DIM(Spinner); ++nCnt)
			npSpin->Pts[nCnt] = Spinner[nCnt];
		ACCEL( npSpin, npSpin->nDir, 30 + nLevel * 2 );
		AddHeadObj( &SpinnerList, npSpin );
		++nBadGuys;
	}
}


/* DrawSpinners - process and draw the spinner list */

void DrawSpinners( void )
{
	OBJ *npSpin, *npNext;
	static int       nNextSpinner = 1000;

	if (nBadGuys && (--nNextSpinner < 0))
	{
		CreateSpinner();
		nNextSpinner = 100 + arand( 900 ) - nLevel * 2;
	}

	for (npSpin = HeadObj( &SpinnerList ); npSpin; npSpin = npNext)
	{
		OBJ *          npShot;
		int             nDelta;
		RECT            rect;

		npNext = NextObj( npSpin );

		MKRECT( &rect, npSpin->Pos, 150 );

		if (PTINRECT( &rect, npPlayer->Pos ))
		{
			HitPlayer( npSpin );
			--npSpin->nCount;
			npSpin->byColor = (BYTE)(MAGENTA - npSpin->nCount);
			if (npSpin->nCount < 1)
			{
				KillBadGuy();
				Explode( npSpin );
				queuesam(BADDIE_CHANNEL,SPINEXPLODE_SAMPLE);
				RemoveObj( &SpinnerList, npSpin );
				AddHeadObj( &FreeList, npSpin );
			}
			else
			{
				queuesam(BADDIE_CHANNEL,BADDIEWOUND_SAMPLE);
			}
		}
		else if (INTRECT(&rect, &rectShotClip))
		{
			for (npShot = HeadObj( &ShotList ); npShot; npShot = NextObj( npShot ))
			{
				if (!PTINRECT( &rect, npShot->Pos )) continue;
				npShot->nCount = 1;
				lScore += npSpin->nCount * 500;
				npSpin->byColor = (BYTE)(MAGENTA - (--npSpin->nCount));
				if (npSpin->nCount < 1)
				{
					KillBadGuy();
					Explode( npSpin );
					queuesam(BADDIE_CHANNEL,SPINEXPLODE_SAMPLE);
					RemoveObj( &SpinnerList, npSpin );
					AddHeadObj( &FreeList, npSpin );
				}
				else
				{
					Hit( npSpin );
					queuesam(BADDIE_CHANNEL,BADDIEWOUND_SAMPLE);
				}
				break;
			}
		}
		nDelta = npPlayer->Pos.x - npSpin->Pos.x;
		while (nDelta < -16 || nDelta > 16) nDelta /= 2;
		npSpin->Vel.x += nDelta - npSpin->Vel.x / 16;
		nDelta = npPlayer->Pos.y - npSpin->Pos.y;
		while (nDelta < -16 || nDelta > 16) nDelta /= 2;
		npSpin->Vel.y += nDelta - npSpin->Vel.y / 16;
		DrawObject( npSpin );
	}
}


/* CreateRoid - make a new asteroid */

void CreateRoid( POINT Pos, POINT Vel, int nSides, BYTE byColor,
							 int nDir, int nSpeed, int nSpin )
{
	OBJ *          npRoid;
	int             nCnt;

	if ((npRoid = RemHeadObj( &FreeList )))
	{
		npRoid->Pos = Pos;
		npRoid->Vel = Vel;
		npRoid->nMass = nSides * 128;
		npRoid->nDir = nDir;
		npRoid->nSpin = nSpin + arand( 11 ) - 5;
		npRoid->nCount = nSides * 100;
		npRoid->byColor = byColor;
		npRoid->byPts = (BYTE)(nSides + 1);
		for (nCnt = 0; nCnt < nSides; ++nCnt)
		{
			npRoid->Pts[nCnt].x = nCnt * DEGREE_SIZE / nSides + arand( 30 );
			npRoid->Pts[nCnt].y = (nSides - 1) * 100 + 20 + arand( 80 );
		}
		npRoid->Pts[nSides] = npRoid->Pts[0];
		ACCEL( npRoid, nDir, nSpeed );
		AddHeadObj( &RoidList, npRoid );
		++nBadGuys;
	}
}


/* BreakRoid - break up an asteroid */

void BreakRoid( OBJ *npRoid, OBJ *npShot )
{
	int             nCnt, nNew;

	lScore += npRoid->nCount;
	if (npShot) npShot->nCount = 1;
	switch (npRoid->byPts)
	{
	case 8:
		nNew = 2 + arand( 3 );
		break;
	case 7:
		nNew = 1 + arand( 3 );
		break;
	case 6:
		nNew = 1 + arand( 2 );
		break;
	case 5:
		nNew = arand( 2 );
		break;
	default:
		nNew = 0;
		break;
	}
	if (nNew == 1)		/* don't explode outward */
	{
		POINT Pt = npRoid->Pos;
		Pt.x += arand( 301 ) - 150; Pt.y += arand( 301 ) - 150;
		CreateRoid( Pt, npRoid->Vel, npRoid->byPts - (nNew + 1),
			npRoid->byColor, npShot?(npShot->nDir):npRoid->nDir,
			8, npRoid->nSpin );
	}
	else if (nNew > 0)
	{
		int nSpeed = npRoid->nSpin * npRoid->nSpin * nNew + 16;
		for (nCnt = 0; nCnt < nNew; ++nCnt)
		{
			POINT Pt = npRoid->Pos;
			Pt.x += arand( 601 ) - 300; Pt.y += arand( 601 ) - 300;
			CreateRoid( Pt, npRoid->Vel, npRoid->byPts - (nNew + 1),
						npRoid->byColor,
						npRoid->nDir + nCnt * DEGREE_SIZE / nNew + arand( 32 ),
						nSpeed + arand( nLevel * 4 ),
						npRoid->nSpin / 2 );
		}
	}
	KillBadGuy();
	++npRoid->byColor;
	npRoid->nCount = 0;
	if (nNew)
	{
		Hit( npRoid );
		DrawObject( npRoid );
		queuesam(ASTEROID_CHANNEL,ROIDSPLIT_SAMPLE);
	}
	else
        {
        	Explode( npRoid );
		queuesam(ASTEROID_CHANNEL,ROIDNOSPLIT_SAMPLE);
	}
	RemoveObj( &RoidList, npRoid );
	AddHeadObj( &FreeList, npRoid );
}


/* DrawRoids - process and draw the asteroid list */

void DrawRoids( void )
{
	OBJ *npRoid, *npNext;

	for (npRoid = HeadObj( &RoidList ); npRoid; npRoid = npNext)
	{
		int             nSize = npRoid->nCount;
		OBJ *          npShot;
		RECT            rect;

		npNext = NextObj( npRoid );

		DrawObject( npRoid );

		MKRECT( &rect, npRoid->Pos, nSize );

		if (PTINRECT( &rect, npPlayer->Pos ) && HitPlayer( npRoid ))
		{
			npPlayer->nCount = -npPlayer->nCount;
			npPlayer->byColor = WHITE;
			Explode( npPlayer );
			BreakRoid( npRoid, NULL );
			if (nBadGuys) SetRestart( RESTART_LEVEL );
			else SetRestart( RESTART_NEXTLEVEL );
		}
		else if (INTRECT(&rect, &rectShotClip))
		{
			for (npShot = HeadObj( &ShotList ); npShot; npShot = NextObj( npShot ))
			{
				if (!PTINRECT( &rect, npShot->Pos )) continue;
				BreakRoid( npRoid, npShot );
				break;
			}
		}
	}
}


/* DrawShots - process and draw the player shot list */

void DrawShots( void )
{
	OBJ *npShot, *npNext;

	if ((npShot = HeadObj( &ShotList )))
	{
		rectShotClip.left = rectShotClip.right = npShot->Pos.x;
		rectShotClip.top = rectShotClip.bottom = npShot->Pos.y;
		while (npShot)
		{
			npNext = NextObj( npShot );
			switch (--npShot->nCount)
			{
			case 10:
				npShot->byColor = DKCYAN;
				break;
			case 5:
				npShot->byColor = DKBLUE;
				break;
			case 0:
				RemoveObj( &ShotList, npShot );
				AddHeadObj( &FreeList, npShot );
				break;
			}
			DrawObject( npShot );
			if (npShot->Pos.x < rectShotClip.left) rectShotClip.left = npShot->Pos.x;
			else if (npShot->Pos.x > rectShotClip.right) rectShotClip.right = npShot->Pos.x;
			if (npShot->Pos.y < rectShotClip.top) rectShotClip.top = npShot->Pos.y;
			else if (npShot->Pos.y > rectShotClip.bottom) rectShotClip.bottom = npShot->Pos.y;
			npShot = npNext;
		}
	}
	else rectShotClip.left = rectShotClip.right = rectShotClip.top = rectShotClip.bottom = 32767;
}


/* DrawFlames - process and draw the flame list */

void DrawFlames( void )
{
	OBJ *npFlame, *npNext;

	for (npFlame = HeadObj( &FlameList ); npFlame; npFlame = npNext)
	{
		npNext = NextObj( npFlame );
		switch (--npFlame->nCount)
		{
		case 7:
			npFlame->byColor = RED;
			break;
		case 3:
			npFlame->byColor = DKRED;
			break;
		case 0:
			RemoveObj( &FlameList, npFlame );
			AddHeadObj( &FreeList, npFlame );
			break;
		}
		DrawObject( npFlame );
	}
}


/* FireShot - fire a bullet */

void FireShot( void )
{
	OBJ *          npShot;

	if ((npShot = RemHeadObj( &FreeList )))
	{
		queuesam(PSHOT_CHANNEL,PSHOT_SAMPLE);
		npShot->Pos.x = npPlayer->Pos.x;
		npShot->Pos.y = npPlayer->Pos.y;
		npShot->Vel.x = npPlayer->Vel.x;
		npShot->Vel.y = npPlayer->Vel.y;
		npShot->nMass = 8;
		npShot->nDir = npPlayer->nDir + arand( 5 ) - 2;
		npShot->nSpin = 0;
		npShot->nCount = 16 + arand( 8 );
		npShot->byColor = CYAN;
		npShot->byPts = 2;
		npShot->Pts[0].x = 128;
		npShot->Pts[0].y = 50;
		npShot->Pts[1].x = 0;
		npShot->Pts[1].y = 50;
		ACCEL( npShot, npShot->nDir, 200 + npShot->nCount );
		AddHeadObj( &ShotList, npShot );
	}
}


/* AccelPlayer - move the player forward */

void AccelPlayer( int nDir, int nAccel )
{
	OBJ *          npFlame;

	queuesam(PTHRUST_CHANNEL,PTHRUST_SAMPLE);
	nDir += npPlayer->nDir;
	if (nAccel) ACCEL( npPlayer, nDir, nAccel );
	if ((npFlame = RemHeadObj( &FreeList )))
	{
		npFlame->Pos.x = npPlayer->Pos.x;
		npFlame->Pos.y = npPlayer->Pos.y;
		npFlame->Vel.x = npPlayer->Vel.x;
		npFlame->Vel.y = npPlayer->Vel.y;
		npFlame->nDir = nDir + 100 + arand( 57 );
		npFlame->nSpin = 0;
		npFlame->nCount = nAccel + arand( 7 );
		npFlame->byColor = YELLOW;
		npFlame->byPts = 1;
		npFlame->Pts[0].x = npFlame->Pts[0].y = 0;
		ACCEL( npFlame, npFlame->nDir, 50 + arand( 10 ) );
		AddHeadObj( &FlameList, npFlame );
	}
}


/* HitList - Hit() a list of things */

void HitList( LIST *npList )
{
	OBJ *          npObj;

	for (npObj = HeadObj( npList ); npObj; npObj = NextObj( npObj ))
		if (npObj->nCount) Hit( npObj );
}


/* ExplodeBadguys - explode a list of badguys */

void ExplodeBadguys( LIST *npList )
{
	OBJ *          npObj;

	while ((npObj = HeadObj( npList )))
	{
		KillBadGuy();
		npObj->nCount = 0;
		Explode( npObj );
		RemoveObj( npList, npObj );
		AddHeadObj( &FreeList, npObj );
	}
}


/* DrawPlayer - process and draw the player */

void DrawPlayer( void )
{
	static int       nBombing = 0;
	static int       nShotDelay = 0;

	if (npPlayer->nCount <= 0) return;

	if (nSafe > 0)
	{
		if (--nSafe == 0)
		{
			npPlayer->byColor = (BYTE)(BLACK + npPlayer->nCount);
			if (npPlayer->byColor > WHITE) npPlayer->byColor = WHITE;
		}
	}
	else if (IsKeyDown( KEY_TAB ) && nShield > 0)
	{
		nSafe = 15;
		if (--nShield > 0) npPlayer->byColor = GREEN;
		else npPlayer->byColor = DKGREEN;
	}

	if (nBombing > 0)
	{
		if (--nBombing == 0)
		{
			ExplodeBadguys( &SpinnerList );
			ExplodeBadguys( &SwarmerList );
			ExplodeBadguys( &HunterList );
			queuesam(EFFECT_CHANNEL,EXPLODE2_SAMPLE);
		}
		else
		{
			HitList( &SpinnerList );
			HitList( &SwarmerList );
			HitList( &HunterList );
		}
	}
	else if (nBomb && IsKeyDown( KEY_S )) --nBomb, nBombing = 5;

	if (IsKeyDown( KEY_LEFT )) npPlayer->nSpin += 8;
	if (IsKeyDown( KEY_RIGHT )) npPlayer->nSpin -= 8;
	if (IsKeyDown( KEY_DOWN )) AccelPlayer( 0, 12 );
	if (IsKeyDown( KEY_UP )) AccelPlayer( 128, 12 );
	if (nShotDelay) --nShotDelay;
	else if (IsKeyDown( KEY_SPACE )) FireShot(), nShotDelay = 2;
	DrawObject( npPlayer );
	npPlayer->nSpin /= 2;
}



/* DrawObjects - transform and redraw everything in the system */

void DrawObjects( void )
{
	/* move and draw things (I don't think the order is important...) */
	DrawPlayer();
	DrawFlames();
	DrawShots();
	DrawRoids();
	DrawSpinners();
	DrawSwarmers();
	DrawHunters();
	DrawHunterShots();
	DrawLetters();
	DrawBonuses();
	/* (...but I'm not changing it!!! :-) */
}


/* CheckScore - show the score and such stuff */

void CheckScore( void )
{
	int nLives;

	if (lScore - lLastLife > EXTRA_LIFE)
	{
		AddExtraLife();
		lLastLife = lScore;
	}

	/* apparently, -ve player lives means we're starting a new
	 * life soon (ouch). -rjm
	 */
	nLives=((npPlayer->nCount > 0) ? npPlayer->nCount : -npPlayer->nCount);

	/* actually do the score/lives/etc-drawing */
	score_graphics(nLevel,lScore,nLives,nShield,nBomb);
}


/* NewGame - start a new game */

void NewGame( void )
{
	npPlayer->nCount = 0;
	npPlayer->byColor = WHITE;
	Explode( npPlayer );
	SetRestart( RESTART_GAME );
	ExplodeBadguys( &RoidList );
	ExplodeBadguys( &SpinnerList );
	ExplodeBadguys( &SwarmerList );
	ExplodeBadguys( &HunterList );
	queuesam(EFFECT_CHANNEL,EXPLODE2_SAMPLE);
}


/* RestartHyperoid - set up a game! */

void RestartHyperoid( void )
{
restart_timer_count=RESTART_DELAY_FRAMES;

	if (npPlayer->nCount == 0)
	{
		POINT Pos, Vel;
		Pos.x = 0;
		Pos.y = -CLIP_COORD / 2;
		Vel.x = 0;
		Vel.y = 150;
		PrintLetters( "HYPEROID", Pos, Vel, YELLOW, 800 );
		queuesam(EFFECT_CHANNEL,TITLE_SAMPLE);
		queuesam(ASTEROID_CHANNEL,TITLE_SAMPLE);
		queuesam(BADDIE_CHANNEL,TITLE_SAMPLE);
		npPlayer->nCount = 3;
		if (lHighScore < lScore) lHighScore = lScore;
		lLastLife = lScore = 0;
		nLevel = 0;
		nShield = nBomb = 3;
	}
	else if (npPlayer->nCount < 0)
	{
		/* cheesy way of restarting after a major collision */
		npPlayer->nCount = -npPlayer->nCount;
		nShield = nBomb = 3;
	}

	npPlayer->Pos.x = npPlayer->Pos.y = 0;
	npPlayer->Vel.x = npPlayer->Vel.y = 0;
	npPlayer->nDir = 64;
	npPlayer->nSpin = 0;
	npPlayer->byColor = GREEN;
	nSafe = 30;

	if (ShotList.npHead)
	{
		OBJ *npShot;
		for (npShot = HeadObj( &ShotList ); npShot; npShot = NextObj( npShot ))
			npShot->nCount = 1;
	}

	/* reseed the asteroid field */
	if (nBadGuys == 0)
	{
		int nCnt;
		++nLevel;
		for (nCnt = 5 + nLevel; nCnt; --nCnt)
		{
			POINT Pos, Vel;
			Pos.x = arand( MAX_COORD * 2 ) - MAX_COORD;
			Pos.y = arand( MAX_COORD * 2 ) - MAX_COORD;
			Vel.x = Vel.y = 0;
			CreateRoid( Pos, Vel, 6 + arand( 2 ),
						(BYTE)(arand( 2 ) ? DKYELLOW : DKGREY),
						arand( DEGREE_MAX ), 30 + arand( nLevel * 8 ), 0 );
		}
	}
}



/* InitHyperoid - initialize everything */

void InitHyperoid( void )
{
	double          dRad;
	int             nCnt;

	/* seed the randomizer */
	dwSeed = time(NULL);	/* XXX GetCurrentTime(); */

	/* create the lookup table */
	for (nCnt = 0; nCnt < DEGREE_SIZE; ++nCnt)
	{
		dRad = nCnt * 6.2831855 / DEGREE_SIZE;
		nCos[nCnt] = (int)(DEGREE_MAX * cos( dRad ));
		nSin[nCnt] = (int)(DEGREE_MAX * sin( dRad ));
	}

	/* allocate all objects as free */
	for (nCnt = 0; nCnt < MAX_OBJS; ++nCnt)
		AddHeadObj( &FreeList, &(Obj[nCnt]) );

	/* set up the player */
	npPlayer = RemHeadObj( &FreeList );
	npPlayer->byPts = DIM(Player);
	npPlayer->nMass = 256;
	for (nCnt = 0; nCnt < DIM(Player); ++nCnt)
		npPlayer->Pts[nCnt] = Player[nCnt];
}


void start_timer(void)
{
struct sigaction sa;
struct itimerval itv;
int tmp=1000000/20;	/* 20 ints/sec */

sigemptyset(&sa.sa_mask);
sa.sa_handler=sighandler;
sa.sa_flags=SA_RESTART;
sigaction(SIGALRM,&sa,NULL);

itv.it_value.tv_sec=itv.it_interval.tv_sec=tmp/1000000;
itv.it_value.tv_usec=itv.it_interval.tv_usec=tmp%1000000;
setitimer(ITIMER_REAL,&itv,NULL);
}


void wait_for_timer(void)
{
#ifdef PITREX
#else
sigset_t mask,oldmask;

sigemptyset(&mask);
sigaddset(&mask,SIGALRM);

/* The procmask stuff is to avoid a race condition (not actually
 * a big deal, would just rarely lose an interrupt, but FWIW...).
 */
sigprocmask(SIG_BLOCK,&mask,&oldmask);
if(!timer_flag)
  while(!timer_flag)
    sigsuspend(&oldmask);
sigprocmask(SIG_UNBLOCK,&mask,NULL);
#endif
timer_flag=0;
}


void stop_sound(void)
{
#ifdef DO_SOUND
queuesam(KILL_SNDSERV,0);
wait(NULL);
#endif
}


int main(int argc,char *argv[])
{
static int skipflag = 0;
int quit=0;

graphics_init(argc,argv,palrgb);

start_sound();

InitHyperoid();
RestartHyperoid();

atexit(stop_sound);

start_timer();

while(!quit)
  {
#ifdef NEVER
  wait_for_timer();
#endif
  v_WaitRecal();
  keyboard_update();
  
  graphics_update();

  if (!skipflag) {
    if (restart_timer_count > (RESTART_DELAY_FRAMES / 1.8) )
      CheckScore();
    DrawObjects();
  }
  skipflag += 1;
  if (skipflag == 3) skipflag = 0;
  if(bRestart)
    {
    restart_timer_count--;
    if(restart_timer_count==0)
      {
      bRestart = FALSE;
      RestartHyperoid();
      }
    }
  
  if(IsKeyDown(KEY_F1))
    NewGame();
  
  if(IsKeyDown(KEY_ESC))
    quit=1;
  }

/* don't rely on it getting here, GTK+ ver may exit alternative way */

graphics_exit();
exit(0);
}
