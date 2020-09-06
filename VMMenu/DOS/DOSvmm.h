/**************************************
DOS-vmmenu.h
Chad Gray, 18 Jun 2011
DOS Specific Function declarations
**************************************/

#ifndef _DOSVMM_H_
#define _DOSVMM_H_

void startZVG(void);                         // Start up ZVG
int  getkey(void);                           // Read keyboard and mouse
int  initmouse(void);                        // Check if mouse present
void processmouse(void);                     // Read mouse counters
void setLEDs(int);                           // Set the keyboard LEDs
int  sendframe(void);                        // Send a frame to the ZVG
void ShutdownAll(void);                      // Shut down everything
void drawvector(point, point, float, float); // draw a vector between 2 points
void RunGame(char*, char*);                  // Start MAME with a gamename
void mousepos(int*, int*);                   // Get the position of the mouse
int  GetModifierStatus(void);                // Get status of modifier keys
void setcolour(int, int);                    // set colour and brightness
void playsound(int);                         // Dummy routine
int  InitialiseSEAL(int);							// Initialise SEAL and load samples
void CloseSEAL(void);								// Close SEAL and free up samples

#endif

