/* $XFree86: xc/programs/Xserver/hw/xfree86/fbdevhw/fbdevhw.c,v 1.35 2006/03/16 16:50:24 dawes Exp $ */

/* all driver need this */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

/* pci stuff */
#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "xf86cmap.h"

#include "fbdevhw.h"
#include "fbpriv.h"
/* This causes trouble, X.org replaces it with the below define for PAGE_MASK
#include "asm/page.h" */	/* #define for PAGE_* */
#define PAGE_MASK               (~(getpagesize() - 1))

#include "globals.h"
#define DPMS_SERVER
#include <X11/extensions/dpms.h>

#define DEBUG 0

#if DEBUG
# define TRACE_ENTER(str)	ErrorF("fbdevHW: " str " %d\n",pScrn->scrnIndex)
#else
# define TRACE_ENTER(str)
#endif

/* -------------------------------------------------------------------- */

#ifdef XFree86LOADER

static MODULESETUPPROTO(fbdevhwSetup);

static XF86ModuleVersionInfo fbdevHWVersRec =
{
	"fbdevhw",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	0, 0, 2,
	ABI_CLASS_VIDEODRV,
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_NONE,
	{0,0,0,0}
};

XF86ModuleData fbdevhwModuleData = { &fbdevHWVersRec, fbdevhwSetup, NULL };

static pointer
fbdevhwSetup(ModuleDescPtr module, pointer opts, int *errmaj, int *errmin)
{
	const char *osname;

	/* Check that we're being loaded on a Linux system */
	LoaderGetOS(&osname, NULL, NULL, NULL);
	if (!osname || strcmp(osname, "linux") != 0) {
		if (errmaj)
			*errmaj = LDR_BADOS;
		if (errmin)
			*errmin = 0;
		return NULL;
	} else {
		/* OK */
		return (pointer)1;
	}
}
	
#else /* XFree86LOADER */

#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>

#endif /* XFree86LOADER */

/* -------------------------------------------------------------------- */
/* our private data, and two functions to allocate/free this            */

#define FBDEVHWPTRLVAL(p) (p)->privates[fbdevHWPrivateIndex].ptr
#define FBDEVHWPTR(p) ((fbdevHWPtr)(FBDEVHWPTRLVAL(p)))

static int fbdevHWPrivateIndex = -1;

typedef struct {
	/* framebuffer device: filename (/dev/fb*), handle, more */
	char*				device;
	int				fd;
	void*				fbmem;
	unsigned int			fbmem_len;
	unsigned int			fboff;
	char*				mmio;
	unsigned int			mmio_len;

	/* current hardware state */
	struct fb_fix_screeninfo	fix;
	struct fb_var_screeninfo	var;

	/* saved video mode */
	struct fb_var_screeninfo	saved_var;

	/* FIXME: unused??? [geert] */
	struct fb_cmap			saved_cmap;
	unsigned short			*saved_red;
	unsigned short			*saved_green;
	unsigned short			*saved_blue;

	/* buildin video mode */
	DisplayModeRec			buildin;

} fbdevHWRec, *fbdevHWPtr;

Bool
fbdevHWGetRec(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr;
	
	if (fbdevHWPrivateIndex < 0)
		fbdevHWPrivateIndex = xf86AllocateScrnInfoPrivateIndex();

	if (FBDEVHWPTR(pScrn) != NULL)
		return TRUE;
	
	fPtr = FBDEVHWPTRLVAL(pScrn) = xnfcalloc(sizeof(fbdevHWRec), 1);
	return TRUE;
}

void
fbdevHWFreeRec(ScrnInfoPtr pScrn)
{
	if (fbdevHWPrivateIndex < 0)
		return;
	if (FBDEVHWPTR(pScrn) == NULL)
		return;
	xfree(FBDEVHWPTR(pScrn));
	FBDEVHWPTRLVAL(pScrn) = NULL;
}

/* -------------------------------------------------------------------- */
/* some helpers for printing debug informations                         */

