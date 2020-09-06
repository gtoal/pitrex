/*
   Ported to Arduino Due by PhOBoZ

   Vector Game "Gyrocks" auf dem Oszilloskop
   Carsten Wartmann 2016/2017 cw@blenderbuch.de
   Fürs Make-Magazin

   Gehackt und basierend auf Trammel Hudsons Arbeit:

   Vector display using the MCP4921 DAC on the teensy3.1.
   More info: https://trmm.net/V.st
*/

/*
   Todo:
   - Alles auf ein System/Skalierung umstellen, fixe integer Mathe
   - implement Bounding Box Collision
   + Enemies und Rocks trennen
   + weniger Schüsse gleichzeitig
   - Enemy und Rocks können Ship schaden
   - Bug in Collisions Erkennung?
    - Enemies schießen
   - Lebenszähler für Ship
   + einfache Punktezählung
   - Feinde Formationen fliegen lassen
   - Alles auf Polarkoord umstellen (Ship/Bullets) für Kollisionsabfrage
   - Explosionen

*/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#define PI 3.14159265358979323846

#include <vectrex/vectrexInterface.h>



#include "std.i"
#include "hershey_font.h"
#include "objects.h"

#define SLOW_MOVE
#define BUFFER_SIZE 4096


/*  *********************** Game Stuff ***************************************************/

// Rock
typedef struct
{
  int16_t t;
  int16_t r;
  int16_t p;
  int16_t x;    // x/y Merker für Kollisionsabfrage
  int16_t y;
  int16_t d;
  int16_t vr;
  int16_t vp;
} rock_t;

// Enemy
typedef struct
{
  int16_t t;
  int16_t r;
  int16_t p;
  int16_t vr;
  int16_t vp;
} enemy_t;

// Ship
typedef struct
{
  int16_t x;
  int16_t y;
  int16_t ax;
  int16_t ay;
  unsigned long firedelay;
} ship_t;

// Star
typedef struct
{
  int16_t x;
  int16_t y;
  int16_t vx;
  int16_t vy;
  int16_t age;
} star_t;

// Bullet
typedef struct
{
  int16_t x;
  int16_t y;
  int16_t rot;
  int16_t vx;
  int16_t vy;
  int16_t age;
} bullet_t;


#define boolean int
#define true 1
#define false 0
#define byte unsigned char

#define HALT  // Auskommentieren um "Handbremse" für zweiten Knopf/Schalter zu lösen (Debug&Screenshot)

// Joystick
#define BUTT 14   // Digital
#define TRIG 15   // Digital
#define THRU 16    // Analog
#define POTX 17   // Analog
#define POTY 18    // Analog
#define DEADX 30  // Deadband X
#define DEADY 30  // Deadband Y

#define FIREDELAY 100   // Zweitverzögerung zwischen zwei Schüssen

// Hintergrundsterne
#define MAX_STARS 30
star_t s[MAX_STARS];

// max. Anzahl der Schüsse
#define MAX_BULLETS 5
bullet_t b[MAX_BULLETS];

// max. Zahl der Asteroiden/Rocks
#define MAX_ROCK 5
rock_t r[MAX_ROCK];

// max. Zahl der Feinde
#define MAX_ENEMY 5
enemy_t e[MAX_ENEMY];

// Infos zum Schiff/Ship speichern
ship_t ship;

// Punktezähler
unsigned int score;

// Frames per Second/Framerate Merker
long fps;

// Schnelle aber ungenaue Sinus Tabelle
const  uint8_t isinTable8[] = {
  0, 4, 9, 13, 18, 22, 27, 31, 35, 40, 44,
  49, 53, 57, 62, 66, 70, 75, 79, 83, 87,
  91, 96, 100, 104, 108, 112, 116, 120, 124, 128,

  131, 135, 139, 143, 146, 150, 153, 157, 160, 164,
  167, 171, 174, 177, 180, 183, 186, 190, 192, 195,
  198, 201, 204, 206, 209, 211, 214, 216, 219, 221,

  223, 225, 227, 229, 231, 233, 235, 236, 238, 240,
  241, 243, 244, 245, 246, 247, 248, 249, 250, 251,
  252, 253, 253, 254, 254, 254, 255, 255, 255, 255,
};

