/**************************************
vchars.h
Chad Gray, 15 Nov 2009
Function declarations
Vector character set function
Pass it a character, receive an array and size
**************************************/

#ifndef _VCHARS_H_
#define _VCHARS_H_

typedef struct {
   int *array;
   int size;
} vShape;

vShape   fnGetChar(char);                    // point to array for given character

#endif