#if DEBUG
static void
print_fbdev_mode(char *txt, struct fb_var_screeninfo *var)
{
	ErrorF( "fbdev %s mode:\t%d   %d %d %d %d   %d %d %d %d   %d %d:%d:%d\n",
		txt,var->pixclock,
		var->xres, var->right_margin, var->hsync_len, var->left_margin,
		var->yres, var->lower_margin, var->vsync_len, var->upper_margin,
		var->bits_per_pixel,
		var->red.length, var->green.length, var->blue.length);
}

static void
print_xfree_mode(char *txt, DisplayModePtr mode)
{
	ErrorF( "xfree %s mode:\t%d   %d %d %d %d   %d %d %d %d\n",
		txt,mode->Clock,
		mode->HDisplay, mode->HSyncStart, mode->HSyncEnd, mode->HTotal,
		mode->VDisplay, mode->VSyncStart, mode->VSyncEnd, mode->VTotal);
}
#endif

/* -------------------------------------------------------------------- */
/* Convert timings between the XFree and the Frame Buffer Device        */

static void
xfree2fbdev_fblayout(ScrnInfoPtr pScrn, struct fb_var_screeninfo *var)
{
	var->xres_virtual   = pScrn->virtualX;
	var->yres_virtual   = pScrn->virtualY;
	var->bits_per_pixel = pScrn->bitsPerPixel;
	var->red.length     = pScrn->weight.red;
	var->green.length   = pScrn->weight.green;
	var->blue.length    = pScrn->weight.blue;
}

static void
xfree2fbdev_timing(DisplayModePtr mode, struct fb_var_screeninfo *var)
{
	var->xres = mode->HDisplay;
	var->yres = mode->VDisplay;
	if (var->xres_virtual < var->xres)
		var->xres_virtual = var->xres;
	if (var->yres_virtual < var->yres)
		var->yres_virtual = var->yres;
	var->xoffset = var->yoffset = 0;
	var->pixclock = mode->Clock ? 1000000000/mode->Clock : 0;
	var->right_margin = mode->HSyncStart-mode->HDisplay;
	var->hsync_len = mode->HSyncEnd-mode->HSyncStart;
	var->left_margin = mode->HTotal-mode->HSyncEnd;
	var->lower_margin = mode->VSyncStart-mode->VDisplay;
	var->vsync_len = mode->VSyncEnd-mode->VSyncStart;
	var->upper_margin = mode->VTotal-mode->VSyncEnd;
	var->sync = 0;
	if (mode->Flags & V_PHSYNC)
		var->sync |= FB_SYNC_HOR_HIGH_ACT;
	if (mode->Flags & V_PVSYNC)
		var->sync |= FB_SYNC_VERT_HIGH_ACT;
	if (mode->Flags & V_PCSYNC)
		var->sync |= FB_SYNC_COMP_HIGH_ACT;
#if 1 /* Badly needed for PAL/NTSC on Amiga (amifb)!! [geert] */
	if (mode->Flags & V_BCAST)
		var->sync |= FB_SYNC_BROADCAST;
#endif
	if (mode->Flags & V_INTERLACE)
		var->vmode = FB_VMODE_INTERLACED;
	else if (mode->Flags & V_DBLSCAN)
		var->vmode = FB_VMODE_DOUBLE;
	else
		var->vmode = FB_VMODE_NONINTERLACED;
}