double sinTable[360];
void buildSinTable()
{
  for (int i=0;i<360; i++)
    sinTable[i] = sin(i*(PI/180))*255;
}

void initVars()
{
 for (int i=0; i<MAX_ROCK; i++)
 {
	r[i].t = -1;
	r[i].r = 0;
	r[i].p = 0;
	r[i].x = 0;
	r[i].y = 0;
	r[i].d = 0;
 }
 for (int i=0; i<MAX_ENEMY; i++)
 {
	e[i].t = -1;
	e[i].r = 0;
	e[i].p = 0;
 }
 for (int i=0; i<MAX_BULLETS; i++)
 {
	b[i].age = -1;
 }
 ship.x = 2048;
 ship.y = 1000;
 buildSinTable();

}


double isin(int angle)
{
  if (angle < 0)
  {
    angle = -angle;
    if (angle >= 360) angle %= 360;
    return -sinTable[angle];
  }
  if (angle >= 360) angle %= 360;
  return sinTable[angle];
}
double icos(int x)
{
  return (isin(x + 90));
}




/*
// Schnelle aber ungenaue Sinus Funktion
int isin(int x)
{
  boolean pos = true;  // positive - keeps an eye on the sign.
  uint8_t idx;
  // remove next 6 lines for fastest execution but without error/wraparound
  if (x < 0)
  {
    x = -x;
    pos = !pos;
  }
  if (x >= 360) x %= 360;
  if (x > 180)
  {
    idx = x - 180;
    pos = !pos;
  }
  else idx = x;
  if (idx > 90) idx = 180 - idx;
  if (pos) return isinTable8[idx] / 2 ;
  return -(isinTable8[idx] / 2);
}

// Cosinus
int icos(int x)
{
  return (isin(x + 90));
}
*/


/* ************************************** vector output stuff **************************************/

// relative functions are not clipped anymore "here"
// clipping could be done in vectrex support
void movetoRelative(int x, int y)
{
  int16_t px, py;
  px = x;
  py = y;

  py = py*16 ;
  px = px*16 ;
  v_directDeltaMove32start(px, py);
  v_directDeltaMoveEnd();
}

void linetoRelative(int x, int y)
{
  int16_t px, py;

  px = x;
  py = y;
  py = py*16 ;
  px = px*16 ;
  v_directDeltaDraw32(px, py, currentZSH);
}

// absolut from 0,0 (center)
void moveto(int x, int y)
{
  int16_t px, py;

  //Test!  Very stupid "Clipping"
  if (x >= 4096) x = 4095;
  if (y >= 4096) y = 4095;
  if (x < 0) x = 0;
  if (y < 0) y = 0;

  px = x & 0xFFF;
  py = y & 0xFFF;

  py = py*16 -32768;
  px = px*16 -32768;
  v_directMove32(px,py);
}

int draw_character(char c, int x, int y, int size)
{
  const hershey_char_t * const f = &hershey_simplex[c - ' '];
  int next_moveto = 1;

  moveto(x , y );
  int posx = 0;
  int posy = 0;

  for (int i = 0 ; i < f->count ; i++)
  {
    int dx = f->points[2 * i + 0];
    int dy = f->points[2 * i + 1];
    if (dx == -1)
    {
      next_moveto = 1;
      continue;
    }
    dx = (dx * size) * 3 / 4;
    dy = (dy * size) * 3 / 4; //??
    if (next_moveto)
      movetoRelative( dx-posx,  dy-posy);
    else
      linetoRelative( dx-posx,  dy-posy);

   posx=(dx);
   posy=(dy);

   next_moveto = 0;
  }
  return (f->width * size) * 3 / 4;
}


void draw_string(const char * s, int x, int y, int size)
{
  while (*s)
  {
    char c = *s++;
    x += draw_character(c, x, y, size);
  }
}



static void  init_stars(star_t * const stars)
{
  for (uint8_t i = 0 ; i < MAX_STARS ; i++)
  {
    star_t * const s = &stars[i];
    s->x = rand() % 500 + 1750;
    s->y = rand() % 500 + 1750;

    s->vx = rand() % 8 - 4 ;
    s->vy = rand() % 8 - 4 ;
    s->age = rand() % 300;
  }
}

