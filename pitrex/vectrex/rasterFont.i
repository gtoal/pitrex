/*
From Vectrex BIOS:


Char_Table:     FDB     $0020,$5050,$20C8,$2010,$1040,$2000,$0000,$0008
                FDB     $3020,$7070,$10F8,$30F8,$7070,$0060,$0000,$0070
                FDB     $7020,$F070,$F0F8,LF878,$8870,$0888,$8088,$88F8
                FDB     $F070,$F070,$F888,$8888,$8888,$F870,$8070,$2000
                FDB     $0020,$0820,$0000,$0038,$1020,$4444,$00FE,$FFFE

                FDB     $0070,$5050,$78C8,$5020,$2020,$A820,$0000,$0008
                FDB     $4860,$8888,$3080,$4008,$8888,$6060,$1000,$4088
                FDB     $8850,$4888,$4880,$8080,$8820,$0890,$80D8,$C888
                FDB     $8888,$8888,$A888,$8888,$8888,$0840,$8008,$5000
                FDB     $0070,$0C20,$7070,$0044,$1070,$0000,$6C82,$FFFE

                FDB     $0070,$50F8,$A010,$5040,$4010,$7020,$0000,$0010
                FDB     $4820,$0808,$50F0,$8010,$8888,$6000,$2078,$2008
                FDB     $A888,$4880,$4880,$8080,$8820,$08A0,$80A8,$A888
                FDB     $8888,$8840,$2088,$8888,$5050,$1040,$4008,$8800
                FDB     $70A8,$0A20,$88F8,$60BA,$3820,$0000,$9282,$FFFE

                FDB     $0020,$0050,$7020,$6000,$4010,$A8F8,$0070,$0020
                FDB     $4820,$7030,$9008,$F020,$7078,$0060,$4000,$1010
                FDB     $B888,$7080,$48E0,$E098,$F820,$08C0,$80A8,$9888
                FDB     $F088,$F020,$2088,$50A8,$2020,$2040,$2008,$0000
                FDB     $FE20,$0820,$88F8,$F0A2,$38F8,$8238,$9282,$FFFE

                FDB     $0000,$00F8,$7040,$A800,$4010,$A820,$4000,$0040
                FDB     $4820,$8008,$F808,$8840,$8808,$6060,$2078,$2020
                FDB     $B0F8,$4880,$4880,$8088,$8820,$08A0,$8088,$8888
                FDB     $80A8,$A010,$2088,$50A8,$5020,$4040,$1008,$0000
                FDB     $FE20,$78A8,$88F8,$F0BA,$7C20,$4444,$6C82,$FFFE

                FDB     $0000,$0050,$2898,$9000,$2020,$0020,$4000,$0080
                FDB     $4820,$8088,$1088,$8880,$8810,$6020,$1000,$4000
                FDB     $8088,$4888,$4880,$8088,$8820,$8890,$8888,$8888
                FDB     $8090,$9088,$2088,$20A8,$8820,$8040,$0808,$0000
                FDB     $4820,$F070,$7070,$6044,$6C50,$3882,$0082,$FFFE

Char_Table_End: FDB     $0020,$0050,$F898,$6800,$1040,$0000,$8000,$8080
                FDB     $3070,$F870,$1070,$7080,$7060,$0040,$0000,$0020
                FDB     $7888,$F070,$F0F8,$8078,$8870,$7088,$F888,$88F8
                FDB     $8068,$8870,$2070,$2050,$8820,$F870,$0870,$00F8
                FDB     $0020,$6020,$0000,$0038,$8288,$0000,$00FE,$FFFE
*/

// ASCI bitmap data
// starting with SPACE = 0x20
// up to XXX = 0x6f
unsigned char rasterline1[]=
{
    0x00,0x20,0x50,0x50,0x20,0xC8,0x20,0x10,0x10,0x40,0x20,0x00,0x00,0x00,0x00,0x08,
    0x30,0x20,0x70,0x70,0x10,0xF8,0x30,0xF8,0x70,0x70,0x00,0x60,0x00,0x00,0x00,0x70,
    0x70,0x20,0xF0,0x70,0xF0,0xF8,0xF8,0x78,0x88,0x70,0x08,0x88,0x80,0x88,0x88,0xF8,
    0xF0,0x70,0xF0,0x70,0xF8,0x88,0x88,0x88,0x88,0x88,0xF8,0x70,0x80,0x70,0x20,0x00,
    0x00,0x20,0x08,0x20,0x00,0x00,0x00,0x38,0x10,0x20,0x44,0x44,0x00,0xFE,0xFF,0xFE
};