static void
fbdev2xfree_timing(struct fb_var_screeninfo *var, DisplayModePtr mode)
{
	mode->Clock = var->pixclock ? 1000000000/var->pixclock : 28000000;
	mode->HDisplay = var->xres;
	mode->HSyncStart = mode->HDisplay+var->right_margin;
	mode->HSyncEnd = mode->HSyncStart+var->hsync_len;
	mode->HTotal = mode->HSyncEnd+var->left_margin;
	mode->VDisplay = var->yres;
	mode->VSyncStart = mode->VDisplay+var->lower_margin;
	mode->VSyncEnd = mode->VSyncStart+var->vsync_len;
	mode->VTotal = mode->VSyncEnd+var->upper_margin;
	mode->Flags = 0;
	mode->Flags |= var->sync & FB_SYNC_HOR_HIGH_ACT ? V_PHSYNC : V_NHSYNC;
	mode->Flags |= var->sync & FB_SYNC_VERT_HIGH_ACT ? V_PVSYNC : V_NVSYNC;
	mode->Flags |= var->sync & FB_SYNC_COMP_HIGH_ACT ? V_PCSYNC : V_NCSYNC;
#if 1 /* Badly needed for PAL/NTSC on Amiga (amifb)!! [geert] */
	if (var->sync & FB_SYNC_BROADCAST)
		mode->Flags |= V_BCAST;
#endif
	if ((var->vmode & FB_VMODE_MASK) == FB_VMODE_INTERLACED)
		mode->Flags |= V_INTERLACE;
	else if ((var->vmode & FB_VMODE_MASK) == FB_VMODE_DOUBLE)
		mode->Flags |= V_DBLSCAN;
	mode->SynthClock = mode->Clock;
	mode->CrtcHDisplay = mode->HDisplay;
	mode->CrtcHSyncStart = mode->HSyncStart;
	mode->CrtcHSyncEnd = mode->HSyncEnd;
	mode->CrtcHTotal = mode->HTotal;
	mode->CrtcVDisplay = mode->VDisplay;
	mode->CrtcVSyncStart = mode->VSyncStart;
	mode->CrtcVSyncEnd = mode->VSyncEnd;
	mode->CrtcVTotal = mode->VTotal;
	mode->CrtcHAdjusted = FALSE;
	mode->CrtcVAdjusted = FALSE;
}


/* -------------------------------------------------------------------- */
/* open correct framebuffer device                                      */

/* try to find the framebuffer device for a given PCI device */
static int
fbdev_open_pci(pciVideoPtr pPci, char **namep)
{
	struct	fb_fix_screeninfo fix;
	char	filename[16];
	int	fd,i,j;
	memType res_start, res_end;

	for (i = 0; i < 8; i++) {
		sprintf(filename,"/dev/fb%d",i);
		if (-1 == (fd = open(filename,O_RDWR,0))) {
			xf86DrvMsg(-1, X_WARNING,
				   "open %s: %s\n", filename, strerror(errno));
			continue;
		}
		if (-1 == ioctl(fd,FBIOGET_FSCREENINFO,(void*)&fix)) {
			close(fd);
			continue;
		}
		for (j = 0; j < 6; j++) {
			res_start = pPci->memBase[j];
			res_end = res_start+pPci->size[j];
			if ((0 != fix.smem_len &&
			     (memType) fix.smem_start >= res_start &&
			     (memType) fix.smem_start < res_end) ||
			    (0 != fix.mmio_len &&
			     (memType) fix.mmio_start >= res_start &&
			     (memType) fix.mmio_start < res_end))
				break;
		}
		if (j == 6) {
			close(fd);
			continue;
		}
		if (namep) {
		    *namep = xnfalloc(16);
		    strncpy(*namep,fix.id,16);
		}
		return fd;
	}
	if (namep)
	    *namep = NULL;
	return -1;
}

static int
fbdev_open(int scrnIndex, char *dev, char** namep)
{
	struct	fb_fix_screeninfo fix;
	int    fd;

	/* try argument (from XF86Config) first */
	if (dev) {
	    fd = open(dev,O_RDWR,0);
	} else {
	    /* second: environment variable */
	    dev = getenv("FRAMEBUFFER");
	    if ((NULL == dev) || ((fd = open(dev,O_RDWR,0)) == -1)) {
		/* last try: default device */
		dev = "/dev/fb0";
		fd = open(dev,O_RDWR,0);
	    }
	}

	if (fd == -1) {
	    xf86DrvMsg(scrnIndex, X_ERROR,
		       "open %s: %s\n", dev, strerror(errno));
	    return -1;
	}

	if (namep) {
	    if (-1 == ioctl(fd,FBIOGET_FSCREENINFO,(void*)(&fix))) {
		*namep = NULL;
		xf86DrvMsg(scrnIndex, X_ERROR,
			   "FBIOGET_FSCREENINFO: %s\n", strerror(errno));
		return -1;
	    } else {
		*namep = xnfalloc(16);
		strncpy(*namep,fix.id,16);
	    }
	}
	return fd;
}

