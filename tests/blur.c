/* Compare the optimized blur against the original neighborhood definition. */
#include "s2.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned rng = 42;
static unsigned next(void)
{
	rng = rng * 1664525u + 1013904223u;
	return rng;
}
static void reference(struct image *im, int x0, int y0, int x1, int y1,
		      int radius)
{
	int lx = x0 < x1 ? x0 : x1, ly = y0 < y1 ? y0 : y1,
	    hx = x0 > x1 ? x0 : x1, hy = y0 > y1 ? y0 : y1;
	if (lx < 0)
		lx = 0;
	if (ly < 0)
		ly = 0;
	if (hx >= im->width)
		hx = im->width - 1;
	if (hy >= im->height)
		hy = im->height - 1;
	if (radius < 1)
		radius = 1;
	unsigned char *original = malloc(im->len);
	assert(original);
	memcpy(original, im->pixels, im->len);
	for (int y = ly; y <= hy; y++)
		for (int x = lx; x <= hx; x++) {
			uint64_t sums[3] = {0, 0, 0}, count = 0;
			for (int yy = ly; yy <= hy; yy++)
				for (int xx = lx; xx <= hx; xx++) {
					if (abs(xx - x) > radius ||
					    abs(yy - y) > radius)
						continue;
					for (int c = 0; c < 3; c++)
						sums[c] +=
						    original[((size_t)yy *
								  im->width +
							      xx) *
								 4 +
							     c];
					count++;
				}
			unsigned char *p =
			    im->pixels + ((size_t)y * im->width + x) * 4;
			for (int c = 0; c < 3; c++)
				p[c] = (unsigned char)(sums[c] / count);
			p[3] = 255;
		}
	free(original);
}
int main(void)
{
	for (int trial = 0; trial < 400; trial++) {
		int width = 1 + next() % 23, height = 1 + next() % 17;
		struct image actual = {.width = width,
				       .height = height,
				       .len = (size_t)width * height * 4},
			     expected = actual;
		actual.pixels = malloc(actual.len);
		expected.pixels = malloc(actual.len);
		assert(actual.pixels && expected.pixels);
		for (size_t i = 0; i < actual.len; i++)
			actual.pixels[i] = next() >> 24;
		memcpy(expected.pixels, actual.pixels, actual.len);
		int x0 = (int)(next() % (width + 8)) - 4,
		    x1 = (int)(next() % (width + 8)) - 4,
		    y0 = (int)(next() % (height + 8)) - 4,
		    y1 = (int)(next() % (height + 8)) - 4,
		    radius = (int)(next() % 40) - 1;
		draw_blur(&actual, x0, y0, x1, y1, radius);
		reference(&expected, x0, y0, x1, y1, radius);
		assert(memcmp(actual.pixels, expected.pixels, actual.len) == 0);
		free(actual.pixels);
		free(expected.pixels);
	}
	puts("PASS: blur matches reference across 400 clipped/reversed/random "
	     "regions and radii");
	return 0;
}
