#include "driver.h"
#include "machine/pd4990a.h"
#include <time.h>

data16_t *neogeo_ram16;
data16_t *neogeo_sram16;

static int sram_locked;
static offs_t sram_protection_hack;

extern int neogeo_has_trackball;


/***************** MEMCARD GLOBAL VARIABLES ******************/
int mcd_action=0;
int mcd_number=0;
int memcard_status=0;		/* 1=Inserted 0=No card */
int memcard_number=0;		/* 000...999, -1=None */
int memcard_manager=0;		/* 0=Normal boot 1=Call memcard manager */
UINT8 *neogeo_memcard;		/* Pointer to 2kb RAM zone */

/*************** MEMCARD FUNCTION PROTOTYPES *****************/
READ16_HANDLER( neogeo_memcard16_r );
WRITE16_HANDLER( neogeo_memcard16_w );
int neogeo_memcard_load(int);
void neogeo_memcard_save(void);
void neogeo_memcard_eject(void);
int neogeo_memcard_create(int);



static void neogeo_custom_memory(void);


/* This function is called on every reset */
void neogeo_init_machine(void)
{
	data16_t src, res, *mem16= (data16_t *)memory_region(REGION_USER1);
	time_t ltime;
	struct tm *today;


	/* Reset variables & RAM */
	memset (neogeo_ram16, 0, 0x10000);

	/* Set up machine country */
	src = readinputport(5);
	res = src & 0x3;

	/* Console/arcade mode */
#ifndef CONSOLE
	res |= 0x8000;
#endif

	/* write the ID in the system BIOS ROM */
	mem16[0x0200] = res;

	if (memcard_manager==1)
	{
		memcard_manager=0;
		mem16[0x11b1a >> 1] = 0x500a;
	}
	else
	{
		mem16[0x11b1a >> 1] = 0x1b6a;
	}

	time(&ltime);
	today = localtime(&ltime);

	pd4990a.seconds = ((today->tm_sec/10)<<4) + (today->tm_sec%10);
	pd4990a.minutes = ((today->tm_min/10)<<4) + (today->tm_min%10);
	pd4990a.hours = ((today->tm_hour/10)<<4) + (today->tm_hour%10);
	pd4990a.days = ((today->tm_mday/10)<<4) + (today->tm_mday%10);
	pd4990a.month = (today->tm_mon + 1);
	pd4990a.year = (((today->tm_year%100)/10)<<4) + (today->tm_year%10);
	pd4990a.weekday = today->tm_wday;
}


/* This function is only called once per game. */
void init_neogeo(void)
{
	extern struct YM2610interface neogeo_ym2610_interface;
	data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
	data8_t *mem08;

	if (memory_region(REGION_SOUND2))
	{
		logerror("using memory region %d for Delta T samples\n",REGION_SOUND2);
		neogeo_ym2610_interface.pcmromb[0] = REGION_SOUND2;
	}
	else
	{
		logerror("using memory region %d for Delta T samples\n",REGION_SOUND1);
		neogeo_ym2610_interface.pcmromb[0] = REGION_SOUND1;
	}

	/* Allocate ram banks */
	neogeo_ram16 = malloc (0x10000);
	cpu_setbank(1, neogeo_ram16);

	/* Set the biosbank */
	cpu_setbank(3, memory_region(REGION_USER1));

	/* Set the 2nd ROM bank */
	mem16 = (data16_t *)memory_region(REGION_CPU1);
	if (memory_region_length(REGION_CPU1) > 0x100000)
	{
		cpu_setbank(4, &mem16[0x80000]);
	}
	else
	{
		cpu_setbank(4, &mem16[0x00000]);
	}

	/* Set the sound CPU ROM banks */
	mem08 = memory_region(REGION_CPU2);
	cpu_setbank(5,&mem08[0x08000]);
	cpu_setbank(6,&mem08[0x0c000]);
	cpu_setbank(7,&mem08[0x0e000]);
	cpu_setbank(8,&mem08[0x0f000]);

	/* Allocate and point to the memcard - bank 5 */
	neogeo_memcard = calloc (0x800, 1);
	memcard_status=0;
	memcard_number=0;


	mem16 = (data16_t *)memory_region(REGION_USER1);

	if (mem16[0x11b00 >> 1] == 0x4eba)
	{
		/* standard bios */
		neogeo_has_trackball = 0;

		/* Remove memory check for now */
		mem16[0x11b00 >> 1] = 0x4e71;
		mem16[0x11b02 >> 1] = 0x4e71;
		mem16[0x11b16 >> 1] = 0x4ef9;
		mem16[0x11b18 >> 1] = 0x00c1;
		mem16[0x11b1a >> 1] = 0x1b6a;

		/* Patch bios rom, for Calendar errors */
		mem16[0x11c14 >> 1] = 0x4e71;
		mem16[0x11c16 >> 1] = 0x4e71;
		mem16[0x11c1c >> 1] = 0x4e71;
		mem16[0x11c1e >> 1] = 0x4e71;

		/* Rom internal checksum fails for now.. */
		mem16[0x11c62 >> 1] = 0x4e71;
		mem16[0x11c64 >> 1] = 0x4e71;
	}
	else
	{
		/* special bios with trackball support */
		neogeo_has_trackball = 1;

		/* TODO: check the memcard manager patch in neogeo_init_machine(), */
		/* it probably has to be moved as well */

		/* Remove memory check for now */
		mem16[0x10c2a >> 1] = 0x4e71;
		mem16[0x10c2c >> 1] = 0x4e71;
		mem16[0x10c40 >> 1] = 0x4ef9;
		mem16[0x10c42 >> 1] = 0x00c1;
		mem16[0x10c44 >> 1] = 0x0c94;

		/* Patch bios rom, for Calendar errors */
		mem16[0x10d3e >> 1] = 0x4e71;
		mem16[0x10d40 >> 1] = 0x4e71;
		mem16[0x10d46 >> 1] = 0x4e71;
		mem16[0x10d48 >> 1] = 0x4e71;

		/* Rom internal checksum fails for now.. */
		mem16[0x10d8c >> 1] = 0x4e71;
		mem16[0x10d8e >> 1] = 0x4e71;
	}

	/* Install custom memory handlers */
	neogeo_custom_memory();
}

