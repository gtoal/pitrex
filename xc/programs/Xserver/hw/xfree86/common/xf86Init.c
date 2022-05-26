/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Init.c,v 3.250 2008/10/31 19:10:40 tsi Exp $ */

/*
 * Loosely based on code bearing the following copyright:
 *
 *   Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 */
/*
 * Copyright (c) 1992-2006 by The XFree86 Project, Inc.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *   1.  Redistributions of source code must retain the above copyright
 *       notice, this list of conditions, and the following disclaimer.
 *
 *   2.  Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer
 *       in the documentation and/or other materials provided with the
 *       distribution, and in the same place and form as other copyright,
 *       license and disclaimer information.
 *
 *   3.  The end-user documentation included with the redistribution,
 *       if any, must include the following acknowledgment: "This product
 *       includes software developed by The XFree86 Project, Inc
 *       (http://www.xfree86.org/) and its contributors", in the same
 *       place and form as other third-party acknowledgments.  Alternately,
 *       this acknowledgment may appear in the software itself, in the
 *       same form and location as other such third-party acknowledgments.
 *
 *   4.  Except as contained in this notice, the name of The XFree86
 *       Project, Inc shall not be used in advertising or otherwise to
 *       promote the sale, use or other dealings in this Software without
 *       prior written authorization from The XFree86 Project, Inc.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE XFREE86 PROJECT, INC OR ITS CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright � 2003, 2004, 2005 David H. Dawes.
 * Copyright � 2003, 2004, 2005 X-Oz Technologies.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions, and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *  3. The end-user documentation included with the redistribution,
 *     if any, must include the following acknowledgment: "This product
 *     includes software developed by X-Oz Technologies
 *     (http://www.x-oz.com/)."  Alternately, this acknowledgment may
 *     appear in the software itself, if and wherever such third-party
 *     acknowledgments normally appear.
 *
 *  4. Except as contained in this notice, the name of X-Oz
 *     Technologies shall not be used in advertising or otherwise to
 *     promote the sale, use or other dealings in this Software without
 *     prior written authorization from X-Oz Technologies.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL X-OZ TECHNOLOGIES OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <stdlib.h>

#undef HAS_UTSNAME
#if !defined(WIN32)
#define HAS_UTSNAME 1
#include <sys/utsname.h>
#endif

#define NEED_EVENTS
#ifdef __UNIXOS2__
#define I_NEED_OS2_H
#endif
#include <X11/X.h>
#include <X11/Xmd.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include "input.h"
#include "servermd.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "site.h"
#include "mi.h"

#include "compiler.h"

#ifdef XFree86LOADER
#include "loaderProcs.h"
#endif
#ifdef XFreeXDGA
#include "dgaproc.h"
#endif

#define XF86_OS_PRIVS
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Config.h"
#include "xf86_OSlib.h"
#include "xf86Version.h"
#include "xf86Date.h"
#include "xf86Build.h"
#include "mipointer.h"
#ifdef XINPUT
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#else
#include "inputstr.h"
#endif
#include "xf86DDC.h"
#include "xf86Xinput.h"
#include "xf86InPriv.h"
#ifdef RENDER
#include "picturestr.h"
#endif

#include "globals.h"

#ifdef XTESTEXT1
#include "atKeynames.h"
extern int xtest_command_key;
#endif /* XTESTEXT1 */

#if !defined(LOADERTEST) || !defined(XFree86LOADER)
#undef LOADERTEST
#define LOADERTEST 0
#endif

#if LOADERTEST
#include "loadertest.h"
static Bool doLoaderTest = FALSE;
#endif

/* forward declarations */

static void PrintBanner(void);
static void PrintMarkers(void);
static void RunVtInit(void);

static Bool autoconfig = FALSE;
static Bool appendauto = FALSE;
static Bool noAppendauto = FALSE;
/* Hardware detection is broken (and pretty useless) on RPi */
#if defined(__arm32__)
static Bool noHardware = TRUE;
#else
static Bool noHardware = FALSE;
#endif
static Bool noVT = FALSE;

static char *cmdline = NULL;

#ifdef __UNIXOS2__
extern void os2ServerVideoAccess();
#endif

void (*xf86OSPMClose)(void) = NULL;

#ifdef XFree86LOADER
static const char **probeModules = NULL;
static int numProbeModules = 0;
static const char **baseModules = NULL;
static int numBaseModules = 0;
#endif

/* Common pixmap formats */

static PixmapFormatRec formats[MAXFORMATS] = {
    { 1,	1,	BITMAP_SCANLINE_PAD },
    { 4,	8,	BITMAP_SCANLINE_PAD },
    { 8,	8,	BITMAP_SCANLINE_PAD },
    { 15,	16,	BITMAP_SCANLINE_PAD },
    { 16,	16,	BITMAP_SCANLINE_PAD },
    { 24,	32,	BITMAP_SCANLINE_PAD },
#ifdef RENDER
    { 32,	32,	BITMAP_SCANLINE_PAD },
#endif
};

#ifdef RENDER
#define NUMDEFFORMATS 7
#else
#define NUMDEFFORMATS 6
#endif

static int numFormats = NUMDEFFORMATS;

static Bool formatsDone = FALSE;

InputDriverRec xf86KEYBOARD = {
    1,
    "keyboard",
    NULL,
    NULL,
    NULL,
    NULL,
    0
};

static Bool
xf86CreateRootWindow(WindowPtr pWin)
{
    int ret = TRUE;
    int err = Success;
    ScreenPtr pScreen = pWin->drawable.pScreen;
    RootWinPropPtr pProp;
    CreateWindowProcPtr CreateWindow =
	    (CreateWindowProcPtr)(pScreen->
				  devPrivates[xf86CreateRootWindowIndex].
				  ptr);

#ifdef DEBUG
    ErrorF("xf86CreateRootWindow(%p)\n", pWin);
#endif

    if (pScreen->CreateWindow != xf86CreateRootWindow) {
	/* Can't find hook we are hung on */
	xf86DrvMsg(pScreen->myNum, X_WARNING /* X_ERROR */ ,
		   "xf86CreateRootWindow %p called when not in "
		   "pScreen->CreateWindow %p n",
		   (void *)xf86CreateRootWindow,
		   (void *)pScreen->CreateWindow);
    }

    /* Unhook this function ... */
    pScreen->CreateWindow = CreateWindow;
    pScreen->devPrivates[xf86CreateRootWindowIndex].ptr = NULL;

    /* ... and call the previous CreateWindow fuction, if any */
    if (NULL != pScreen->CreateWindow) {
	ret = (*pScreen->CreateWindow)(pWin);
    }

    /* Now do our stuff */
    if (xf86RegisteredPropertiesTable != NULL) {
	if (pWin->parent == NULL && xf86RegisteredPropertiesTable != NULL) {
	    for (pProp = xf86RegisteredPropertiesTable[pScreen->myNum];
		 pProp != NULL && err == Success; pProp = pProp->next) {
		Atom prop;

		prop = MakeAtom(pProp->name, strlen(pProp->name), TRUE);
		err = ChangeWindowProperty(pWin,
					   prop, pProp->type,
					   pProp->format, PropModeReplace,
					   pProp->size, pProp->data, FALSE);
	    }

	    /* Look at err */
	    ret &= (err == Success);

	} else {
	    xf86Msg(X_ERROR, "xf86CreateRootWindow unexpectedly called with "
		    "non-root window %p (parent %p)\n",
		    (void *)pWin, (void *)pWin->parent);
	    ret = FALSE;
	}
    }
#ifdef DEBUG
    ErrorF("xf86CreateRootWindow() returns %d\n", ret);
#endif
    return (ret);
}

/*
 * InitOutput --
 *	Initialize screenInfo for all actually accessible framebuffers.
 *      That includes vt-manager setup, querying all possible devices and
 *      collecting the pixmap formats.
 */

static void
PostConfigInit(void)
{
    static Bool done = FALSE;

    if (done)
	return;

    done = TRUE;

    xf86OSPMClose = xf86OSPMOpen();

    if (!noVT) {
	/* Run an external VT Init program if specified in the config file */
	RunVtInit();
    }

    /* Do this after XF86Config is read (it's normally in OsInit()) */
    OsInitColors();
}

