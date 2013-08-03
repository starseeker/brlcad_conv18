/*                      D E C I M A T E . C
 * BRL-CAD
 *
 * Copyright (c) 2013 United States Government as represented by
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
/** @file decimate.c
 *
 * This file contains routines to scale down an image to a lower
 * resolution.
 *
 */

#include "bu.h"
#include "icv.h"
#include "vmath.h"

HIDDEN void shrink_image(icv_image_t* bif, int factor)
{
    double *data_p, *res_p; /**< input and output pointers */
    double *p;
    int facsq,x,y,py,px,c;
    size_t widthstep =  bif->width*bif->channels;

    facsq = factor*factor,c;
    res_p = bif->data;
    p = bu_malloc(bif->channels*sizeof(double), "shrink_image : Pixel Values Temp Buffer");

    for (y=0; y<bif->height; y+=factor)
        for (x=0; x<bif->width; x+=factor) {

            for (c=0; c<bif->channels; c++) {
                p[c]= 0;
            }

            for (py = 0; py < factor; py++) {
                data_p = bif->data + (y+py)*widthstep;
                for (px = 0; px < factor; px++) {
                    for (c=0; c<bif->channels; c++) {
                        p[c] += *data_p++;
                    }
                }
            }

            for (c=0; c<bif->channels; c++)
                *res_p++ = p[c]/facsq;
        }

    bif->width = (int) bif->width/factor;
    bif->height = (int) bif->height/factor;
    bif->data = bu_realloc(bif->data, (size_t) (bif->width*bif->height*bif->channels), "shrink_image : Reallocation");

    return;

}

HIDDEN void under_sample(icv_image_t* bif, int factor)
{
    double *data_p, *res_p;
    int x,y,widthstep;

    widthstep = bif->width*bif->channels;

    res_p = data_p = bif->data;

    for (y=0; y<bif->height; y+=factor) {
        data_p = bif->data + widthstep*y;
        for (x=0; x<bif->width; x+=factor, res_p+=bif->channels, data_p+=factor*bif->channels)
            VMOVEN(res_p,data_p, bif->channels);
    }

    bif->width = (int) bif->width/factor;
    bif->height = (int) bif->height/factor;
    bif->data = bu_realloc(bif->data, (size_t) (bif->width*bif->height*bif->channels), "under_sample : Reallocation");

    return;
}

HIDDEN void nintrep(icv_image_t* bif, int out_width, int out_height)
{
    double xstep, ystep;
    int i,j;
    int x,y;
    int widthstep;
    double *in_r, *in_c; /*<< Pointer to row and col of input buffers*/
    double *out_data, *out_p;
    xstep = (double) (bif->width-1) / (double) (out_width) - 1.0e-06;
    ystep = (double) (bif->height-1) / (double) (out_height) - 1.0e-06;

    out_p = out_data = bu_malloc(out_width*out_height*sizeof(double), "intrep : out_data");

    widthstep= bif->width*bif->channels;

    for (j=0; j<out_height; j++) {
        y = (int) (j*ystep);

        in_r = bif->data + y*widthstep;

        for (i = 0; i < out_width; i++) {
            x =  (int) (i*xstep);

            in_c = in_r + x*bif->channels;

            VMOVEN(out_p, in_c, bif->channels);
            out_p += bif->channels;
        }
    }

    bu_free(bif->data, "intrep : in_data");

    bif->data = out_data;

    bif->width = out_width;
    bif->height = out_height;

    return;

}
HIDDEN void binterp(icv_image_t *bif, int out_width, int out_height)
{
    int i, j, c;
    double x, y, dx, dy, mid1, mid2;
    double xstep, ystep;
    double *out_data, *out_p;
    double *upp_r, *low_r; /*<< upper and lower row */
    double *upp_c, *low_c;
    int widthstep;

    xstep = (double) (bif->width - 1) / (double)out_width - 1.0e-6;
    ystep = (double) (bif->height -1) / (double)out_height - 1.0e-6;

    out_p = out_data = bu_malloc(out_width*out_height*bif->channels, "binterp : out data");

    widthstep = bif->width*bif->channels;

    for (j = 0; j < out_height; j++) {
        y = j*ystep;
        dy = y - (int)y;

        low_r = bif->data + widthstep* (int)y;
        upp_r = bif->data + widthstep* (int) (y+1);

        for (i = 0; i < out_width; i++) {
            x = i*xstep;
            dx = x - (int)x;

            upp_c = upp_r + (int)x*bif->channels;
            low_c = low_r + (int)x*bif->channels;

            for(c=0; c<bif->channels; c++) {
                mid1 = low_c[0] + dx * ((double) low_c[bif->channels] - (double) low_c[0] );
                mid2 = upp_c[0] + dx * ((double) upp_c[bif->channels] - (double) upp_c[0] );
                *out_p = mid1 + dy * (mid2 - mid1);

                out_p++;
                upp_c++;
                low_c++;
            }
        }
    }
    bu_free(bif->data, "binterep : Input Data");
    bif->data = out_data;
    bif->widht = out_width;
    bif->height = bif->height;
}

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