/******************************************************************************/
/* Routines to speed up the main processor 				      */
/******************************************************************************/
#define NEO_CYCLE_R(name,pc,hit,other) static READ16_HANDLER( name##_cycle_r ) {if (cpu_get_pc()==pc) {cpu_spin/*until_int*/(); return hit;} return other;}
#define NEO_CYCLE_RX(name,pc,hit,other,xxx) static READ16_HANDLER( name##_cycle_r ) {if (cpu_get_pc()==pc) {if(other==xxx) cpu_spin/*until_int*/(); return hit;} return other;}

NEO_CYCLE_R(puzzledp,0x12f2,1, neogeo_ram16[0x0000>>1])
NEO_CYCLE_R(samsho4, 0xaffc,0, neogeo_ram16[0x830c>>1])
NEO_CYCLE_R(karnovr, 0x5b56,0, neogeo_ram16[0x3466>>1])
NEO_CYCLE_R(wjammers,0x1362e,neogeo_ram16[0x5a>>1]&0x7fff,neogeo_ram16[0x005a>>1])
NEO_CYCLE_R(strhoops,0x029a,0, neogeo_ram16[0x1200>>1])
//NEO_CYCLE_R(magdrop3,0xa378,neogeo_ram16[0x60>>1]&0x7fff,neogeo_ram16[0x0060>>1])
NEO_CYCLE_R(neobombe,0x09f2,0xffff, neogeo_ram16[0x448c>>1])
NEO_CYCLE_R(trally, 0x1295c,neogeo_ram16[0x206>>1]-1,neogeo_ram16[0x0206>>1])
//NEO_CYCLE_R(joyjoy, 0x122c,0xffff, neogeo_ram16[0x0554>>1])
NEO_CYCLE_RX(blazstar,0x3b62,0xffff, neogeo_ram16[0x1000>>1],0)
//NEO_CYCLE_R(ridhero, 0xedb0,0, neogeo_ram16[0x00ca>>1])
NEO_CYCLE_R(cyberlip,0x2218,0x0f0f, neogeo_ram16[0x7bb4>>1])
NEO_CYCLE_R(lbowling,0x37b0,0, neogeo_ram16[0x0098>>1])
NEO_CYCLE_R(superspy,0x07ca,0xffff, neogeo_ram16[0x108c>>1])
NEO_CYCLE_R(ttbb, 0x0a58,0xffff, neogeo_ram16[0x000e>>1])
NEO_CYCLE_R(alpham2,0x076e,0xffff, neogeo_ram16[0xe2fe>>1])
NEO_CYCLE_R(eightman,0x12fa,0xffff, neogeo_ram16[0x046e>>1])
NEO_CYCLE_R(roboarmy,0x08e8,0, neogeo_ram16[0x4010>>1])
NEO_CYCLE_R(fatfury1,0x133c,0, neogeo_ram16[0x4282>>1])
NEO_CYCLE_R(burningf,0x0736,0xffff, neogeo_ram16[0x000e>>1])
NEO_CYCLE_R(bstars, 0x133c,0, neogeo_ram16[0x000a>>1])
NEO_CYCLE_R(kotm, 0x1284,0, neogeo_ram16[0x0020>>1])
NEO_CYCLE_R(gpilots, 0x0474,0, neogeo_ram16[0xa682>>1])
NEO_CYCLE_R(lresort, 0x256a,0, neogeo_ram16[0x4102>>1])
NEO_CYCLE_R(fbfrenzy,0x07dc,0, neogeo_ram16[0x0020>>1])
NEO_CYCLE_R(socbrawl,0xa8dc,0xffff, neogeo_ram16[0xb20c>>1])
NEO_CYCLE_R(mutnat, 0x1456,0, neogeo_ram16[0x1042>>1])
NEO_CYCLE_R(aof, 0x6798,0, neogeo_ram16[0x8100>>1])
//NEO_CYCLE_R(countb, 0x16a2,0, neogeo_ram16[0x8002>>1])
NEO_CYCLE_R(ncombat, 0xcb3e,0, neogeo_ram16[0x0206>>1])
NEO_CYCLE_R(sengoku, 0x12f4,0, neogeo_ram16[0x0088>>1])
//NEO_CYCLE_R(ncommand,0x11b44,0, neogeo_ram16[0x8206>>1])
NEO_CYCLE_R(wh1, 0xf62d4,0xffff, neogeo_ram16[0x8206>>1])
NEO_CYCLE_R(androdun,0x26d6,0xffff, neogeo_ram16[0x0080>>1])
NEO_CYCLE_R(bjourney,0xe8aa,neogeo_ram16[0x206>>1]+1,neogeo_ram16[0x0206>>1])
NEO_CYCLE_R(maglord, 0xb16a,neogeo_ram16[0x206>>1]+1,neogeo_ram16[0x0206>>1])
//NEO_CYCLE_R(janshin, 0x06a0,0, neogeo_ram16[0x0026>>1])
NEO_CYCLE_RX(pulstar, 0x2052,0xffff, neogeo_ram16[0x1000>>1],0)
//NEO_CYCLE_R(mslug ,0x200a,0xffff, neogeo_ram16[0x6ed8>>1])
NEO_CYCLE_R(neodrift,0x0b76,0xffff, neogeo_ram16[0x0424>>1])
NEO_CYCLE_R(spinmast,0x00f6,neogeo_ram16[0xf0>>1]+1, neogeo_ram16[0x00f0>>1])
NEO_CYCLE_R(sonicwi2,0x1e6c8,0xffff, neogeo_ram16[0xe5b6>>1])
NEO_CYCLE_R(sonicwi3,0x20bac,0xffff, neogeo_ram16[0xea2e>>1])
NEO_CYCLE_R(goalx3, 0x5298,neogeo_ram16[0x6>>1]+1, neogeo_ram16[0x0006>>1])
//NEO_CYCLE_R(turfmast,0xd5a8,0xffff, neogeo_ram16[0x2e54>>1])
NEO_CYCLE_R(kabukikl,0x10b0,0, neogeo_ram16[0x428a>>1])
NEO_CYCLE_R(panicbom,0x3ee6,0xffff, neogeo_ram16[0x009c>>1])
NEO_CYCLE_R(wh2, 0x2063fc,neogeo_ram16[0x8206>>1]+1,neogeo_ram16[0x8206>>1])
NEO_CYCLE_R(wh2j, 0x109f4,neogeo_ram16[0x8206>>1]+1,neogeo_ram16[0x8206>>1])
NEO_CYCLE_R(aodk, 0xea62,neogeo_ram16[0x8206>>1]+1,neogeo_ram16[0x8206>>1])
NEO_CYCLE_R(whp, 0xeace,neogeo_ram16[0x8206>>1]+1,neogeo_ram16[0x8206>>1])
NEO_CYCLE_R(overtop, 0x1736,neogeo_ram16[0x8202>>1]+1,neogeo_ram16[0x8202>>1])
NEO_CYCLE_R(twinspri,0x492e,neogeo_ram16[0x8206>>1]+1,neogeo_ram16[0x8206>>1])
NEO_CYCLE_R(stakwin, 0x0596,0xffff, neogeo_ram16[0x0b92>>1])
NEO_CYCLE_R(shocktro,0xdd28,0, neogeo_ram16[0x8344>>1])
NEO_CYCLE_R(tws96, 0x17f4,0xffff, neogeo_ram16[0x010e>>1])
//static READ16_HANDLER( zedblade_cycle_r )
//{
// int pc=cpu_get_pc();
// if (pc==0xa2fa || pc==0xa2a0 || pc==0xa2ce || pc==0xa396 || pc==0xa3fa) {cpu_spinuntil_int(); return 0;}
// return neogeo_ram16[0x9004>>1];
//}
//NEO_CYCLE_R(doubledr,0x3574,0, neogeo_ram16[0x1c30>>1])
NEO_CYCLE_R(galaxyfg,0x09ea,neogeo_ram16[0x1858>>1]+1,neogeo_ram16[0x1858>>1])
NEO_CYCLE_R(wakuwak7,0x1a3c,neogeo_ram16[0x0bd4>>1]+1,neogeo_ram16[0x0bd4>>1])
static READ16_HANDLER( mahretsu_cycle_r )
{
 int pc=cpu_get_pc();
 if (pc==0x1580 || pc==0xf3ba ) {cpu_spinuntil_int(); return 0;}
 return neogeo_ram16[0x13b2>>1];
}
NEO_CYCLE_R(nam1975, 0x0a1c,0xffff, neogeo_ram16[0x12e0>>1])
NEO_CYCLE_R(tpgolf, 0x105c,0, neogeo_ram16[0x00a4>>1])
NEO_CYCLE_R(legendos,0x1864,0xffff, neogeo_ram16[0x0002>>1])
//NEO_CYCLE_R(viewpoin,0x0c16,0, neogeo_ram16[0x1216>>1])
NEO_CYCLE_R(fatfury2,0x10ea,0, neogeo_ram16[0x418c>>1])
NEO_CYCLE_R(bstars2, 0x7e30,0xffff, neogeo_ram16[0x001c>>1])
NEO_CYCLE_R(ssideki, 0x20b0,0xffff, neogeo_ram16[0x8c84>>1])
NEO_CYCLE_R(kotm2, 0x045a,0, neogeo_ram16[0x1000>>1])
static READ16_HANDLER( samsho_cycle_r )
{
 int pc=cpu_get_pc();
 if (pc==0x3580 || pc==0x0f84 ) {cpu_spinuntil_int(); return 0xffff;}
 return neogeo_ram16[0x0a76>>1];
}
NEO_CYCLE_R(fatfursp,0x10da,0, neogeo_ram16[0x418c>>1])
NEO_CYCLE_R(fatfury3,0x9c50,0, neogeo_ram16[0x418c>>1])
NEO_CYCLE_R(tophuntr,0x0ce0,0xffff, neogeo_ram16[0x008e>>1])
NEO_CYCLE_R(savagere,0x056e,0, neogeo_ram16[0x8404>>1])
NEO_CYCLE_R(aof2, 0x8c74,0, neogeo_ram16[0x8280>>1])
//NEO_CYCLE_R(ssideki2,0x7850,0xffff, neogeo_ram16[0x4292>>1])
NEO_CYCLE_R(samsho2, 0x1432,0xffff, neogeo_ram16[0x0a30>>1])
NEO_CYCLE_R(samsho3, 0x0858,0, neogeo_ram16[0x8408>>1])
NEO_CYCLE_R(kof95, 0x39474,0xffff, neogeo_ram16[0xa784>>1])
NEO_CYCLE_R(rbff1, 0x80a2,0, neogeo_ram16[0x418c>>1])
//NEO_CYCLE_R(aof3, 0x15d6,0, neogeo_ram16[0x4ee8>>1])
NEO_CYCLE_R(ninjamas,0x2436,neogeo_ram16[0x8206>>1]+1,neogeo_ram16[0x8206>>1])
NEO_CYCLE_R(kof96, 0x8fc4,0xffff, neogeo_ram16[0xa782>>1])
NEO_CYCLE_R(rbffspec,0x8704,0, neogeo_ram16[0x418c>>1])
NEO_CYCLE_R(kizuna, 0x0840,0, neogeo_ram16[0x8808>>1])
NEO_CYCLE_R(kof97, 0x9c54,0xffff, neogeo_ram16[0xa784>>1])
//NEO_CYCLE_R(mslug2, 0x1656,0xffff, neogeo_ram16[0x008c>>1])
NEO_CYCLE_R(rbff2, 0xc5d0,0, neogeo_ram16[0x418c>>1])
NEO_CYCLE_R(ragnagrd,0xc6c0,0, neogeo_ram16[0x0042>>1])
NEO_CYCLE_R(lastblad,0x1868,0xffff, neogeo_ram16[0x9d4e>>1])
NEO_CYCLE_R(gururin, 0x0604,0xffff, neogeo_ram16[0x1002>>1])
//NEO_CYCLE_R(magdrop2,0x1cf3a,0,neogeo_ram16[0x0064>>1])
//NEO_CYCLE_R(miexchng,0x,,READ_WORD(&neogeo_ram[0x]))

