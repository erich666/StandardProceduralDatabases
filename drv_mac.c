/*==============================================================================
Project:	SPD

File:	drv_mac.c

Description:
Mac-specific code for initialization & displaying command-line options & output window.
------------------------------------------------------------------------------
Author:
	Alexander Enzmann, [70323,2461]
	Eduard Schwan, [espsw@compuserve.com]
------------------------------------------------------------------------------
Change History:
	9212??	[ae]	Created.
	921214	[esp]	Updated to compile under MPW C as well as Think C.
	930117	[esp]	Added MacInit, MacMultiTask, MacShutDown, GetOptions code
	930317	[esp]	Added MoveToMaxDevice, environs check, collapsed multiple
					dialogs to 1, changed display_init to accept bg_color
	930418	[esp]	Added driver checks.
	930803	[esp]	Changed argv parameters to match new 3.1a format.
	930805	[esp]	Added radios for the -t/-c output_format switches.
	930817	[esp]	Added ColorQD check in Display_Init.
	930918	[esp]	Fixed up logic for input parm prompting.
	930918	[esp]	Added grody quick fix to redraw please-wait dlg on switchin.
	940725	[esp]	Removed special palette, use System palette to avoid Finder color snatching
	950422	[esp]	Changed popup menu handling to simpler System 7-only control style
	970421	[esp]	Updated to CW11 Universal interfaces
	980102	[esp]	Fixed argv/argc bug which was stomping on memory, updated to CWC Pro 2
==============================================================================*/

#include <stdio.h>		/* sprintf, etc */
#include <stdlib.h>		/* atoi, etc */

/*==== Macintosh-specific headers ====*/
#include <Controls.h>
#include <Dialogs.h>
#include <Files.h>
// Note: Memory.h & Windows.h have been renamed by Apple in Univ. Headers 3!
#include <MacMemory.h>			/* BlockMove */
#include <MacWindows.h>
#include <Menus.h>
#include <Events.h>
#include <OSUtils.h>
#include <Packages.h>
#include <QuickDraw.h>
#include <Palettes.h>
#include <Resources.h>
#include <Types.h>
#include <sound.h>
#include <AppleEvents.h>
#include <Gestalt.h>
#include <Folders.h>
#include <errors.h>			/* dupFNErr, etc */
#include <Fonts.h>			/* checkMark */
#include <segload.h>		/* UnloadSeg */
#include <traps.h>			/* _Unimplemented */
#include <StandardFile.h>	/* SFPutFile */
#include <string.h>			/* strcpy/cat */
#include <toolutils.h>		/* BitTst, GetCursor, etc */

#ifdef applec
#include <strings.h>		/* p2cstr */
#endif

#include "def.h"
#include "drv.h"
#include "lib.h"		/* for output types */


/*------------------------------------------------------------*/
#define	kSuspendResumeMessage	1		/* high byte of suspend/resume event message */

#define kMouseCursorID 1000	// mouse cursor resource ID

/* turn on to attach special palette */
/* #define	USE_CUSTOM_PALETTE		1


/*------------------------------------------------------------*/
// constants for positioning the default popup item within its box
#define	SLOP_LEFT	13		// leave this much space on left of title
#define	SLOP_RIGHT	12		// this much on right
#define	SLOP_BOTTOM	 5		// this much below baseline

// general dialog constants
#define	DlogID_BADENV		1000
#define	DlogID_WAIT			1001
#define	DlogID_NOCOLORQD	1002
#define	DlogID_GETOPTS		2000

// # of lines or pixels to draw between multitask calls
#define	MAX_TASK_WAIT	20



/*------------------------------------------------------------*/
#define kDEFAULT_MAC_RT		OUTPUT_POVRAY_30
short		gMacRayTracerKind = kDEFAULT_MAC_RT;
short		gMacParmSize = 2;
Boolean		gMacDoBuiltIn = true;	/* TRUE= OUTPUT_CURVES, FALSE = OUTPUT_PATCHES */
char		gInFileName[64];

/*------------------------------------------------------------*/
typedef struct SPDFileInfoRec
{
	OSType		fdType;
	OSType		fdCreator;
} SPDFileInfoRec;

#define	kDefaultCreator		'CWIE'
#define	k3DMFFileType		'3DMF'