// Debug draw
void draw_rect(int x0, int y0, int x1, int y1)
{
  return;    //Debug!
/*
  moveto(x0, y0);
  lineto(x1, y0);
  lineto(x1, y1);
  lineto(x0, y1);
  lineto(x0, y0);
*/
}

/* ***************************** Game Stuff **************************************************/

// Ähnlich draw_string aber mit definierter Rotation
void draw_object(byte c, int x, int y, int size, int rot)
{
  const objects_char_t * const f = &gobjects[c];
      moveto(x , y );

  int next_moveto = 1;
  int dxx, dyy;
  int posx = 0;
  int posy = 0;

  for (int i = 0 ; i < f->count ; i++)
  {
    int dx = f->points[2 * i + 0];
    int dy = f->points[2 * i + 1];
    if (dx == -127)
    {
      next_moveto = 1;
      continue;
    }
    dxx = ((int)((dx * icos(rot)) - dy * isin(rot))) >> 7 ; // Würg irgendwie nicht Standard, KoordSys komisch?
    dyy = ((int)((dy * icos(rot)) + dx * isin(rot))) >> 7 ;

    dx = (dxx * size) * 3 / 4 ;
    dy = (dyy * size) ;

    if (next_moveto)
      movetoRelative( dx-posx,  dy-posy);
    else
      linetoRelative( dx-posx,  dy-posy);

   posx=(dx);
   posy=(dy);

    next_moveto = 0;
  }
}



// Irgndwie verallgemeinern?
int collision_rock(int x, int y, int d)
{
  int x0, y0, x1, y1;

  d = d * 10;
  x0 = x - d;
  y0 = y - d ;
  x1 = x + d;
  y1 = y + d ;

  draw_rect(x0, y0, x1, y1);
  for (uint8_t i = 0 ; i < MAX_ROCK ; i++)
  {
    if (r[i].t >= 0)
    {
      if (r[i].x > x0 && r[i].x < x1 && r[i].y > y0 && r[i].y < y1)
      {
        //        r[i].t = -1;    //Kill rock also?
        return 1; //Collision with Bullet
      }
    }
  }
  return 0; // No Collision
}



int collision_bullet(int x, int y, int d)
{
  int x0, y0, x1, y1;

  d = d * 10;
  x0 = x - d / 2;
  y0 = y - d / 2;
  x1 = x + d / 2;
  y1 = y + d / 2;

  for (uint8_t i = 0 ; i < MAX_BULLETS ; i++)
  {
    //    bullet_t * const b = &bullets[i];
    if (b[i].age >= 0)
    {
      if (b[i].x > x0 && b[i].x < x1 && b[i].y > y0 && b[i].y < y1)
      {
        b[i].age = -1;
        return 1; //Kollision mit Schuss/Bullet
      }
    }
  }
  return 0; // Keine Kollision
}


// Neuer Schuß wenn eine Slot frei (age==-1)
static void add_bullet(bullet_t * const bullets, ship_t * const ship, int rot)
{
  for (uint8_t i = 0 ; i < MAX_BULLETS ; i++)
  {
    bullet_t * const b = &bullets[i];
    if (b->age < 0)
    {
      b->x = ship->x;
      b->y = ship->y;

      b->rot = rot;
      b->vx = ship->ax ;
      b->vy = ship->ay ;
      b->age = 1;
      break;
    }
  }
}


// Updating bullets/Schüsse
static void update_bullets(bullet_t * const bullets)
{
  for (uint8_t i = 0 ; i < MAX_BULLETS ; i++)
  {
    bullet_t * const b = &bullets[i];
    if (b->age >= 0)
    {
      if (b->age > 100 || (b->x > 2000 - 96 && b->x < 2000 + 96 && b->y > 2000 - 96 && b->y < 2000 + 96))
      {
        b->age = -1;
      }
      else
      {
        b->age++;
        b->x = b->x + (b->vx >> 1);
        b->y = b->y + (b->vy >> 1);
        draw_object(6, b->x, b->y, 10, b->rot);
      }
    }
  }
}


