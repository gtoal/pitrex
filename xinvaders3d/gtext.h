#ifndef XINVADERS3D_GAMETEXT
#define XINVADERS3D_GAMETEXT

/* XInvaders3d game Text */


/* use a null terminator, so code doesn't have to
	be changed in many places. Don't use \n; assume
	a line break suitable to the particular platform
	should be generated after each line. TBB */
char *game_about_info[] =
{
   "Thanks for playing XInvaders 3D.",
   "XInvaders 3D v1.3.6, Copyright 2000 Don Llopis",
   "XInvaders 3D comes with ABSOLUTELY NO WARRANTY.",
   "This is free software, and you are welcome to",
   "redistribute it under certain conditions. See",
   "file named COPYING for more information.",
   "Latest version of the game can be found at:",
   "http://xinvaders3d.sourceforge.net",
   "Author can be reached at: del23@users.sourceforge.net",
   NULL
};

char *game_rules_info[] =
{
   "Keys:",
   "SPACE: Fire Missile",
   "Left Arrow: move left",
   "Right Arrow: move right",
   "Up Arrow: move up",
   "Down Arrow: move down",
   "f: Show frame-rate",
   "p: Pause game",
   "q: reset game",
   "ESC: Quit program",
   NULL
};


/* 
 * Not so nice vector-fonts:
 * perhaps one day, when the universe is kinder to me,
 * I'll implement a nice vector-font system. But
 * until the universe lets me go on my trip,
 * the stuff below stays!
 */
static int A[17] = 
{ 
   4,
   0, 0, 24, 0,
   0, 12, 24, 12,
   0, 0, 0, 24,
   24, 0, 24, 24
};

static int C[13] =
{
   3,
   0, 0, 24, 0,
   0, 0, 0, 24,
   0, 24, 24, 24
};

static int D[13] = 
{
   3,
   0, 0, 0, 24,
   0, 0, 24, 12,
   0, 24, 24, 12
};

static int E[17] =
{
   4,
   0, 0, 24, 0,
   0, 12, 24, 12,
   0, 24, 24, 24,
   0, 0, 0, 24

};

static int EXCLAMATION[33] =
{
   8,
   0, 0, 24, 0,
   5, 19, 15, 19,
   0, 0, 5, 19,
   24, 0, 15, 19,
   5, 25, 15, 25,
   5, 30, 15, 30,
   5, 25, 5, 30,
   15, 25, 15, 30
};

static int F[13] =
{
   3,
   0, 0, 0, 24,
   0, 0, 24, 0,
   0, 12, 24, 12
};

static int G[17] =
{
   4, 
   24, 0, 0, 12,
   0, 12, 24, 24,
   24, 24, 24, 12,
   24, 12, 12, 12
};

static int H[13] =
{
   3,
   0, 0, 0, 24,
   0, 12, 24, 12,
   24, 0, 24, 24
};

static int I[13] =
{
   3,
   0, 0, 24, 0,
   0, 24, 24, 24,
   12, 0, 12, 24
};

static int M[17] =
{
   4,
   0, 0, 0, 24,
   0, 0, 12, 12,
   12, 12, 24, 0,
   24, 0, 24, 24
};

static int N[13] =
{
   3,
   0, 0, 0, 24,
   0, 0, 24, 24,
   24, 0, 24, 24
};

static int O[17] =
{
   4,
   0, 0, 24, 0,
   0, 0, 0, 24,
   0, 24, 24, 24,
   24, 0, 24, 24
};


static int P[13] =
{
   3,
   0, 0, 0, 24,
   0, 0, 24, 12,
   24, 12, 0, 12
};

static int R[17] =
{
   4,
   0, 0, 0, 24,
   0, 0, 24, 12,
   0, 12, 24, 12,
   0, 12, 24, 24
};

static int S[13] =
{
   3,
   24, 0, 0, 5,
   0, 5, 24, 15,
   24, 15, 0, 24
};

static int T[9] =
{
   2,
   0, 0, 24, 0,
   12, 0, 12, 24
};