static SPDFileInfoRec	gFileInfo[] =
{
	{0L, 0L},						// OUTPUT_VIDEO
	{'TEXT', kDefaultCreator},		// OUTPUT_NFF
	{'TEXT', kDefaultCreator},		// OUTPUT_POVRAY_10
	{'TEXT', 'PvRy'},				// OUTPUT_POVRAY_20
	{'TEXT', 'POV3'},				// OUTPUT_POVRAY_30
	{'TEXT', kDefaultCreator},		// OUTPUT_POLYRAY
	{'TEXT', kDefaultCreator},		// OUTPUT_VIVID
	{'TEXT', kDefaultCreator},		// OUTPUT_QRT
	{'TEXT', kDefaultCreator},		// OUTPUT_RAYSHADE
	{'TEXT', kDefaultCreator},		// OUTPUT_RTRACE
	{'TEXT', kDefaultCreator},		// OUTPUT_PLG
	{'TEXT', kDefaultCreator},		// OUTPUT_RAWTRI
	{'TEXT', kDefaultCreator},		// OUTPUT_ART
	{'TEXT', kDefaultCreator},		// OUTPUT_RIB
	{'TEXT', kDefaultCreator},		// OUTPUT_DXF
	{'TEXT', kDefaultCreator},		// OUTPUT_OBJ
	{'TEXT', kDefaultCreator},		// OUTPUT_RWX
	{k3DMFFileType, 'ttxt'},		// OUTPUT_3DMF
	{'TEXT', kDefaultCreator},		// OUTPUT_VRML2
	{'TEXT', kDefaultCreator}		// OUTPUT_DELAYED
};

/*------------------------------------------------------------*/
static WindowPtr	myWindow;

#if defined(USE_CUSTOM_PALETTE)
static PaletteHandle PolyPalette;
#endif

static int			maxx = 460;
static int			maxy = 300;
static int			gMultiTaskCount = 0;

static Boolean		gHasSizePrompt;
static Boolean		gHasInFilePrompt;
static Boolean		gHasPatchPrompt;

static COORD3		bkgnd_color;
static int			display_active_flag = 0;
static double		X_Display_Scale = 1.0;
static double		Y_Display_Scale = 1.0;
static SysEnvRec	gSysEnvirons;
static DialogPtr	gDialogPtr = NULL;

static char			*macArgv[] =	{NULL, NULL, NULL, NULL, NULL,
									NULL, NULL, NULL, NULL, NULL};
static char			macArgvBuffer[200];		// fake argc/argv
static char			*argvCurrent;


/*------------------------------------------------------------*/
void MacMultiTask();


/*------------------------------------------------------------*/
#pragma segment main


/*------------------------------------------------------------*/
/* Returns the short version string in the application's version resource */
static void GetAppVersionPString(short versID, Str31 versionString)
{
    VersRecHndl	versHandle;		// VersRecHndl declared in MPW's <files.h>

	/* Get the resource string from app, 'vers' versID (1 or 2) resource! */
	versionString[0] = '\0';
	versHandle = (VersRecHndl)GetResource('vers',versID);
	if (versHandle)
	{
		HLock((Handle)versHandle);
		BlockMove((**versHandle).shortVersion, versionString,
					(**versHandle).shortVersion[0]+1);
		ReleaseResource((Handle)versHandle);
	}
} // GetAppVersionPString


/*------------------------------------------------------------*/
static void MoveWindowToMaxDevice(WindowPtr theWindow)
{
	Point		upperCorner;
	Rect		mainRect;
	Rect		deviceRect;
	Rect		windRect;
	Rect		maxDragBounds;
	GDHandle	theMainGDevice, theMaxGDevice;

	if (theWindow == NULL)
		return;

	// Set up bounds for all devices
	SetRect(&maxDragBounds, -16000, -16000, 16000, 16000);

	// Find main screen bounds
	theMainGDevice = GetMainDevice();
	mainRect = (**theMainGDevice).gdRect;

	// Find deepest screen bounds
	theMaxGDevice = GetMaxDevice(&maxDragBounds);
	deviceRect = (**theMaxGDevice).gdRect;

	// if Max is the same as Main, we need do nothing! Already in place!
	if (EqualRect(&mainRect, &deviceRect))
		return;

	// where's the window, relative to main screen
	windRect = theWindow->portRect;
	// yah, but where is it really?
	SetPort(theWindow);
	LocalToGlobal((Point*)&windRect.top);
	LocalToGlobal((Point*)&windRect.bottom);

	// find relative spot on new device
	upperCorner.h = windRect.left;
	upperCorner.v = windRect.top;
	// now relative to new screen
	upperCorner.h = upperCorner.h + deviceRect.left;
	upperCorner.v = upperCorner.v + deviceRect.top;

	// now move it there
	MoveWindow(theWindow, upperCorner.h, upperCorner.v, true);

} // MoveWindowToMaxDevice



/*------------------------------------------------------------*/
// This is a user item proc for drawing default dialog button outlines */
static pascal void outlineDefaultButton(DialogPtr theDialog, short theItem)
{
#pragma unused (theItem)
	PenState	SavePen;
	GrafPtr		oldPort;
	short		itemType;
	Handle		itemHandle;
	Rect		dispRect;

	/* remember original port, and set to dialog port */
	GetPort(&oldPort);
	SetPort(theDialog);

	GetPenState(&SavePen);
	PenNormal();

	/* use 'ok' (#1) item's rectangle */
	GetDialogItem(theDialog, ok, &itemType, &itemHandle, &dispRect);
	InsetRect(&dispRect, -4, -4);
	if ((**(ControlHandle)itemHandle).contrlHilite != 0)
	{	/* draw gray outline */
		PenPat(&qd.gray);
	}
	else
	{	/* draw solid outline */
		PenPat(&qd.black);
	}
	/* draw outline */
	PenSize(3, 3);
	FrameRoundRect(&dispRect, 16, 16);
	/* restore */
	SetPenState(&SavePen);
	SetPort(oldPort);
} // outlineDefaultButton