NEO_CYCLE_R(kof98,0xa146,0xfff, neogeo_ram16[0xa784>>1])
NEO_CYCLE_R(marukodq,0x070e,0, neogeo_ram16[0x0210>>1])
static READ16_HANDLER( minasan_cycle_r )
{
        int mem;
        if (cpu_get_pc()==0x17766)
        {
                cpu_spinuntil_int();
                mem=neogeo_ram16[0x00ca>>1];
                mem--;
                neogeo_ram16[0x00ca>>1] = mem;
                return mem;
        }
        return neogeo_ram16[0x00ca>>1];
}
NEO_CYCLE_R(stakwin2,0x0b8c,0, neogeo_ram16[0x0002>>1])
static READ16_HANDLER( bakatono_cycle_r )
{
        int mem;
        if (cpu_get_pc()==0x197cc)
        {
                cpu_spinuntil_int();
                mem=neogeo_ram16[0x00fa>>1];
                mem--;
                neogeo_ram16[0x00fa>>1] = mem;
                return mem;
        }
        return neogeo_ram16[0x00fa>>1];
}
NEO_CYCLE_R(quizkof,0x0450,0, neogeo_ram16[0x4464>>1])
NEO_CYCLE_R(quizdais, 0x0730,0, neogeo_ram16[0x59f2>>1])
NEO_CYCLE_R(quizdai2, 0x1afa,0xffff, neogeo_ram16[0x0960>>1])
NEO_CYCLE_R(popbounc, 0x1196,0xffff, neogeo_ram16[0x1008>>1])
NEO_CYCLE_R(sdodgeb, 0xc22e,0, neogeo_ram16[0x1104>>1])
NEO_CYCLE_R(shocktr2, 0xf410,0, neogeo_ram16[0x8348>>1])
NEO_CYCLE_R(figfever, 0x20c60,0, neogeo_ram16[0x8100>>1])
NEO_CYCLE_R(irrmaze, 0x104e,0, neogeo_ram16[0x4b6e>>1])

