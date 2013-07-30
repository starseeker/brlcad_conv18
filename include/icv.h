/*                           I C V . H
 * BRL-CAD
 *
 * Copyright (c) 2011-2013 United States Government as represented by
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
/** @file icv.h
 *
 * Functions provided by the LIBICV image processing library.
 *
 */

#ifndef __ICV_H__
#define __ICV_H__

__BEGIN_DECLS

#ifndef ICV_EXPORT
#  if defined(ICV_DLL_EXPORTS) && defined(ICV_DLL_IMPORTS)
#    error "Only ICV_DLL_EXPORTS or ICV_DLL_IMPORTS can be defined, not both."
#  elif defined(ICV_DLL_EXPORTS)
#    define ICV_EXPORT __declspec(dllexport)
#  elif defined(ICV_DLL_IMPORTS)
#    define ICV_EXPORT __declspec(dllimport)
#  else
#    define ICV_EXPORT
#  endif
#endif

/**
 * I C V _ R O T
 *
 * Rotate an image.
 * %s [-rifb | -a angle] [-# bytes] [-s squaresize] [-w width] [-n height] [-o outputfile] inputfile [> outputfile]
 *
 */
ICV_EXPORT extern int icv_rot(int argv, char **argc);


/** @addtogroup image */
/** @ingroup data */
/** @{ */
/** @file libicv/fileformat.c
 *
 * image save/load routines
 *
 * save or load images in a variety of formats.
 *
 */

typedef enum {
    ICV_IMAGE_AUTO,
    ICV_IMAGE_PIX,
    ICV_IMAGE_BW,
    ICV_IMAGE_ALIAS,
    ICV_IMAGE_BMP,
    ICV_IMAGE_CI,
    ICV_IMAGE_ORLE,
    ICV_IMAGE_PNG,
    ICV_IMAGE_PPM,
    ICV_IMAGE_PS,
    ICV_IMAGE_RLE,
    ICV_IMAGE_SPM,
    ICV_IMAGE_SUN,
    ICV_IMAGE_YUV,
    ICV_IMAGE_UNKNOWN
} ICV_IMAGE_FORMAT;

typedef enum {
    ICV_COLOR_SPACE_RGB,
    ICV_COLOR_SPACE_GRAY
    /* Add here for format addition like CMYKA, HSV, others  */
} ICV_COLOR_SPACE;

typedef enum {
    ICV_DATA_DOUBLE,
    ICV_DATA_UCHAR
} ICV_DATA;

struct icv_image {
    uint32_t magic;
    ICV_COLOR_SPACE color_space;
    double *data;
    float gamma_corr;
    int width, height, channels, alpha_channel;
    unsigned long flags;
};

typedef struct icv_image icv_image_t;
#define ICV_IMAGE_NULL ((struct icv_image *)0)

/**
 * asserts the integrity of a icv_image_file struct.
 */
#define ICV_CK_IMAGE(_i) ICV_CKMAG(_i, ICV_IMAGE_MAGIC, "icv_image")

/**
 * initializes a icv_image_file struct without allocating any memory.
 */
#define ICV_IMAGE_INIT(_i) { \
	    (_i)->magic = ICV_IMAGE_MAGIC; \
	    (_i)->width = (_i)->height = (_i)->channels = (_i)->alpha_channel = 0; \
	    (_i)->gamma_corr = 0.0; \
	    (_i)->data = NULL; \
	    (_i)->flags = 0; \
    }

/**
 * returns truthfully whether a icv_image_file has been initialized.
 */
#define ICV_IMAGE_IS_INITIALIZED(_i) (((struct icv_image *)(_i) != ICV_IMAGE_NULL) && LIKELY((_i)->magic == ICV_IMAGE_MAGIC))

/**
 * Finds the Image format based on heuristics depending on the file name.
 * @param filename Filename of the image whose format is to be know.
 * @param trimmedname Buffer for storing filename after removing extensions
 * @return File Format
 */
ICV_EXPORT extern int icv_guess_file_format(const char *filename, char *trimmedname);