/*------------------------------------------------------------*/
/* Sets dialog #3 item's display proc to draw outline around item #1 */
static void SetupDefaultButton(DialogPtr theDialog)
{
    short	itemtype;
    Rect	itemrect;
    Handle	tempHandle;
	UserItemUPP	drawProcUPP;

	/* Set up User item (always #3) to display a border around OK button (#1) */
	drawProcUPP = NewUserItemProc((ProcPtr)outlineDefaultButton);
	GetDialogItem(theDialog, 3, &itemtype, &tempHandle, &itemrect);
    SetDialogItem(theDialog, 3, itemtype, (Handle)&outlineDefaultButton, &itemrect);
} // SetupDefaultButton


/*------------------------------------------------------------*/
// Move a dialog item (whatever type it is) to absolute position h,v
static void MoveDItem(DialogPtr theDialog, short theItemID, short h, short v)
{
    short	itemtype;
    Rect	itemrect;
    Handle	tempHandle;

	// NOTE: We should check for CtrlItem here and call MoveControl instead, just lazy!
	// get item
	GetDialogItem(theDialog, theItemID, &itemtype, &tempHandle, &itemrect);
	// move its view rect (to absolute pos)
	OffsetRect(&itemrect, h-itemrect.left, v-itemrect.top);
	// set new rect value back into item
    SetDialogItem(theDialog, theItemID, itemtype, tempHandle, &itemrect);
} // MoveDItem


/*------------------------------------------------------------*/
static DialogPtr SetupNewDialog(short theDialogID, Boolean doOutlineDefault)
{
	DialogPtr	aDialog;

	aDialog = GetNewDialog(theDialogID, NULL, (WindowPtr)-1);

	/* "default" the OK button */
	if ((aDialog != NULL) && doOutlineDefault)
		SetupDefaultButton(aDialog);

	return aDialog;

} // SetupNewDialog


/*------------------------------------------------------------*/
static void FatalErrDialog(short DlogID)
{
	short		itemHit;

	SysBeep(4);
	gDialogPtr = SetupNewDialog(DlogID, true);
	if (gDialogPtr)
	{
		ShowWindow(gDialogPtr);
		ModalDialog(NULL, &itemHit);
	}
	else
	{
		/* two beeps means something is very wrong! */
		SysBeep(4);
	}
	ExitToShell();
} // FatalErrDialog


/*------------------------------------------------------------*/
static Boolean GetInputFile(char * theInFileName)
{
	SFReply		reply;
	Point		where;
	SFTypeList	theTypes;

	where.h = 30;
	where.v = 50;

	/* prompt */
	theTypes[0] = 'TEXT';
	SFGetFile(where, "\p", (FileFilterProcPtr)NULL, 1, theTypes, (DlgHookProcPtr)NULL, &reply);
	if (reply.good)
	{
		/* return it */
		BlockMove(reply.fName, theInFileName, 1+reply.fName[0]);
		p2cstr((StringPtr)theInFileName);
		SetVol(NULL, reply.vRefNum); /* set current folder */
		return true;
	}
	else
		return false;
} // GetInputFile


/*------------------------------------------------------------*/
#ifdef FUTURE_USE
static Boolean GetOutputFile(char * theOutFileName)
{
	SFReply		reply;
	Point		where;
	char		pOutFileName[64];

	GetBestDlgPoint(&where);
	/* pre-fill output filename with old one */
	strcpy(pOutFileName, theOutFileName);
	c2pstr(pOutFileName);
	/* prompt */
	SFPutFile(where, "\pCreate output file.", pOutFileName,
				(DlgHookProcPtr)NULL, &reply);
	if (reply.good)
	{
		/* return it */
		BlockMove(reply.fName, theOutFileName, 1+reply.fName[0]);
		p2cstr(theOutFileName);
		SetVol(NULL, reply.vRefNum); /* set current folder */
		return true;
	}
	else
		return false;
} // GetOutputFile

#endif FUTURE_USE


/*------------------------------------------------------------*/
static void
ToolBoxInit()
{
	int			k;
	EventRecord	anEvent;

	InitGraf(&qd.thePort);
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(0L);
	InitCursor();

	/* create master pointer blocks for heap o' mallocs */
	for (k=0; k<8; k++)
		MoreMasters();

	// DTS Hocus Pocus to bring our app to the front
	for (k = 1; k <= 3; k++)
		EventAvail(everyEvent, &anEvent);

	// what kind of environment are we in, anyway?
	SysEnvirons(1, &gSysEnvirons);
	if	(
		(gSysEnvirons.machineType < envMachUnknown)
	||	(gSysEnvirons.systemVersion < 0x0700)
	||	(gSysEnvirons.processor < env68020)
		)
	{
	    FatalErrDialog(DlogID_BADENV);
	}
}


