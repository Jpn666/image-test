#include <pngreader.h>
#include <stdio.h>


static TImageInfo imageinfo;


static uint8*
encoderow(uint8* row, uint8* buffer, TImageInfo* info)
{
	uintxx i;

	switch (info->cmode) {
		case IMAGE_GRAY:
			if (info->bpc == 8) {
				for (i = 0; i < info->sizex; i++) {
					*buffer++ = row[0];
					*buffer++ = row[0];
					*buffer++ = row[0];
					row++;
				}
			}
			else {
				for (i = 0; i < info->sizex; i++) {
					*buffer++ = (uint8) ((row[0] << 8) | row[1]);
					*buffer++ = (uint8) ((row[0] << 8) | row[1]);
					*buffer++ = (uint8) ((row[0] << 8) | row[1]);
					row += 2;
				}
			}
			return buffer;

		case IMAGE_GRAYALPHA:
			if (info->bpc == 8) {
				for (i = 0; i < info->sizex; i++) {
					*buffer++ = row[0];
					*buffer++ = row[0];
					*buffer++ = row[0];
					*buffer++ = row[1];
					row += 2;
				}
			}
			else {
				for (i = 0; i < info->sizex; i++) {
					*buffer++ = (uint8) ((row[0] << 8) | row[1]);
					*buffer++ = (uint8) ((row[0] << 8) | row[1]);
					*buffer++ = (uint8) ((row[0] << 8) | row[1]);
					*buffer++ = (uint8) ((row[2] << 8) | row[3]);
					row += 4;
				}
			}
			return buffer;

		case IMAGE_RGB:
			if (info->bpc == 8) {
				for (i = 0; i < info->sizex; i++) {
					*buffer++ = row[2];
					*buffer++ = row[1];
					*buffer++ = row[0];
					row += 3;
				}
			}
			else {
				for (i = 0; i < info->sizex; i++) {
					*buffer++ = (uint8) ((row[4] << 8) | row[5]);
					*buffer++ = (uint8) ((row[2] << 8) | row[3]);
					*buffer++ = (uint8) ((row[0] << 8) | row[1]);
					row += 6;
				}
			}
			return buffer;

		case IMAGE_RGBALPHA:
			if (info->bpc == 8) {
				for (i = 0; i < info->sizex; i++) {
					*buffer++ = row[2];
					*buffer++ = row[1];
					*buffer++ = row[0];
					*buffer++ = row[3];
					row += 4;
				}
			}
			else {
				for (i = 0; i < info->sizex; i++) {
					*buffer++ = (uint8) ((row[4] << 8) | row[5]);
					*buffer++ = (uint8) ((row[2] << 8) | row[3]);
					*buffer++ = (uint8) ((row[0] << 8) | row[1]);
					*buffer++ = (uint8) ((row[6] << 8) | row[7]);
					row += 8;
				}
			}
		break;
	}
	return buffer;
}

static bool
writeheader(FILE* handler, TImageInfo* info, uintxx bpp)
{
	uint8 c;
	uint16 sizex;
	uint16 sizey;
	
	c = 0x00; fwrite(&c, 1, 1, handler);
	c = 0x00; fwrite(&c, 1, 1, handler);
	c = 0x02; fwrite(&c, 1, 1, handler);
	
	c = 0x00; fwrite(&c, 1, 1, handler);
	c = 0x00; fwrite(&c, 1, 1, handler);
	c = 0x00; fwrite(&c, 1, 1, handler);
	c = 0x00; fwrite(&c, 1, 1, handler);
	c = 0x00; fwrite(&c, 1, 1, handler);
	c = 0x00; fwrite(&c, 1, 1, handler);
	c = 0x00; fwrite(&c, 1, 1, handler);
	c = 0x00; fwrite(&c, 1, 1, handler);
	c = 0x00; fwrite(&c, 1, 1, handler);

	sizex = (uint16) info->sizex;
	sizey = (uint16) info->sizey;
	fwrite(&sizex, 1, 2, handler);
	fwrite(&sizey, 1, 2, handler);

	/* image type: rgb(24) or rgba(32) */
	c = bpp;
	fwrite(&c, 1, 1, handler);

	/* pixel origin (top-left) */
	c = 1 << 5;
	fwrite(&c, 1, 1, handler);
	if (ferror(handler)) {
		return 0;
	}
	return 1;
}

static bool
writetga(FILE* handler, TImageInfo* info, uint8* image)
{
	uintxx i;
	uintxx j;
	uintxx rowsize;
	uint8* row;
	uint8* buffer;
	
	j = 24;
	if (info->cmode == IMAGE_RGBALPHA || info->cmode == IMAGE_GRAYALPHA) {
		j = 32;
	}

	if (writeheader(handler, info, j) == 0) {
		return 0;
	}
	
	j = (j >> 3) * info->sizex;
	buffer = malloc(j);
	if (buffer == NULL) {
		return 0;
	}

	rowsize = imginfo_getrowsize(info);
	row = image;
	for (i = 0; i < info->sizey; i++) {
		encoderow(row, buffer, info);
		fwrite(buffer, 1, j, handler);
		if (ferror(handler)) {
			free(buffer);
			return 0;
		}
		row += rowsize;
	}
	free(buffer);
	return 1;
}

static intxx
rcallback(uint8* buffer, uintxx size, void* user)
{
	intxx r;

	r = fread(buffer, 1, size, (FILE*) user);
	if (r ^ size) {
		if (ferror((FILE*) user)) {
			return -1;
		}
	}
	return r;
}

int
main(int argc, char* argv[])
{
	TPNGReader* pngr;
	FILE* pngfile;
	FILE* tgafile;

	if (argc != 3) {
		puts("Usage: thisprogram <png file> <target file>");
		return 0;
	}

	pngfile = fopen(argv[1], "rb");
	tgafile = fopen(argv[2], "wb");
	if (pngfile == NULL || tgafile == NULL) {
		puts("IO error, failed to open or create file");
		goto L_ERROR2;
	}

	pngr = pngr_create(PNGR_IGNOREICCP | PNGR_NOCRCCHECK);
	if (pngr == NULL) {
		puts("Error: failed to create reader");
		goto L_ERROR2;
	}

	pngr_setinputfn(pngr, rcallback, pngfile);
	if (pngr_initdecoder(pngr, &imageinfo)) {
		uint8* image;
		uint8* dcmem;   /* decoder memory */

		image = malloc(imageinfo.imgsize);
		dcmem = malloc(imageinfo.rmemory);
		if (image == NULL || dcmem == NULL) {
			free(image);
			free(dcmem);
			goto L_ERROR2;
		}

		pngr_setbuffers(pngr, dcmem, image, NULL);
		if (pngr_decodeimg(pngr)) {
			if (writetga(tgafile, &imageinfo, image) == 0) {
				puts("Error: ");
				goto L_ERROR1;
			}
		}
		else {
			puts("Error: failed to decode image");
		}

L_ERROR1:
		free(image);
		free(dcmem);
	}
	else {
		puts("Error: failed to decode image");
	}
	
	pngr_destroy(pngr);
L_ERROR2:
	if (pngfile)
		fclose(pngfile);
	if (tgafile)
		fclose(tgafile);
	return 0;
}