/******************************************************************************/
/* Routines to speed up the sound processor AVDB 24-10-1998		      */
/******************************************************************************/

/*
 *	Sound V3.0
 *
 *	Used by puzzle de pon and Super Sidekicks 2
 *
 */
static READ_HANDLER( cycle_v3_sr )
{
	unsigned char *RAM = memory_region(REGION_CPU2);

	if (cpu_get_pc()==0x0137) {
		cpu_spinuntil_int();
		return RAM[0xfeb1];
	}
	return RAM[0xfeb1];
}

/*
 *	Also sound revision no 3.0, but different types.
 */
static READ_HANDLER( ssideki_cycle_sr )
{
	unsigned char *RAM = memory_region(REGION_CPU2);

	if (cpu_get_pc()==0x015A) {
		cpu_spinuntil_int();
		return RAM[0xfef3];
	}
	return RAM[0xfef3];
}

static READ_HANDLER( aof_cycle_sr )
{
	unsigned char *RAM = memory_region(REGION_CPU2);

	if (cpu_get_pc()==0x0143) {
		cpu_spinuntil_int();
		return RAM[0xfef3];
	}
	return RAM[0xfef3];
}

/*
 *	Sound V2.0
 *
 *	Used by puzzle Bobble and Goal Goal Goal
 *
 */

static READ_HANDLER( cycle_v2_sr )
{
	unsigned char *RAM = memory_region(REGION_CPU2);

	if (cpu_get_pc()==0x0143) {
		cpu_spinuntil_int();
		return RAM[0xfeef];
	}
	return RAM[0xfeef];
}

static READ_HANDLER( vwpoint_cycle_sr )
{
	unsigned char *RAM = memory_region(REGION_CPU2);

	if (cpu_get_pc()==0x0143) {
		cpu_spinuntil_int();
		return RAM[0xfe46];
	}
	return RAM[0xfe46];
}

/*
 *	Sound revision no 1.5, and some 2.0 versions,
 *	are not fit for speedups, it results in sound drops !
 *	Games that use this one are : Ghost Pilots, Joy Joy, Nam 1975
 */

/*
static READ_HANDLER( cycle_v15_sr )
{
	unsigned char *RAM = memory_region(REGION_CPU2);

	if (cpu_get_pc()==0x013D) {
		cpu_spinuntil_int();
		return RAM[0xfe46];
	}
	return RAM[0xfe46];
}
*/

/*
 *	Magician Lord uses a different sound core from all other
 *	Neo Geo Games.
 */

static READ_HANDLER( maglord_cycle_sr )
{
	unsigned char *RAM = memory_region(REGION_CPU2);

	if (cpu_get_pc()==0xd487) {
		cpu_spinuntil_int();
		return RAM[0xfb91];
	}
	return RAM[0xfb91];
}


/******************************************************************************/

static int prot_data;

static READ16_HANDLER( fatfury2_protection_16_r )
{
	data16_t res = (prot_data >> 24) & 0xff;

	switch (offset)
	{
	case 0x55550 >> 1:
	case 0xffff0 >> 1:
	case 0x00000 >> 1:
	case 0xff000 >> 1:
	case 0x36000 >> 1:
	case 0x36008 >> 1:
		return res;

	case 0x36004 >> 1:
	case 0x3600c >> 1:
		return ((res & 0xf0) >> 4) | ((res & 0x0f) << 4);

	default:
logerror("unknown protection read at pc %06x, offset %08x\n",cpu_get_pc(),offset<<1);
		return 0;
	}
}

static WRITE16_HANDLER( fatfury2_protection_16_w )
{
	switch (offset)
	{
	case 0x55552 >> 1:	 /* data == 0x5555; read back from 55550, ffff0, 00000, ff000 */
		prot_data = 0xff00ff00;
		break;

	case 0x56782 >> 1:	 /* data == 0x1234; read back from 36000 *or* 36004 */
		prot_data = 0xf05a3601;
		break;

	case 0x42812 >> 1:	 /* data == 0x1824; read back from 36008 *or* 3600c */
		prot_data = 0x81422418;
		break;

	case 0x55550 >> 1:
	case 0xffff0 >> 1:
	case 0xff000 >> 1:
	case 0x36000 >> 1:
	case 0x36004 >> 1:
	case 0x36008 >> 1:
	case 0x3600c >> 1:
		prot_data <<= 8;
		break;

	default:
logerror("unknown protection write at pc %06x, offset %08x, data %02x\n",cpu_get_pc(),offset,data);
		break;
	}
}