void
InitOutput(ScreenInfo * pScreenInfo, const int argc, const char **argv)
{
    int i, j, k, scr_index;
    static unsigned long generation = 0;

#ifdef XFree86LOADER
    const char **modulelist;
    pointer *optionlist;
    Bool haveScreens = FALSE;
#endif
    const char **driverlist;
    screenLayoutPtr layout;
    Pix24Flags screenpix24, pix24;
    MessageType pix24From = X_DEFAULT;
    Bool pix24Fail = FALSE;
    Bool autoretry = FALSE;
    int found = 0;

#ifdef __UNIXOS2__
    os2ServerVideoAccess();	/*
				 * See if we have access to the screen
				 * before doing anything.
				 */
#endif

    xf86Initialising = TRUE;

    /* Do this early? */
    if (generation != serverGeneration) {
	xf86ScreenIndex = AllocateScreenPrivateIndex();
	xf86CreateRootWindowIndex = AllocateScreenPrivateIndex();
	xf86PixmapIndex = AllocatePixmapPrivateIndex();
	generation = serverGeneration;
    }

    if (serverGeneration == 1) {

	pScreenInfo->numScreens = 0;

	if ((xf86ServerName = strrchr(argv[0], '/')) != 0)
	    xf86ServerName++;
	else
	    xf86ServerName = argv[0];

	/* Set default paths. */
	xf86FileCmdline.handle = &xf86FileCmdline;
	xf86FileDefaults.handle = &xf86FileDefaults;
	xf86FileDefaults.fontPath = xnfstrdup(defaultFontPath);
	xf86FileDefaults.fontPathFrom = X_DEFAULT;
	xf86FileDefaults.rgbPath = xnfstrdup(rgbPath);
	xf86FileDefaults.rgbPathFrom = X_DEFAULT;
	/* Combine the defaults with the command line data. */
	xf86FilePaths = xf86ConfCombineFilesData(&xf86FileCmdline, X_CMDLINE,
						 &xf86FileDefaults,
						 X_DEFAULT);
	if (!xf86FilePaths)
	    FatalError("File path initialisation failed.\n");

	xf86FilePaths->identifier = xstrdup("<combined pre-config>");

	PrintBanner();
	PrintMarkers();
	if (xf86FilePaths->logFile) {
	    time_t t;
	    const char *ct;

	    t = time(NULL);
	    ct = ctime(&t);
	    xf86MsgVerb(xf86FilePaths->logFileFrom, 0,
			"Log file: \"%s\", Time: %s", xf86FilePaths->logFile,
			ct);
	}

	/* Read and parse the config file */
	if (!autoconfig && !xf86DoProbe && !xf86DoConfigure) {
	    switch (xf86LoadConfigFile(NULL, FALSE)) {
	    case CONFIG_OK:
		if (!noAppendauto
		    && !xf86CheckForLayoutOrScreen(DEFAULT_CONFIG)
		    && !noHardware) {
		    xf86MsgVerb(X_INFO, 0,
				"No Screen or Layout sections in the "
				"config file.\n"
				"\tAppending default built-in "
				"configuration.\n");
		    appendauto = TRUE;
		} else if (appendauto) {
		    xf86MsgVerb(X_CMDLINE, 0,
				"Appending default built-in configuration.\n");
		}
		break;
	    case CONFIG_PARSE_ERROR:
		xf86Msg(X_ERROR, "Error parsing the config file.\n");
		return;
	    case CONFIG_NOFILE:
		/* For now, don't enable autoconfig when -nohw is enabled. */
		if (!noHardware)
		    autoconfig = TRUE;
		break;
	    }
	} else {
	    appendauto = FALSE;
	}

	/*
	 * Install signal handlers earlier to catch hardware problems.
	 * Perhaps there should be a command line flag to control
	 * xf86Info.notrapSignals.
	 */
	xf86Info.caughtSignal = FALSE;
	if (!xf86Info.notrapSignals) {
	    signal(SIGSEGV, xf86SigHandler);
	    signal(SIGILL, xf86SigHandler);
#ifdef SIGEMT
	    signal(SIGEMT, xf86SigHandler);
#endif
	    signal(SIGFPE, xf86SigHandler);
#ifdef SIGBUS
	    signal(SIGBUS, xf86SigHandler);
#endif
#ifdef SIGSYS
	    signal(SIGSYS, xf86SigHandler);
#endif
#ifdef SIGXCPU
	    signal(SIGXCPU, xf86SigHandler);
#endif
#ifdef SIGXFSZ
	    signal(SIGXFSZ, xf86SigHandler);
#endif
#ifdef MEMDEBUG
	    signal(SIGUSR2, xf86SigMemDebug);
#endif
	}
#ifdef XFree86LOADER
	/* Initialise the loader */
	LoaderInit();

	/* Setup probe and base module lists. */
	LoaderSetPath(xf86FilePaths->modulePath);
	/* Load modules required for hardware probing. */
	if (!noHardware) {
	    numProbeModules = 1;
	    probeModules =
		    xnfalloc(sizeof(*probeModules) * (numProbeModules + 1));
	    probeModules[0] = "pcidata";
	    probeModules[numProbeModules] = NULL;
	}
	numBaseModules = 1;
	baseModules = xnfalloc(sizeof(*baseModules) * (numBaseModules + 1));
	baseModules[0] = "bitmap";
	baseModules[numBaseModules] = NULL;
#endif

	if (autoconfig || appendauto) {
	    if (!noVT)
		xf86OpenConsole();

	    if (!noHardware) {

#ifdef XFree86LOADER
		/* Load modules required for hardware probing. */
		if (probeModules) {
		    if (!xf86LoadModules(probeModules, NULL)) {
			FatalError
				("Unable to load required probe modules, "
				 "Exiting...\n");
		    }
		    xfree(probeModules);
		    probeModules = NULL;
		}
#endif

		/* Enable full I/O access */
		xf86EnableIO();

		/*
		 * Do a general bus probe.
		 * This will be a PCI probe for x86 platforms.
		 */
		xf86BusProbe();
	    }
	    if (!xf86AutoConfig()) {
		xf86Msg(X_ERROR, "Auto configuration failed.\n");
		return;
	    }
	}

	if (!xf86DoProbe && !xf86DoConfigure) {
	    if (xf86ProcessConfiguration() != CONFIG_OK) {
		xf86Msg(X_ERROR, "Error processing configuration data.\n");
		return;
	    }
	}
#ifdef XFree86LOADER
	LoaderSetPath(xf86FilePaths->modulePath);

#ifdef TESTING
	{
	    char **list, **l;
	    const char *subdirs[] = {
		"drivers",
		NULL
	    };
	    const char *patlist[] = {
		"(.*)_drv\\.so",
		"(.*)_drv\\.o",
		NULL
	    };
	    ErrorF("Getting module listing...\n");
	    list = LoaderListDirs(NULL, NULL);
	    if (list)
		for (l = list; *l; l++)
		    ErrorF("module: %s\n", *l);
	    LoaderFreeDirList(list);
	    ErrorF("Getting video driver listing...\n");
	    list = LoaderListDirs(subdirs, NULL);
	    if (list)
		for (l = list; *l; l++)
		    ErrorF("video driver: %s\n", *l);
	    LoaderFreeDirList(list);
	    ErrorF("Getting driver listing...\n");
	    list = LoaderListDirs(NULL, patlist);
	    if (list)
		for (l = list; *l; l++)
		    ErrorF("video driver: %s\n", *l);
	    LoaderFreeDirList(list);
	}
#endif

#if LOADERTEST
	if (doLoaderTest)
	    LoaderTest();
#endif

	/* Load mandatory probe and base modules. */
	if (probeModules) {
	    if (!xf86LoadModules(probeModules, NULL))
		FatalError
			("Unable to load required probe modules, Exiting...\n");
	    xfree(probeModules);
	    probeModules = NULL;
	}
	if (baseModules) {
	    if (!xf86LoadModules(baseModules, NULL))
		FatalError
			("Unable to load required base modules, Exiting...\n");
	    xfree(baseModules);
	    baseModules = NULL;
	}
#endif

	if (!autoconfig && !appendauto) {
	    if (!noVT)
		xf86OpenConsole();

	    if (!noHardware) {
		/* Enable full I/O access */
		xf86EnableIO();

		/*
		 * Do a general bus probe.
		 * This will be a PCI probe for x86 platforms.
		 */
		xf86BusProbe();

		if (xf86DoProbe)
		    DoProbe();

		if (xf86DoConfigure)
		    DoConfigure();
	    }
	}

	PostConfigInit();

      retry:

	if (!noHardware) {
	    /* Initialise the resource broker */
	    xf86ResourceBrokerInit();
	}

	/*
	 * Do whatever is needed to setup the initial driver list.  Don't
	 * redo this again when doing an autoconfig retry.
	 */

	if (!autoretry) {
#ifdef XFree86LOADER
	    /* Load all modules specified explicitly in the config file */
	    if ((modulelist = xf86ModulelistFromConfig(&optionlist))) {
		xf86LoadModules(modulelist, optionlist);
		xf86ModulelistFree(modulelist, optionlist);
	    }

	    /* Load all driver modules specified in the config file */
	    if ((driverlist = xf86DriverlistFromConfig())) {
		xf86LoadModules(driverlist, NULL);
		xfree(driverlist);
	    }

	    /* Setup the builtin input drivers */
	    xf86AddInputDriver(&xf86KEYBOARD, NULL, 0);
	    /* Load all input driver modules specified in the config file. */
	    if ((modulelist = xf86InputDriverlistFromConfig())) {
		xf86LoadModules(modulelist, NULL);
		xfree(modulelist);
	    }

	    /*
	     * It is expected that xf86AddDriver()/xf86AddInputDriver will be
	     * called for each driver as it is loaded.  Those functions
	     * save the module pointers for drivers.
	     * XXX Nothing keeps track of them for other modules.
	     */
	    /* XXX What do we do if not all of these could be loaded? */

#else

	    /* Re-order the driver list for the benefit of autoconfiguration. */
	    if (autoconfig && (driverlist = xf86DriverlistFromConfig())) {
		DriverPtr *newList =
			xnfcalloc(1, xf86NumDrivers * sizeof(DriverPtr));
		int numNew;

		for (i = 0; driverlist[i]; i++) {
		    for (j = 0; j < xf86NumDrivers; j++) {
			if (xf86DriverList[j] && xf86DriverList[j]->driverName
			    && xf86NameCmp(xf86DriverList[j]->driverName,
					   driverlist[i]) == 0) {
			    newList[i] = xf86DriverList[j];
			    xf86DriverList[j] = NULL;
			    break;
			}
		    }
		}
		numNew = i;
		/* Write the new list out. */
		for (i = 0; i < xf86NumDrivers; i++) {
		    if (i < numNew)
			xf86DriverList[i] = newList[i];
		    else
			xf86DriverList[i] = NULL;
		}
		xf86NumDrivers = numNew;
		xfree(newList);
		xfree(driverlist);
	    }
#endif
	}

	/*
	 * At this point, xf86DriverList[] is all filled in with entries for
	 * each of the drivers to try and xf86NumDrivers has the number of
	 * drivers.  If there are none, return now.
	 */

	if (xf86NumDrivers == 0) {
	    xf86Msg(X_ERROR, "No drivers available.\n");
	    return;
	}

	/*
	 * Call each of the Identify functions.  The Identify functions print
	 * out some identifying information, and anything else that might be
	 * needed at this early stage.
	 */

	for (i = 0; i < xf86NumDrivers; i++) {
	    if (xf86DriverList[i]) {
		/*
		 * The Identify function is mandatory, but if it isn't there
		 * continue.
		 */
		if (xf86DriverList[i]->Identify != NULL)
		    xf86DriverList[i]->Identify(0);
		else {
		    xf86Msg(X_WARNING,
			    "Driver `%s' has no Identify function\n",
			    xf86DriverList[i]->
			    driverName ? xf86DriverList[i]->
			    driverName : "noname");
		}
	    }
	}

	if (!noHardware) {
	    /*
	     * Locate bus slot that had register IO enabled at server startup
	     */

	    xf86AccessInit();
	    xf86FindPrimaryDevice();
	}

	/*
	 * Now call each of the Probe functions.  Each successful probe will
	 * result in an extra entry added to the xf86Screens[] list for each
	 * instance of the hardware found.
	 */

	/*
	 * For autoconfiguration, where several drivers will be tried in
	 * sequence, we need to make sure that those with a failing Probe() up
	 * until the first successful Probe() are deleted here.  This is
	 * important for the autoconfiguration retry mechanism.  Also, stop
	 * probing at the first successful probe.  The remaining fallback
	 * drivers will be retried on preinit failure.
	 */

	found = 0;
	for (i = 0; i < xf86NumDrivers; i++) {
	    if (xf86DriverList[i]) {
		if (xf86DriverList[i]->Probe != NULL) {
		    if (xf86DriverList[i]->
			Probe(xf86DriverList[i], PROBE_DEFAULT))
			found++;
		    if (!found) {
			xf86DeleteDriver(i);
		    }
		} else {
		    xf86MsgVerb(X_WARNING, 0,
				"Driver `%s' has no Probe function "
				"(ignoring)\n",
				xf86DriverList[i]->driverName ?
				xf86DriverList[i]->driverName : "noname");
		}
		xf86SetPciVideo(NULL, NONE);
		if (found && autoconfig)
		    break;
	    }
	}

	/*
	 * If nothing was detected, return now.
	 */

	if (xf86NumScreens == 0) {
	    xf86Msg(X_ERROR, "No devices detected.\n");
	    return;
	}

	/*
	 * Match up the screens found by the probes against those specified
	 * in the config file.  Remove the ones that won't be used.  Sort
	 * them in the order specified.
	 */

	/*
	 * What is the best way to do this?
	 *
	 * For now, go through the screens allocated by the probes, and
	 * look for screen config entry which refers to the same device
	 * section as picked out by the probe.
	 *
	 */

	for (i = 0; i < xf86NumScreens; i++) {
	    for (k = 0; k < xf86Info.serverLayout->numScreens; k++) {
		layout = xf86Info.serverLayout->screenLayouts[k];
		if (!layout || !layout->screen)
		    continue;
		found = FALSE;
		for (j = 0; j < xf86Screens[i]->numEntities; j++) {

		    GDevPtr dev =
			    xf86GetDevFromEntity(xf86Screens[i]->
						 entityList[j],
						 xf86Screens[i]->
						 entityInstanceList[j]);

		    if (dev == layout->screen->device) {
			/* A match has been found */
			xf86Screens[i]->confScreen = layout->screen;
			found = TRUE;
			break;
		    }
		}
		if (found)
		    break;
	    }
	    if (k == xf86Info.serverLayout->numScreens) {
		/* No match found */
		xf86Msg(X_ERROR,
			"Screen %d deleted because of no matching "
			"config section.\n", i);
		xf86DeleteScreen(i--, 0);
	    }
	}

	/*
	 * If no screens left, return now.
	 */

	if (xf86NumScreens == 0) {
	    xf86Msg(X_ERROR,
		    "Device(s) detected, but none match those in the "
		    "config file.\n");
	    return;
	}

	if (!noHardware) {
	    xf86PostProbe();
	    xf86EntityInit();
	}

	/*
	 * Sort the drivers to match the requested ordering.  Using a slow
	 * bubble sort.
	 */
	for (j = 0; j < xf86NumScreens - 1; j++) {
	    for (i = 0; i < xf86NumScreens - j - 1; i++) {
		if (xf86Screens[i + 1]->confScreen->screennum <
		    xf86Screens[i]->confScreen->screennum) {
		    ScrnInfoPtr tmpScrn = xf86Screens[i + 1];

		    xf86Screens[i + 1] = xf86Screens[i];
		    xf86Screens[i] = tmpScrn;
		}
	    }
	}
	/* Fix up the indexes */
	for (i = 0; i < xf86NumScreens; i++) {
	    xf86Screens[i]->scrnIndex = i;
	}

	/*
	 * Call the driver's PreInit()'s to complete initialisation for the
	 * first generation.
	 */

	for (i = 0; i < xf86NumScreens; i++) {
	    xf86EnableAccess(xf86Screens[i]);
	    if (xf86Screens[i]->PreInit &&
		xf86Screens[i]->PreInit(xf86Screens[i], 0)) {
		xf86Screens[i]->configured = TRUE;
#ifdef XFree86LOADER
		haveScreens = TRUE;
#endif
	    }
	}

#ifdef XFree86LOADER
	if (!haveScreens)
	    LoaderCheckUnresolved(0);
#endif

	for (i = 0; i < xf86NumScreens; i++)
	    if (!xf86Screens[i]->configured)
		xf86DeleteScreen(i--, 0);

	/*
	 * If autoconfig, try again.  The first driver remaining in the list
	 * must be the one that had a successful Probe() but an unsuccessful
	 * PreInit().  Remove that driver before trying again.
	 */
	if (xf86NumScreens == 0 && autoconfig) {
	    for (i = 0; i < xf86NumDrivers; i++) {
		if (xf86DriverList[i]) {
		    xf86DeleteDriver(i);
		    autoretry = TRUE;
		    /* Clear claimed config sections. */
		    for (j = 0; j < xf86Info.serverLayout->numScreens; j++) {
			layout = xf86Info.serverLayout->screenLayouts[j];
			if (layout && layout->screen)
			    layout->screen->device->claimed = FALSE;
		    }
		    goto retry;
		}
	    }
	    xf86Msg(X_ERROR,
		    "Auto configuration failed.  No drivers left to try.\n");
	    return;
	}

	if (xf86NumScreens == 0) {
	    xf86Msg(X_ERROR,
		    "Screen(s) found, but none have a usable configuration.\n");
	    return;
	}

	/* This could be moved into a separate function */

	/*
	 * Check that all screens have initialised the mandatory function
	 * entry points.  Delete those which have not.
	 */

#define WARN_SCREEN(func) \
    xf86Msg(X_ERROR, "Driver `%s' has no %s function, deleting.\n", \
	   xf86Screens[i]->name, (warned++, func))

	for (i = 0; i < xf86NumScreens; i++) {
	    int warned = 0;

	    if (!xf86Screens[i]->name) {
		char c;
		if (i < 10)
		    c = i + '0';
		else
		    c = i - 10 + 'A';
		xasprintf(&xf86Screens[i]->name, "screen%c", c);
		if (!xf86Screens[i]->name)
		    FatalAlloc();
		xf86MsgVerb(X_WARNING, 0,
			    "Screen driver %d has no name set, using `%s'.\n",
			    i, xf86Screens[i]->name);
	    }
	    if (xf86Screens[i]->ScreenInit == NULL)
		WARN_SCREEN("ScreenInit");
	    if (xf86Screens[i]->EnterVT == NULL)
		WARN_SCREEN("EnterVT");
	    if (xf86Screens[i]->LeaveVT == NULL)
		WARN_SCREEN("LeaveVT");
	    if (warned)
		xf86DeleteScreen(i--, 0);
	}

	/*
	 * If no screens left, return now.
	 */

	if (xf86NumScreens == 0) {
	    xf86Msg(X_ERROR, "Screen(s) found, but drivers were unusable.\n");
	    return;
	}

	/* XXX Should this be before or after loading dependent modules? */
	if (xf86ProbeOnly) {
	    OsCleanup(TRUE);
	    AbortDDX();
	    fflush(stderr);
	    exit(0);
	}
#ifdef XFree86LOADER
	/* Remove (unload) drivers that are not required. */
	for (i = 0; i < xf86NumDrivers; i++)
	    if (xf86DriverList[i] && xf86DriverList[i]->refCount <= 0) {
		xf86DeleteDriver(i);
	    }
#endif

	/*
	 * At this stage we know how many screens there are.
	 */

	for (i = 0; i < xf86NumScreens; i++)
	    xf86InitViewport(xf86Screens[i]);

	/*
	 * Collect all pixmap formats and check for conflicts at the display
	 * level.  Should we die here?  Or just delete the offending screens?
	 * Also, should this be done for -probeonly?
	 */
	screenpix24 = Pix24DontCare;
	for (i = 0; i < xf86NumScreens; i++) {
	    if (xf86Screens[i]->imageByteOrder !=
		xf86Screens[0]->imageByteOrder)
		FatalError("Inconsistent display bitmapBitOrder.  Exiting\n");
	    if (xf86Screens[i]->bitmapScanlinePad !=
		xf86Screens[0]->bitmapScanlinePad)
		FatalError
			("Inconsistent display bitmapScanlinePad.  Exiting\n");
	    if (xf86Screens[i]->bitmapScanlineUnit !=
		xf86Screens[0]->bitmapScanlineUnit)
		FatalError
			("Inconsistent display bitmapScanlineUnit.  Exiting\n");
	    if (xf86Screens[i]->bitmapBitOrder !=
		xf86Screens[0]->bitmapBitOrder)
		FatalError("Inconsistent display bitmapBitOrder.  Exiting\n");

	    /* Determine the depth 24 pixmap format the screens would like */
	    if (xf86Screens[i]->pixmap24 != Pix24DontCare) {
		if (screenpix24 == Pix24DontCare)
		    screenpix24 = xf86Screens[i]->pixmap24;
		else if (screenpix24 != xf86Screens[i]->pixmap24)
		    FatalError
			    ("Inconsistent depth 24 pixmap format.  Exiting\n");
	    }
	}
	/* check if screenpix24 is consistent with the config/cmdline */
	if (xf86Info.pixmap24 != Pix24DontCare) {
	    pix24 = xf86Info.pixmap24;
	    pix24From = xf86Info.pix24From;
	    if (screenpix24 != Pix24DontCare
		&& screenpix24 != xf86Info.pixmap24)
		pix24Fail = TRUE;
	} else if (screenpix24 != Pix24DontCare) {
	    pix24 = screenpix24;
	    pix24From = X_PROBED;
	} else
	    pix24 = Pix24Use32;

	if (pix24Fail)
	    FatalError
		    ("Screen(s) can't use the required depth 24 pixmap format"
		     " (%d).  Exiting\n", PIX24TOBPP(pix24));

	/* Initialise the depth 24 format */
	for (j = 0; j < numFormats && formats[j].depth != 24; j++) ;
	formats[j].bitsPerPixel = PIX24TOBPP(pix24);

	/* Collect additional formats */
	for (i = 0; i < xf86NumScreens; i++) {
	    for (j = 0; j < xf86Screens[i]->numFormats; j++) {
		for (k = 0;; k++) {
		    if (k >= numFormats) {
			if (k >= MAXFORMATS)
			    FatalError("Too many pixmap formats!  Exiting\n");
			formats[k] = xf86Screens[i]->formats[j];
			numFormats++;
			break;
		    }
		    if (formats[k].depth == xf86Screens[i]->formats[j].depth) {
			if ((formats[k].bitsPerPixel ==
			     xf86Screens[i]->formats[j].bitsPerPixel) &&
			    (formats[k].scanlinePad ==
			     xf86Screens[i]->formats[j].scanlinePad))
			    break;
			FatalError("Inconsistent pixmap format for depth %d."
				   "  Exiting\n", formats[k].depth);
		    }
		}
	    }
	}
	formatsDone = TRUE;

	if (xf86Info.vtno >= 0) {
#define VT_ATOM_NAME         "XFree86_VT"
	    Atom VTAtom;
	    CARD32 *VT = NULL;
	    int ret;

	    /* This memory needs to stay available until the screen has been
	     * initialized, and we can create the property for real.
	     */
	    if ((VT = xalloc(sizeof(CARD32))) == NULL) {
		FatalError
			("Unable to make VT property - out of memory.  "
			 "Exiting...\n");
	    }
	    *VT = xf86Info.vtno;

	    VTAtom = MakeAtom(VT_ATOM_NAME, sizeof(VT_ATOM_NAME), TRUE);

	    for (i = 0, ret = Success; i < xf86NumScreens && ret == Success;
		 i++) {
		ret = xf86RegisterRootWindowProperty(xf86Screens[i]->
						     scrnIndex, VTAtom,
						     XA_INTEGER, 32, 1, VT);
		if (ret != Success)
		    xf86DrvMsg(xf86Screens[i]->scrnIndex, X_WARNING,
			       "Failed to register VT property\n");
	    }
	}

	/* If a screen uses depth 24, show what the pixmap format is */
	for (i = 0; i < xf86NumScreens; i++) {
	    if (xf86Screens[i]->depth == 24) {
		xf86Msg(pix24From, "Depth 24 pixmap format is %d bpp\n",
			PIX24TOBPP(pix24));
		break;
	    }
	}

#ifdef XKB
	xf86InitXkb();
#endif
	/* set up the proper access funcs */
	xf86PostPreInit();

	AddCallback(&ServerGrabCallback, xf86GrabServerCallback, NULL);

    } else {
	/*
	 * serverGeneration != 1; some OSs have to do things here, too.
	 */
	xf86OpenConsole();

	/*
	 * should we reopen it here? We need to deal with an already opened
	 * device. We could leave this to the OS layer. For now we simply
	 * close it here
	 */
	if (xf86OSPMClose)
	    xf86OSPMClose();
	if ((xf86OSPMClose = xf86OSPMOpen()) != NULL)
	    xf86MsgVerb(X_INFO, 3, "APM registered successfully\n");

	/* Make sure full I/O access is enabled */
	xf86EnableIO();
    }

    /*
     * Use the previously collected parts to setup pScreenInfo
     */

    pScreenInfo->imageByteOrder = xf86Screens[0]->imageByteOrder;
    pScreenInfo->bitmapScanlinePad = xf86Screens[0]->bitmapScanlinePad;
    pScreenInfo->bitmapScanlineUnit = xf86Screens[0]->bitmapScanlineUnit;
    pScreenInfo->bitmapBitOrder = xf86Screens[0]->bitmapBitOrder;
    pScreenInfo->numPixmapFormats = numFormats;
    for (i = 0; i < numFormats; i++)
	pScreenInfo->formats[i] = formats[i];

    /* Make sure the server's VT is active */

    if (serverGeneration != 1) {
	xf86Resetting = TRUE;
	/* All screens are in the same state, so just check the first */
	if (!noVT && !xf86Screens[0]->vtSema) {
#ifdef HAS_USL_VTS
	    ioctl(xf86Info.consoleFd, VT_RELDISP, VT_ACKACQ);
#endif
	    xf86AccessEnter();
	    xf86EnterServerState(SETUP);
	}
    }
#ifdef __SCO__
    else {
	/*
	 * Under SCO we must ack that we got the console at startup,
	 * I think this is the safest way to assure it.
	 */
	static int once = 1;

	if (once) {
	    once = 0;
	    if (ioctl(xf86Info.consoleFd, VT_RELDISP, VT_ACKACQ) < 0)
		xf86Msg(X_WARNING, "VT_ACKACQ failed");
	}
    }
#endif /* __SCO__ */

    for (i = 0; i < xf86NumScreens; i++) {
	xf86EnableAccess(xf86Screens[i]);
	/*
	 * Almost everything uses these defaults, and many of those that
	 * don't, will wrap them.
	 */
	xf86Screens[i]->EnableDisableFBAccess = xf86EnableDisableFBAccess;
	xf86Screens[i]->SetDGAMode = xf86SetDGAMode;
	xf86Screens[i]->DPMSSet = NULL;
	xf86Screens[i]->LoadPalette = NULL;
	xf86Screens[i]->SetOverscan = NULL;
	scr_index = AddScreen(xf86Screens[i]->ScreenInit, argc, argv);
	if (scr_index == i) {
	    /*
	     * Hook in our ScrnInfoRec, and initialise some other pScreen
	     * fields.
	     */
	    screenInfo.screens[scr_index]->devPrivates[xf86ScreenIndex].ptr
		    = (pointer) xf86Screens[i];
	    xf86Screens[i]->pScreen = screenInfo.screens[scr_index];
	    /* The driver should set this, but make sure it is set anyway */
	    xf86Screens[i]->vtSema = TRUE;
	} else {
	    /* This shouldn't normally happen */
	    FatalError("AddScreen/ScreenInit failed for driver %d\n", i);
	}

#ifdef DEBUG
	ErrorF("InitOutput - xf86Screens[%d]->pScreen = %p\n",
	       i, xf86Screens[i]->pScreen);
	ErrorF("xf86Screens[%d]->pScreen->CreateWindow = %p\n",
	       i, xf86Screens[i]->pScreen->CreateWindow);
#endif

	screenInfo.screens[scr_index]->devPrivates[xf86CreateRootWindowIndex].
		ptr = (void *)(xf86Screens[i]->pScreen->CreateWindow);
	xf86Screens[i]->pScreen->CreateWindow = xf86CreateRootWindow;

#ifdef RENDER
	if (PictureGetSubpixelOrder(xf86Screens[i]->pScreen) ==
	    SubPixelUnknown) {
	    xf86MonPtr DDC = (xf86MonPtr)(xf86Screens[i]->monitor->DDC);

	    PictureSetSubpixelOrder(xf86Screens[i]->pScreen,
				    DDC ?
				    (DDC->features.input_type ?
				     SubPixelHorizontalRGB : SubPixelNone) :
				    SubPixelUnknown);
	}
#endif
#ifdef RANDR
	if (!xf86Info.disableRandR)
	    xf86RandRInit(screenInfo.screens[scr_index]);
	xf86Msg(xf86Info.randRFrom, "RandR %s\n",
		xf86Info.disableRandR ? "disabled" : "enabled");
#endif
#ifdef NOT_USED
	/*
	 * Here we have to let the driver getting access of the VT. Note that
	 * this doesn't mean that the graphics board may access automatically
	 * the monitor. If the monitor is shared this is done in xf86CrossScreen!
	 */
	if (!xf86Info.sharedMonitor)
	    xf86Screens[i]->EnterLeaveMonitor(ENTER);
#endif
    }

#ifdef XFree86LOADER
    if ((serverGeneration == 1) && LoaderCheckUnresolved(0)) {
	/* For now, just a warning */
	xf86Msg(X_WARNING, "Some symbols could not be resolved!\n");
    }
#endif

    xf86PostScreenInit();

    xf86InitOrigins();

    xf86Resetting = FALSE;
    xf86Initialising = FALSE;

    RegisterBlockAndWakeupHandlers((BlockHandlerProcPtr) NoopDDA, xf86Wakeup,
				   NULL);
}