/**
 * This function allocates memory for an image and returns the
 * resultant image.
 * @param width Width of the image to be created
 * @param height Height of the image to be created.
 * @param color_space Specifies the color_space of the image to be
 * created
 * @return Image structure with allocated space and zeroed data array
 */
ICV_EXPORT extern icv_image_t* icv_image_create(int width, int height, ICV_COLOR_SPACE color_space);

/**
 * Write an image line to the data of ICV struct. Can handle unsinged char buffers.
 *
 * Note : This function require memory allocation for ICV_UCHAR_DATA, which inturn aquires
 * BU_SEM_SYSCALL semaphore.
 *
 * @param bif ICV struct where data is to be written
 * @param y Index of the line at which data is to be written. 0 for the first line
 * @data Line Data to be written
 * @type Type of data, for unsigned char data specify ICV_DATA_UCHAR or 1
 * @return on success 0, on failure -1
 */
ICV_EXPORT int icv_image_writeline(icv_image_t *bif, int y, void *data, ICV_DATA type);

/**
 * Writes a pixel to the specified coordinates in the data of ICV struct.
 * @param bif ICV struct where data is to be written
 * @param x x-dir coordinate of the pixel
 * @param y y-dir coordinate of the pixel. (0,0) coordinate is taken as bottom left
 * @data Data to be written
 * @return on success 0, on failure -1
 */
ICV_EXPORT int icv_image_writepixel(icv_image_t *bif, int x, int y, double *data);

/**
 * Saves Image to a file of respective format
 * @param bif Image structure of file.
 * @param filename Filename of the file to be saved
 * @param format Specific format of the file to be saved.
 * @return on success 0, on failure -1 with log messages.
 */
ICV_EXPORT extern int icv_image_save(icv_image_t* bif, const char*filename, ICV_IMAGE_FORMAT format);

/**
 * Load a file into an ICV struct. For most formats, this will be called with
 * format=ICV_IMAGE_AUTO, hint_format=0, hint_width=0, hint_height=0 and
 * hint_depth=0 for default values. At the moment, the data is packed into the
 * data field as rgb24 (raw pix style).
 *
 * For pix and bw files, having width and height set to 0 will trigger a
 * heuristic sizing algorithm based on file size, assuming that the image is
 * square at first, then looking through a set of common sizes, finally assuming
 * 512x512.
 *
 * @param filename File to load
 * @param hint_format Probable format of the file, typically ICV_IMAGE_AUTO
 * @param hint_width Width when passed as parameter from calling program. 0 for default
 * @param hint_height Height when passed as parameter from calling program. 0 for default
 * @param hint_depth Default depth field, 0 for default.
 * @return A newly allocated struct holding the loaded image info.
 */
ICV_EXPORT extern icv_image_t* icv_image_load(const char *filename, int format, int width, int height);

/**
 * This function zeroes all the data enteries of an image
 * @param img Image Strucure
 */
ICV_EXPORT extern icv_image_t* icv_image_zero(icv_image_t* bif);

/**
 * This function frees the allocated memory for a ICV Structure and
 * data.
 */
ICV_EXPORT extern void icv_image_free(icv_image_t* bif);

/** @file libicv/color_space.c
 *
 * This file contains routines to change the icv image from one
 * colorspace to other.
 *
 */

/**
 * Converts a single channel image to three channel image.
 * Replicates the pixel as done by bw-pix utility
 * returns a three channel image.
 * If a three channel image is passed, this function returns the same image.
 */


ICV_EXPORT int icv_image_gray2rgb(icv_image_t *img);

typedef enum {
    ICV_PIX_NTSC,
    ICV_PIX_CRT,
    ICV_PIX_SET_EQUAL,
    ICV_PIX_SELECT_CHANNEL
}ICV_DEPTH_METHOD;

typedef enum {
    ICV_COLOR_RGB,
    ICV_COLOR_R,
    ICV_COLOR_G,
    ICV_COLOR_B,
    ICV_COLOR_RG,
    ICV_COLOR_RB,
    ICV_COLOR_BG
}ICV_COLOR;