/*------------------------------------------------------------*/
#if defined(USE_CUSTOM_PALETTE)
static int
determine_color_index(color)
   COORD3 color;
{
   int i;
   unsigned char r, g, b;

   i = 255.0 * color[Z];
   if (i<0) i=0;
   else if (i>=256) i = 255;
   b = (unsigned char)i;

   i = 255.0 * color[Y];
   if (i<0) i=0;
   else if (i>=256) i = 255;
   g = (unsigned char)i;

   i = 255.0 * color[X];
   if (i<0) i=0;
   else if (i>=256) i = 255;
   r = (unsigned char)i;

   return (r & 0xE0) | ((g & 0xE0) >> 3) | (b >> 6);
} // determine_color_index
#endif


/*------------------------------------------------------------*/
static void Coord3ToRGBColor(COORD3 c3color, RGBColor	* rgbc)
{
   rgbc->red    = c3color[R_COLOR]*65535.0;
   rgbc->green  = c3color[G_COLOR]*65535.0;
   rgbc->blue   = c3color[B_COLOR]*65535.0;
} // Coord3ToRGBColor


/*------------------------------------------------------------*/
void display_clear()
{
   Rect re;

#if defined(USE_CUSTOM_PALETTE)
   PmForeColor(determine_color_index(bkgnd_color));
#else
   RGBColor	rgbc;

   Coord3ToRGBColor(bkgnd_color, &rgbc);
   RGBForeColor(&rgbc);
#endif
   re.top = 0;
   re.left = 0;
   re.right = maxx;
   re.bottom = maxy;
   PaintRect(&re);
}


/*------------------------------------------------------------*/
void
display_init(xres, yres, bk_color)
   int xres, yres;
   COORD3 bk_color;
{
//   RGBColor c;
//   int i;
//   int r, g, b;
   Rect re;
   COORD3 cColor;

	// die if on a machine that has no Color QD
   if (!gSysEnvirons.hasColorQD)
   {
	    FatalErrDialog(DlogID_NOCOLORQD);
   }

   // remember the background color
   COPY_COORD3(bkgnd_color, bk_color);

   // if already displaying, just clear the screen and return
   if (display_active_flag) {
      display_clear();
      return;
      }

   display_active_flag = 1;

   re.top = 24;
   re.left = 4;
   re.right = re.left + maxx;
   re.bottom = re.top + maxy;
   myWindow = NewCWindow(0L, &re, "\pObject Display",
			FALSE, plainDBox,
			(WindowPtr)-1L, FALSE, 0L);

#if defined(USE_CUSTOM_PALETTE)
   PolyPalette = NewPalette(256, 0L, pmTolerant, 0x0000);
   c.red = c.green = c.blue = 0;
   SetEntryColor(PolyPalette, 0, &c);

   i = 0;
   for (r=0;r<8;r++)
      for (g=0;g<8;g++)
	 for (b=0;b<4;b++) {
	    c.red   = r << 13;
	    c.green = g << 13;
	    c.blue  = b << 14;
	    SetEntryColor(PolyPalette, i, &c);
		i++;
	    }

   /* Make sure the last entry is true white */
   c.red   = 0xFFFF;
   c.green = 0xFFFF;
   c.blue  = 0xFFFF;
   SetEntryColor(PolyPalette, i-1, &c);
   SetEntryColor(PolyPalette, 0xFF, &c);

   SetPalette(myWindow, PolyPalette, TRUE);
#endif

   /* Now open the window. */
   MoveWindowToMaxDevice(myWindow);
   ShowWindow(myWindow);
   SetPort(myWindow);

   display_clear();

   if (xres > maxx)
      X_Display_Scale = (double)maxx / (double)xres;
   else
      X_Display_Scale = 1.0;
   if (yres > maxy)
      Y_Display_Scale = (double)maxy / (double)yres;
   else
      Y_Display_Scale = 1.0;

   if (X_Display_Scale < Y_Display_Scale)
      Y_Display_Scale = X_Display_Scale;
   else if (Y_Display_Scale < X_Display_Scale)
      X_Display_Scale = Y_Display_Scale;

   /* Outline the actual "visible" display area in the window */
   SET_COORD3(cColor, 0.5, 0.5, 0.5); // grey
   if (X_Display_Scale == 1.0) {
      display_line(-xres/2, -yres/2, -xres/2, yres/2, cColor);
      display_line( xres/2, -yres/2,  xres/2, yres/2, cColor);
      }
   if (Y_Display_Scale == 1.0) {
      display_line(-xres/2,  yres/2,  xres/2,  yres/2, cColor);
      display_line(-xres/2, -yres/2,  xres/2, -yres/2, cColor);
      }

   return;
}