static READ16_HANDLER( popbounc_sfix_16_r )
{
	if (cpu_get_pc()==0x6b10)
		return 0;
	return neogeo_ram16[0x4fbc >> 1];
}

static void neogeo_custom_memory(void)
{
	/* NeoGeo intro screen cycle skip, used by all games */
//	install_mem_read16_handler(0, 0x10fe8c, 0x10fe8d, bios_cycle_skip_r);

    /* Individual games can go here... */

#if 1
//	if (!strcmp(Machine->gamedrv->name,"joyjoy"))   install_mem_read16_handler(0, 0x100554, 0x100555, joyjoy_cycle_r);	// Slower
//	if (!strcmp(Machine->gamedrv->name,"ridhero"))  install_mem_read16_handler(0, 0x1000ca, 0x1000cb, ridhero_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"bstars"))   install_mem_read16_handler(0, 0x10000a, 0x10000b, bstars_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"cyberlip")) install_mem_read16_handler(0, 0x107bb4, 0x107bb4, cyberlip_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"lbowling")) install_mem_read16_handler(0, 0x100098, 0x100099, lbowling_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"superspy")) install_mem_read16_handler(0, 0x10108c, 0x10108d, superspy_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"ttbb"))     install_mem_read16_handler(0, 0x10000e, 0x10000f, ttbb_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"alpham2"))  install_mem_read16_handler(0, 0x10e2fe, 0x10e2ff, alpham2_cycle_r);	// Very little increase.
	if (!strcmp(Machine->gamedrv->name,"eightman")) install_mem_read16_handler(0, 0x10046e, 0x10046f, eightman_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"roboarmy")) install_mem_read16_handler(0, 0x104010, 0x104011, roboarmy_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"fatfury1")) install_mem_read16_handler(0, 0x104282, 0x104283, fatfury1_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"burningf")) install_mem_read16_handler(0, 0x10000e, 0x10000f, burningf_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"kotm"))     install_mem_read16_handler(0, 0x100020, 0x100021, kotm_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"gpilots"))  install_mem_read16_handler(0, 0x10a682, 0x10a683, gpilots_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"lresort"))  install_mem_read16_handler(0, 0x104102, 0x104103, lresort_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"fbfrenzy")) install_mem_read16_handler(0, 0x100020, 0x100021, fbfrenzy_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"socbrawl")) install_mem_read16_handler(0, 0x10b20c, 0x10b20d, socbrawl_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"mutnat"))   install_mem_read16_handler(0, 0x101042, 0x101043, mutnat_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"aof"))      install_mem_read16_handler(0, 0x108100, 0x108101, aof_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"countb"))   install_mem_read16_handler(0, 0x108002, 0x108003, countb_cycle_r);   // doesn't seem to speed it up.
	if (!strcmp(Machine->gamedrv->name,"ncombat"))  install_mem_read16_handler(0, 0x100206, 0x100207, ncombat_cycle_r);
//**	if (!strcmp(Machine->gamedrv->name,"crsword"))  install_mem_read16_handler(0, 0x10, 0x10, crsword_cycle_r);			// Can't find this one :-(
	if (!strcmp(Machine->gamedrv->name,"trally"))   install_mem_read16_handler(0, 0x100206, 0x100207, trally_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"sengoku"))  install_mem_read16_handler(0, 0x100088, 0x100089, sengoku_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"ncommand")) install_mem_read16_handler(0, 0x108206, 0x108207, ncommand_cycle_r);	// Slower
	if (!strcmp(Machine->gamedrv->name,"wh1"))      install_mem_read16_handler(0, 0x108206, 0x108207, wh1_cycle_r);
//**	if (!strcmp(Machine->gamedrv->name,"sengoku2")) install_mem_read16_handler(0, 0x10, 0x10, sengoku2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"androdun")) install_mem_read16_handler(0, 0x100080, 0x100081, androdun_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"bjourney")) install_mem_read16_handler(0, 0x100206, 0x100207, bjourney_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"maglord"))  install_mem_read16_handler(0, 0x100206, 0x100207, maglord_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"janshin"))  install_mem_read16_handler(0, 0x100026, 0x100027, janshin_cycle_r);	// No speed difference
	if (!strcmp(Machine->gamedrv->name,"pulstar"))  install_mem_read16_handler(0, 0x101000, 0x101001, pulstar_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"blazstar")) install_mem_read16_handler(0, 0x101000, 0x101001, blazstar_cycle_r);
//**	if (!strcmp(Machine->gamedrv->name,"pbobble"))  install_mem_read16_handler(0, 0x10, 0x10, pbobble_cycle_r);		// Can't find this one :-(
	if (!strcmp(Machine->gamedrv->name,"puzzledp")) install_mem_read16_handler(0, 0x100000, 0x100001, puzzledp_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"neodrift")) install_mem_read16_handler(0, 0x100424, 0x100425, neodrift_cycle_r);
//**	if (!strcmp(Machine->gamedrv->name,"neomrdo"))  install_mem_read16_handler(0, 0x10, 0x10, neomrdo_cycle_r);		// Can't find this one :-(
	if (!strcmp(Machine->gamedrv->name,"spinmast")) install_mem_read16_handler(0, 0x100050, 0x100051, spinmast_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"karnovr"))  install_mem_read16_handler(0, 0x103466, 0x103467, karnovr_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"wjammers")) install_mem_read16_handler(0, 0x10005a, 0x10005b, wjammers_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"strhoops")) install_mem_read16_handler(0, 0x101200, 0x101201, strhoops_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"magdrop3")) install_mem_read16_handler(0, 0x100060, 0x100061, magdrop3_cycle_r);	// The game starts glitching.
//**	if (!strcmp(Machine->gamedrv->name,"pspikes2")) install_mem_read16_handler(0, 0x10, 0x10, pspikes2_cycle_r);		// Can't find this one :-(
	if (!strcmp(Machine->gamedrv->name,"sonicwi2")) install_mem_read16_handler(0, 0x10e5b6, 0x10e5b7, sonicwi2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"sonicwi3")) install_mem_read16_handler(0, 0x10ea2e, 0x10ea2f, sonicwi3_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"goalx3"))   install_mem_read16_handler(0, 0x100006, 0x100007, goalx3_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"mslug"))    install_mem_read16_handler(0, 0x106ed8, 0x106ed9, mslug_cycle_r);		// Doesn't work properly.
