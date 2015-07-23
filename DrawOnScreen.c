// use -std=gnu99 -lX11 2 compile
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

int main(){
	Display* display;
	int screen_num;
	Screen *screen;
	Window root_win;
	XGCValues gc_val;
	XEvent report;
	XButtonEvent *xb = (XButtonEvent *)&report;
	XKeyEvent *xk = (XKeyEvent *)&report;
	int status, x,y,oldx = -1, oldy = -1;
	Cursor cursor;

	display = XOpenDisplay(0);
	if (display == NULL){
		perror("Cannot connect to X server");
		exit (-1);
	}
	//XGrabServer(display);
	screen_num = DefaultScreen(display);
	screen = XScreenOfDisplay(display, screen_num);
	root_win = RootWindow(display, XScreenNumberOfScreen(screen));

	cursor = XCreateFontCursor(display, XC_crosshair);
	status = XGrabPointer(display, root_win, False,
				ButtonReleaseMask | ButtonPressMask|Button1MotionMask, GrabModeSync,
				GrabModeAsync, root_win, cursor, CurrentTime);
	if(status != GrabSuccess){
		perror("Can't grab the mouse");
		exit(-1);
	}
	status = XGrabKeyboard(display, root_win, False,
				GrabModeSync, GrabModeAsync, CurrentTime);
	if(status != GrabSuccess){
		perror("Can't grab the keyboard");
		exit(-1);
	}

	gc_val.function           = GXxor;
	gc_val.plane_mask         = AllPlanes;
	gc_val.foreground         = 0x00ff0000L;
	gc_val.background         = BlackPixel(display, screen_num);
	gc_val.line_width         = 3;
	gc_val.line_style         = LineSolid;
	gc_val.cap_style          = CapButt;
	gc_val.join_style         = JoinMiter;
	gc_val.fill_style         = FillOpaqueStippled;
	gc_val.fill_rule          = WindingRule;
	gc_val.graphics_exposures = False;
	gc_val.clip_x_origin      = 0;
	gc_val.clip_y_origin      = 0;
	gc_val.clip_mask          = None;
	gc_val.subwindow_mode     = IncludeInferiors;

#define MKGC()  XCreateGC(display, root_win, GCFunction | GCPlaneMask |  GCForeground | GCBackground | GCLineWidth | GCLineStyle | \
							GCCapStyle  | GCJoinStyle  |  GCFillStyle  |  GCFillRule  |  GCGraphicsExposures | \
							GCClipXOrigin |  GCClipYOrigin  |  GCClipMask  | GCSubwindowMode, &gc_val)
	GC  gc_red = MKGC();
	gc_val.foreground = 0x0000ff00L;
	GC  gc_green = MKGC();
	gc_val.foreground = 0x000000ffL;
	GC gc_blue = MKGC();
	GC *gc_cur = &gc_red;
	do{
		XAllowEvents(display, SyncPointer, CurrentTime);
		XWindowEvent(display, root_win, ButtonMotionMask | KeyPressMask| ButtonPressMask | ButtonReleaseMask, &report);
		switch(report.type){
			case KeyPress: // color
				switch(xk->keycode){
					case 9: goto End; break; // ESC
					case 27: gc_cur = &gc_red; break; // R
					case 42: gc_cur = &gc_green; break; // G
					case 56: gc_cur = &gc_blue; break; // B
				}
			break;
			case MotionNotify:
			xb->button = 1;
			case ButtonPress:
				x = xb->x_root, y = xb->y_root;
				switch(xb->button){
					case 1:
						if(oldx>-1 && oldy>-1)
						XDrawLine(display, root_win, *gc_cur,
									oldx, oldy, x, y);
						oldx = x; oldy = y;
					break;
					case 2:
						if(oldx>-1 && oldy>-1){
							int lx,ly,rx,ry;
							lx = (oldx < x) ? oldx : x;
							ly = (oldy < y) ? oldy : y;
							rx = (oldx > x) ? oldx : x;
							ry = (oldy > y) ? oldy : y;
							XDrawRectangle(display, root_win, *gc_cur,
										lx, ly, rx-lx, ry-ly);
							oldx = -1; oldy = -1;
						}else{
							oldx = x; oldy = y;}
					break;
					case 3:
						if(oldx>-1 && oldy>-1)
						XDrawLine(display, root_win, *gc_cur,
									oldx, oldy, x, y);
						oldx = x; oldy = y;
					break;
				}
			break;
			case ButtonRelease:
				if(xb->button == 1){oldx = -1; oldy = -1;}
			break;
		}
	}while(1);
End:
	XFlush(display);
	XUngrabServer(display);
	XCloseDisplay( display );
	return 0;
}
