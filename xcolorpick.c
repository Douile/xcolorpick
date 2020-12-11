#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <X11/Xos.h>
#include <X11/Xfuncs.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

const char *program_name;             /* Name of this program */
#define EXIT_FAILURE 2
#define INIT_NAME program_name=argv[0]        /* use this in main to setup
                                                 program_name */

Display *dpy;

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

typedef struct {
  int x;
  int y;
} Point;

/* https://gitlab.freedesktop.org/xorg/app/xprop/-/blob/master/dsimple.c */
void select_pixel(Display *dpy, Point *p) {
  int status;
  Cursor cursor;
  XEvent event;
  Window root = XRootWindow(dpy, XDefaultScreen(dpy));
  int buttons = 0;
  bool done = false;

  /* Make the target cursor */
  cursor = XCreateFontCursor(dpy, XC_crosshair);

  /* Grab the pointer using target cursor, letting it room all over */
  status = XGrabPointer(dpy, root, False,
      ButtonPressMask|ButtonReleaseMask, GrabModeSync,
      GrabModeAsync, root, cursor, CurrentTime);
  if (status != GrabSuccess) Fatal_Error("Can't grab the mouse.");

  /* Let the user select a window... */
  while ((!done) || (buttons != 0)) {
    /* allow one more event */
    XAllowEvents(dpy, SyncPointer, CurrentTime);
    XWindowEvent(dpy, root, ButtonPressMask|ButtonReleaseMask, &event);
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

  XUngrabPointer(dpy, CurrentTime);      /* Done with pointer */
}

/* https://stackoverflow.com/a/17525571 */
int main(int argc, char** argv)
{
    XColor c;
    dpy = XOpenDisplay((char *) NULL);

    Point p;
    select_pixel(dpy, &p);

    XImage *image;
    image = XGetImage (dpy, XRootWindow(dpy, XDefaultScreen(dpy)), p.x, p.y, 1, 1, AllPlanes, XYPixmap);
    c.pixel = XGetPixel(image, 0, 0);
    XFree(image);
    XQueryColor(dpy, XDefaultColormap(dpy, XDefaultScreen(dpy)), &c);
    printf("%d %d %d\n", c.red/256, c.green/256, c.blue/256);

    XCloseDisplay(dpy);

    return 0;
}
