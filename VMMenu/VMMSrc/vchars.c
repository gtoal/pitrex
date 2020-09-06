/*****************************************************************************
* Vector Mame Menu - Character set
*
* Author:  Chad Gray
* Created: 10/11/09 - 22/06/11
*
*****************************************************************************/

//#include <stdio.h>
#include "vchars.h"


/**************************************
       Select vector character
**************************************/
vShape fnGetChar(char cEncode)
{
   vShape c;
// Major Havoc style charcters, you'll need to adjust the printing width=4 (4x4)
/*
   static int char_A[] = { 0,0,0,2, 0,2,2,4, 2,4,4,4, 4,4,4,0, 0,2,4,2 };
   static int char_B[] = { 0,0,0,4, 0,4,3,4, 3,4,4,3, 4,3,4,0, 4,0,0,0, 0,2,4,2 };
   static int char_C[] = { 4,4,1,4, 1,4,0,2, 0,2,2,0, 2,0,4,0 };
   static int char_D[] = { 0,0,0,4, 0,4,3,4, 3,4,4,2, 4,2,3,0, 3,0,0,0 };
   static int char_E[] = { 4,0,0,0, 0,0,0,4, 0,4,4,4, 0,2,3,2 };
   static int char_F[] = { 0,0,0,4, 0,4,4,4, 0,2,3,2 };
   static int char_G[] = { 4,4,1,4, 1,4,0,2, 0,2,1,0, 1,0,4,0, 4,0,4,2, 4,2,2,2 };
   static int char_H[] = { 0,0,0,4, 0,2,4,2, 4,0,4,4 };
   static int char_I[] = { 1,0,3,0, 2,0,2,4, 1,4,3,4 };
   static int char_J[] = { 0,1,2,0, 2,0,3,1, 3,1,3,4 };
   static int char_K[] = { 0,0,0,4, 0,2,3,2, 3,2,4,4, 3,2,4,0 };
   static int char_L[] = { 0,4,0,0, 0,0,4,0 };
   static int char_M[] = { 0,0,0,4, 0,4,2,2, 2,2,4,4, 4,4,4,0 };
   static int char_N[] = { 0,0,0,4, 0,4,4,0, 4,0,4,4 };
   static int char_O[] = { 0,2,1,4, 1,4,3,4, 3,4,4,2, 4,2,3,0, 3,0,1,0, 1,0,0,2 };
   static int char_P[] = { 0,0,0,4, 0,4,3,4, 3,4,4,3, 4,3,3,2, 3,2,0,2 };
   static int char_Q[] = { 0,2,1,4, 1,4,3,4, 3,4,4,2, 4,2,3,0, 3,0,1,0, 1,0,0,2, 3,1,4,0 };
   static int char_R[] = { 0,0,0,4, 0,4,3,4, 3,4,4,3, 4,3,3,2, 3,2,0,2, 3,2,4,0 };
   static int char_S[] = { 0,0,3,0, 3,0,4,1, 4,1,0,3, 0,3,1,4, 1,4,3,4 };
   static int char_T[] = { 0,4,4,4, 2,4,2,0 };
   static int char_U[] = { 0,4,0,1, 0,1,2,0, 2,0,4,1, 4,1,4,4 };
   static int char_V[] = { 0,4,3,0, 3,0,3,4 };
   static int char_W[] = { 0,4,2,0, 2,0,2,4, 2,4,4,0, 4,0,4,4 };
   static int char_X[] = { 0,0,4,4, 0,4,4,0 };
   static int char_Y[] = { 0,4,2,2, 0,0,4,4 };
   static int char_Z[] = { 0,4,4,4, 4,4,0,0, 0,0,4,0 };
*/

// Standard character set (2x4)
   static int char_A[] = {  0,0,0,2, 0,2,1,4, 1,4,2,2, 2,2,2,0, 0,2,2,2 };
   //static int char_A[] = {  0,0,2,4, 2,4,2,0, 1,2,2,2 };
   //static int char_B[] = {  0,0,0,4, 0,4,2,3, 2,3,1,2, 1,2,0,2, 1,2,2,1, 2,1,0,0 };
   static int char_B[] = {  0,0,0,4, 0,4,1,4, 1,4,2,3, 2,3,1,2, 1,2,2,1, 2,1,1,0, 1,0,0,0, 0,2,1,2 };
   //static int char_C[] = {  2,1,1,0, 1,0,0,1, 0,1,0,3, 0,3,1,4, 1,4,2,3 };
   static int char_C[] = {  2,0,0,0, 0,0,0,4, 0,4,2,4 };
   static int char_D[] = {  0,0,1,0, 1,0,2,1, 2,1,2,3, 2,3,1,4, 1,4,0,4, 0,4,0,0 };
   static int char_E[] = {  2,0,0,0, 0,0,0,4, 0,4,2,4, 0,2,1,2 };
   static int char_F[] = {  0,0,0,4, 0,4,2,4, 0,2,1,2 };
   //static int char_G[] = {  2,1,1,0, 1,0,0,1, 0,1,0,3, 0,3,1,4, 1,4,2,3, 1,2,2,2, 2,2,2,0 };
   static int char_G[] = {  2,4,0,4, 0,4,0,0, 0,0,2,0, 2,0,2,2, 2,2,1,2 };
   static int char_H[] = {  0,0,0,4, 2,0,2,4, 0,2,2,2 };
   static int char_I[] = {  1,0,1,4, 0,4,2,4, 0,0,2,0 };
   //static int char_J[] = {  0,4,2,4, 2,4,2,1, 2,1,1,0, 1,0,0,1, 0,1,0,2 };
   static int char_J[] = {  2,4,2,0, 2,0,0,0, 0,0,0,1 };
   static int char_K[] = {  0,0,0,4, 0,2,2,4, 0,2,2,0 };
   static int char_L[] = {  0,4,0,0, 0,0,2,0 };
   static int char_M[] = {  0,0,0,4, 0,4,1,2, 1,2,2,4, 2,4,2,0 };
   static int char_N[] = {  0,0,0,4, 0,4,2,0, 2,0,2,4 };
   //static int char_O[] = {  1,0,2,1, 2,1,2,3, 2,3,1,4, 1,4,0,3, 0,3,0,1, 0,1,1,0 };
   static int char_O[] = {  0,0,2,0, 2,0,2,4, 2,4,0,4, 0,4,0,0 };
   //static int char_P[] = {  0,0,0,4, 0,4,2,3, 2,3,1,2, 1,2,0,2 };
   //static int char_P[] = {  0,0,0,4, 0,4,1,4, 1,4,2,3, 2,3,1,2, 1,2,0,2 };
   static int char_P[] = {  0,0,0,4, 0,4,2,4, 2,4,2,2, 2,2,0,2 };
   //static int char_Q[] = {  1,0,2,1, 2,1,2,3, 2,3,1,4, 1,4,0,3, 0,3,0,1, 0,1,1,0, 1,1,2,0 };
   static int char_Q[] = {  0,0,0,4, 0,4,2,4, 2,4,2,0, 2,0,0,0, 1,1,2,0 };
   //static int char_R[] = {  0,0,0,4, 0,4,2,3, 2,3,1,2, 1,2,0,2, 1,2,2,0 };
   static int char_R[] = {  0,0,0,4, 0,4,2,4, 2,4,2,2, 2,2,0,2, 1,2,2,0 };
   //static int char_S[] = {  0,1,0,0, 0,0,2,0, 2,0,2,2, 2,2,0,2, 0,2,0,4, 0,4,2,4, 2,4,2,3 };
   static int char_S[] = {  0,0,2,0, 2,0,2,2, 2,2,0,2, 0,2,0,4, 0,4,2,4 };
   static int char_T[] = {  1,0,1,4, 0,4,2,4 };
   //static int char_U[] = {  0,4,0,1, 0,1,1,0, 1,0,2,1, 2,1,2,4 };
   static int char_U[] = {  0,4,0,0, 0,0,2,0, 2,0,2,4 };
   static int char_V[] = {  0,4,1,0, 1,0,2,4 };
   static int char_W[] = {  0,4,0,0, 0,0,1,2, 1,2,2,0, 2,0,2,4 };
   static int char_X[] = {  0,4,2,0, 0,0,2,4 };
   static int char_Y[] = {  0,4,1,2, 1,2,2,4, 1,2,1,0 };
   static int char_Z[] = {  0,4,2,4, 2,4,0,0, 0,0,2,0 };


   static int char_1[] = {  0,2,1,4, 1,4,1,0, 0,0,2,0 };
   static int char_2[] = {  0,3,1,4, 1,4,2,3, 2,3,2,2, 2,2,0,0, 0,0,2,0 };
   //static int char_3[] = {  0,4,2,4, 2,4,2,0, 2,0,0,0, 1,2,2,2 };
   static int char_3[] = {  0,4,2,4, 2,4,1,2, 1,2,2,2, 2,2,2,0, 2.0,0,0 };
   static int char_4[] = {  1,0,1,4, 1,4,0,1, 0,1,2,1 };
   static int char_5[] = {  2,4,0,4, 0,4,0,3, 0,3,1,3, 1,3,2,2, 2,2,2,1, 2,1,1,0, 1,0,0,1 };
   static int char_6[] = {  2,4,1,4, 1,4,0,2, 0,2,0,0, 0,0,2,0, 2,0,2,2, 2,2,0,2 };
   static int char_7[] = {  1,0,1,2, 1,2,2,4, 2,4,0,4 };
   static int char_8[] = {  0,1,1,0, 1,0,2,1, 2,1,0,3, 0,3,1,4, 1,4,2,3, 2,3,0,1 };
   static int char_9[] = {  0,0,1,0, 1,0,2,2, 2,2,2,4, 2,4,0,4, 0,4,0,2, 0,2,2,2 };
   static int char_0[] = {  1,0,2,1, 2,1,2,3, 2,3,1,4, 1,4,0,3, 0,3,0,1, 0,1,1,0, 0,1,2,3 };

   static int char_excl[] = {  1,0,1,0, 1,1,1,4 };
   static int char_ques[] = {  1,0,1,0, 1,1,1,2, 1,2,2,3, 2,3,1,4, 1,4,0,3 };
   static int char_plus[] = {  1,1,1,3, 0,2,2,2 };
   static int char_minus[] ={  0,2,2,2 };
   static int char_dot[] =  {  1,0,1,0 };
   static int char_comma[] ={  1,0,0,-1 };
//   static int char_at[] =   {  0,0,0,2, 0,2,2,2, 2,2,2,0, 2,0,1,0, 1,0,1,1, 1,1,2,1 };
   static int char_at[] =   {  2,0,1,0, 1,0,0,1, 0,1,0,2, 0,2,1,3, 1,3,2,3, 2,3,3,2, 3,2,3,1, 3,1,1,1, 1,1,1,2, 1,2,2,2, 2,2,2,1 };
   static int char_bra[] =  {  2,0,1,1, 1,1,1,3, 1,3,2,4 };
   static int char_ket[] =  {  1,0,2,1, 2,1,2,3, 2,3,1,4 };
   static int char_slash[] ={  0,0,2,4 };
   static int char_gt[] =   {  0,1,2,1, 2,1,2,0, 2,0,5,2, 5,2,2,4, 2,4,2,3, 2,3,0,3, 0,3,0,1 };
   static int char_lt[] =   {  0,2,3,0, 3,0,3,1, 3,1,5,1, 5,1,5,3, 5,3,3,3, 3,3,3,4, 3,4,0,2 };
   static int char_star[] = {  1,0,1,4, -1,2,3,2,   0,1,2,3, 2,1,0,3 };
   static int char_hash[] = {  0,1,1,0, 1,0,3,4 };
   static int char_space[] ={  } ;

//   static int char_a[] = {  2,1,0,1, 0,1,0,0, 0,0,2,0, 2,0,2,1, 2,1,1,2, 1,2,0,2 };
   static int char_a[] = {  0,2,2,2, 2,2,2,0, 2,0,0,0, 0,0,0,1, 0,1,2,1 };

//   static int char_b[] = {  0,4,0,0, 0,1,1,0, 1,0,2,1, 2,1,1,2, 1,2,0,2 };
   static int char_b[] = {  0,4,0,0, 0,0,2,0, 2,0,2,2, 2,2,0,2 };

//   static int char_c[] = {  2,2,1,2, 1,2,0,1, 0,1,1,0, 1,0,2,0 };
   static int char_c[] = {  2,2,0,2, 0,2,0,0, 0,0,2,0 };

//   static int char_d[] = {  2,4,2,0, 2,1,1,0, 1,0,0,1, 0,1,1,2, 1,2,2,2 };
   static int char_d[] = {  2,4,2,0, 2,0,0,0, 0,0,0,2, 0,2,2,2 };

//   static int char_e[] = {  2,0,1,0, 1,0,0,1, 0,1,1,2, 1,2,2,1, 2,1,0,1 };
   static int char_e[] = {  2,0,0,0, 0,0,0,2, 0,2,2,2, 2,2,2,1, 2,1,0,1 };

//   static int char_f[] = {  1,0,1,3, 1,3,2,4, 0,2,2,2 };
   static int char_f[] = {  1,0,1,4, 1,4,2,4, 0,2,2,2 };

//   static int char_g[] = {  2,0,1,0, 1,0,0,1, 0,1,1,2, 1,2,2,1, 2,2,2,-1,   2,-1,1,-2,  1,-2,0,-2 };
   static int char_g[] = {  2,0,0,0, 0,0,0,2, 0,2,2,2, 2,2,2,-2, 2,-2,0,-2 };

//   static int char_h[] = {  0,0,0,4, 0,1,1,2, 1,2,2,1, 2,1,2,0 };
   static int char_h[] = {  0,0,0,4, 0,2,2,2, 2,2,2,0 };

   static int char_i[] = {  1,0,1,2, 1,2,0,2, 1,3,1,3 };
   static int char_j[] = {  0,2,1,2, 1,2,1,-1,   1,-1,0,-2,  1,3,1,3 };
   static int char_k[] = {  0,0,0,4, 2,0,0,1, 0,1,2,2 };

//   static int char_l[] = {  1,0,0,0, 0,0,0,4 };
   static int char_l[] = {  2,0,1,0, 1,0,1,4 };

//   static int char_m[] = {  0,0,0,2, 0,2,1,1, 1,1,2,2, 2,2,2,0 };
   static int char_m[] = {  0,0,0,2, 0,2,2,2, 2,2,2,0, 1,2,1,0 };

//   static int char_n[] = {  0,0,0,2, 0,1,1,2, 1,2,2,2, 2,2,2,0 };
   static int char_n[] = {  0,0,0,2, 0,2,2,2, 2,2,2,0 };

//   static int char_o[] = {  1,0,0,1, 0,1,1,2, 1,2,2,1, 2,1,1,0 };
   static int char_o[] = {  0,0,2,0, 2,0,2,2, 2,2,0,2, 0,2,0,0 };

//   static int char_p[] = {  0,0,1,0, 1,0,2,1, 2,1,1,2, 1,2,0,1, 0,2,0,-2 };
   static int char_p[] = {  0,0,2,0, 2,0,2,2, 2,2,0,2, 0,2,0,-2 };

//   static int char_q[] = {  2,0,1,0, 1,0,0,1, 0,1,1,2, 1,2,2,1, 2,2,2,-2 };
   static int char_q[] = {  2,0,0,0, 0,0,0,2, 0,2,2,2, 2,2,2,-2 };

//   static int char_r[] = {  0,0,0,2, 0,1,1,2, 1,2,2,2 };
   static int char_r[] = {  0,0,0,2, 0,2,2,2 };

//   static int char_s[] = {  0,0,1,0, 1,0,2,1, 2,1,0,1, 0,1,1,2, 1,2,2,2 };
   static int char_s[] = {  0,0,2,0, 2,0,2,1, 2,1,0,1, 0,1,0,2, 0,2,2,2 };

//   static int char_t[] = {  0,3,0,1, 0,1,1,0, 1,0,2,0, 0,2,1,2 };
   static int char_t[] = {  0,3,0,0, 0,0,2,0, 0,2,1,2 };

//   static int char_u[] = {  0,2,0,0, 0,0,1,0, 1,0,2,1, 2,0,2,2 };
   static int char_u[] = {  0,2,0,0, 0,0,2,0, 2,0,2,2 };

   static int char_v[] = {  0,2,1,0, 1,0,2,2 };

//   static int char_w[] = {  0,2,0,0, 0,0,1,1, 1,1,2,0, 2,0,2,2 };
   static int char_w[] = {  0,2,0,0, 0,0,2,0, 2,0,2,2, 1,0,1,2 };

   static int char_x[] = {  0,0,2,2, 0,2,2,0 };
//   static int char_y[] = {  0,2,0,1, 0,1,1,0, 1,0,2,1, 2,2,2,-1,   2,-1,1,-2,  1,-2,0,-2 };
   static int char_y[] = {  0,2,0,0, 0,0,2,0, 2,2,2,-2, 2,-2,0,-2 };

   static int char_z[] = {  0,2,2,2, 2,2,0,0, 0,0,2,0 };

//10   static int char_backslash[] = { 0,0,2,0, 2,0,3,1, 3,1,3,3, 3,3,2,4, 2,4,0,4, 0,4,-1,3, -1,3,-1,1, -1,1,0,0, 0,1,0,3, 1,1,2,1, 2,1,2,3, 2,3,1,3, 1,3,1,1 };
//1c   static int char_backslash[] = { 0,0,2,0, 2,0,3,1, 3,1,3,3, 3,3,2,4, 2,4,0,4, 0,4,-1,3, -1,3,-1,1, -1,1,0,0, 0,1,0,3, 2,1,1,1, 1,1,1,2, 1,2,2,2 };
   static int char_pipe[] = { -1,0,3,0, 3,0,4,1, 4,1,4,5, 4,5,3,6, 3,6,-1,6, -1,6,-2,5, -2,5,-2,1, -2,1,-1,0,
                                   -1,2,-1,4, 0,2,0,4, 0,4,1,4, 1,4,1,2, 1,2,0,2, 2,1,2,3, 2,3,3,3, 3,3,3,2, 3,2,2,2 };

   switch (cEncode)
   {
   case 'A':
      c.array = char_A;
      c.size = sizeof(char_A)/sizeof(*char_A);
      break;
   case 'B':
      c.array = char_B;
      c.size = sizeof(char_B)/sizeof(*char_B);
      break;
   case 'C':
      c.array = char_C;
      c.size = sizeof(char_C)/sizeof(*char_C);
      break;
   case 'D':
      c.array = char_D;
      c.size = sizeof(char_D)/sizeof(*char_D);
      break;
   case 'E':
      c.array = char_E;
      c.size = sizeof(char_E)/sizeof(*char_E);
      break;
   case 'F':
      c.array = char_F;
      c.size = sizeof(char_F)/sizeof(*char_F);
      break;
   case 'G':
      c.array = char_G;
      c.size = sizeof(char_G)/sizeof(*char_G);
      break;
   case 'H':
      c.array = char_H;
      c.size = sizeof(char_H)/sizeof(*char_H);
      break;
   case 'I':
      c.array = char_I;
      c.size = sizeof(char_I)/sizeof(*char_I);
      break;
   case 'J':
      c.array = char_J;
      c.size = sizeof(char_J)/sizeof(*char_J);
      break;
   case 'K':
      c.array = char_K;
      c.size = sizeof(char_K)/sizeof(*char_K);
      break;
   case 'L':
      c.array = char_L;
      c.size = sizeof(char_L)/sizeof(*char_L);
      break;
   case 'M':
      c.array = char_M;
      c.size = sizeof(char_M)/sizeof(*char_M);
      break;
   case 'N':
      c.array = char_N;
      c.size = sizeof(char_N)/sizeof(*char_N);
      break;
   case 'O':
      c.array = char_O;
      c.size = sizeof(char_O)/sizeof(*char_O);
      break;
   case 'P':
      c.array = char_P;
      c.size = sizeof(char_P)/sizeof(*char_P);
      break;
   case 'Q':
      c.array = char_Q;
      c.size = sizeof(char_Q)/sizeof(*char_Q);
      break;
   case 'R':
      c.array = char_R;
      c.size = sizeof(char_R)/sizeof(*char_R);
      break;
   case 'S':
      c.array = char_S;
      c.size = sizeof(char_S)/sizeof(*char_S);
      break;
   case 'T':
      c.array = char_T;
      c.size = sizeof(char_T)/sizeof(*char_T);
      break;
   case 'U':
      c.array = char_U;
      c.size = sizeof(char_U)/sizeof(*char_U);
      break;
   case 'V':
      c.array = char_V;
      c.size = sizeof(char_V)/sizeof(*char_V);
      break;
   case 'W':
      c.array = char_W;
      c.size = sizeof(char_W)/sizeof(*char_W);
      break;
   case 'X':
      c.array = char_X;
      c.size = sizeof(char_X)/sizeof(*char_X);
      break;
   case 'Y':
      c.array = char_Y;
      c.size = sizeof(char_Y)/sizeof(*char_Y);
      break;
   case 'Z':
      c.array = char_Z;
      c.size = sizeof(char_Z)/sizeof(*char_Z);
      break;
   case 'a':
      c.array = char_a;
      c.size = sizeof(char_a)/sizeof(*char_a);
      break;
   case 'b':
      c.array = char_b;
      c.size = sizeof(char_b)/sizeof(*char_b);
      break;
   case 'c':
      c.array = char_c;
      c.size = sizeof(char_c)/sizeof(*char_c);
      break;
   case 'd':
      c.array = char_d;
      c.size = sizeof(char_d)/sizeof(*char_d);
      break;
   case 'e':
      c.array = char_e;
      c.size = sizeof(char_e)/sizeof(*char_e);
      break;
   case 'f':
      c.array = char_f;
      c.size = sizeof(char_f)/sizeof(*char_f);
      break;
   case 'g':
      c.array = char_g;
      c.size = sizeof(char_g)/sizeof(*char_g);
      break;
   case 'h':
      c.array = char_h;
      c.size = sizeof(char_h)/sizeof(*char_h);
      break;
   case 'i':
      c.array = char_i;
      c.size = sizeof(char_i)/sizeof(*char_i);
      break;
   case 'j':
      c.array = char_j;
      c.size = sizeof(char_j)/sizeof(*char_j);
      break;
   case 'k':
      c.array = char_k;
      c.size = sizeof(char_k)/sizeof(*char_k);
      break;
   case 'l':
      c.array = char_l;
      c.size = sizeof(char_l)/sizeof(*char_l);
      break;
   case 'm':
      c.array = char_m;
      c.size = sizeof(char_m)/sizeof(*char_m);
      break;
   case 'n':
      c.array = char_n;
      c.size = sizeof(char_n)/sizeof(*char_n);
      break;
   case 'o':
      c.array = char_o;
      c.size = sizeof(char_o)/sizeof(*char_o);
      break;
   case 'p':
      c.array = char_p;
      c.size = sizeof(char_p)/sizeof(*char_p);
      break;
   case 'q':
      c.array = char_q;
      c.size = sizeof(char_q)/sizeof(*char_q);
      break;
   case 'r':
      c.array = char_r;
      c.size = sizeof(char_r)/sizeof(*char_r);
      break;
   case 's':
      c.array = char_s;
      c.size = sizeof(char_s)/sizeof(*char_s);
      break;
   case 't':
      c.array = char_t;
      c.size = sizeof(char_t)/sizeof(*char_t);
      break;
   case 'u':
      c.array = char_u;
      c.size = sizeof(char_u)/sizeof(*char_u);
      break;
   case 'v':
      c.array = char_v;
      c.size = sizeof(char_v)/sizeof(*char_v);
      break;
   case 'w':
      c.array = char_w;
      c.size = sizeof(char_w)/sizeof(*char_w);
      break;
   case 'x':
      c.array = char_x;
      c.size = sizeof(char_x)/sizeof(*char_x);
      break;
   case 'y':
      c.array = char_y;
      c.size = sizeof(char_y)/sizeof(*char_y);
      break;
   case 'z':
      c.array = char_z;
      c.size = sizeof(char_z)/sizeof(*char_z);
      break;
   case '1':
      c.array = char_1;
      c.size = sizeof(char_1)/sizeof(*char_1);
      break;
   case '2':
      c.array = char_2;
      c.size = sizeof(char_2)/sizeof(*char_2);
      break;
   case '3':
      c.array = char_3;
      c.size = sizeof(char_3)/sizeof(*char_3);
      break;
   case '4':
      c.array = char_4;
      c.size = sizeof(char_4)/sizeof(*char_4);
      break;
   case '5':
      c.array = char_5;
      c.size = sizeof(char_5)/sizeof(*char_5);
      break;
   case '6':
      c.array = char_6;
      c.size = sizeof(char_6)/sizeof(*char_6);
      break;
   case '7':
      c.array = char_7;
      c.size = sizeof(char_7)/sizeof(*char_7);
      break;
   case '8':
      c.array = char_8;
      c.size = sizeof(char_8)/sizeof(*char_8);
      break;
   case '9':
      c.array = char_9;
      c.size = sizeof(char_9)/sizeof(*char_9);
      break;
   case '0':
      c.array = char_0;
      c.size = sizeof(char_0)/sizeof(*char_0);
      break;
   case '!':
      c.array = char_excl;
      c.size = sizeof(char_excl)/sizeof(*char_excl);
      break;
   case '?':
      c.array = char_ques;
      c.size = sizeof(char_ques)/sizeof(*char_ques);
      break;
   case '+':
      c.array = char_plus;
      c.size = sizeof(char_plus)/sizeof(*char_plus);
      break;
   case '-':
      c.array = char_minus;
      c.size = sizeof(char_minus)/sizeof(*char_minus);
      break;
   case '.':
      c.array = char_dot;
      c.size = sizeof(char_dot)/sizeof(*char_dot);
      break;
   case ',':
      c.array = char_comma;
      c.size = sizeof(char_comma)/sizeof(*char_comma);
      break;
   case '@':
      c.array = char_at;
      c.size = sizeof(char_at)/sizeof(*char_at);
      break;
   case '(':
      c.array = char_bra;
      c.size = sizeof(char_bra)/sizeof(*char_bra);
      break;
   case ')':
      c.array = char_ket;
      c.size = sizeof(char_ket)/sizeof(*char_ket);
      break;
   case '/':
      c.array = char_slash;
      c.size = sizeof(char_slash)/sizeof(*char_slash);
      break;
   case '>':
      c.array = char_gt;
      c.size = sizeof(char_gt)/sizeof(*char_gt);
      break;
   case '<':
      c.array = char_lt;
      c.size = sizeof(char_lt)/sizeof(*char_lt);
      break;
   case '*':
      c.array = char_star;
      c.size = sizeof(char_star)/sizeof(*char_star);
      break;
   case '|':
      c.array = char_pipe;
      c.size = sizeof(char_pipe)/sizeof(*char_pipe);
      break;
   case '#':
      c.array = char_hash;
      c.size = sizeof(char_hash)/sizeof(*char_hash);
      break;
   default:
      c.array = char_space;
      c.size = 0;
      break;
   }
   return c;
}