static InputDriverPtr
MatchInput(IDevPtr pDev)
{
    int i;

    for (i = 0; i < xf86NumInputDrivers; i++) {
	if (xf86InputDriverList[i] && xf86InputDriverList[i]->driverName &&
	    xf86NameCmp(pDev->driver,
			xf86InputDriverList[i]->driverName) == 0)
	    return xf86InputDriverList[i];
    }
    return NULL;
}

/*
 * InitInput --
 *      Initialize all supported input devices.
 */

void
InitInput(const int argc, const char **argv)
{
    int i;
    IDevPtr pDev;
    InputDriverPtr pDrv;
    InputInfoPtr pInfo;
    static InputInfoPtr coreKeyboard = NULL, corePointer = NULL;

    xf86Info.vtRequestsPending = FALSE;
    xf86Info.inputPending = FALSE;
#ifdef XTESTEXT1
    xtest_command_key = KEY_Begin + MIN_KEYCODE;
#endif /* XTESTEXT1 */

    if (serverGeneration == 1) {
	/* Call the PreInit function for each input device instance. */
	for (i = 0; i < xf86Info.serverLayout->numInputs; i++) {
	    pDev = xf86Info.serverLayout->inputDevs[i];
	    if (!pDev)
		continue;
	    /* XXX The keyboard driver is a special case for now. */
	    if (!xf86NameCmp(pDev->driver, "keyboard")) {
		xf86Msg(X_INFO, "Keyboard \"%s\" handled by legacy driver\n",
			pDev->identifier);
		continue;
	    }
	    if ((pDrv = MatchInput(pDev)) == NULL) {
		xf86Msg(X_ERROR, "No Input driver matching `%s'\n",
			pDev->driver);
		/* XXX For now, just continue. */
		continue;
	    }
	    if (!pDrv->PreInit) {
		xf86MsgVerb(X_WARNING, 0,
			    "Input driver `%s' has no PreInit function "
			    "(ignoring)\n", pDrv->driverName);
		continue;
	    }
	    pInfo = pDrv->PreInit(pDrv, pDev, 0);
	    if (!pInfo) {
		xf86Msg(X_ERROR, "PreInit returned NULL for \"%s\"\n",
			pDev->identifier);
		continue;
	    } else if (!(pInfo->flags & XI86_CONFIGURED)) {
		xf86Msg(X_ERROR, "PreInit failed for input device \"%s\"\n",
			pDev->identifier);
		xf86DeleteInput(pInfo, 0);
		continue;
	    }
	    if (pInfo->flags & XI86_CORE_KEYBOARD) {
		if (coreKeyboard) {
		    xf86Msg(X_ERROR,
			    "Attempt to register more than one core keyboard "
			    "(%s)\n", pInfo->name);
		    pInfo->flags &= ~XI86_CORE_KEYBOARD;
		} else {
		    if (!(pInfo->flags & XI86_KEYBOARD_CAPABLE)) {
			/* XXX just a warning for now */
			xf86Msg(X_WARNING,
				"%s: does not have core keyboard "
				"capabilities\n", pInfo->name);
		    }
		    coreKeyboard = pInfo;
		}
	    }
	    if (pInfo->flags & XI86_CORE_POINTER) {
		if (corePointer) {
		    xf86Msg(X_ERROR,
			    "Attempt to register more than one core pointer "
			    "(%s)\n", pInfo->name);
		    pInfo->flags &= ~XI86_CORE_POINTER;
		} else {
		    if (!(pInfo->flags & XI86_POINTER_CAPABLE)) {
			/* XXX just a warning for now */
			xf86Msg(X_WARNING,
				"%s: does not have core pointer capabilities\n",
				pInfo->name);
		    }
		    corePointer = pInfo;
		}
	    }
	}
	if (!corePointer) {
	    xf86Msg(X_WARNING, "No core pointer registered\n");
	    /* XXX register a dummy core pointer */
	}
#ifdef NEW_KBD
	if (!coreKeyboard) {
	    xf86Msg(X_WARNING, "No core keyboard registered\n");
	    /* XXX register a dummy core keyboard */
	}
#endif
    }

    /* Initialise all input devices. */
    pInfo = xf86InputDevs;
    while (pInfo) {
	xf86ActivateDevice(pInfo);
	pInfo = pInfo->next;
    }

    if (coreKeyboard) {
	xf86Info.pKeyboard = coreKeyboard->dev;
	/* Clear kbdEvents to prevent internal keybord driver usage. */
	xf86Info.kbdEvents = NULL;
    } else {
	xf86Info.pKeyboard = AddInputDevice(xf86Info.kbdProc, TRUE);
    }
    if (corePointer)
	xf86Info.pMouse = corePointer->dev;
    RegisterKeyboardDevice(xf86Info.pKeyboard);

    miRegisterPointerDevice(screenInfo.screens[0], xf86Info.pMouse);
#ifdef XINPUT
    xf86eqInit((DevicePtr) xf86Info.pKeyboard, (DevicePtr) xf86Info.pMouse);
#else
    mieqInit((DevicePtr) xf86Info.pKeyboard, (DevicePtr) xf86Info.pMouse);
#endif
}