//	if (!strcmp(Machine->gamedrv->name,"turfmast")) install_mem_read16_handler(0, 0x102e54, 0x102e55, turfmast_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"kabukikl")) install_mem_read16_handler(0, 0x10428a, 0x10428b, kabukikl_cycle_r);

	if (!strcmp(Machine->gamedrv->name,"panicbom")) install_mem_read16_handler(0, 0x10009c, 0x10009d, panicbom_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"neobombe")) install_mem_read16_handler(0, 0x10448c, 0x10448d, neobombe_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"wh2"))      install_mem_read16_handler(0, 0x108206, 0x108207, wh2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"wh2j"))     install_mem_read16_handler(0, 0x108206, 0x108207, wh2j_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"aodk"))     install_mem_read16_handler(0, 0x108206, 0x108207, aodk_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"whp"))      install_mem_read16_handler(0, 0x108206, 0x108207, whp_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"overtop"))  install_mem_read16_handler(0, 0x108202, 0x108203, overtop_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"twinspri")) install_mem_read16_handler(0, 0x108206, 0x108207, twinspri_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"stakwin"))  install_mem_read16_handler(0, 0x100b92, 0x100b93, stakwin_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"shocktro")) install_mem_read16_handler(0, 0x108344, 0x108345, shocktro_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"tws96"))    install_mem_read16_handler(0, 0x10010e, 0x10010f, tws96_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"zedblade")) install_mem_read16_handler(0, 0x109004, 0x109005, zedblade_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"doubledr")) install_mem_read16_handler(0, 0x101c30, 0x101c31, doubledr_cycle_r);
//**	if (!strcmp(Machine->gamedrv->name,"gowcaizr")) install_mem_read16_handler(0, 0x10, 0x10, gowcaizr_cycle_r);		// Can't find this one :-(
	if (!strcmp(Machine->gamedrv->name,"galaxyfg")) install_mem_read16_handler(0, 0x101858, 0x101859, galaxyfg_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"wakuwak7")) install_mem_read16_handler(0, 0x100bd4, 0x100bd5, wakuwak7_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"mahretsu")) install_mem_read16_handler(0, 0x1013b2, 0x1013b3, mahretsu_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"nam1975"))  install_mem_read16_handler(0, 0x1012e0, 0x1012e1, nam1975_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"tpgolf"))   install_mem_read16_handler(0, 0x1000a4, 0x1000a5, tpgolf_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"legendos")) install_mem_read16_handler(0, 0x100002, 0x100003, legendos_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"viewpoin")) install_mem_read16_handler(0, 0x101216, 0x101217, viewpoin_cycle_r);	// Doesn't work
	if (!strcmp(Machine->gamedrv->name,"fatfury2")) install_mem_read16_handler(0, 0x10418c, 0x10418d, fatfury2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"bstars2"))  install_mem_read16_handler(0, 0x10001c, 0x10001c, bstars2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"ssideki"))  install_mem_read16_handler(0, 0x108c84, 0x108c85, ssideki_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"kotm2"))    install_mem_read16_handler(0, 0x101000, 0x101001, kotm2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"samsho"))   install_mem_read16_handler(0, 0x100a76, 0x100a77, samsho_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"fatfursp")) install_mem_read16_handler(0, 0x10418c, 0x10418d, fatfursp_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"fatfury3")) install_mem_read16_handler(0, 0x10418c, 0x10418d, fatfury3_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"tophuntr")) install_mem_read16_handler(0, 0x10008e, 0x10008f, tophuntr_cycle_r);	// Can't test this at the moment, it crashes.
	if (!strcmp(Machine->gamedrv->name,"savagere")) install_mem_read16_handler(0, 0x108404, 0x108405, savagere_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"kof94"))    install_mem_read16_handler(0, 0x10, 0x10, kof94_cycle_r);				// Can't do this I think. There seems to be too much code in the idle loop.
	if (!strcmp(Machine->gamedrv->name,"aof2"))     install_mem_read16_handler(0, 0x108280, 0x108281, aof2_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"ssideki2")) install_mem_read16_handler(0, 0x104292, 0x104293, ssideki2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"samsho2"))  install_mem_read16_handler(0, 0x100a30, 0x100a31, samsho2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"samsho3"))  install_mem_read16_handler(0, 0x108408, 0x108409, samsho3_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"kof95"))    install_mem_read16_handler(0, 0x10a784, 0x10a785, kof95_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"rbff1"))    install_mem_read16_handler(0, 0x10418c, 0x10418d, rbff1_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"aof3"))     install_mem_read16_handler(0, 0x104ee8, 0x104ee9, aof3_cycle_r);		// Doesn't work properly.
	if (!strcmp(Machine->gamedrv->name,"ninjamas")) install_mem_read16_handler(0, 0x108206, 0x108207, ninjamas_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"kof96"))    install_mem_read16_handler(0, 0x10a782, 0x10a783, kof96_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"samsho4"))  install_mem_read16_handler(0, 0x10830c, 0x10830d, samsho4_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"rbffspec")) install_mem_read16_handler(0, 0x10418c, 0x10418d, rbffspec_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"kizuna"))   install_mem_read16_handler(0, 0x108808, 0x108809, kizuna_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"kof97"))    install_mem_read16_handler(0, 0x10a784, 0x10a785, kof97_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"mslug2"))   install_mem_read16_handler(0, 0x10008c, 0x10008d, mslug2_cycle_r);	// Breaks the game
	if (!strcmp(Machine->gamedrv->name,"rbff2"))    install_mem_read16_handler(0, 0x10418c, 0x10418d, rbff2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"ragnagrd")) install_mem_read16_handler(0, 0x100042, 0x100043, ragnagrd_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"lastblad")) install_mem_read16_handler(0, 0x109d4e, 0x109d4f, lastblad_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"gururin"))  install_mem_read16_handler(0, 0x101002, 0x101003, gururin_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"magdrop2")) install_mem_read16_handler(0, 0x100064, 0x100065, magdrop2_cycle_r);	// Graphic Glitches