unsigned char rasterline2[]=
{
	 0x00,0x70,0x50,0x50,0x78,0xC8,0x50,0x20,0x20,0x20,0xA8,0x20,0x00,0x00,0x00,0x08,
	 0x48,0x60,0x88,0x88,0x30,0x80,0x40,0x08,0x88,0x88,0x60,0x60,0x10,0x00,0x40,0x88,
	 0x88,0x50,0x48,0x88,0x48,0x80,0x80,0x80,0x88,0x20,0x08,0x90,0x80,0xD8,0xC8,0x88,
	 0x88,0x88,0x88,0x88,0xA8,0x88,0x88,0x88,0x88,0x88,0x08,0x40,0x80,0x08,0x50,0x00,
	 0x00,0x70,0x0C,0x20,0x70,0x70,0x00,0x44,0x10,0x70,0x00,0x00,0x6C,0x82,0xFF,0xFE
};
unsigned char rasterline3[]=
{
	 0x00,0x70,0x50,0xF8,0xA0,0x10,0x50,0x40,0x40,0x10,0x70,0x20,0x00,0x00,0x00,0x10,
	 0x48,0x20,0x08,0x08,0x50,0xF0,0x80,0x10,0x88,0x88,0x60,0x00,0x20,0x78,0x20,0x08,
	 0xA8,0x88,0x48,0x80,0x48,0x80,0x80,0x80,0x88,0x20,0x08,0xA0,0x80,0xA8,0xA8,0x88,
	 0x88,0x88,0x88,0x40,0x20,0x88,0x88,0x88,0x50,0x50,0x10,0x40,0x40,0x08,0x88,0x00,
	 0x70,0xA8,0x0A,0x20,0x88,0xF8,0x60,0xBA,0x38,0x20,0x00,0x00,0x92,0x82,0xFF,0xFE
};
unsigned char rasterline4[]=
{
	 0x00,0x20,0x00,0x50,0x70,0x20,0x60,0x00,0x40,0x10,0xA8,0xF8,0x00,0x70,0x00,0x20,
	 0x48,0x20,0x70,0x30,0x90,0x08,0xF0,0x20,0x70,0x78,0x00,0x60,0x40,0x00,0x10,0x10,
	 0xB8,0x88,0x70,0x80,0x48,0xE0,0xE0,0x98,0xF8,0x20,0x08,0xC0,0x80,0xA8,0x98,0x88,
	 0xF0,0x88,0xF0,0x20,0x20,0x88,0x50,0xA8,0x20,0x20,0x20,0x40,0x20,0x08,0x00,0x00,
	 0xFE,0x20,0x08,0x20,0x88,0xF8,0xF0,0xA2,0x38,0xF8,0x82,0x38,0x92,0x82,0xFF,0xFE
};
unsigned char rasterline5[]=
{
	 0x00,0x00,0x00,0xF8,0x70,0x40,0xA8,0x00,0x40,0x10,0xA8,0x20,0x40,0x00,0x00,0x40,
	 0x48,0x20,0x80,0x08,0xF8,0x08,0x88,0x40,0x88,0x08,0x60,0x60,0x20,0x78,0x20,0x20,
	 0xB0,0xF8,0x48,0x80,0x48,0x80,0x80,0x88,0x88,0x20,0x08,0xA0,0x80,0x88,0x88,0x88,
	 0x80,0xA8,0xA0,0x10,0x20,0x88,0x50,0xA8,0x50,0x20,0x40,0x40,0x10,0x08,0x00,0x00,
	 0xFE,0x20,0x78,0xA8,0x88,0xF8,0xF0,0xBA,0x7C,0x20,0x44,0x44,0x6C,0x82,0xFF,0xFE
};
unsigned char rasterline6[]=
{
	 0x00,0x00,0x00,0x50,0x28,0x98,0x90,0x00,0x20,0x20,0x00,0x20,0x40,0x00,0x00,0x80,
	 0x48,0x20,0x80,0x88,0x10,0x88,0x88,0x80,0x88,0x10,0x60,0x20,0x10,0x00,0x40,0x00,
	 0x80,0x88,0x48,0x88,0x48,0x80,0x80,0x88,0x88,0x20,0x88,0x90,0x88,0x88,0x88,0x88,
	 0x80,0x90,0x90,0x88,0x20,0x88,0x20,0xA8,0x88,0x20,0x80,0x40,0x08,0x08,0x00,0x00,
	 0x48,0x20,0xF0,0x70,0x70,0x70,0x60,0x44,0x6C,0x50,0x38,0x82,0x00,0x82,0xFF,0xFE
};
unsigned char rasterline7[]=
{
	 0x00,0x20,0x00,0x50,0xF8,0x98,0x68,0x00,0x10,0x40,0x00,0x00,0x80,0x00,0x80,0x80,
	 0x30,0x70,0xF8,0x70,0x10,0x70,0x70,0x80,0x70,0x60,0x00,0x40,0x00,0x00,0x00,0x20,
	 0x78,0x88,0xF0,0x70,0xF0,0xF8,0x80,0x78,0x88,0x70,0x70,0x88,0xF8,0x88,0x88,0xF8,
	 0x80,0x68,0x88,0x70,0x20,0x70,0x20,0x50,0x88,0x20,0xF8,0x70,0x08,0x70,0x00,0xF8,
	 0x00,0x20,0x60,0x20,0x00,0x00,0x00,0x38,0x82,0x88,0x00,0x00,0x00,0xFE,0xFF,0xFE
};