// Schiff Verwaltung
static void update_ship(ship_t * const ship)
{
  long d;
  int rot;

  d = ((2048 - ship->x) * (2048 - ship->x) + (2048 - ship->y) * (2048 - ship->y)) / 75000;
  if (d > 15) d = 15;
  if (d < 1) d = 1;

  if (collision_rock(ship->x, ship->y, d))
  {
    score = 0;
  }

  rot = atan2(2048 - ship->y, 2048 - ship->x) * 180.0 / PI - 90;  // different coord sys...?! Float... hmm

  // button 4 fires
  if ((((currentButtonState&0x08) == (0x08))) && v_millis() > (ship->firedelay + FIREDELAY))
  {
    ship->firedelay = v_millis();
    ship->ax = ((int)-isin(rot)) >> 1  ;
    ship->ay = ((int)icos(rot)) >> 1  ;
    add_bullet(b, ship, rot);
  }

  draw_object(3, ship->x, ship->y, d, rot);               // Ship
  draw_object(4, ship->x, ship->y, d + rand() % d, rot);  // Engine
}


// Hintergrund
static void  update_stars(star_t * const stars)
{
  int age2;
  for (uint8_t i = 0 ; i < MAX_STARS ; i++)
  {
    star_t * const s = &stars[i];
    s->age++;
    age2 = s->age * s->age >> 12;
    s->x = s->x + (s->vx * age2);
    s->y = s->y + (s->vy * age2);
    if (s->x > 4000 || s->x < 96 || s->y > 4000 || s->y < 96 || s->age > 200)
    {
      s->x = rand() % 50 + 2000;
      s->y = rand() % 50 + 2000;
      s->vx = rand() % 8 - 4 ;
      s->vy = rand() % 8 - 4 ;
      s->age = 0;
    }
    draw_character(43, s->x, s->y, age2 >> 1);  // Using a "+" char...
  }
}


// Felsen/Rock/Asteroid
static void  add_rock(rock_t * const rock)
{
  for (uint8_t i = 0 ; i < MAX_ROCK ; i++)
  {
    rock_t * const rr = &rock[i];
    if (rr->t == -1)
    {
      rr->t = rand() % 2 + 13;
      rr->r = rand() % 1000 + 1;
      rr->p = rand() % 359 * 16;

      rr->vr = rand() % 30 + 15;
      rr->vp = rand() % 10 - 5  ;
      break;
    }
  }
}

static void  update_rocks(rock_t * const rr)
{
  int x, y;
  for (uint8_t i = 0 ; i < MAX_ROCK ; i++)
  {
    //rock_t * const rr = &rock[i];
    if (rr[i].t >= 0) // nur wenn live/type gesetzt
    {
      if (rr[i].r < 30000 )
      {
        rr[i].r += rr[i].vr;
      }
      else
      {
        rr[i].t = -1;
        continue;
      }
      rr[i].p = (rr[i].p + rr[i].vp) % (360 * 16) ;
      x = 2048 + (rr[i].r / 16 * icos(rr[i].p / 16)) / 100;
      y = 2048 + (rr[i].r / 16 * isin(rr[i].p / 16)) / 100;
      rr[i].x = x;
      rr[i].y = y;  // Keep track, x,y ToDo: raus oder auf Polarkoords
      rr[i].d = rr[i].r / 512;

      if (collision_bullet(x, y, rr[i].r / 512))
      {
        rr[i].t = -1;
        rr[i].r = 0;
        score += 10;
      }
      else
      {
        draw_object(rr[i].t, x, y, rr[i].d, -rr[i].p / 4);
        draw_rect(r[i].x - r[i].d * 6, r[i].y - r[i].d * 6, r[i].x + r[i].d * 6, r[i].y + r[i].d * 6); // debug
      }
    }
  }
}