//	if (!strcmp(Machine->gamedrv->name,"miexchng")) install_mem_read16_handler(0, 0x10, 0x10, miexchng_cycle_r);			// Can't do this.
	if (!strcmp(Machine->gamedrv->name,"kof98"))    install_mem_read16_handler(0, 0x10a784, 0x10a785, kof98_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"marukodq")) install_mem_read16_handler(0, 0x100210, 0x100211, marukodq_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"minasan"))  install_mem_read16_handler(0, 0x1000ca, 0x1000cb, minasan_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"stakwin2")) install_mem_read16_handler(0, 0x100002, 0x100003, stakwin2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"bakatono")) install_mem_read16_handler(0, 0x1000fa, 0x1000fb, bakatono_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"quizkof"))  install_mem_read16_handler(0, 0x104464, 0x104465, quizkof_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"quizdais")) install_mem_read16_handler(0, 0x1059f2, 0x1059f3, quizdais_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"quizdai2")) install_mem_read16_handler(0, 0x100960, 0x100961, quizdai2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"popbounc")) install_mem_read16_handler(0, 0x101008, 0x101009, popbounc_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"sdodgeb"))  install_mem_read16_handler(0, 0x101104, 0x101105, sdodgeb_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"shocktr2")) install_mem_read16_handler(0, 0x108348, 0x108349, shocktr2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"figfever")) install_mem_read16_handler(0, 0x108100, 0x108101, figfever_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"irrmaze"))  install_mem_read16_handler(0, 0x104b6e, 0x104b6f, irrmaze_cycle_r);

#endif

	/* AVDB cpu spins based on sound processor status */
	if (!strcmp(Machine->gamedrv->name,"puzzledp")) install_mem_read_handler(1, 0xfeb1, 0xfeb1, cycle_v3_sr);
//	if (!strcmp(Machine->gamedrv->name,"ssideki2")) install_mem_read_handler(1, 0xfeb1, 0xfeb1, cycle_v3_sr);

	if (!strcmp(Machine->gamedrv->name,"ssideki"))  install_mem_read_handler(1, 0xfef3, 0xfef3, ssideki_cycle_sr);
	if (!strcmp(Machine->gamedrv->name,"aof"))      install_mem_read_handler(1, 0xfef3, 0xfef3, aof_cycle_sr);

	if (!strcmp(Machine->gamedrv->name,"pbobble")) install_mem_read_handler(1, 0xfeef, 0xfeef, cycle_v2_sr);
	if (!strcmp(Machine->gamedrv->name,"goalx3")) install_mem_read_handler(1, 0xfeef, 0xfeef, cycle_v2_sr);
	if (!strcmp(Machine->gamedrv->name,"fatfury1")) install_mem_read_handler(1, 0xfeef, 0xfeef, cycle_v2_sr);
	if (!strcmp(Machine->gamedrv->name,"mutnat")) install_mem_read_handler(1, 0xfeef, 0xfeef, cycle_v2_sr);

	if (!strcmp(Machine->gamedrv->name,"maglord")) install_mem_read_handler(1, 0xfb91, 0xfb91, maglord_cycle_sr);
	if (!strcmp(Machine->gamedrv->name,"vwpoint")) install_mem_read_handler(1, 0xfe46, 0xfe46, vwpoint_cycle_sr);

//	if (!strcmp(Machine->gamedrv->name,"joyjoy")) install_mem_read_handler(1, 0xfe46, 0xfe46, cycle_v15_sr);
//	if (!strcmp(Machine->gamedrv->name,"nam1975")) install_mem_read_handler(1, 0xfe46, 0xfe46, cycle_v15_sr);
//	if (!strcmp(Machine->gamedrv->name,"gpilots")) install_mem_read_handler(1, 0xfe46, 0xfe46, cycle_v15_sr);

	/* kludges */

	if (!strcmp(Machine->gamedrv->name,"gururin"))
	{
		/* Fix a really weird problem. The game clears the video RAM but goes */
		/* beyond the tile RAM, corrupting the zoom control RAM. After that it */
		/* initializes the control RAM, but then corrupts it again! */
		data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
		mem16[0x1328 >> 1] = 0x4e71;
		mem16[0x132a >> 1] = 0x4e71;
		mem16[0x132c >> 1] = 0x4e71;
		mem16[0x132e >> 1] = 0x4e71;
	}

	if (!Machine->sample_rate &&
			!strcmp(Machine->gamedrv->name,"popbounc"))
	/* the game hangs after a while without this patch */
		install_mem_read16_handler(0, 0x104fbc, 0x104fbd, popbounc_sfix_16_r);

	/* hacks to make the games which do protection checks run in arcade mode */
	/* we write protect a SRAM location so it cannot be set to 1 */
	sram_protection_hack = ~0;
	if (!strcmp(Machine->gamedrv->name,"fatfury3") ||
			 !strcmp(Machine->gamedrv->name,"samsho3") ||
			 !strcmp(Machine->gamedrv->name,"samsho4") ||
			 !strcmp(Machine->gamedrv->name,"aof3") ||
			 !strcmp(Machine->gamedrv->name,"rbff1") ||
			 !strcmp(Machine->gamedrv->name,"rbffspec") ||
			 !strcmp(Machine->gamedrv->name,"kof95") ||
			 !strcmp(Machine->gamedrv->name,"kof96") ||
			 !strcmp(Machine->gamedrv->name,"kof97") ||
			 !strcmp(Machine->gamedrv->name,"kof98") ||
			 !strcmp(Machine->gamedrv->name,"kof99p") ||
			 !strcmp(Machine->gamedrv->name,"kizuna") ||
			 !strcmp(Machine->gamedrv->name,"lastblad") ||
			 !strcmp(Machine->gamedrv->name,"lastbld2") ||
			 !strcmp(Machine->gamedrv->name,"rbff2") ||
			 !strcmp(Machine->gamedrv->name,"mslug2") ||
			 !strcmp(Machine->gamedrv->name,"garoup"))
		sram_protection_hack = 0x100 >> 1;

	if (!strcmp(Machine->gamedrv->name,"pulstar"))
		sram_protection_hack = 0x35a >> 1;

	if (!strcmp(Machine->gamedrv->name,"ssideki"))
	{
		/* patch out protection check */
		/* the protection routines are at 0x25dcc and involve reading and writing */
		/* addresses in the 0x2xxxxx range */
		data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
		mem16[0x2240 >> 1] = 0x4e71;
	}

	/* Hacks the program rom of Fatal Fury 2, needed either in arcade or console mode */
	/* otherwise at level 2 you cannot hit the opponent and other problems */
	if (!strcmp(Machine->gamedrv->name,"fatfury2"))
	{
		/* there seems to also be another protection check like the countless ones */
		/* patched above by protecting a SRAM location, but that trick doesn't work */
		/* here (or maybe the SRAM location to protect is different), so I patch out */
		/* the routine which trashes memory. Without this, the game goes nuts after */
		/* the first bonus stage. */
		data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
		mem16[0xb820 >> 1] = 0x4e71;
		mem16[0xb822 >> 1] = 0x4e71;

		/* again, the protection involves reading and writing addresses in the */
		/* 0x2xxxxx range. There are several checks all around the code. */
		install_mem_read16_handler(0, 0x200000, 0x2fffff, fatfury2_protection_16_r);
		install_mem_write16_handler(0, 0x200000, 0x2fffff, fatfury2_protection_16_w);
	}

	if (!strcmp(Machine->gamedrv->name,"fatfury3"))
	{
		/* patch the first word, it must be 0x0010 not 0x0000 (initial stack pointer) */
		data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
		mem16[0x0000 >> 1] = 0x0010;
	}

	if (!strcmp(Machine->gamedrv->name,"mslugx"))
	{
		/* patch out protection checks */
		int i;
		data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);

		for (i = 0;i < (0x100000 >> 1) - 4;i++)
		{
			if (mem16[i+0] == 0x0243 &&
				mem16[i+1] == 0x0001 && 	/* andi.w  #$1, D3 */
				mem16[i+2] == 0x6600)		/* bne xxxx */
			{
				mem16[i+2] = 0x4e71;
				mem16[i+3] = 0x4e71;
			}
		}

		mem16[0x3bdc >> 1] = 0x4e71;
		mem16[0x3bde >> 1] = 0x4e71;
		mem16[0x3be0 >> 1] = 0x4e71;
		mem16[0x3c0c >> 1] = 0x4e71;
		mem16[0x3c0e >> 1] = 0x4e71;
		mem16[0x3c10 >> 1] = 0x4e71;

		mem16[0x3c36 >> 1] = 0x4e71;
		mem16[0x3c38 >> 1] = 0x4e71;
	}
}



