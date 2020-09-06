/**************************************
VMM-SDL.h
Chad Gray, 18 Jun 2011
SDL Specific Function declarations
**************************************/

#ifndef _VMM_SDL_H_
#define _VMM_SDL_H_

void  startZVG(void);
int   getkey(void);
int   initmouse(void);
void  processmouse(void);
int   sendframe(void);
void  ShutdownAll(void);
void  drawvector(point, point, float, float);					// draw a vector between 2 points
void	RunGame(char*, char*);
void  FrameSendSDL(void);
void  SDLvector(float, float, float, float, int, int);
void  InitialiseSDL(int);
void  CloseSDL(int);
char* itoa(int, char*, int);
void  mousepos(int*, int*);
void  setcolour(int, int);                                    // set colour and brightness
void  playsound(int);

#endif

