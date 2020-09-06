/* zblast - simple shoot-em-up.
 * Copyright (C) 1993-2000 Russell Marks. See zblast.c for license.
 *
 * anim3d.c - simple anim of (3D version of) ship.
 *
 * I decided to use a fairly noddy 3D algorithm rather than
 * the `synthetic camera' approach, as I'm only rotating the thing
 * about the origin.
 */

#include <stdio.h>
#include <math.h>


/* this is in zblast.c */
extern int drawofsline();


/*

this rough ascii sketch should give you some idea what the co-ords mean:

    H           C            I
     |          /\          |    plan
     |         /  \         |
     |        /    \        |
     |       /      \       |
     |   _,-'B  FG  D`-._   |
     |,-'       '`       `-.|
    A^^^^^^^^^^^^^^^^^^^^^^^^E

                 F
           ___---+--___
   AH___---_B____|___D_---___EI  front
        ---___   |  ___---
              ---+--
                 G

and the co-ords are:

a -3,1,0
b -1,0,0
c 0,-2,0
d 1,0,0
e 3,1,0
f 0,0.5,0.5
g 0,0.5,-0.5
h -3,-2,0
i 3,-2,0

*/

struct line_tag
  {
  double x1,y1,z1;
  double x2,y2,z2;
  };

static struct line_tag lines[]=
  {
  /* AH */	{-3,1,0,	-3,-2,0},
  /* AB */	{-3,1,0,	-1,0,0},
  /* AE */	{-3,1,0,	3,1,0},
  /* BC */	{-1,0,0,	0,-2,0},
  /* CD */	{0,-2,0,	1,0,0},
  /* DE */	{1,0,0,		3,1,0},
  /* EI */	{3,1,0,		3,-2,0},
  /* AF */	{-3,1,0,	0,0.5,0.5},
  /* BF */	{-1,0,0,	0,0.5,0.5},
  /* CF */	{0,-2,0,	0,0.5,0.5},
  /* DF */	{1,0,0,		0,0.5,0.5},
  /* EF */	{3,1,0,		0,0.5,0.5},
  /* AG */	{-3,1,0,	0,0.5,-0.5},
  /* BG */	{-1,0,0,	0,0.5,-0.5},
  /* CG */	{0,-2,0,	0,0.5,-0.5},
  /* DG */	{1,0,0,		0,0.5,-0.5},
  /* EG */	{3,1,0,		0,0.5,-0.5},
  };

#define NUM_OBJS	(sizeof(lines)/sizeof(struct line_tag))

double xang,yang,zang;



int doublesign(double n)
{
if(n>0.0)
  return(1);
else
  {
  if(n<0.0)
    return(-1);
  else
    return(0);
  }
}


void remap3d(int *xs,int *ys,double x,double y,double z,
	double xo,double yo,double zo,
	double xang,double yang,double zang,double scaler)
{
double xnew,ynew,znew,length,angle,pi,xx,yy;
pi=3.1415927;
xnew=x; ynew=y; znew=z;

/* can you say sub-optimal? uh-huh, I knew you could :-) */

length=sqrt(xnew*xnew+ynew*ynew);
if(xnew==0.) angle=pi/2*doublesign(ynew); else angle=atan(ynew/xnew);
angle+=xang;
if(x<0.)
  xnew=length*-cos(angle),ynew=length*-sin(angle);
else
  xnew=length*cos(angle),ynew=length*sin(angle);

length=sqrt(xnew*xnew+znew*znew);
if(xnew==0.) angle=pi/2*doublesign(znew); else angle=atan(znew/xnew);
angle+=yang;
xx=xnew;
if(xx<0.)
  xnew=length*-cos(angle),znew=length*-sin(angle);
else
  xnew=length*cos(angle),znew=length*sin(angle);

length=sqrt(ynew*ynew+znew*znew);
if(ynew==0.) angle=pi/2*doublesign(znew); else angle=atan(znew/ynew);
angle+=zang;
yy=ynew;
if(yy<0.)
  ynew=length*-cos(angle),znew=length*-sin(angle);
else
  ynew=length*cos(angle),znew=length*sin(angle);

(*xs)=(int)(scaler*(xnew+0.002*znew*xnew));
(*ys)=(int)(scaler*(ynew+0.002*znew*ynew));
}


void draw3dobject(int objnum,double xo,double yo,double zo,
	double xang,double yang,double zang,double scaler)
{
int xs1,ys1,xs2,ys2;
struct line_tag *l=&(lines[objnum]);

remap3d(&xs1,&ys1,l->x1,l->y1,l->z1,xo,yo,zo,xang,yang,zang,scaler);
remap3d(&xs2,&ys2,l->x2,l->y2,l->z2,xo,yo,zo,xang,yang,zang,scaler);

drawofsline(xs1,-ys1,xs2,-ys2);
}


void view3d(double xo,double yo,double zo,
	double xang,double yang,double zang,double scaler)
{
int objnum;

for(objnum=0;objnum<NUM_OBJS;objnum++)
  draw3dobject(objnum,xo,yo,zo,xang,yang,zang,scaler);
}


void init3d(void)
{
xang=yang=zang=0;
}


void do3dframe(void)
{
view3d(0.,0.,0.,xang,yang,zang,30.);
}


void incr3dpos(void)
{
xang+=.05;
yang+=.15;
zang-=.1;
}
