#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "s2.h"

int
clipboard_copy_png(const struct image *img)
{
	FILE *pipe;
	FILE *pipe2;
	int rc;
	int status;
	int rc2;
	int status2;

	pipe = popen("xclip -selection clipboard -target image/png -in", "w");
	if (!pipe) {
		fprintf(stderr, "s2: cannot run xclip: %s\n", strerror(errno));
		return -1;
	}

	rc = image_write_png_stream(img, pipe);
	status = pclose(pipe);
	if (rc != 0) {
		fprintf(stderr, "s2: failed writing png stream to xclip\n");
		return -1;
	}
	if (status != 0) {
		fprintf(stderr, "s2: xclip failed with status %d\n", status);
		return -1;
	}

	pipe2 = popen("xclip -selection primary -target image/png -in", "w");
	if (!pipe2) {
		return 0;
	}
	rc2 = image_write_png_stream(img, pipe2);
	status2 = pclose(pipe2);
	if (rc2 != 0 || status2 != 0) {
		return 0;
	}

	return 0;
}