#ifndef SET_STDERR_NONBLOCKING
#define SET_STDERR_NONBLOCKING 1
#endif

/*
 * OsVendorPreInit --
 *      OS/Vendor-specific initialisations.  Called early in OsInit().
 */

void
OsVendorPreInit(void)
{
    static Bool beenHere = FALSE;

    if (!beenHere) {
#ifdef XFree86LOADER
	xf86WrapperInit();
#endif
	beenHere = TRUE;
    }
}

/*
 * OsVendorInit --
 *      OS/Vendor-specific initialisations.  Called from OsInit(), which
 *      is called by dix before establishing the well known sockets.
 */

void
OsVendorInit()
{
    static Bool beenHere = FALSE;

#ifdef SIGCHLD
    signal(SIGCHLD, SIG_DFL);	/* Need to wait for child processes */
#endif
    OsDelayInitColors = TRUE;
#ifdef XFree86LOADER
    loadableFonts = TRUE;
#endif

    if (!beenHere)
	xf86LogInit();

#if SET_STDERR_NONBLOCKING
    /* Set stderr to non-blocking. */
#ifndef O_NONBLOCK
#if defined(FNDELAY)
#define O_NONBLOCK FNDELAY
#elif defined(O_NDELAY)
#define O_NONBLOCK O_NDELAY
#endif
#endif

#ifdef O_NONBLOCK
    if (!beenHere) {
#if !defined(__EMX__)
	if (PRIVS_ELEVATED)
#endif
	{
	    int status;

	    status = fcntl(fileno(stderr), F_GETFL, 0);
	    if (status != -1) {
		fcntl(fileno(stderr), F_SETFL, status | O_NONBLOCK);
	    }
	}
    }
#endif
#endif

    beenHere = TRUE;
}