unsigned char *rasterlines[7]=
{
	rasterline1,rasterline2,rasterline3,rasterline4,rasterline5,rasterline6,rasterline7
};

// positions using scale $7f
// position in 8bit vectrex format
// zeroes before positioning
// leaves zeroed
// print strings given in "C"/vectrex format (depending on delimiter)
//
// only large letters supported
// coordinates given in vectrex 8 bit

// ySize is a negative coordinate in $7f scale, which
// represents the downward movement for each line
//
// !!! empty string not allowed!
int v_printStringRaster(int8_t x, int8_t y, char* _string, int8_t xSize, int8_t ySize, unsigned char delimiter)
{
	int cycles = 1811 -3*234;

	// I don't know WHY - we sometimes need this!
	SET(VIA_port_b, 0x80); // disable ramp, mux = y integrator, enable mux
	
	int8_t yoffset = 0;
	int8_t movementScale = 0x7f;
	int halfOffset =0;
	if ((ABS(x)<64) && (ABS(y)<64))
	{
	  movementScale = 0x7f>>1;
	  x=x<<1;
	  y=y<<1;
	}
	if (ABS(ySize*7)<64) 
	{
	  halfOffset = 1;
	}
	for (int i=0; i<7; i++)
	{
		ZERO_AND_WAIT();
		unsigned char* string = (unsigned char*)_string;
		unsigned char *currentRasterline = rasterlines[i];

		UNZERO();

		// move to position
		v_setScale(movementScale);
		v_moveToImmediate8(x, y);

		// move to line in String (only y-movement)
		if (yoffset!=0)
		{
		  if (halfOffset)
		  {
		    SET_YSH_IMMEDIATE_8(yoffset<<1);
		    SET_XSH_IMMEDIATE_8(0);
		    v_setScale(10);
		  }
		  else
		  {
		    SET_YSH_IMMEDIATE_8(yoffset);
		    SET_XSH_IMMEDIATE_8(0);
		    v_setScale(19);
		  }
		  START_T1_TIMER();
		  WAIT_T1_END();
		}
		else
		{
		  SET(VIA_port_a, 0x00);
		  DELAY_XSH();
		}
#define RASTER_WAIT 17
		// disable ramp and set y moevemtn to 0
		SET(VIA_port_b, 0x80); // disable ramp, mux = y integrator, enable mux
		DELAY_YSH();
		SET(VIA_port_b, 0x81); // disable ramp, mux = y integrator, disable mux
		DELAY_PORT_B_BEFORE_PORT_A();

		// set width of text 
		SET(VIA_port_a, xSize);
		DELAY_XSH();
		
		// prepare for raster output
		SET(VIA_aux_cntl, 0x18);

		SET(VIA_port_b, 0x01); // enable ramp, mux = y integrator, disable mux
		
		// the vectorbeam is moving... now fill the shiftregister ASAP with the bitmap, and wait for 18 cycles to pass

		// print this rasterline!
		do 
		{
			if (i==0) cycles+=234;
			DELAY_CYCLES(RASTER_WAIT); // wait for enable ramp, and printing the last letter

			// draw one char bitmap
			unsigned char charBitmap = currentRasterline[*string-0x20];
			string++;
			SET(VIA_shift_reg, charBitmap);
		} while (*string != delimiter);
		DELAY_CYCLES(6); // disabling ramp takes a certain amount of time! - reduce that from the original shift wait

		SET(VIA_port_b, 0x81); // disable ramp, mux = y integrator, disable mux
		DELAY_CYCLES(4);

		// enable non raster output
#ifdef BEAM_LIGHT_BY_SHIFT
	SET (VIA_aux_cntl, 0x98); //Shift Reg. Enabled, T1 PB7 Enabled
#endif
#ifdef BEAM_LIGHT_BY_CNTL
	SET (VIA_aux_cntl, 0x80); // Shift reg mode = 000 free disable, T1 PB7 enabled 
#endif
		DELAY_CYCLES(2);
		yoffset=yoffset+ySize;
	}
	SET(VIA_shift_reg, 0);
//	ZERO_AND_WAIT();
	ZERO_AND_CONTINUE();
	return cycles;
}