/* -------------------------------------------------------------------- */

Bool
fbdevHWProbe(pciVideoPtr pPci, char *device,char **namep)
{
	int fd;

	if (pPci)
		fd = fbdev_open_pci(pPci,namep);
	else
		fd = fbdev_open(-1,device,namep);

	if (-1 == fd)
		return FALSE;
	close(fd);
	return TRUE;
}

Bool
fbdevHWInit(ScrnInfoPtr pScrn, pciVideoPtr pPci, char *device)
{
	fbdevHWPtr fPtr;

	TRACE_ENTER("Init");

	fbdevHWGetRec(pScrn);
	fPtr = FBDEVHWPTR(pScrn);

	/* open device */
	if (pPci)
		fPtr->fd = fbdev_open_pci(pPci,NULL);
	else
		fPtr->fd = fbdev_open(pScrn->scrnIndex,device,NULL);
	if (-1 == fPtr->fd) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "Failed to open framebuffer device, consult warnings"
			   " and/or errors above for possible reasons\n"
			   "\t(you may have to look at the server log to see"
			   " warnings)\n");
		return FALSE;
	}

	/* get current fb device settings */
	if (-1 == ioctl(fPtr->fd,FBIOGET_FSCREENINFO,(void*)(&fPtr->fix))) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "ioctl FBIOGET_FSCREENINFO: %s\n",
			   strerror(errno));
		return FALSE;
	}
	if (-1 == ioctl(fPtr->fd,FBIOGET_VSCREENINFO,(void*)(&fPtr->var))) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "ioctl FBIOGET_VSCREENINFO: %s\n",
			   strerror(errno));
		return FALSE;
	}

	/* we can use the current settings as "buildin mode" */
	fbdev2xfree_timing(&fPtr->var, &fPtr->buildin);
	fPtr->buildin.name  = "current";
	fPtr->buildin.next  = &fPtr->buildin;
	fPtr->buildin.prev  = &fPtr->buildin;
	fPtr->buildin.type |= M_T_BUILTIN;
	
	return TRUE;
}

char*
fbdevHWGetName(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
	return fPtr->fix.id;
}

int
fbdevHWGetDepth(ScrnInfoPtr pScrn, int *fbbpp)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	if (fbbpp)
	    *fbbpp = fPtr->var.bits_per_pixel;

	if (fPtr->fix.visual == FB_VISUAL_TRUECOLOR ||
	    fPtr->fix.visual == FB_VISUAL_DIRECTCOLOR)
		return fPtr->var.red.length+fPtr->var.green.length+
			fPtr->var.blue.length;
	else
		return fPtr->var.bits_per_pixel;
}

int
fbdevHWGetLineLength(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	if (fPtr->fix.line_length)
		return fPtr->fix.line_length;
	else
		return fPtr->var.xres_virtual*fPtr->var.bits_per_pixel/8;
}

int
fbdevHWGetType(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
	return fPtr->fix.type;
}

int
fbdevHWGetVidmem(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
	return fPtr->fix.smem_len;
}