/**
 * converts a three plane image to single plane image.
 * This function will combine or select planes of the image based on
 * the input arguments
 *
 * A normal calling of this functions is as follows :
 * icv_image_rgb2gray(bif, 0 ,0 ,0 ,0 ,0); where bif is the rgb
 * image to be converted.
 *
 * @param img Image to be converted
 * @param method Conversion Method
 * ICV_PIX_NTSC:  converts to single plan image by combining three
 *                planes using weights based on NTSC primaries and
 *                D6500 white.
 * ICV_PIX_CRT :   converts to single plane image by combining three
 *                planes using weights based on CRT phosphor
 *                chrmaticities and D6500 white.
 * ICV_PIX_SET_EQUAL: Combines the three planes using equal weights.
 * ICV_PIX_SELECT_CHANNEL: lets to select the channels
 * @param color Choses color planes to be selected for combination
 *    This function will need color to be specified from
 *              ICV_COLOR_R
 *              ICV_COLOR_G
 *              ICV_COLOR_B
 *              ICV_COLOR_RG
 *              ICV_COLOR_RB
 *              ICV_COLOR_BG
 *              ICV_COLOR_RGB
 * @param rweight Weight for r-plane
 * @param gweight Weight for g-plane
 * @param bweight Weight for b-plane
 * @retunr 0 on success on failure return 1
 *
 *  User can specify weights in the arguments, for the selected
 *    color planes. If 0 weight is chosen this utility assigns equal
 *    weights.
 *
 */

ICV_EXPORT int icv_image_rgb2gray(icv_image_t *img,
				  ICV_DEPTH_METHOD method,
				  ICV_COLOR color,
				  double rweight,
				  double gweight,
				  double bweight);

/** @file libicv/crop.c
 *
 * This file contains functions for cropping images.
 * There are two types of cropping rectangular and skeyed.
 */

/**
 * This function crops an input image.
 * Note : (0,0) corresponds to the Bottom Left of an Image.
 *
 * @param img Input image struct to be cropped.
 * @param xorig X-Cordinate of offset of image to be extracted from.
 * @param yorig Y-Cordinate of offset of image to be extracted from.
 * @param xnum Legnth of the output image to be extracted from input
 * data in horizontal direction.
 * @param ynum Legnth of the output image to be extracted from input
 * data in vertical direction.
 * @return 0 on success.
 */

ICV_EXPORT extern int icv_rect(icv_image_t *img, int xorig, int yorig, int xnum, int ynum);

/**
 * This function crops an input image.
 * This can do a screwed cropping i.e given any four points of
 * quadrilateral in an image, Maps it to a rectangle of xnumXynum
 * dimension
 *
 *        (ulx,uly)         (urx,ury)
 *             __________________
 *            /                 |
 *           /                  |
 *          /                   |
 *         /                    |
 *        /                     |
 *       /______________________|
 *     (llx,lly)             (lrx,lry)
 *
 * @return 0 on success on failure -1 and logs the error message.
 */

ICV_EXPORT extern int icv_crop(icv_image_t *img,
			       int ulx, int uly,
			       int urx, int ury,
			       int lrx, int lry,
			       int llx, int lly,
			       unsigned int ynum,
			       unsigned int xnum);

/** @file libicv/filter.c
 *
 * This file contains routines for image filtering. This is done
 * mainly using the convolution of images. Both Gray Scale and RGB
 * images are taken care.
 *
 */

typedef enum {
    ICV_FILTER_LOW_PASS,
    ICV_FILTER_LAPLACIAN,
    ICV_FILTER_HORIZONTAL_GRAD,
    ICV_FILTER_VERTICAL_GRAD,
    ICV_FILTER_HIGH_PASS,
    ICV_FILTER_NULL,
    ICV_FILTER_BOXCAR_AVERAGE,
    ICV_FILTER_3_LOW_PASS,
    ICV_FILTER_3_HIGH_PASS,
    ICV_FILTER_3_BOXCAR_AVERAGE,
    ICV_FILTER_3_ANIMATION_SMEAR,
    ICV_FILTER_3_NULL
}ICV_FILTER;


/** @} */
/* end image utilities */

__END_DECLS

#endif /* __ICV_H__ */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
