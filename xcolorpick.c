/*
xcolorpick
Copyright (C) 2020  Douile

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <X11/Xos.h>
#include <X11/Xfuncs.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#define FORMAT_HEX "#%02X%02X%02X\n"
#define FORMAT_RGB "rgb(%d, %d, %d)\n"
const char * FORMAT_RAW = "%lld\n";

const char *program_name;             /* Name of this program */
#define EXIT_FAILURE 2
#define INIT_NAME program_name=argv[0]        /* use this in main to setup
                                                 program_name */
Display *dpy;

typedef struct {
 int x;
 int y;
} Point;

/*
 * Close_Display: Close display
 */
void Close_Display(void)
{
    if (dpy == NULL)
      return;

    XCloseDisplay(dpy);
    dpy = NULL;
}

/*
 * Standard fatal error routine - call like printf but maximum of 7 arguments.
 * Does not require dpy or screen defined.
 */
void Fatal_Error(const char *msg, ...)
{
	va_list args;
	fflush(stdout);
	fflush(stderr);
	fprintf(stderr, "%s: error: ", program_name);
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
	fprintf(stderr, "\n");
  Close_Display();
	exit(EXIT_FAILURE);
}

/* https://gitlab.freedesktop.org/xorg/app/xprop/-/blob/master/dsimple.c */
void select_pixel(Display *d, int s, Point *p) {
  int status;
  Cursor cursor;
  XEvent event;
  Window root = XRootWindow(d, s);
  int buttons = 0;
  bool done = false;

  /* Make the target cursor */
  cursor = XCreateFontCursor(d, XC_crosshair);

  /* Grab the pointer using target cursor, letting it room all over */
  status = XGrabPointer(d, root, False,
      ButtonPressMask|ButtonReleaseMask, GrabModeSync,
      GrabModeAsync, root, cursor, CurrentTime);
  if (status != GrabSuccess) Fatal_Error("Can't grab the mouse.");

  /* Let the user select a window... */
  while ((!done) || (buttons != 0)) {
    /* allow one more event */
    XAllowEvents(d, SyncPointer, CurrentTime);
    XWindowEvent(d, root, ButtonPressMask|ButtonReleaseMask, &event);
    switch (event.type) {
    case ButtonPress:
      if (!done) {
        p->x = event.xbutton.x; /* (x,y) coords of press */
        p->y = event.xbutton.y;
        done = true;
      }
      buttons++;
      break;
    case ButtonRelease:
      if (buttons > 0) /* there may have been some down before we started */
  buttons--;
       break;
    }
  }

  XUngrabPointer(d, CurrentTime);      /* Done with pointer */
}


void
print_help (void)
{
    static const char *help_message =
"where options include:\n"
"    -h, --help               print out a summary of command line options\n"
"    --rgb                    output a CSS rgb string\n"
"    -q, --qhex               output hex without the leading #\n"
"    -r, --raw                output raw number (decimal)\n"
"    -f, --format format      use your own format string\n"
"    -v, --version            print program version\n";


    fflush (stdout);

    fprintf (stderr,
	     "usage:  %s [-options ...]\n\n",
	     program_name);
    fprintf (stderr, "%s\n", help_message);
}


void help (void) {
	print_help();
	exit(0);
}

void usage (const char *errmsg)
{
    if (errmsg != NULL)
	    fprintf (stderr, "%s: %s\n\n", program_name, errmsg);

    print_help();
    exit (1);
}


/* https://stackoverflow.com/a/17525571 */
int main(int argc, char** argv)
{
    INIT_NAME;

    char * format = FORMAT_HEX;

    while (argv++, --argc>0 && **argv == '-') {
      if (!strcmp(argv[0], "-")) continue;
      if (!strcmp(argv[0], "-v") || !strcmp(argv[0], "--version")) {
        puts(PACKAGE_STRING);
        exit(0);
      }
      if (!strcmp(argv[0], "-h") || !strcmp(argv[0], "--help")) {
        help();
      }
      if (!strcmp(argv[0], "--rgb")) {
        format = FORMAT_RGB;
        continue;
      }
      if (!strcmp(argv[0], "-q") || !strcmp(argv[0], "--qhex")) {
        format = FORMAT_HEX+1;
        continue;
      }
      if (!strcmp(argv[0], "-r") || !strcmp(argv[0], "--raw")) {
        format = FORMAT_RAW;
        continue;
      }
      if (!strcmp(argv[0], "-f") || !strcmp(argv[0], "--format")) {
        if (++argv, --argc == 0) usage("format requires an argument");
        format = argv[0];
        continue;
      }
      fprintf (stderr, "%s: unrecognized argument %s\n\n", program_name, argv[0]);
      usage(NULL);
    }

    XColor c;
    dpy = XOpenDisplay((char *) NULL);
    int screen = XDefaultScreen(dpy);

    Point p;
    select_pixel(dpy, screen, &p);

    XImage *image;
    image = XGetImage (dpy, XRootWindow(dpy, screen), p.x, p.y, 1, 1, AllPlanes, XYPixmap);
    c.pixel = XGetPixel(image, 0, 0);
    XFree(image);
    XQueryColor(dpy, XDefaultColormap(dpy, XDefaultScreen(dpy)), &c);

    XCloseDisplay(dpy);

    if (format == FORMAT_RAW) {
      long long color = c.red;
      color <<= 16;
      color |= c.green;
      color <<= 16;
      color |= c.blue;
      printf(format, color);
    } else {
      int r = c.red/256, g = c.green/256, b = c.blue/256;
      printf(format, r, g, b);
    }
    return 0;
}