void
fbdevHWSetVideoModes(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
	int virtX = pScrn->display->virtualX;
	int virtY = pScrn->display->virtualY;
	struct fb_var_screeninfo var;
	char **modename;
	DisplayModePtr mode,this,last = pScrn->modes;

	TRACE_ENTER("VerifyModes");
	if (NULL == pScrn->display->modes)
		return;

	for (modename = pScrn->display->modes; *modename != NULL; modename++) {
		for (mode = pScrn->monitor->Modes; mode != NULL; mode = mode->next)
			if (0 == strcmp(mode->name,*modename))
				break;
		if (NULL == mode) {
			xf86DrvMsg(pScrn->scrnIndex, X_INFO,
				   "\tmode \"%s\" not found\n", *modename);
			continue;
		}
		memset(&var,0,sizeof(var));
		xfree2fbdev_timing(mode,&var);
		var.xres_virtual = virtX;
		var.yres_virtual = virtY;
		var.bits_per_pixel = pScrn->bitsPerPixel;
		var.red.length = pScrn->weight.red;
		var.green.length = pScrn->weight.green;
		var.blue.length = pScrn->weight.blue;

		var.activate = FB_ACTIVATE_TEST;
		if (var.xres_virtual < var.xres) var.xres_virtual = var.xres;
		if (var.yres_virtual < var.yres) var.yres_virtual = var.yres;
		if (-1 == ioctl(fPtr->fd,FBIOPUT_VSCREENINFO,(void*)(&var))) {
			xf86DrvMsg(pScrn->scrnIndex, X_INFO,
				   "\tmode \"%s\" test failed\n", *modename);
			continue;
		}
		xf86DrvMsg(pScrn->scrnIndex, X_INFO,
			   "\tmode \"%s\" ok\n", *modename);
		if (virtX < var.xres) virtX = var.xres;
		if (virtY < var.yres) virtY = var.yres;
		if (NULL == pScrn->modes) {
			pScrn->modes = xnfalloc(sizeof(DisplayModeRec));
			this = pScrn->modes;
			memcpy(this,mode,sizeof(DisplayModeRec));
			this->next = this;
			this->prev = this;
		} else {
			this = xnfalloc(sizeof(DisplayModeRec));
			memcpy(this,mode,sizeof(DisplayModeRec));
			this->next = pScrn->modes;
			this->prev = last;
			last->next = this;
			pScrn->modes->prev = this;
		}
		last = this;
	}
	pScrn->virtualX     = virtX;
	pScrn->virtualY     = virtY;
}

DisplayModePtr
fbdevHWGetBuildinMode(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
	return &fPtr->buildin;
}

void
fbdevHWUseBuildinMode(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("UseBuildinMode");
	pScrn->modes    = &fPtr->buildin;
	pScrn->virtualX = pScrn->display->virtualX;
	pScrn->virtualY = pScrn->display->virtualY;
	if (pScrn->virtualX < fPtr->buildin.HDisplay)
		pScrn->virtualX = fPtr->buildin.HDisplay;
	if (pScrn->virtualY < fPtr->buildin.VDisplay)
		pScrn->virtualY = fPtr->buildin.VDisplay;
}

/* -------------------------------------------------------------------- */

static void
calculateFbmem_len(fbdevHWPtr fPtr)
{
	fPtr->fboff = (unsigned long) fPtr->fix.smem_start & ~PAGE_MASK;
	fPtr->fbmem_len = (fPtr->fboff+fPtr->fix.smem_len+~PAGE_MASK) &
			  PAGE_MASK;
}


void*
fbdevHWMapVidmem(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("MapVidmem");
	if (NULL == fPtr->fbmem) {
		calculateFbmem_len(fPtr);
		fPtr->fbmem = mmap(NULL, fPtr->fbmem_len, PROT_READ | PROT_WRITE,
				   MAP_SHARED, fPtr->fd, 0);
		if (-1 == (long)fPtr->fbmem) {
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
				   "mmap fbmem: %s\n", strerror(errno));
			fPtr->fbmem = NULL;
		} else {
		    /* Perhaps we'd better add fboff to fbmem and return 0 in
		       fbdevHWLinearOffset()? Of course we then need to mask
		       fPtr->fbmem with PAGE_MASK in fbdevHWUnmapVidmem() as
		       well. [geert] */
		}
	}
	pScrn->memPhysBase = (unsigned long)fPtr->fix.smem_start & (unsigned long)(PAGE_MASK);
	pScrn->fbOffset = (unsigned long)fPtr->fix.smem_start & (unsigned long)(~PAGE_MASK);
	return fPtr->fbmem;
}

int
fbdevHWLinearOffset(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("LinearOffset");
	return fPtr->fboff;
}

Bool
fbdevHWUnmapVidmem(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("UnmapVidmem");
	if (NULL != fPtr->fbmem) {
		if (-1 == munmap(fPtr->fbmem, fPtr->fbmem_len))
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
				   "munmap fbmem: %s\n", strerror(errno));
		fPtr->fbmem = NULL;
	}
	return TRUE;
}