/*------------------------------------------------------------*/
void
display_close(wait_flag)
   int wait_flag;
{
	EventRecord	anEvent;
	CursHandle	mouseCursorH;

	InitCursor(); // set it back to arrow
	if (gDialogPtr)
		DisposeDialog(gDialogPtr);

	mouseCursorH = GetCursor(kMouseCursorID);
	if (mouseCursorH)
		SetCursor(*mouseCursorH);

	/* hang around until the user is finished (this is grody, but works) */
	if (wait_flag)
	{
		SysBeep(4);
		while (!Button())
			EventAvail(everyEvent, &anEvent);
	}

	/* when all done, close the window on exit */
	if (myWindow)
		CloseWindow(myWindow);

	InitCursor(); // set it back to arrow
}

/*------------------------------------------------------------*/
static void
putpixel(x, y, color)
   int x, y;
   COORD3 color;
{
   Rect r;
#if defined(USE_CUSTOM_PALETTE)
   int color_index;
#else
   RGBColor	rgbc;
#endif

   r.top = y;
   r.left = x;
   r.bottom = y+1;
   r.right = x+1;
#if defined(USE_CUSTOM_PALETTE)
   color_index = determine_color_index(color);
   PmForeColor(color_index);
#else
   Coord3ToRGBColor(color, &rgbc);
   RGBForeColor(&rgbc);
#endif
   PaintRect(&r);
   return;
}

/*------------------------------------------------------------*/
void
display_plot(x, y, color)
   int x, y;
   COORD3 color;
{
   double xt, yt;

   MacMultiTask();
   yt = maxy/2 - Y_Display_Scale * y;
   xt = maxx/2 + X_Display_Scale * x;

   if (xt < 0.0) x = 0;
   else if (xt > 320.0) x = 320;
   else x = (int)xt;
   if (yt < 0.0) y = 0;
   else if (yt > 240.0) y = 240;
   else y = (int)yt;

   putpixel(x, y, color);
}

/*------------------------------------------------------------*/
void
display_line(x0, y0, x1, y1, color)
   int x0, y0, x1, y1;
   COORD3 color;
{
   double xt, yt;
//   int color_index;

   if (gMultiTaskCount++ > MAX_TASK_WAIT)
   {
   	   gMultiTaskCount = 0;
	   MacMultiTask();
   }

   /* Scale from image size to actual screen pixel size */
   yt = maxy/2 - Y_Display_Scale * y0;
   xt = maxx/2 + X_Display_Scale * x0;

   /* Clip the line to the viewport */
   if (xt < 0.0)
      x0 = 0;
   else if (xt > maxx) {
      x0 = maxx - 1;
      }
   else x0 = (int)xt;
   if (yt < 0.0)
      y0 = 0;
   else if (yt > maxy) {
      y0 = maxy;
      }
   else
      y0 = (int)yt;

   yt = maxy/2 - Y_Display_Scale * y1;
   xt = maxx/2 + X_Display_Scale * x1;
   if (xt < 0.0)
      x1 = 0;
   else if (xt > maxx) {
      x1 = maxx - 1;
      }
   else x1 = (int)xt;
   if (yt < 0.0)
      y1 = 0;
   else if (yt > maxy) {
      y1 = maxy;
      }
   else
      y1 = (int)yt;

#if defined(USE_CUSTOM_PALETTE)
   color_index = determine_color_index(color);
   PmForeColor(color_index);
#else
   {
   RGBColor	rgbc;

   Coord3ToRGBColor(color, &rgbc);
   RGBForeColor(&rgbc);
   }
#endif
   MoveTo(x0, y0);
   LineTo(x1, y1);
}


/*------------------------------------------------------------*/
/* general dialog constants */
#define	Ditem_ET_SizePrompt		4
#define	Ditem_BT_GetInFile		5
#define	Ditem_ST_InFileName		6
#define	Ditem_RB_RTO_BuiltIn	7
#define	Ditem_RB_RTO_TriMesh	8
#define	Ditem_PU_OutFormat		9
#define	Ditem_ST_SizeText1		10
#define	Ditem_ST_SizeText2		11
#define	Ditem_ST_RTO_Title		12
#define	Ditem_MAX				Ditem_ST_RTO_Title

// STR# resource IDs for this dialog
#define	StrID_APP_NAME			2000
#define	StrID_SIZE_PROMPT		2001
#define	StrID_INFILE_PROMPT		2002
#define	StrID_PATCH_PROMPT		2003

// popup menu ID
#define	MenuID_OutFormat		1000