// Enemies
static void  add_enemy(enemy_t * const enemy)
{
  for (uint8_t i = 0 ; i < MAX_ENEMY ; i++)
  {
    enemy_t * const rr = &enemy[i];
    if (rr->t == -1)
    {
      rr->t = rand() % 4 + 18;
      rr->r = rand() % 1000 + 1;
      rr->p = rand() % 359 * 16;

      rr->vr = rand() % 30 + 15;
      rr->vp = rand() % 10 - 5  ;
      break;
    }
  }
}

static void  update_enemies(enemy_t * const rr)
{
  int x, y;
  for (uint8_t i = 0 ; i < MAX_ENEMY ; i++)
  {
    if (rr[i].t >= 0) // nur wenn live/type gesetzt
    {
      if (rr[i].r < 10000 )
      {
        rr[i].r += rr[i].vr;
      }

      rr[i].p = (rr[i].p + rr[i].vp) % (360 * 16) ;
      x = 2048 + (rr[i].r / 16 * icos(rr[i].p / 16)) / 100;
      y = 2048 + (rr[i].r / 16 * isin(rr[i].p / 16)) / 100;

      if (collision_bullet(x, y, rr[i].r / 512))
      {
        rr[i].t = -1;
        rr[i].r = 0;
        score += 100;
      }
      else
      {
        draw_object(rr[i].t, x, y, rr[i].r / 512, -rr[i].p / 4);
      }
    }
  }
}

void draw_field()
{
#define CORNER 500

  moveto(0, CORNER/16);
  linetoRelative(0, (int)(-CORNER/16));
  linetoRelative((int)(CORNER/16), 0);

  moveto(4095 - CORNER/16, 0);
  linetoRelative((int)(CORNER/16), 0);
  linetoRelative(0, (int)(CORNER/16));

  moveto(4095, 4095 - CORNER/16);
  linetoRelative(0, (int)(CORNER/16));
  linetoRelative((int)(-CORNER/16), 0);

  moveto(CORNER/16, 4095);
  linetoRelative( (int)(-CORNER/16), 0);
  linetoRelative(0,  (int)(-CORNER/16));
}




int constrain(int in, int min, int max)
{
	return in < min ?min :(in > max ? max : in);
}


// Anzeige Funktion
void video()
{
  // Joystick auslesen
  if (currentJoy1X > DEADX || currentJoy1X < -DEADX)
  {
    ship.x = ship.x + currentJoy1X;
  }
  if (currentJoy1Y > DEADY || currentJoy1Y < -DEADY)
  {
    ship.y = ship.y + currentJoy1Y;
  }

  ship.x = constrain(ship.x, 400, 3700);
  ship.y = constrain(ship.y, 400, 3700);

  update_stars(s);
  update_bullets(b);
  update_rocks(r);
  if (rand() % 500 == 1) add_rock(r);
  update_enemies(e);
  if (rand() % 500 == 1) add_enemy(e);
  update_ship(&ship);
  draw_field();
}

extern int vectrexinit (char viaconfig);

#define SETTINGS_SIZE 1024
unsigned char settingsBlob[SETTINGS_SIZE];

/* Setup all */
void setup()
{
  vectrexinit(1);
  v_init();

  v_loadSettings("gyrocks", settingsBlob, SETTINGS_SIZE);

  initVars();
  init_stars(s);
}

void startFrame()
{
    v_readButtons();
    v_readJoystick1Analog();
    v_WaitRecal();
    v_setBrightness(64);        /* set intensity of vector beam... */
}

// Hauptfunktion
void main()
{
  printf("Gyrocks Main\r\n");
  setup();

  int clockAvailable = 0;

  long start_time = v_micros();
  clockAvailable = start_time!=0;

  printf("setup done\r\n");
  while (1)
  {
    char buf[12];

    video();
    // Punktezähler ausgeben
    draw_string("Points:", 100, 150, 6);
    draw_string(itoa(score, buf, 10), 800, 150, 6);

    // FPS Todo: Debug Switch?!
    draw_string("FPS:", 3000, 150, 6);

    if (clockAvailable)
    {
      draw_string(itoa(fps, buf, 10), 3400, 150, 6);
      fps = 1000000 / (v_micros() - start_time);
      start_time = v_micros();
    }
    else
    {
      draw_string("n/a", 3400, 150, 6);
    }
    startFrame();
  }
}