static int U[13] =
{
   3,
   0, 0, 0, 24,
   0, 24, 24, 24,
   24, 24, 24, 0
};

static int V[9] =
{
   2,
   0, 0, 12, 24,
   24, 0, 12, 24
};

static int X[9] =
{
   2,
   0, 0, 24, 24,
   24, 0, 0,24 
};

static int Y[13] =
{
   3,
   0, 0, 12, 12,
   12, 12, 24, 0,
   12, 12, 12, 24
};

static int ZERO[21] =
{
   5,
   0, 0, 24, 24,
   0, 0, 24, 0,
   0, 24, 24, 24,
   0, 0, 0, 24,
   24, 0, 24, 24
};

static int ONE[5] =
{
   1,
   12, 0, 12, 24
};

static int TWO[21] =
{
   5,
   0, 0, 24, 0,
   24, 0, 24, 12,
   24, 12, 0, 12,
   0, 12, 0, 24,
   0, 24, 24, 24
};

static int THREE[17] =
{
   4,
   0, 0, 24, 0, 
   24, 12, 0, 12,
   0, 24, 24, 24,
   24, 0, 24, 24
};
static int FOUR[13] =
{
   3,
   0, 0, 0, 12,
   0, 12, 24, 12,
   24, 0, 24, 24
};

static int FIVE[21] =
{
   5,
   0, 0, 24, 0,
   0, 0, 0, 12,
   0, 12, 24, 12,
   24, 12, 24, 24,
   24, 24, 0, 24
};

static int SIX[17] =
{
   4,
   0, 0, 0, 24,
   0, 24, 24, 24,
   24, 24, 24, 12,
   24, 12, 0, 12 
};

static int SEVEN[9] = 
{
   2, 
   0, 0, 24, 0, 
   24, 0, 0, 24
};
static int EIGHT[21] =
{
   5,
   0, 0, 24, 0,
   0, 12, 24, 12,
   0, 24, 24, 24,
   0, 0, 0, 24,
   24, 0, 24, 24
};

static int NINE[17] =
{
   4,
   0, 0, 24, 0,
   0, 12, 24, 12,
   0, 0, 0, 12,
   24, 0, 24, 24
};

static int *NUMBER[10] =
{
   &ZERO[0],
   &ONE[0],
   &TWO[0],
   &THREE[0],
   &FOUR[0],
   &FIVE[0],
   &SIX[0],
   &SEVEN[0],
   &EIGHT[0],
   &NINE[0]
};

static int *XINVADERS3D_LOGO[] = 
{ 
   &X[0], &I[0], &N[0], &V[0], &A[0], &D[0], &E[0], &R[0], &S[0], 
   &THREE[0], &D[0], NULL
};

static int *GET[] =
{
   &G[0], &E[0], &T[0], NULL
};

static int *READY[] =
{
   &R[0], &E[0], &A[0], &D[0], &Y[0], &EXCLAMATION[0],
   &EXCLAMATION[0], NULL
};

static int *GAME[] =
{
   &G[0], &A[0], &M[0], &E[0], NULL
};
static int *OVER[] =
{
   &O[0], &V[0], &E[0], &R[0], NULL
};

static int *PAUSED[] =
{
   &P[0], &A[0], &U[0], &S[0], &E[0], &D[0], NULL
};

static int *SCORE[] =
{
   &S[0], &C[0], &O[0], &R[0], &E[0], NULL
};

static int *HI_SCORE[] =
{
   &H[0], &I[0], &S[0], &C[0], &O[0], &R[0], &E[0], NULL
};

static int *PRESS[] =
{
   &P[0], &R[0], &E[0], &S[0], &S[0], NULL
};

static int *FIRE[] =
{
   &F[0], &I[0], &R[0], &E[0], NULL
};

static int *TO[] =
{
   &T[0], &O[0], NULL
};

static int *START[] =
{
   &S[0], &T[0], &A[0], &R[0], &T[0], NULL
};

#endif