/*
 * ddxGiveUp --
 *      Device dependent cleanup. Called by by dix before normal server death.
 *      For SYSV386 we must switch the terminal back to normal mode. No error-
 *      checking here, since there should be restored as much as possible.
 */

void
ddxGiveUp()
{
    int i;

    if (xf86OSPMClose)
	xf86OSPMClose();
    xf86OSPMClose = NULL;

    xf86AccessLeaveState();

    for (i = 0; i < xf86NumScreens; i++) {
	/*
	 * zero all access functions to
	 * trap calls when switched away.
	 */
	xf86Screens[i]->vtSema = FALSE;
	xf86Screens[i]->access = NULL;
	xf86Screens[i]->busAccess = NULL;
    }

#ifdef USE_XF86_SERVERLOCK
    xf86UnlockServer();
#endif

    if (!noVT)
	xf86CloseConsole();

    xf86CloseLog();

    /* If an unexpected signal was caught, dump a core for debugging */
    if (xf86Info.caughtSignal) {
	abort();
    }
}

/*
 * AbortDDX --
 *      DDX - specific abort routine.  Called by AbortServer(). The attempt is
 *      made to restore all original setting of the displays. Also all devices
 *      are closed.
 */

void
AbortDDX()
{
    int i;

    /*
     * Try to deinitialize all input devices.
     */
    if (xf86Info.pKeyboard)
	xf86Info.kbdProc(xf86Info.pKeyboard, DEVICE_CLOSE);

    /*
     * Try to restore the original video state.
     */
#ifdef HAS_USL_VTS
    /* Need the sleep when starting X from within another X session. */
    if (!noVT)
	sleep(1);
#endif
    if (xf86Screens && !noVT) {
#ifdef XFreeXDGA
	DGAShutdown();
#endif
	if (xf86Screens[0]->vtSema)
	    xf86EnterServerState(SETUP);
	for (i = 0; i < xf86NumScreens; i++) {
	    if (xf86Screens[i]->vtSema) {
		/*
		 * If we are aborting before ScreenInit() has finished
		 * we might not have been wrapped yet. Therefore enable
		 * screen explicitly.
		 */
		xf86EnableAccess(xf86Screens[i]);
		xf86Screens[i]->LeaveVT(i, 0);
	    }
	}
    }

    xf86AccessLeave();

    /*
     * This is needed for an abnormal server exit, since the normal exit stuff
     * MUST also be performed (i.e. the vt must be left in a defined state).
     */
    ddxGiveUp();
}

