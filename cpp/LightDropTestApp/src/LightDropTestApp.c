#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>

// default framebuffer palette
typedef enum {
	BLACK = 0, /*   0,   0,   0 */
	BLUE = 1, /*   0,   0, 172 */
	GREEN = 2, /*   0, 172,   0 */
	CYAN = 3, /*   0, 172, 172 */
	RED = 4, /* 172,   0,   0 */
	PURPLE = 5, /* 172,   0, 172 */
	ORANGE = 6, /* 172,  84,   0 */
	LTGREY = 7, /* 172, 172, 172 */
	GREY = 8, /*  84,  84,  84 */
	LIGHT_BLUE = 9, /*  84,  84, 255 */
	LIGHT_GREEN = 10, /*  84, 255,  84 */
	LIGHT_CYAN = 11, /*  84, 255, 255 */
	LIGHT_RED = 12, /* 255,  84,  84 */
	LIGHT_PURPLE = 13, /* 255,  84, 255 */
	YELLOW = 14, /* 255, 255,  84 */
	WHITE = 15 /* 255, 255, 255 */
} COLOR_INDEX_T;

static unsigned short def_r[] = { 0, 0, 0, 0, 172, 172, 172, 168, 84, 84, 84,
		84, 255, 255, 255, 255 };
static unsigned short def_g[] = { 0, 0, 168, 168, 0, 0, 84, 168, 84, 84, 255,
		255, 84, 84, 255, 255 };
static unsigned short def_b[] = { 0, 172, 0, 168, 0, 172, 0, 168, 84, 255, 84,
		255, 84, 255, 84, 255 };

// 'global' variables to store screen info
char *fbp = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

// helper function to 'plot' a pixel in given color
void put_pixel(int x, int y, int c) {
	// calculate the pixel's byte offset inside the buffer
	unsigned int pix_offset = x + y * finfo.line_length;

	// now this is about the same as 'fbp[pix_offset] = value'
	*((char*) (fbp + pix_offset)) = c;

}

void put_pixel_RGB24(int x, int y, int r, int g, int b) {
	// remember to change main(): vinfo.bits_per_pixel = 24;
	// and: screensize = vinfo.xres * vinfo.yres *
	//                   vinfo.bits_per_pixel / 8;

	// calculate the pixel's byte offset inside the buffer
	// note: x * 3 as every pixel is 3 consecutive bytes
	unsigned int pix_offset = x * 3 + y * finfo.line_length;

	// now this is about the same as 'fbp[pix_offset] = value'
	*((char*) (fbp + pix_offset)) = b;
	*((char*) (fbp + pix_offset + 1)) = g;
	*((char*) (fbp + pix_offset + 2)) = r;

}

void put_pixel_RGB32(int x, int y, int r, int g, int b) {
	// remember to change main(): vinfo.bits_per_pixel = 24;
	// and: screensize = vinfo.xres * vinfo.yres *
	//                   vinfo.bits_per_pixel / 8;

	// calculate the pixel's byte offset inside the buffer
	// note: x * 3 as every pixel is 3 consecutive bytes
	unsigned int pix_offset = x * 4 + y * finfo.line_length;

	// now this is about the same as 'fbp[pix_offset] = value'
	*((char*) (fbp + pix_offset)) = 255;
	*((char*) (fbp + pix_offset + 1)) = b;
	*((char*) (fbp + pix_offset + 2)) = g;
	*((char*) (fbp + pix_offset + 3)) = r;

}

void put_pixel_RGB565(int x, int y, int r, int g, int b) {
	// remember to change main(): vinfo.bits_per_pixel = 16;
	// or on RPi just comment out the whole 'Change vinfo'
	// and: screensize = vinfo.xres * vinfo.yres *
	//                   vinfo.bits_per_pixel / 8;

	// calculate the pixel's byte offset inside the buffer
	// note: x * 2 as every pixel is 2 consecutive bytes
	unsigned int pix_offset = x * 2 + y * finfo.line_length;

	// now this is about the same as 'fbp[pix_offset] = value'
	// but a bit more complicated for RGB565
	unsigned short c = ((r / 8) << 11) + ((g / 4) << 5) + (b / 8);
	// or: c = ((r / 8) * 2048) + ((g / 4) * 32) + (b / 8);
	// write 'two bytes at once'
	*((unsigned short*) (fbp + pix_offset)) = c;

}

// helper function for drawing - no more need to go mess with
// the main function when just want to change what to draw...
void drawColourCircle() {

	int x, y;
	int r, g, b;
	int dr;
	int cr = vinfo.yres / 3;
	int cg = vinfo.yres / 3 + vinfo.yres / 4;
	int cb = vinfo.yres / 3 + vinfo.yres / 4 + vinfo.yres / 4;

	for (y = 0; y < (vinfo.yres); y++) {
		for (x = 0; x < vinfo.xres; x++) {
			dr = (int) sqrt((cr - x) * (cr - x) + (cr - y) * (cr - y));
			r = 255 - 256 * dr / cr;
			r = (r >= 0) ? r : 0;
			dr = (int) sqrt((cg - x) * (cg - x) + (cr - y) * (cr - y));
			g = 255 - 256 * dr / cr;
			g = (g >= 0) ? g : 0;
			dr = (int) sqrt((cb - x) * (cb - x) + (cr - y) * (cr - y));
			b = 255 - 256 * dr / cr;
			b = (b >= 0) ? b : 0;

			if (vinfo.bits_per_pixel == 16) {
				put_pixel_RGB565(x, y, r, g, b);
			} else {
				put_pixel_RGB24(x, y, r, g, b);
			}
		}
	}
}

