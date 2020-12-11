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

/* https://stackoverflow.com/a/17525571 */
int main(int argc, char** argv)
{
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
    int r = c.red/256, g = c.green/256, b = c.blue/256;
    printf("rgb(%d, %d, %d)\n", r, g, b);
    printf("#%02X%02X%02X\n", r, g, b);
    return 0;
}