void
OsVendorFatalError()
{
    ErrorF("\nWhen reporting a problem related to a server crash, "
	   "please send\n"
	   "the full server output, not just the last messages.\n");
    if (xf86FilePaths && xf86FilePaths->logFile && xf86LogFileWasOpened)
	ErrorF("This can be found in the log file \"%s\".\n",
	       xf86FilePaths->logFile);
    ErrorF("Please report problems to %s.\n", BUILDERADDR);
    ErrorF("\n");
}

int
xf86SetVerbosity(int verb)
{
    int save = xf86Verbose;

    xf86Verbose = verb;
    LogSetParameter(XLOG_VERBOSITY, verb);
    return save;
}

int
xf86SetLogVerbosity(int verb)
{
    int save = xf86LogVerbose;

    xf86LogVerbose = verb;
    LogSetParameter(XLOG_FILE_VERBOSITY, verb);
    return save;
}

/*
 * ddxProcessArgument --
 *	Process device-dependent command line args. Returns 0 if argument is
 *      not device dependent, otherwise Count of number of elements of argv
 *      that are part of a device dependent commandline option.
 *
 */

/* ARGSUSED */
int
ddxProcessArgument(int argc, const char **argv, int i)
{
    /* Make a copy of the command line to log later. */
    if (!cmdline) {
	int j, len;

	for (j = 0; j < argc; j++) {
	    if (cmdline) {
		len = strlen(argv[j]) + 1 + strlen(cmdline) + 1;
		cmdline = xrealloc(cmdline, len);
		if (!cmdline)
		    FatalError
			    ("Cannot allocate memory for the command line.\n");
		strlcat(cmdline, " ", len);
		strlcat(cmdline, argv[j], len);
	    } else {
		len = strlen(argv[j]) + 1;
		cmdline = xalloc(len);
		if (!cmdline)
		    FatalError
			    ("Cannot allocate memory for the command line.\n");
		strlcpy(cmdline, argv[j], len);
	    }
	}
    }

    /* First the options that are only allowed for root */
    if (!PRIVS_ELEVATED) {
	if (!strcmp(argv[i], "-modulepath")) {
	    if (!argv[++i])
		return 0;
	    xfree(xf86FileCmdline.modulePath);
	    xf86FileCmdline.modulePath = xstrdup(argv[i]);
	    if (!xf86FileCmdline.modulePath)
		FatalError
			("Cannot allocate memory for the module path name.\n");
	    xf86FileCmdline.modulePathFrom = X_CMDLINE;
	    return 2;
	} else if (!strcmp(argv[i], "-logfile")) {
	    if (!argv[++i])
		return 0;
	    xfree(xf86FileCmdline.logFile);
	    xf86FileCmdline.logFile = xstrdup(argv[i]);
	    if (!xf86FileCmdline.logFile)
		FatalError("Cannot allocate memory for the log file name.\n");
	    xf86FileCmdline.logFileFrom = X_CMDLINE;
	    return 2;
	}
    }
    if (!strcmp(argv[i], "-xf86config")) {
	if (!argv[++i])
	    return 0;
	if (PRIVS_ELEVATED && !xf86PathIsSafe(argv[i])) {
	    FatalError("\nInvalid argument for -xf86config\n"
		       "\tFor non-root users, the file specified with "
		       "-xf86config must be\n"
		       "\ta relative path and must not contain any \"..\" "
		       "elements.\n"
		       "\tUsing default XF86Config search path.\n\n");
	}
	xf86ConfigFile = argv[i];
	return 2;
    }
    if (!strcmp(argv[i], "-showunresolved")) {
	xf86ShowUnresolved = TRUE;
	return 1;
    }
    if (!strcmp(argv[i], "-autoconfig")) {
	autoconfig = TRUE;
	return 1;
    }
    if (!strcmp(argv[i], "-appendauto")) {
	appendauto = TRUE;
	return 1;
    }
    if (!strcmp(argv[i], "-noappendauto")) {
	noAppendauto = TRUE;
	return 1;
    }
    if (!strcmp(argv[i], "-probeonly")) {
	xf86ProbeOnly = TRUE;
	return 1;
    }
    if (!strcmp(argv[i], "-flipPixels")) {
	xf86FlipPixels = TRUE;
	return 1;
    }
#ifdef XF86VIDMODE
    if (!strcmp(argv[i], "-disableVidMode")) {
	xf86VidModeDisabled = TRUE;
	return 1;
    }
    if (!strcmp(argv[i], "-allowNonLocalXvidtune")) {
	xf86VidModeAllowNonLocal = TRUE;
	return 1;
    }
#endif
#ifdef XF86MISC
    if (!strcmp(argv[i], "-disableModInDev")) {
	xf86MiscModInDevDisabled = TRUE;
	return 1;
    }
    if (!strcmp(argv[i], "-allowNonLocalModInDev")) {
	xf86MiscModInDevAllowNonLocal = TRUE;
	return 1;
    }
#endif
    if (!strcmp(argv[i], "-allowMouseOpenFail")) {
	xf86AllowMouseOpenFail = TRUE;
	return 1;
    }
    if (!strcmp(argv[i], "-bestRefresh")) {
	xf86BestRefresh = TRUE;
	return 1;
    }
    if (!strcmp(argv[i], "-ignoreABI")) {
#ifdef XFree86LOADER
	LoaderSetOptions(LDR_OPT_ABI_MISMATCH_NONFATAL);
#endif
	return 1;
    }
#ifdef XFree86LOADER
#if LOADERTEST
    if (!strcmp(argv[i], "-loadertest")) {
	doLoaderTest = TRUE;
	return 1;
    }
#endif
    if (!strcmp(argv[i], "-loaderdebug")) {
	if (++i >= argc)
	    return 0;
	LoaderSetDebug(atoi(argv[i]));
	return 2;
    }
    if (!strcmp(argv[i], "-loaderdebugmod")) {
	if (++i >= argc)
	    return 0;
	LoaderDebugAddModule(argv[i]);
	return 2;
    }
#endif
    if (!strcmp(argv[i], "-verbose")) {
	if (++i < argc && argv[i]) {
	    char *end;
	    long val;

	    val = strtol(argv[i], &end, 0);
	    if (*end == '\0') {
		xf86SetVerbosity(val);
		return 2;
	    }
	}
	xf86SetVerbosity(++xf86Verbose);
	return 1;
    }
    if (!strcmp(argv[i], "-logverbose")) {
	if (++i < argc && argv[i]) {
	    char *end;
	    long val;

	    val = strtol(argv[i], &end, 0);
	    if (*end == '\0') {
		xf86SetLogVerbosity(val);
		return 2;
	    }
	}
	xf86SetLogVerbosity(++xf86LogVerbose);
	return 1;
    }
    if (!strcmp(argv[i], "-quiet")) {
	xf86SetVerbosity(0);
	return 1;
    }
    if (!strcmp(argv[i], "-showconfig") || !strcmp(argv[i], "-version")) {
	PrintBanner();
	exit(0);
    }
    /* Snoop the -fp flag, still allowing it to pass to the dix layer. */
    if (!strcmp(argv[i], "-fp")) {
	if (++i < argc && argv[i]) {
	    xf86FileCmdline.fontPath = strdup(argv[i]);
	    if (!xf86FileCmdline.fontPath)
		FatalError("Cannot allocate memory for the font path.\n");
	    xf86FileCmdline.fontPathFrom = X_CMDLINE;
	}
	return 0;
    }
    /* Snoop the -co flag, still allowing it to pass to the dix layer. */
    if (!strcmp(argv[i], "-co")) {
	if (++i < argc && argv[i]) {
	    xfree(xf86FileCmdline.rgbPath);
	    xf86FileCmdline.rgbPath = xstrdup(argv[i]);
	    if (!xf86FileCmdline.rgbPath)
		FatalError("Cannot allocate memory for RGBPath.\n");
	    xf86FileCmdline.rgbPathFrom = X_CMDLINE;
	}
	return 0;
    }
    /* Notice the -bs flag, but allow it to pass to the dix layer. */
    if (!strcmp(argv[i], "-bs")) {
	xf86bsDisableFlag = TRUE;
	return 0;
    }
    /* Notice the +bs flag, but allow it to pass to the dix layer. */
    if (!strcmp(argv[i], "+bs")) {
	xf86bsEnableFlag = TRUE;
	return 0;
    }
    /* Notice the -s flag, but allow it to pass to the dix layer. */
    if (!strcmp(argv[i], "-s")) {
	xf86sFlag = TRUE;
	return 0;
    }
    if (!strcmp(argv[i], "-bpp")) {
	if (++i >= argc)
	    return 0;
	ErrorF("The -bpp option is no longer supported.\n"
	       "\tUse -depth to set the color depth, and use -fbbpp if "
	       "you really\n"
	       "\tneed to force a non-default framebuffer (hardware) "
	       "pixel format.\n");
	return 2;
    }
    if (!strcmp(argv[i], "-pixmap24")) {
	xf86Pix24 = Pix24Use24;
	return 1;
    }
    if (!strcmp(argv[i], "-pixmap32")) {
	xf86Pix24 = Pix24Use32;
	return 1;
    }
    if (!strcmp(argv[i], "-fbbpp")) {
	int bpp;

	if (++i >= argc)
	    return 0;
	if (sscanf(argv[i], "%d", &bpp) == 1) {
	    xf86FbBpp = bpp;
	    return 2;
	} else {
	    ErrorF("Invalid fbbpp\n");
	    return 0;
	}
    }
    if (!strcmp(argv[i], "-depth")) {
	int depth;

	if (++i >= argc)
	    return 0;
	if (sscanf(argv[i], "%d", &depth) == 1) {
	    xf86Depth = depth;
	    return 2;
	} else {
	    ErrorF("Invalid depth\n");
	    return 0;
	}
    }
    if (!strcmp(argv[i], "-weight")) {
	int red, green, blue;

	if (++i >= argc)
	    return 0;
	if (sscanf(argv[i], "%1d%1d%1d", &red, &green, &blue) == 3) {
	    xf86Weight.red = red;
	    xf86Weight.green = green;
	    xf86Weight.blue = blue;
	    return 2;
	} else {
	    ErrorF("Invalid weighting\n");
	    return 0;
	}
    }
    if (!strcmp(argv[i], "-gamma") || !strcmp(argv[i], "-rgamma") ||
	!strcmp(argv[i], "-ggamma") || !strcmp(argv[i], "-bgamma")) {
	double gamma;

	if (++i >= argc)
	    return 0;
	if (sscanf(argv[i], "%lf", &gamma) == 1) {
	    if (gamma < GAMMA_MIN || gamma > GAMMA_MAX) {
		ErrorF("gamma out of range, only  %.2f <= gamma_value <= %.1f"
		       " is valid\n", GAMMA_MIN, GAMMA_MAX);
		return 0;
	    }
	    if (!strcmp(argv[i - 1], "-gamma"))
		xf86Gamma.red = xf86Gamma.green = xf86Gamma.blue = gamma;
	    else if (!strcmp(argv[i - 1], "-rgamma"))
		xf86Gamma.red = gamma;
	    else if (!strcmp(argv[i - 1], "-ggamma"))
		xf86Gamma.green = gamma;
	    else if (!strcmp(argv[i - 1], "-bgamma"))
		xf86Gamma.blue = gamma;
	    return 2;
	}
    }
    if (!strcmp(argv[i], "-layout")) {
	if (++i >= argc)
	    return 0;
	xf86LayoutName = argv[i];
	return 2;
    }
    if (!strcmp(argv[i], "-screen")) {
	if (++i >= argc)
	    return 0;
	xf86ScreenName = argv[i];
	return 2;
    }
    if (!strcmp(argv[i], "-pointer")) {
	if (++i >= argc)
	    return 0;
	xf86PointerName = argv[i];
	return 2;
    }
    if (!strcmp(argv[i], "-keyboard")) {
	if (++i >= argc)
	    return 0;
	xf86KeyboardName = argv[i];
	return 2;
    }
    if (!strcmp(argv[i], "-nosilk")) {
	xf86silkenMouseDisableFlag = TRUE;
	return 1;
    }
    if (!strcmp(argv[i], "-scanpci")) {
	DoScanPci(argc, argv, i);
    }
    if (!strcmp(argv[i], "-nohw")) {
	noHardware = TRUE;
	return 1;
    }
    if (!strcmp(argv[i], "-novt")) {
	noVT = TRUE;
	return 1;
    }
    if (!strcmp(argv[i], "-probe")) {
	xf86DoProbe = TRUE;
	return 1;
    }
    if (!strcmp(argv[i], "-configure")) {
	if (PRIVS_ELEVATED) {
	    ErrorF("The '-configure' option can only be used by root.\n");
	    exit(1);
	}
	xf86DoConfigure = TRUE;
	xf86AllowMouseOpenFail = TRUE;
	return 1;
    }
    /* OS-specific processing */
    return xf86ProcessArgument(argc, argv, i);
}