/*------------------------------------------------------------*/
/* Put up dialog of options, and return TRUE if user clicks ok, else FALSE if cancel */
static Boolean GetUserOptions(int spdType)
{
	short			itemHit;
	short			k, dummy;
	DialogPtr		myDialog;
	Rect			displayRect;
	ControlHandle	theDItems[Ditem_MAX+1];
	char			aString[64];
	Str15			thePatchPrompt;
	Str31			appVers;
	Str63			theInFilePrompt;
	Str63			theAppName;
	Str255			theSizePrompt;

	// load the dialog
	myDialog = SetupNewDialog(DlogID_GETOPTS, true);
	if (!myDialog)
	{
		SysBeep(4);
		ExitToShell();
	}

	// preload all the dialog items we care about
	for (k = 1; k<=Ditem_MAX; k++)
		GetDialogItem(myDialog, k, &dummy, (Handle *) &theDItems[k], &displayRect);

	// Get app-specific strings from resources
	GetIndString(theAppName,		StrID_APP_NAME,		spdType);
	GetIndString(theSizePrompt,		StrID_SIZE_PROMPT,	spdType);
	GetIndString(theInFilePrompt,	StrID_INFILE_PROMPT,spdType);
	GetIndString(thePatchPrompt,	StrID_PATCH_PROMPT, spdType);

	// SPD Version
//	strcpy(aString, lib_get_version_str());
//	c2pstr(aString);
	// get Mac version from resource instead
	GetAppVersionPString(1, appVers);

	// fill in app-specific strings in dialog
	ParamText(theAppName, theSizePrompt, theInFilePrompt, appVers);

	// see which dialog items to remove.  Some of these items are specific
	// to certain apps only.  If the Size prompt resource string is empty,
	// don't display the size prompt stuff.
	gHasSizePrompt		= (theSizePrompt[0] != '\0');

	// If infile prompt is empty, don't display it.
	gHasInFilePrompt	= (theInFilePrompt[0] != '\0');

	// If infile prompt is empty, don't display it.
	gHasPatchPrompt	= (thePatchPrompt[0] != '\0');

	// Hide any items that we don't want now
	if (!gHasSizePrompt)
	{
		MoveDItem(myDialog, Ditem_ET_SizePrompt, -1000, -1000);
		MoveDItem(myDialog, Ditem_ST_SizeText1,  -1000, -1000);
		MoveDItem(myDialog, Ditem_ST_SizeText2,  -1000, -1000);
	}
	if (!gHasInFilePrompt)
	{
		MoveControl(theDItems[Ditem_BT_GetInFile], -1000, -1000);
		MoveDItem(myDialog, Ditem_ST_InFileName,   -1000, -1000);
	}
	if (!gHasPatchPrompt)
	{
		MoveDItem(myDialog, Ditem_ST_RTO_Title,      -1000, -1000);
		MoveControl(theDItems[Ditem_RB_RTO_BuiltIn], -1000, -1000);
		MoveControl(theDItems[Ditem_RB_RTO_TriMesh], -1000, -1000);
	}

	// fill initial size value into dialog
	sprintf(aString, "%d", gMacParmSize);
	c2pstr(aString);
	SetDialogItemText((Handle) theDItems[Ditem_ET_SizePrompt], (StringPtr)aString);

	// Preset the popup to POV-Ray 3 for now
	SetControlValue(theDItems[Ditem_PU_OutFormat], gMacRayTracerKind+1);

	// select something..
	SelectDialogItemText(myDialog, Ditem_ET_SizePrompt, 0, -1);

	// finally show the user our dialog
	MoveWindowToMaxDevice(myDialog);
	ShowWindow(myDialog);

	// prompt until user clicks ok or cancel
	do
	{
		// Set the radio buttons
		SetControlValue(theDItems[Ditem_RB_RTO_BuiltIn], gMacDoBuiltIn);
		SetControlValue(theDItems[Ditem_RB_RTO_TriMesh], !gMacDoBuiltIn);

		// get user input
		ModalDialog(NULL, &itemHit);

		// process some user interface elements
		switch (itemHit)
		{
			case Ditem_RB_RTO_BuiltIn:
				gMacDoBuiltIn = true;
				break;

			case Ditem_RB_RTO_TriMesh:
				gMacDoBuiltIn = false;
				break;

			case Ditem_BT_GetInFile:
				GetInputFile(gInFileName);
				strcpy(aString, gInFileName);
				c2pstr(aString);
				SetDialogItemText((Handle) theDItems[Ditem_ST_InFileName], (StringPtr)aString);
				break;

			case Ditem_PU_OutFormat:
				break;
		}
	} while ((itemHit != ok) && (itemHit != cancel));

	if (itemHit == ok)
	{

		// Size
		if (gHasSizePrompt)
		{
			GetDialogItemText((Handle) theDItems[Ditem_ET_SizePrompt], (StringPtr)aString);
			p2cstr((StringPtr)aString);
			gMacParmSize = atoi(aString);
			if ((gMacParmSize < 1) || (gMacParmSize > 9))
				gMacParmSize = 2;
		}

		// output format from popup
		gMacRayTracerKind = GetControlValue(theDItems[Ditem_PU_OutFormat]) - 1;
	}

	DisposeDialog(myDialog);

	// return TRUE if they hit OK
	return (itemHit == ok);

} /* GetUserOptions */


/*------------------------------------------------------------*/
static void AddArgvOpt(int *argcp, char ***argvp, char * optionStr)
{
	strcpy(argvCurrent, optionStr);
	(*argvp)[*argcp] = argvCurrent;
	argvCurrent += strlen(argvCurrent)+1; /* skip over string and null */
	(*argcp)++;
}


