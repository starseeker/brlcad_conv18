/*                           I M A G E . C
 * BRL-CAD
 *
 * Copyright (c) 2007-2008 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @addtogroup bu */
/** @{ */
/** @file image.c
 *
 *  @brief image save/load routines
 *
 *  save or load images in a variety of formats.
 *
 *  @author
 *	Erik Greenwald
 *
 *  @par Source -
 *	The U. S. Army Research Laboratory
 * @n	Aberdeen Proving Ground, Maryland  21005-5068  USA
 */
/** @} */


#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>	/* for file mode info in WRMODE */

#include <png.h>

#include "machine.h"
#include "bu.h"

/*** bif flags ***/
/* streaming output (like pix and bw) vs buffer output (like png) */
#define BIF_STREAM 0x0
#define BIF_BUFFER 0x1
/*** end bif flags ***/

/* this might be a little better than saying 0444 */
#define WRMODE S_IRUSR|S_IRGRP|S_IROTH

/* private functions */

/* Save functions use the return value not only for success/failure, but also to
 * note if further action is needed.
 *   0 - failure.
 *   1 - success, no further action needed.
 *   2 - success, close() required on fd.
 * This might be better just using the f* functions instead of mixing...
 */

/*
 * Attempt to guess the file type. Understands ImageMagick style
 * FMT:filename as being preferred, but will attempt to guess
 * based on extension as well.
 *
 * I suck. I'll fix this later. Honest.
 */
static int
guess_file_format(char *filename, char *trimmedname)
{
    /* look for the FMT: header */
#define CMP(name) if(!strncmp(filename,#name":",strlen(#name))){strncpy(trimmedname,filename+strlen(#name)+1,BUFSIZ);return BU_IMAGE_##name; }
    CMP(PIX);
    CMP(PNG);
    CMP(BMP);
    CMP(BW);
#undef CMP

    /* no format header found, copy the name as it is */
    strncpy(trimmedname, filename, BUFSIZ);

    /* and guess based on extension */
#define CMP(name,ext) if(!strncmp(filename+strlen(filename)-strlen(#name)-1,"."#ext,strlen(#name)+1)) return BU_IMAGE_##name;
    CMP(PNG,png);
    CMP(BMP,bmp);
    CMP(BW,bw);
#undef CMP
    /* defaulting to PIX */
    return BU_IMAGE_PIX;
}

static int
png_save(int fd, char *rgb, int width, int height)
{
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    int i = 0;
    FILE *fh;

    fh = fdopen(fd,"wb");
    if(fh==NULL) {
	perror("fdopen");
	bu_exit(-1, "png_save trying to get FILE pointer for descriptor\n");
    }

    png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(png_ptr == NULL) return 0;

    info_ptr = png_create_info_struct (png_ptr);
    if(info_ptr == NULL || setjmp (png_jmpbuf (png_ptr))) {
	png_destroy_read_struct (&png_ptr, info_ptr ? &info_ptr : NULL, NULL);
	bu_exit(-1, "Ohs Noes!\n");
    }

    png_init_io (png_ptr, fh);
    png_set_IHDR (png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
                  PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
                  PNG_FILTER_TYPE_BASE);
    png_write_info (png_ptr, info_ptr);
    for (i = height-1; i >= 0; --i)
        png_write_row (png_ptr, (png_bytep) (rgb + width*3*i));
    png_write_end (png_ptr, info_ptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fh);
    return 1;
}

static int
bmp_save(int fd, char *rgb, int width, int height)
{
    return 0;
}

static int
pix_save(int fd, char *rgb, int size) { write(fd, rgb, size); return 2; }

/* size is bytes of PIX data, bw output file will be 1/3 this size.
 * Also happens to munge up the contents of rgb. */
static int
bw_save(int fd, char *rgb, int size)
{
    int bwsize = size/3, i;
    if(bwsize*3 != size) {
	bu_exit(-1, "Huh, size=%d is not a multiple of 3.\n", size);
    }
    /* an ugly na�ve pixel grey-scale hack. Does not take human color curves. */
    for(i=0;i<bwsize;++i) rgb[i] = (int)((float)rgb[i*3]+(float)rgb[i*3+1]+(float)rgb[i*3+2]/3.0);
    write(fd, rgb, bwsize);
    return 2;
}

/* end if private functions */

/* begin public functions */

int
bu_image_load()
{
    return 0;
}

int
bu_image_save(char *data, int width, int height, int depth, char *filename, int filetype)
{
    int i;
    struct bu_image_file *bif = bu_image_save_open(filename,filetype,width,height,depth);
    if(bif==NULL) return -1;
    for(i=0;i<height;++i) {
	if(bu_image_save_writeline(bif,i,(unsigned char*)(data+i*width*depth))==-1) {
	    bu_log("Uh?");
	}
    }
    return bu_image_save_close(bif);
}

struct bu_image_file *
bu_image_save_open(char *filename, int format, int width, int height, int depth)
{
    struct bu_image_file *bif = (struct bu_image_file *)bu_malloc(sizeof(struct bu_image_file),"bu_image_save_open");
    bif->magic = BU_IMAGE_FILE_MAGIC;
    if(format == BU_IMAGE_AUTO) {
	char buf[BUFSIZ];
	bif->format = guess_file_format(filename,buf);
	bif->filename = strdup(buf);
    } else {
	bif->format = format;
	bif->filename = strdup(filename);
    }

    /* if we want the ability to "continue" a stopped output, this would be
     * where to check for an existing "partial" file. */
    bif->fd = open(bif->filename,O_WRONLY|O_CREAT|O_TRUNC, WRMODE);
    if(bif->fd < 0) {
	char buf[BUFSIZ];
	perror("open");
	free(bif);
	snprintf(buf,BUFSIZ,"ERROR opening output file \"%s\" for writing\n",bif->filename);
	bu_exit(-1, buf);
    }
    bif->width = width;
    bif->height = height;
    bif->depth = depth;
    bif->data = (char *)bu_malloc(width*height*depth, "bu_image_file data");
    return bif;
}

int
bu_image_save_writeline(struct bu_image_file *bif, int y, unsigned char *data)
{
    if(bif==NULL) { printf("trying to write a line with a null bif\n"); return -1; }
    memcpy(bif->data + bif->width*bif->depth*y, data, bif->width*bif->depth);
    return 0;
}

int
bu_image_save_close(struct bu_image_file *bif)
{
    int r = 0;
    switch(bif->format) {
	case BU_IMAGE_BMP: r = bmp_save(bif->fd,bif->data,bif->width,bif->height); break;
	case BU_IMAGE_PNG: r = png_save(bif->fd,bif->data,bif->width,bif->height); break;
	case BU_IMAGE_PIX: r = pix_save(bif->fd,bif->data,bif->width*bif->height*bif->depth); break;
	case BU_IMAGE_BW: r = bw_save(bif->fd, bif->data, bif->width*bif->height*bif->depth); break;
    }
    switch(r) {
	case 0: bu_log("Failed to write image\n"); break;
	/* 1 signals success with no further action needed */
	case 2: close(bif->fd); break;
    }
    bu_free(bif->filename,"bu_image_file filename");
    bu_free(bif->data,"bu_image_file data");
    bu_free(bif,"bu_image_file");
    return 0;
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