/*
 * ddxUseMsg --
 *	Print out correct use of device dependent commandline options.
 *      Maybe the user now knows what really to do ...
 */

void
ddxUseMsg()
{
    ErrorF("\n");
    ErrorF("\n");
    ErrorF("Device Dependent Usage\n");
    if (!PRIVS_ELEVATED) {
	ErrorF("-xf86config file       specify a configuration file\n");
	ErrorF("-modulepath paths      specify the module search path\n");
	ErrorF("-logfile file          specify a log file name\n");
	ErrorF("-configure             probe for devices and write an "
				       "XF86Config\n");
    } else {
	ErrorF("-xf86config file       specify a configuration file, "
				       "relative to the\n");
	ErrorF("                       XF86Config search path, only root "
				       "can use absolute\n");
    }
    ErrorF("-autoconfig            automatic configuration, even when a "
				   "config file exits\n");
    ErrorF("-appendauto            append automatic config to existing "
				   "config file\n");
    ErrorF("-noappendauto          do not append automatic config\n");
    ErrorF("-probeonly             probe for devices, then exit\n");
    ErrorF("-scanpci               execute the scanpci module and exit\n");
    ErrorF("-nohw                  disable video hardware and hardware "
				   "probing\n");
    ErrorF("-novt                  disable console/VT use\n");
    ErrorF("-verbose [n]           verbose startup messages\n");
    ErrorF("-logverbose [n]        verbose log messages\n");
    ErrorF("-quiet                 minimal startup messages\n");
    ErrorF("-pixmap24              use 24bpp pixmaps for depth 24\n");
    ErrorF("-pixmap32              use 32bpp pixmaps for depth 24\n");
    ErrorF("-fbbpp n               set bpp for the framebuffer. Default: 8\n");
    ErrorF("-depth n               set colour depth. Default: 8\n");
    ErrorF("-gamma f               set gamma value (0.1 < f < 10.0) "
				   "Default: 1.0\n");
    ErrorF("-rgamma f              set gamma value for red phase\n");
    ErrorF("-ggamma f              set gamma value for green phase\n");
    ErrorF("-bgamma f              set gamma value for blue phase\n");
    ErrorF("-weight nnn            set RGB weighting at 16 bpp.  "
				   "Default: 565\n");
    ErrorF("-layout name           specify the ServerLayout section name\n");
    ErrorF("-screen name           specify the Screen section name\n");
    ErrorF("-keyboard name         specify the core keyboard InputDevice "
				   "name\n");
    ErrorF("-pointer name          specify the core pointer InputDevice "
				   "name\n");
    ErrorF("-nosilk                disable Silken Mouse\n");
    ErrorF("-flipPixels            swap default black/white Pixel values\n");
#ifdef XF86VIDMODE
    ErrorF("-disableVidMode        disable mode adjustments with xvidtune\n");
    ErrorF("-allowNonLocalXvidtune allow xvidtune to be run as a non-local "
				   "client\n");
#endif
#ifdef XF86MISC
    ErrorF("-disableModInDev       disable dynamic modification of input "
				   "device settings\n");
    ErrorF("-allowNonLocalModInDev allow changes to keyboard and mouse "
				   "settings\n");
    ErrorF("                       from non-local clients\n");
    ErrorF("-allowMouseOpenFail    start server even if the mouse can't be "
				   "initialized\n");
#endif
    ErrorF("-bestRefresh           choose modes with the best refresh rate\n");
    ErrorF("-ignoreABI             make module ABI mismatches non-fatal\n");
    ErrorF("-version               show the server version\n");
    /* OS-specific usage */
    xf86UseMsg();
    ErrorF("\n");
}