/*------------------------------------------------------------*/
/*  gProgressItemValue will be set to 0 to 100 by CalculateProgressValue() */

#define	kProgressItemNum	3
static unsigned long	gProgressItemValue;
static short			gProgressItemNum;
static Rect				gProgressItemRect;
static RgnHandle		gProgressItemRgn = NULL;


/*------------------------------------------------------------*/
// Change the type/creator of the output file
static void SetOutputFileType(FSSpec *anFSSpec)
{
	OSErr	anError;
	FInfo	myFileInfo;

	// Get file info, so we can change it
	anError = FSpGetFInfo(anFSSpec, &myFileInfo);

	if (anError==noErr)
		{
		myFileInfo.fdType		= gFileInfo[gMacRayTracerKind].fdType;
		myFileInfo.fdCreator	= gFileInfo[gMacRayTracerKind].fdCreator;
		anError = FSpSetFInfo(anFSSpec, &myFileInfo);
		}
}


/*------------------------------------------------------------*/
// Set up a user item proc for drawing progress bar
static pascal void showProgress_UProc(DialogPtr theDialog, short theItem)
{
#pragma unused (theItem)
	PenState	SavePen;
	short		progressPos;
	Rect		dispRect,
				outerRect,
				progressRect;
	RGBColor	pBackColor	= {0xf000,0xf000,0xf000};	// background
	RGBColor	pProgColor	= {0,0x7000,0};					// progress bar
	RGBColor	pFrameColor	= {0,0,0};					// outer frame rect

	// remember original penstate
	SetPort(theDialog);
	GetPenState(&SavePen);

	// find progress bar rectangle
	dispRect = gProgressItemRect;

	// outer frame
	outerRect = dispRect;
	InsetRect(&outerRect, 1, 1);
	PenSize(1, 1);
	RGBForeColor(&pFrameColor);
	FrameRect(&outerRect);

	// set up progress rect
	progressRect = dispRect;
	InsetRect(&progressRect, 3, 3);

	// calculate inner bar progress position
	if (gProgressItemValue > 100)
		gProgressItemValue = 100;
	progressPos = ((unsigned long)(progressRect.right - progressRect.left) * gProgressItemValue) / 100L;

	// draw inner bar (left filled side)
	if (progressPos > 0)
	{
		progressRect.right = dispRect.left + progressPos;
		RGBForeColor(&pProgColor);
		PaintRect(&progressRect);

		// draw inner bar (right open side)
		if (progressPos < 100)
		{
			progressRect.left = dispRect.left + progressPos;
			RGBForeColor(&pBackColor);
			PaintRect(&progressRect);
		}
	}

	// restore state
	SetPenState(&SavePen);

} // showProgress_UProc


/*------------------------------------------------------------*/
// Sets dialog item's display proc to draw progress bar
static void SetupProgressItem(DialogPtr theDialog, short theItemNum)
{
    short	itemtype;
    Handle	tempHandle;
	UserItemUPP	drawProcUPP;

	drawProcUPP = NewUserItemProc((ProcPtr)showProgress_UProc);

	gProgressItemValue = 0;

	// Set up User item to display a progress bar
	GetDialogItem(theDialog, theItemNum, &itemtype, &tempHandle, &gProgressItemRect);
	SetDialogItem(theDialog, theItemNum, itemtype, (Handle)drawProcUPP, &gProgressItemRect);

	// remember.. for later updates
	gProgressItemRgn = NewRgn();
	RectRgn(gProgressItemRgn, &gProgressItemRect);
	gProgressItemNum = theItemNum;
} // SetupProgressItem


/*------------------------------------------------------------*/
// Loads Dialog resource and sets up progress bar user item proc
static DialogPtr GetNewProgressDialog(short theDialogID, short theProgressItemNum)
{
	DialogPtr	theDialog = NULL;

	theDialog = GetNewDialog(theDialogID, NULL, (WindowPtr)-1);

	if (theDialog)
		SetupProgressItem(theDialog, theProgressItemNum);

	return (theDialog);

} // GetNewProgressDialog


/*------------------------------------------------------------*/
// Calculates current value for progress bar
static void CalculateProgressValue(long lowestVal, long highestVal, long currentVal)
{
	// Clip against upper & lower bounds
	if (currentVal < lowestVal)
		currentVal = lowestVal;
	else
		if (currentVal > highestVal)
			currentVal = highestVal;

	// Now calculate current value
	gProgressItemValue = (currentVal-lowestVal) * 100L / (highestVal-lowestVal);

} // CalculateProgressValue


/*------------------------------------------------------------*/
// Redisplay dialog
static void redrawProgressDialog(DialogPtr pDialogPtr, RgnHandle updRgn)
{
	BeginUpdate(pDialogPtr);
	UpdateDialog(pDialogPtr, updRgn);
	EndUpdate(pDialogPtr);
}