// this function prints a string at the current position
// it doesn't "know" its position
// the function uses the same technique as the vectrex
// print one line forth... than
// goes back the same amount of time and prints the next line
// due to that it is also prone to "drift"

// not working good, timer of pi not exact enough!

void v_printStringRaster_here(char* _string, int8_t xSize, int8_t ySize, unsigned char delimiter)
{
	// prepare for raster output
	SET(VIA_aux_cntl, 0x18);
	for (int i=0; i<7; i++)
	{
		unsigned char* string = (unsigned char*)_string;
		unsigned char *currentRasterline = rasterlines[i];

		// disable ramp and set y moevemtn to 0
		SET(VIA_port_b, 0x81); // disable ramp, mux = y integrator, disable mux
		DELAY_YSH();
		SET(VIA_port_a, 0x00);
		DELAY_PORT_B_BEFORE_PORT_A();
		SET(VIA_port_b, 0x80); // disable ramp, mux = y integrator, enable mux
		DELAY_YSH();
		SET(VIA_port_b, 0x81); // disable ramp, mux = y integrator, disable mux
		DELAY_PORT_B_BEFORE_PORT_A();

		// set width of text 
		SET(VIA_port_a, xSize);
		DELAY_YSH();

		setMarkStart(); // set a clean timer mark befor ramping
		SET(VIA_port_b, 0x01); // enable ramp, mux = y integrator, disable mux
		// the vectorbeam is moving... now fill the shiftregister ASAP with the bitmap, and wait for 18 cycles to pass

		// print this rasterline!
		do 
		{
			DELAY_CYCLES(15); // wait for enable ramp, and printing the last letter
			// draw one char bitmap
			unsigned char charBitmap	= currentRasterline[*string-0x20];
			string++;
			SET(VIA_shift_reg, charBitmap);
		} while (*string != delimiter);
		DELAY_CYCLES(6); // disabling ramp takes a certain amount of time! - reduce that from the original shift wait

		// get a clean timer count, just before disabling the ramp
		int timeTacken = waitFullMicro();

	        SET(VIA_port_b, 0x81); // disable ramp, mux = y integrator, disable mux
		DELAY_CYCLES(4);

// no difference DELAY_CYCLES(14);

/* */

// now go back for exactly the same time


		// set width of text 
		SET(VIA_port_a, -xSize);
		DELAY_YSH();

		setMarkStart(); // set a clean timer mark befor ramping
		SET(VIA_port_b, 0x01); // enable ramp, mux = y integrator, disable mux
		waitMarkEnd(timeTacken); // wait till exactly that timer expired

	        SET(VIA_port_b, 0x81); // disable ramp, mux = y integrator, disable mux
		DELAY_CYCLES(4);
// no difference DELAY_CYCLES(14);

// go down a tiny little bit		
		// set height of text 
		SET(VIA_port_a, ySize);
		DELAY_YSH();
		
		SET(VIA_port_b, 0x80); // disable ramp, mux = y integrator, enable mux
		DELAY_YSH();
		SET(VIA_port_b, 0x81); // disable ramp, mux = y integrator, disable mux
		DELAY_PORT_B_BEFORE_PORT_A();
		SET(VIA_port_a, 0); // x zero
		DELAY_YSH();
		
		SET(VIA_port_b, 0x01); // enable ramp, mux = y integrator, disable mux
		// the vectorbeam is moving... now fill the shiftregister ASAP with the bitmap, and wait for 18 cycles to pass
		DELAY_CYCLES(4);
		SET(VIA_port_b, 0x81); // disable ramp, mux = y integrator, disable mux
// no difference      DELAY_CYCLES(18);
	}
// enable non raster output
#ifdef BEAM_LIGHT_BY_SHIFT
	SET (VIA_aux_cntl, 0x98); //Shift Reg. Enabled, T1 PB7 Enabled
#endif
#ifdef BEAM_LIGHT_BY_CNTL
	SET (VIA_aux_cntl, 0x80); // Shift reg mode = 000 free disable, T1 PB7 enabled 
#endif
}