#ifndef OSNAME
#define OSNAME " unknown"
#endif
#ifndef OSVENDOR
#define OSVENDOR ""
#endif
#ifndef PRE_RELEASE
#define PRE_RELEASE XF86_VERSION_SNAP
#endif

static void
PrintBanner()
{
#if defined(XF86_CUSTOM_BANNER)
    ErrorF("%s", XF86_CUSTOM_BANNER);
#else
#if PRE_RELEASE
    ErrorF("\n"
     "This is a pre-release version of XFree86, and is not supported in any\n"
     "way.  Bugs may be reported to XFree86@XFree86.Org and patches submitted\n"
     "to fixes@XFree86.Org.  Before reporting bugs in pre-release versions,\n"
     "please check the latest version in the XFree86 CVS repository\n"
     "(http://www.XFree86.Org/cvs).\n");
#endif
#endif
    ErrorF("\nXFree86 Version %d.%d.%d", XF86_VERSION_MAJOR,
	   XF86_VERSION_MINOR, XF86_VERSION_PATCH);
#if XF86_VERSION_SNAP > 0
    ErrorF(".%d", XF86_VERSION_SNAP);
#endif

#if XF86_VERSION_SNAP >= 900
    ErrorF(" (%d.%d.0 RC %d)", XF86_VERSION_MAJOR, XF86_VERSION_MINOR + 1,
	   XF86_VERSION_SNAP - 900);
#endif

#ifdef XF86_CUSTOM_VERSION
    ErrorF(" (%s)", XF86_CUSTOM_VERSION);
#endif
    ErrorF("\nRelease Date: %s\n", XF86_DATE);
    ErrorF("X Protocol Version %d, Revision %d\n",
	   X_PROTOCOL, X_PROTOCOL_REVISION);
    ErrorF("Build Operating System:%s%s\n", OSNAME, OSVENDOR);
#ifdef HAS_UTSNAME
    {
	struct utsname name;

	if (uname(&name) >= 0) {
	    ErrorF("Current Operating System: %s %s %s %s %s\n",
		   name.sysname, name.nodename, name.release, name.version,
		   name.machine);
	}
    }
#endif
#if defined(BUILD_DATE) && (BUILD_DATE > 19000000)
    {
	struct tm t;
	char buf[100];

	bzero(&t, sizeof(t));
	bzero(buf, sizeof(buf));
	t.tm_mday = BUILD_DATE % 100;
	t.tm_mon = (BUILD_DATE / 100) % 100 - 1;
	t.tm_year = BUILD_DATE / 10000 - 1900;
	if (strftime(buf, sizeof(buf), "%d %B %Y", &t))
	    ErrorF("Build Date: %s\n", buf[0] == '0' ? buf + 1 : buf);
    }
#endif
#if defined(CLOG_DATE) && (CLOG_DATE > 19000000)
    {
	struct tm t;
	char buf[100];

	bzero(&t, sizeof(t));
	bzero(buf, sizeof(buf));
	t.tm_mday = CLOG_DATE % 100;
	t.tm_mon = (CLOG_DATE / 100) % 100 - 1;
	t.tm_year = CLOG_DATE / 10000 - 1900;
	if (strftime(buf, sizeof(buf), "%d %B %Y", &t))
	    ErrorF("Changelog Date: %s\n", buf[0] == '0' ? buf + 1 : buf);
    }
#endif
#if defined(BUILDERSTRING)
    ErrorF("%s\n", BUILDERSTRING);
#endif
#if defined(XF86_PROBLEM_STRING)
    ErrorF("%s", XF86_PROBLEM_STRING);
#else
    ErrorF("\tBefore reporting problems, check http://www.XFree86.Org/\n"
	   "\tto make sure that you have the latest version.\n");
#endif
#ifdef XFree86LOADER
    ErrorF("Module Loader present\n");
#endif
    if (cmdline)
	ErrorF("Command line: %s\n", cmdline);
}

static void
PrintMarkers()
{
    LogPrintMarkers();
}

static void
RunVtInit(void)
{
    int i;

    /*
     * If VTInit was set, run that program with consoleFd as stdin and stdout
     */

    if (xf86Info.vtinit) {
	switch (fork()) {
	case -1:
	    FatalError("RunVtInit: fork failed (%s)\n", strerror(errno));
	    break;
	case 0:		/* child */
	    setuid(getuid());
	    /* set stdin, stdout to the consoleFd */
	    for (i = 0; i < 2; i++) {
		if (xf86Info.consoleFd != i) {
		    close(i);
		    dup(xf86Info.consoleFd);
		}
	    }
	    execl("/bin/sh", "sh", "-c", xf86Info.vtinit, (void *)NULL);
	    xf86Msg(X_WARNING, "exec of /bin/sh failed for VTInit (%s)\n",
		    strerror(errno));
	    exit(255);
	    break;
	default:		/* parent */
	    wait(NULL);
	}
    }
}

#ifdef XFree86LOADER
/*
 * xf86LoadModules iterates over a list that is being passed in.
 */
Bool
xf86LoadModules(const char **list, pointer * optlist)
{
    int errmaj, errmin;
    pointer opt;
    int i;
    char *name;
    Bool failed = FALSE;

    if (!list)
	return TRUE;

    for (i = 0; list[i] != NULL; i++) {

#ifndef NORMALISE_MODULE_NAME
	name = xstrdup(list[i]);
#else
	/* Normalise the module name */
	name = xf86NormalizeName(list[i]);
#endif

	/* Skip empty names */
	if (name == NULL || *name == '\0')
	    continue;

	if (optlist)
	    opt = optlist[i];
	else
	    opt = NULL;

	if (!LoadModule(name, NULL, NULL, NULL, opt, NULL, &errmaj, &errmin)) {
	    LoaderErrorMsg(NULL, name, errmaj, errmin);
	    failed = TRUE;
	}
	xfree(name);
    }
    return !failed;
}

#endif

/* Pixmap format stuff */

PixmapFormatPtr
xf86GetPixFormat(ScrnInfoPtr pScrn, int depth)
{
    int i;
    static PixmapFormatRec format;	/* XXX not reentrant */

    /*
     * When the formats[] list initialisation isn't complete, check the
     * depth 24 pixmap config/cmdline options and screen-specified formats.
     */

    if (!formatsDone) {
	if (depth == 24) {
	    Pix24Flags pix24 = Pix24DontCare;

	    format.depth = 24;
	    format.scanlinePad = BITMAP_SCANLINE_PAD;
	    if (xf86Info.pixmap24 != Pix24DontCare)
		pix24 = xf86Info.pixmap24;
	    else if (pScrn->pixmap24 != Pix24DontCare)
		pix24 = pScrn->pixmap24;
	    if (pix24 == Pix24Use24)
		format.bitsPerPixel = 24;
	    else
		format.bitsPerPixel = 32;
	    return &format;
	}
    }

    for (i = 0; i < numFormats; i++)
	if (formats[i].depth == depth)
	    break;
    if (i != numFormats)
	return &formats[i];
    else if (!formatsDone) {
	/* Check for screen-specified formats */
	for (i = 0; i < pScrn->numFormats; i++)
	    if (pScrn->formats[i].depth == depth)
		break;
	if (i != pScrn->numFormats)
	    return &pScrn->formats[i];
    }
    return NULL;
}

int
xf86GetBppFromDepth(ScrnInfoPtr pScrn, int depth)
{
    PixmapFormatPtr format;

    format = xf86GetPixFormat(pScrn, depth);
    if (format)
	return format->bitsPerPixel;
    else
	return 0;
}