void drawLines() {
	int x, y;

	for (y = 0; y < (vinfo.yres / 2); y++) {
		for (x = 0; x < vinfo.xres; x++) {

			// color based on the 16th of the screen width
			int c = 16 * x / vinfo.xres;

			if (vinfo.bits_per_pixel == 8) {
				put_pixel(x, y, c);
			} else if (vinfo.bits_per_pixel == 16) {
				put_pixel_RGB565(x, y, def_r[c], def_g[c], def_b[c]);
			} else if (vinfo.bits_per_pixel == 24) {
				put_pixel_RGB24(x, y, def_r[c], def_g[c], def_b[c]);
			} else if (vinfo.bits_per_pixel == 32) {
				put_pixel_RGB32(x, y, def_r[c], def_g[c], def_b[c]);
			}

		}
	}

}


void drawHorizontalLines(){
	  int x, y;

	    // fill the screen with blue
	    memset(fbp, 1, vinfo.xres * vinfo.yres);

	    // white horizontal lines every 10 pixel rows
	    for (y = 0; y < (vinfo.yres); y+=10) {
	        for (x = 0; x < vinfo.xres; x++) {
	            put_pixel(x, y, 15);
	        }
	    }

	    // white vertical lines every 10 pixel columns
	    for (x = 0; x < vinfo.xres; x+=10) {
	        for (y = 0; y < (vinfo.yres); y++) {
	            put_pixel(x, y, 15);
	        }
	    }

	    int n;
	    // select smaller extent
	    // (just in case of a portrait mode display)
	    n = (vinfo.xres < vinfo.yres) ? vinfo.xres : vinfo.yres;
	    // red diagonal line from top left
	    for (x = 0; x < n; x++) {
	        put_pixel(x, x, 4);
	    }
}

void drawAnim(){
	void fill_rect(int x, int y, int w, int h, int c) {
	    int cx, cy;
	    for (cy = 0; cy < h; cy++) {
	        for (cx = 0; cx < w; cx++) {
	            put_pixel(x + cx, y + cy, c);
	        }
	    }
	}

	// helper function to clear the screen - fill whole
	// screen with given color
	void clear_screen(int c) {
	    memset(fbp, c, vinfo.xres * vinfo.yres);
	}

	void draw() {

	    int i, x, y, w, h, dx, dy;

	    // start position (upper left)
	    x = 0;
	    y = 0;
	    // rectangle dimensions
	    w = vinfo.yres / 10;
	    h = w;
	    // move step 'size'
	    dx = 1;
	    dy = 1;

	    int fps = 100;
	    int secs = 10;

	    // loop for a while
	    for (i = 0; i < (fps * secs); i++) {

	        // clear the previous image (= fill entire screen)
	        clear_screen(8);

	        // draw the bouncing rectangle
	        fill_rect(x, y, w, h, 15);

	        // move the rectangle
	        x = x + dx;
	        y = y + dy;

	        // check for display sides
	        if ((x < 0) || (x > (vinfo.xres - w))) {
	            dx = -dx; // reverse direction
	            x = x + 2 * dx; // counteract the move already done above
	        }
	        // same for vertical dir
	        if ((y < 0) || (y > (vinfo.yres - h))) {
	            dy = -dy;
	            y = y + 2 * dy;
	        }

	        usleep(1000000 / fps);
	        // to be exact, would need to time the above and subtract...
	    }

	}
}

// application entry point
int main(int argc, char* argv[]) {

	int fbfd = 0;
	struct fb_var_screeninfo orig_vinfo;
	long int screensize = 0;

	// Open the file for reading and writing
	fbfd = open("/dev/fb0", O_RDWR);
	if (fbfd == -1) {
		printf("Error: cannot open framebuffer device.\n");
		return (1);
	}
	printf("The framebuffer device was opened successfully.\n");

	// Get variable screen information
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
		printf("Error reading variable information.\n");
	}
	printf("Original %dx%d, %dbpp\n", vinfo.xres, vinfo.yres,
			vinfo.bits_per_pixel);

	// Store for reset (copy vinfo to vinfo_orig)
	memcpy(&orig_vinfo, &vinfo, sizeof(struct fb_var_screeninfo));

	// Change variable info
	/* use: 'fbset -depth x' to test different bpps
	 vinfo.bits_per_pixel = 8;
	 if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo)) {
	 printf("Error setting variable information.\n");
	 }
	 */

	// Get fixed screen information
	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
		printf("Error reading fixed information.\n");
	}

	// map fb to user mem
	screensize = finfo.smem_len;
	fbp = (char*) mmap(0, screensize,
	PROT_READ | PROT_WRITE,
	MAP_SHARED, fbfd, 0);

	if ((int) fbp == -1) {
		printf("Failed to mmap.\n");
	} else {
		// draw...
		//drawLines();
		//drawColourCircle();
		//drawHorizontalLines();
		drawAnim();
		sleep(5);
	}

	// cleanup
	// unmap fb file from memory
	munmap(fbp, screensize);
	// reset the display mode
	if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &orig_vinfo)) {
		printf("Error re-setting variable information.\n");
	}
	// close fb file
	close(fbfd);

	return 0;

}