void*
fbdevHWMapMMIO(ScrnInfoPtr pScrn)
{
	unsigned int mmio_off;

	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("MapMMIO");
	if (NULL == fPtr->mmio) {
		/* tell the kernel not to use accels to speed up console scrolling */
		fPtr->var.accel_flags = 0;
		if (0 != ioctl(fPtr->fd,FBIOPUT_VSCREENINFO,(void*)(&fPtr->var))) {
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
				   "FBIOPUT_VSCREENINFO: %s\n", strerror(errno));
			return FALSE;
		}
		mmio_off = (unsigned long) fPtr->fix.mmio_start & ~PAGE_MASK;
		fPtr->mmio_len = (mmio_off+fPtr->fix.mmio_len+~PAGE_MASK) &
				  PAGE_MASK;
		if (NULL == fPtr->fbmem)
			calculateFbmem_len(fPtr);
		fPtr->mmio = mmap(NULL, fPtr->mmio_len, PROT_READ | PROT_WRITE,
				  MAP_SHARED, fPtr->fd, fPtr->fbmem_len);
		if (-1 == (long)fPtr->mmio) {
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
				   "mmap mmio: %s\n", strerror(errno));
			fPtr->mmio = NULL;
		} else
			fPtr->mmio += mmio_off;
	}
	return fPtr->mmio;
}

Bool
fbdevHWUnmapMMIO(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("UnmapMMIO");
	if (NULL != fPtr->mmio) {
		if (-1 == munmap((void *)((unsigned long)fPtr->mmio & PAGE_MASK), fPtr->mmio_len))
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
				   "munmap mmio: %s\n", strerror(errno));
		fPtr->mmio = NULL;
		/* FIXME: restore var.accel_flags [geert] */
	}
	return TRUE;
}

/* -------------------------------------------------------------------- */

Bool
fbdevHWModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{	
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
	
	TRACE_ENTER("ModeInit");
	xfree2fbdev_fblayout(pScrn, &fPtr->var);
	xfree2fbdev_timing(mode,  &fPtr->var);
#if DEBUG
	print_xfree_mode("init",mode);
	print_fbdev_mode("init",&fPtr->var);
#endif
	pScrn->vtSema = TRUE;

	/* set */
	if (0 != ioctl(fPtr->fd,FBIOPUT_VSCREENINFO,(void*)(&fPtr->var))) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "FBIOPUT_VSCREENINFO: %s\n", strerror(errno));
		return FALSE;
	}
	/* read back */
	if (0 != ioctl(fPtr->fd,FBIOGET_FSCREENINFO,(void*)(&fPtr->fix))) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "FBIOGET_FSCREENINFO: %s\n", strerror(errno));
		return FALSE;
	}
	if (0 != ioctl(fPtr->fd,FBIOGET_VSCREENINFO,(void*)(&fPtr->var))) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "FBIOGET_VSCREENINFO: %s\n", strerror(errno));
		return FALSE;
	}
	return TRUE;
}

/* -------------------------------------------------------------------- */
/* video mode save/restore                                              */

/* TODO: colormap */
void
fbdevHWSave(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("Save");
	if (0 != ioctl(fPtr->fd,FBIOGET_VSCREENINFO,(void*)(&fPtr->saved_var)))
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "FBIOGET_VSCREENINFO: %s\n", strerror(errno));
}

void
fbdevHWRestore(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("Restore");
	if (0 != ioctl(fPtr->fd,FBIOPUT_VSCREENINFO,(void*)(&fPtr->saved_var)))
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "FBIOPUT_VSCREENINFO: %s\n", strerror(errno));
}

/* -------------------------------------------------------------------- */
/* callback for xf86HandleColormaps                                     */