WRITE16_HANDLER( neogeo_sram16_lock_w )
{
	sram_locked = 1;
}

WRITE16_HANDLER( neogeo_sram16_unlock_w )
{
	sram_locked = 0;
}

READ16_HANDLER( neogeo_sram16_r )
{
	return neogeo_sram16[offset];
}

WRITE16_HANDLER( neogeo_sram16_w )
{
	if (sram_locked)
	{
logerror("PC %06x: warning: write %02x to SRAM %04x while it was protected\n",cpu_get_pc(),data,offset<<1);
	}
	else
	{
		if (offset == sram_protection_hack)
		{
			if (ACCESSING_LSB && (data & 0xff) == 0x01)
				return; /* fake protection pass */
		}

		COMBINE_DATA(&neogeo_sram16[offset]);
	}
}

void neogeo_nvram_handler(void *file,int read_or_write)
{
	if (read_or_write)
	{
		/* Save the SRAM settings */
		osd_fwrite_msbfirst(file,neogeo_sram16,0x2000);

		/* save the memory card */
		neogeo_memcard_save();
	}
	else
	{
		/* Load the SRAM settings for this game */
		if (file)
			osd_fread_msbfirst(file,neogeo_sram16,0x2000);
		else
			memset(neogeo_sram16,0,0x10000);

		/* load the memory card */
		neogeo_memcard_load(memcard_number);
	}
}



/*
	INFORMATION:

	Memory card is a 2kb battery backed RAM.
	It is accessed thru 0x800000-0x800FFF.
	Even bytes are always 0xFF
	Odd bytes are memcard data (0x800 bytes)

	Status byte at 0x380000: (BITS ARE ACTIVE *LOW*)

	0 PAD1 START
	1 PAD1 SELECT
	2 PAD2 START
	3 PAD2 SELECT
	4 --\  MEMORY CARD
	5 --/  INSERTED
	6 MEMORY CARD WRITE PROTECTION
	7 UNUSED (?)
*/




/********************* MEMCARD ROUTINES **********************/
READ16_HANDLER( neogeo_memcard16_r )
{
	if (memcard_status==1)
		return neogeo_memcard[offset] | 0xff00;
	else
		return ~0;
}

WRITE16_HANDLER( neogeo_memcard16_w )
{
	if (ACCESSING_MSB)
		return;

	if (memcard_status==1)
		neogeo_memcard[offset] = data & 0xff;
}

int neogeo_memcard_load(int number)
{
	char name[16];
	void *f;

	sprintf(name, "MEMCARD.%03d", number);
	if ((f=osd_fopen(0, name, OSD_FILETYPE_MEMCARD,0))!=0)
	{
		osd_fread(f,neogeo_memcard,0x800);
		osd_fclose(f);
		return 1;
	}
	return 0;
}

void neogeo_memcard_save(void)
{
	char name[16];
	void *f;

	if (memcard_number!=-1)
	{
		sprintf(name, "MEMCARD.%03d", memcard_number);
		if ((f=osd_fopen(0, name, OSD_FILETYPE_MEMCARD,1))!=0)
		{
			osd_fwrite(f,neogeo_memcard,0x800);
			osd_fclose(f);
		}
	}
}

void neogeo_memcard_eject(void)
{
   if (memcard_number!=-1)
   {
	   neogeo_memcard_save();
	   memset(neogeo_memcard, 0, 0x800);
	   memcard_status=0;
	   memcard_number=-1;
   }
}

int neogeo_memcard_create(int number)
{
	char buf[0x800];
	char name[16];
	void *f1, *f2;

	sprintf(name, "MEMCARD.%03d", number);
	if ((f1=osd_fopen(0, name, OSD_FILETYPE_MEMCARD,0))==0)
	{
		if ((f2=osd_fopen(0, name, OSD_FILETYPE_MEMCARD,1))!=0)
		{
			osd_fwrite(f2,buf,0x800);
			osd_fclose(f2);
			return 1;
		}
	}
	else
		osd_fclose(f1);

	return 0;
}