/*------------------------------------------------------------*/
// Recalculate progress bar and redisplay dialog
static void updateProgressDialog(DialogPtr pDialogPtr, long lowestVal, long highestVal, long currentVal)
{
	CalculateProgressValue(lowestVal, highestVal, currentVal);
	SetPort(pDialogPtr);
	InvalRect(&gProgressItemRect);

	redrawProgressDialog(pDialogPtr, gProgressItemRgn);

} // updateProgressDialog


/*------------------------------------------------------------*/
// dispose the progress bar dialog
static void disposeProgressDialog(DialogPtr pDialogPtr)
{
	if (pDialogPtr)
	{
		DisposeDialog(pDialogPtr);
		pDialogPtr = NULL;
	}

	if (gProgressItemRgn)
	{
		DisposeRgn(gProgressItemRgn);
		gProgressItemRgn = NULL;
	}
} // disposeProgressDialog



/*------------------------------------------------------------*/
#pragma segment main
void MacInit(int *argcp, char ***argvp, int spdType)
{
	char	strTemp[10];

	// give us another 80k of stack space, 'cause we're so recursive
	SetApplLimit(GetApplLimit() - 80000);
	MaxApplZone();

	ToolBoxInit();
	if (!GetUserOptions(spdType))
		ExitToShell();
	else
	{
		SetCursor(*GetCursor(watchCursor));
		if (gMacRayTracerKind != OUTPUT_VIDEO)
		{
			/* put up "please wait" dialog */
			gDialogPtr	= GetNewProgressDialog(DlogID_WAIT, kProgressItemNum);
			if (gDialogPtr)
			{
				ShowWindow(gDialogPtr);
				redrawProgressDialog(gDialogPtr, NULL);
			}
			else
			{
				SysBeep(4);
				ExitToShell();
			}
		}

		*argcp = 0;			/* Set argc to 1 parm initially */
//		*argvp = (char **)&macArgv;	/* point argv to our buffer */
		argvCurrent = (char *)&macArgvBuffer;	/* start at beginning of buffer */

		/*==== Program name is always first ====*/
		AddArgvOpt(argcp, argvp, "MacSPD");

		/*==== Raytracer Format ====*/
		AddArgvOpt(argcp, argvp, "-r");
		sprintf(strTemp, "%d", gMacRayTracerKind);
		AddArgvOpt(argcp, argvp, strTemp);

		/*==== Output Format (OUTPUT_CURVES,OUTPUT_PATCHES) ====*/
		if (gHasPatchPrompt)
		{
			if (gMacDoBuiltIn)
				AddArgvOpt(argcp, argvp, "-c"); /*OUTPUT_CURVES*/
			else
				AddArgvOpt(argcp, argvp, "-t"); /*OUTPUT_PATCHES*/
		}

		/*==== Size ====*/
		if (gHasSizePrompt)
		{
			AddArgvOpt(argcp, argvp, "-s");
			sprintf(strTemp, "%d", gMacParmSize);
			AddArgvOpt(argcp, argvp, strTemp);
		}

		/*==== Input File ====*/
		if (gHasInFilePrompt)
		{
			/* Input file name */
			AddArgvOpt(argcp, argvp, "-f");
			AddArgvOpt(argcp, argvp, gInFileName);
		}
	}
} /* MacInit */

/*------------------------------------------------------------*/
void MacProgress(int STARTVAL,int CURRVAL,int ENDVAL)
{
	if (gDialogPtr)
		updateProgressDialog(gDialogPtr, STARTVAL, ENDVAL, CURRVAL);
	MacMultiTask();
}


/*------------------------------------------------------------*/
void MacMultiTask(void)
{
	Boolean	inBackground;
	EventRecord	anEvent;

   if (gMultiTaskCount++ > MAX_TASK_WAIT)
   {
   	   gMultiTaskCount = 0;

		WaitNextEvent(everyEvent, &anEvent, 4, NULL);

		/* grody hack to redraw dialog on suspend/resume */
		if (anEvent.what == osEvt)
			if (((anEvent.message >> 24) & 0x0FF) == kSuspendResumeMessage)
			{	/* high byte of message */
				inBackground = (anEvent.message & resumeFlag) == 0;
				/* suspend/resume is also an activate/deactivate */
				if (!inBackground)
					{
					redrawProgressDialog(gDialogPtr, NULL);
					// restore watch cursor
					SetCursor(*GetCursor(watchCursor));
					}
			}
	}

} /* MacMultiTask */


/*------------------------------------------------------------*/
void MacShutDown(void)
{
	FSSpec	anFSSpec;
	/* convert output filename to a Mac FSSpec */
	c2pstr(gOutfileName);
	FSMakeFSSpec(0, 0, (StringPtr)gOutfileName, &anFSSpec);
	/* change output file type */
	SetOutputFileType(&anFSSpec);

	/* remove the Please Wait dialog */
	if (gDialogPtr)
		disposeProgressDialog(gDialogPtr);
} /* MacShutDown */