void
fbdevHWLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
		 LOCO *colors, VisualPtr pVisual)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
	struct fb_cmap cmap;
	unsigned short red,green,blue;
	int i;

	TRACE_ENTER("ModeInit");
	cmap.len   = 1;
	cmap.red   = &red;
	cmap.green = &green;
	cmap.blue  = &blue;
	cmap.transp = NULL;
	for (i = 0; i < numColors; i++) {
		cmap.start = indices[i];
		red   = (colors[indices[i]].red   << 8) |
			colors[indices[i]].red;
		green = (colors[indices[i]].green << 8) |
			colors[indices[i]].green;
		blue  = (colors[indices[i]].blue  << 8) |
			colors[indices[i]].blue;
		if (-1 == ioctl(fPtr->fd,FBIOPUTCMAP,(void*)&cmap))
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
				   "FBIOPUTCMAP: %s\n", strerror(errno));
	}
}

/* -------------------------------------------------------------------- */
/* these can be hooked directly into ScrnInfoRec                        */

ModeStatus
fbdevHWValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
	struct fb_var_screeninfo var;

	TRACE_ENTER("ValidMode");
	memcpy(&var,&fPtr->var,sizeof(var));
	xfree2fbdev_timing(mode, &var);
	var.activate = FB_ACTIVATE_TEST;
	if (0 != ioctl(fPtr->fd,FBIOPUT_VSCREENINFO,(void*)(&fPtr->var))) {
		xf86DrvMsg(scrnIndex, X_ERROR,
			   "FBIOPUT_VSCREENINFO: %s\n", strerror(errno));
		return MODE_BAD;
	}
	return MODE_OK;
}

Bool
fbdevHWSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("SwitchMode");
	xfree2fbdev_timing(mode, &fPtr->var);
	if (0 != ioctl(fPtr->fd,FBIOPUT_VSCREENINFO,(void*)(&fPtr->var))) {
		xf86DrvMsg(scrnIndex, X_ERROR,
			   "FBIOPUT_VSCREENINFO: %s\n", strerror(errno));
		return FALSE;
	}
	return TRUE;
}

void
fbdevHWAdjustFrame(int scrnIndex, int x, int y, int flags)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("AdjustFrame");

	if ( x < 0 || x + fPtr->var.xres > fPtr->var.xres_virtual || 
	     y < 0 || y + fPtr->var.yres > fPtr->var.yres_virtual )
		return;

	fPtr->var.xoffset = x;
	fPtr->var.yoffset = y;
	if (-1 == ioctl(fPtr->fd,FBIOPAN_DISPLAY,(void*)&fPtr->var))
		xf86DrvMsgVerb(scrnIndex,5, X_WARNING,
			   "FBIOPAN_DISPLAY: %s\n", strerror(errno));
}

Bool
fbdevHWEnterVT(int scrnIndex, int flags)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
	
	TRACE_ENTER("EnterVT");
	if (!fbdevHWModeInit(pScrn, pScrn->currentMode))
		return FALSE;
	fbdevHWAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);
	return TRUE;
}

void
fbdevHWLeaveVT(int scrnIndex, int flags)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];

	TRACE_ENTER("LeaveVT");
	fbdevHWRestore(pScrn);
}

void
fbdevHWDPMSSet(ScrnInfoPtr pScrn, int mode, int flags)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
	unsigned long fbmode;

	if (!pScrn->vtSema)
		return;

	switch (mode) {
		case DPMSModeOn:
			fbmode = 0;
			break;
		case DPMSModeStandby:
			fbmode = 2;
			break;
		case DPMSModeSuspend:
			fbmode = 3;
			break;
		case DPMSModeOff:
			fbmode = 4;
			break;
		default:
			return;
	}

	if (-1 == ioctl(fPtr->fd, FBIOBLANK, (void *)fbmode))
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "FBIOBLANK: %s\n", strerror(errno));
}

Bool
fbdevHWSaveScreen(ScreenPtr pScreen, int mode)
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
	unsigned long unblank;

	if (!pScrn->vtSema)
		return TRUE;

	unblank = xf86IsUnblank(mode);

	if (-1 == ioctl(fPtr->fd, FBIOBLANK, (void *)(1-unblank))) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "FBIOBLANK: %s\n", strerror(errno));
		return FALSE;
	}

	return TRUE;
}
