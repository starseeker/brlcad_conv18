/*                        M A K E _ P N T S . C
 * BRL-CAD
 *
 * Copyright (c) 1985-2009 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */

/** @file make_pnts.c
 *
 * The "make_pnts" command makes a point-cloud from a points data file.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "bio.h"

#include "rtgeom.h"
#include "wdb.h"

#include "./ged_private.h"

#define INSERT_COORDINATE_INTO_STRUCTURE(_structure_type, _control_variable, _variable_to_insert) \
    switch (_control_variable) { \
        case 'x': \
            ((struct _structure_type *)point)->v[X] = _variable_to_insert; \
            break; \
        case 'y': \
            ((struct _structure_type *)point)->v[Y] = _variable_to_insert; \
            break; \
        case 'z': \
            ((struct _structure_type *)point)->v[Z] = _variable_to_insert; \
            break; \
    }

#define INSERT_COLOR_INTO_STRUCTURE(_structure_type, _control_variable, _variable_to_insert) \
    switch (_control_variable) { \
        case 'r': \
            ((struct _structure_type *)point)->c.buc_magic = BU_COLOR_MAGIC; \
            ((struct _structure_type *)point)->c.buc_rgb[0] = _variable_to_insert; \
            break; \
        case 'g': \
            ((struct _structure_type *)point)->c.buc_rgb[1] = _variable_to_insert; \
            break; \
        case 'b': \
            ((struct _structure_type *)point)->c.buc_rgb[2] = _variable_to_insert; \
            break; \
    }

#define INSERT_SCALE_INTO_STRUCTURE(_structure_type, _control_variable, _variable_to_insert) \
    switch (_control_variable) { \
        case 's': \
            ((struct _structure_type *)point)->s = _variable_to_insert; \
            break; \
    }

#define INSERT_NORMAL_INTO_STRUCTURE(_structure_type, _control_variable, _variable_to_insert) \
    switch (_control_variable) { \
        case 'i': \
            ((struct _structure_type *)point)->n[X] = _variable_to_insert; \
            break; \
        case 'j': \
            ((struct _structure_type *)point)->n[Y] = _variable_to_insert; \
            break; \
        case 'k': \
            ((struct _structure_type *)point)->n[Z] = _variable_to_insert; \
            break; \
    }

static char *p_make_pnts[] = {
    "Enter point-cloud name: ",
    "Enter point file path and name: ",
    "Enter file data format (xyzrgbsijk): ",
    "Enter file data units (um|mm|cm|m|km|in|ft|yd|mi)\nor conversion factor from file data units to millimeters: ",
    "Enter point size (>= 0.0, -1 for default)"
};


/*
 * Character compare function used by qsort function.
 */
int
compare_char(const char *a, const char *b)
{
    return (int)(*a - *b);
}


/*
 * Remove pre and post whitespace from a string. 
 */
void
remove_whitespace(char *input_string)
{
    char *idx = '\0';
    int idx2 = 0;
    int input_string_length = 0;
    char *firstp = '\0';
    char *lastp = '\0';
    int found_start = 0;
    int found_end = 0; 
    int cleaned_string_length = 0;

    input_string_length = strlen(input_string);
    firstp = input_string;
    /* initially lastp points to null at end of input string */
    lastp = firstp + input_string_length;
    /* test for zero length input_string, if zero then do nothing */
    if (input_string_length == 0) {
	return;
    }

    /* find start character and set pointer firstp to this character */
    found_start = 0;
    idx = firstp;
    while ((found_start == 0) && (idx < lastp)) {
	if (isspace(idx[0]) == 0) {
	    /* execute if non-space found */
	    found_start = 1;
	    firstp = idx;
	}
	idx++;
    }
    /* if found_start is false then string must be all whitespace */
    if (found_start == 0) {
	/* set null to first character a do nothing more */
	input_string[0] = '\0';
	return;
    }

    /* If found_start is true, check for trailing whitespace.  Find
     * last character and set pointer lastp to next character after,
     * i.e. where null would be.  There as at least one non-space
     * character in this string therefore will not need to deal with
     * an empty string condition in the loop looking for the string
     * end.
     */
    found_end = 0;
    idx = lastp - 1;
    while ((found_end == 0) && (idx >= firstp)) {
	if (isspace(idx[0]) == 0) {
	    /* execute if non-space found */
	    found_end = 1;
	    lastp = idx + 1;
	}
	idx--;
    }

    /* Test if characters in string need to be shifted left and then
     * do nothing more.
     */
    if (firstp > input_string) {
	/* Execute if need to shift left, this would happen only */
	/* if input_string contained pre whitspace. */
	cleaned_string_length = lastp - firstp;
	for (idx2 = 0; idx2 < cleaned_string_length; idx2++) {
	    input_string[idx2] = firstp[idx2];
	}
	input_string[cleaned_string_length] = '\0';
	return;
    }

    /* no need to shift left, set null to location of lastp */
    lastp[0] = '\0';
}


/*
 * Validate 'point file data format string', determine and output the
 * point-cloud type. A valid null terminated string is expected as input.
 * The function returns GED_ERROR if the format string is invalid or
 * if null pointers were passed to the function.
 */
int
str2type(char *format_string, rt_pnt_type *pnt_type)
{
    int format_string_length = 0;
    int index = 0;
    int index2 = 0;

    if ((format_string == (char *)NULL) || (pnt_type == (rt_pnt_type *)NULL)) {
        bu_log("ERROR: NULL pointer(s) passed to function 'str2type'.\n");
        return GED_ERROR;
    }

    format_string_length = strlen(format_string);
    char temp_string[format_string_length+1];

    /* remove any '?' from format string before testing for point-cloud type */
    for (index = 0 ; index < format_string_length ; index++) {
        if (format_string[index] != '?') {
            temp_string[index2] = format_string[index];
            index2++;
        }
    }
    temp_string[index2] = (char)NULL;

    remove_whitespace(temp_string);

    qsort(temp_string, strlen(temp_string), sizeof(char), (int (*)(const void *a, const void *b))compare_char);

    if (strcmp(temp_string, "xyz") == 0) {
        *pnt_type = RT_PNT_TYPE_PNT;
        bu_log("Set point-cloud type to 'RT_PNT_TYPE_PNT'.\n");
        bu_log("Sorted format string: '%s'\n", temp_string);
        return GED_OK;
    }

    if (strcmp(temp_string, "bgrxyz") == 0) {
        *pnt_type = RT_PNT_TYPE_COL;
        bu_log("Set point-cloud type to 'RT_PNT_TYPE_COL'.\n");
        bu_log("Sorted format string: '%s'\n", temp_string);
        return GED_OK;
    }

    if (strcmp(temp_string, "sxyz") == 0) {
        *pnt_type = RT_PNT_TYPE_SCA;
        bu_log("Set point-cloud type to 'RT_PNT_TYPE_SCA'.\n");
        bu_log("Sorted format string: '%s'\n", temp_string);
        return GED_OK;
    }

    if (strcmp(temp_string, "ijkxyz") == 0) {
        *pnt_type = RT_PNT_TYPE_NRM;
        bu_log("Set point-cloud type to 'RT_PNT_TYPE_NRM'.\n");
        bu_log("Sorted format string: '%s'\n", temp_string);
        return GED_OK;
    }

    if (strcmp(temp_string, "bgrsxyz") == 0) {
        *pnt_type = RT_PNT_TYPE_COL_SCA;
        bu_log("Set point-cloud type to 'RT_PNT_TYPE_COL_SCA'.\n");
        bu_log("Sorted format string: '%s'\n", temp_string);
        return GED_OK;
    }

    if (strcmp(temp_string, "bgijkrxyz") == 0) {
        *pnt_type = RT_PNT_TYPE_COL_NRM;
        bu_log("Set point-cloud type to 'RT_PNT_TYPE_COL_NRM'.\n");
        bu_log("Sorted format string: '%s'\n", temp_string);
        return GED_OK;
    }

    if (strcmp(temp_string, "ijksxyz") == 0) {
        *pnt_type = RT_PNT_TYPE_SCA_NRM;
        bu_log("Set point-cloud type to 'RT_PNT_TYPE_SCA_NRM'.\n");
        bu_log("Sorted format string: '%s'\n", temp_string);
        return GED_OK;
    }

    if (strcmp(temp_string, "bgijkrsxyz") == 0) {
        *pnt_type = RT_PNT_TYPE_COL_SCA_NRM;
        bu_log("Set point-cloud type to 'RT_PNT_TYPE_COL_SCA_NRM'.\n");
        bu_log("Sorted format string: '%s'\n", temp_string);
        return GED_OK;
    }

    bu_log("Invalid format string: '%s'\n", format_string);
    return GED_ERROR;
}


/*
 * Validate points data file unit string and output conversion factor to
 * milimeters. If string is not a standard units identifier, the function
 * assumes a custom conversion factor was specified. A valid null terminated
 * string is expected as input. The function returns GED_ERROR if the unit
 * string is invalid or if null pointers were passed to the function.
 */
int
str2mm(char *units_string, double *conv_factor)
{
    double tmp_value = 0.0;
    char *tmp_ptr = (char *)NULL;
    char *endp = (char *)NULL;
    int units_string_length = 0;

    if ((units_string == (char *)NULL) || (conv_factor == (double *)NULL)) {
        bu_log("ERROR: NULL pointer(s) passed to function 'str2mm'.\n");
        return GED_ERROR;
    }

    units_string_length = strlen(units_string);
    char temp_string[units_string_length+1];

    tmp_ptr = strcpy(temp_string, units_string);

    remove_whitespace(temp_string);

    tmp_value = strtod(temp_string, &endp);
    if ((temp_string != endp) && (*endp == '\0')) {
        /* convert to double success */
        *conv_factor = tmp_value;
        bu_log("Using custom conversion factor '%lf'\n", *conv_factor);
        bu_log("User entered units string: '%s'\n", units_string);
        return GED_OK;
    }

    if ((tmp_value = bu_mm_value(temp_string)) > 0.0) {
        *conv_factor = tmp_value;
        bu_log("Using units string '%s', conversion factor '%lf'\n", temp_string, *conv_factor);
        bu_log("User entered units string: '%s'\n", units_string);
        return GED_OK;
    }

    bu_log("Invalid units string: '%s'\n", units_string);
    return GED_ERROR;
}


void
report_import_error_location(unsigned long int num_doubles_read, int num_doubles_per_point, 
                                  unsigned long int start_offset_of_current_double, char field)
{
    /* The num_doubles_read is the number of doubles successfully read. This error
     * report is for the double where the error occurred, i.e. the next double.
     */
    unsigned long int point_number =  ceil((num_doubles_read + 1) / (double)num_doubles_per_point);

    bu_log("ERROR: Failed reading point %lu field %c at file byte offset %lu\n",
            point_number, field, start_offset_of_current_double);
}


int
ged_make_pnts(struct ged *gedp, int argc, const char *argv[])
{
    char *obj_name;
    char *filename;
    char *format_string;
    char *units_str;
    char *default_point_size_str;
#if 0
    struct rt_db_internal *intern;    /* old */
#endif
    struct directory *dp;
    struct rt_db_internal internal;   /* new from 3ptarb, allocates structure, not just pointer */

    rt_pnt_type type;
    double local2base;
    unsigned long numPoints = 0;
    struct rt_pnts_internal *pnts;
    double defaultSize = 0.0;
    void *headPoint;

    FILE *fp;

    int temp_string_index = 0; /* index into temp_string, set to first character in temp_string, i.e. the start */
    unsigned long int num_doubles_read = 0; /* counters of double read from file, use unsigned long int */

    int current_character_double = 0; /* flag indicating if current character read is part of a double or delimiter */
    int previous_character_double = 0; /* flag indicating if previously read character was part of a double or delimiter */

    unsigned long int num_characters_read_from_file = 0;  /* counter of number of characters read from file, use unsigned long int */
    unsigned long int start_offset_of_current_double = 0; /* character offset from start of file for current double, use unsigned long int */
    int found_double = 0; /* flag indicating if double encountered in file and needs to be processed */
    int found_eof = 0; /* flag indicating if end-of-file encountered when reading file */
    int done_processing_format_string = 0; /* flag indicating if loop processing format string should be exited */

    char *temp_char_ptr = (char *)NULL;
    int buf = 0; /* raw character read from file */
    double temp_double = 0.0;
    char temp_string[1024];
    int temp_string_size = 1024;  /* number of characters that can be stored in temp_string including null terminator character */
                                  /* it is expected that the character representation of a double will never exceed this size string */
    char *endp = (char *)NULL;

    int format_string_index = 0; /* index into format_string */
    int format_string_length = 0; /* number of characters in format_string, NOT including null terminator character */

    int num_doubles_per_point = 0;

    void *point;

    char **prompt; /* new from 3ptarb */

/* new from 3ptarb */
    static const char *usage = "point_cloud_name filename_with_path file_format file_data_units default_point_size";

    prompt = &p_make_pnts[0];

    GED_CHECK_DATABASE_OPEN(gedp, GED_ERROR);
    GED_CHECK_READ_ONLY(gedp, GED_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, GED_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    if (argc > 6) {
        bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
        return GED_ERROR;
    }

    /* prompt for point-cloud name */
    if (argc < 2) {
        bu_vls_printf(&gedp->ged_result_str, "%s", prompt[0]);
        return GED_MORE;
    }

    if ( db_lookup( gedp->ged_wdbp->dbip, argv[1], LOOKUP_QUIET) != DIR_NULL ) {
        bu_vls_printf(&gedp->ged_result_str, "%s: %s already exists\n", argv[0], argv[1]);
        return GED_ERROR;
    }

    /* prompt for data file name with path */
    if (argc < 3) {
        bu_vls_printf(&gedp->ged_result_str, "%s", prompt[1]);
        return GED_MORE;
    }

    /* prompt for data file format */
    if (argc < 4) {
        bu_vls_printf(&gedp->ged_result_str, "%s", prompt[2]);
        return GED_MORE;
    }

    /* prompt for data file units */
    if (argc < 5) {
        bu_vls_printf(&gedp->ged_result_str, "%s", prompt[3]);
        return GED_MORE;
    }

    /* prompt for default point size */
    if (argc < 6) {
        bu_vls_printf(&gedp->ged_result_str, "%s", prompt[4]);
        return GED_MORE;
    }

    obj_name = argv[1];
    filename = argv[2];
    format_string = argv[3];
    units_str = argv[4];
    default_point_size_str = argv[5];

#if 0
argv[0] command name (i.e. make_pnts)
argv[1] name of point-cloud
argv[2] data file name with path
argv[3] data file format 
argv[4] data file units
argv[5] default point size
#endif

/* start here 6/26/09 */

    format_string_length = strlen(format_string);

    bu_log("the format string length is: '%i'\n", format_string_length);


    bu_log("Entered function 'ged_make_pnts'.\n");

    bu_log("filename string: '%s'\n", filename);
    bu_log("format string: '%s'\n", format_string);
    bu_log("units string: '%s'\n", units_str);
    bu_log("default point size string: '%s'\n", default_point_size_str);

    defaultSize = atof(default_point_size_str);
    if (defaultSize < 0.0) {
        defaultSize = 0.0;
        bu_log("WARNING: default point size must be non-negative, using zero\n");
    }

    /* Validate 'point file data format string' and return the
     * point-cloud type.
     */
    if (str2type(format_string, &type) == GED_ERROR) {
        return GED_ERROR;
    }

    /* Validate the unit string and return the converion factor
     * to millimeters.
     */
    if (str2mm(units_str, &local2base) == GED_ERROR) {
        return GED_ERROR;
    }

    remove_whitespace(format_string);

    /* init database structure */
    RT_INIT_DB_INTERNAL( &internal );  /* new from 3ptarb */
    internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
    internal.idb_type = ID_PNTS;
    internal.idb_meth = &rt_functab[ID_PNTS];
    internal.idb_ptr = (genptr_t) bu_malloc(sizeof(struct rt_pnts_internal), "rt_pnts_internal");

    /* init internal structure */
    pnts = (struct rt_pnts_internal *) internal.idb_ptr;
    pnts->magic = RT_PNTS_INTERNAL_MAGIC;
    pnts->scale = defaultSize;
    pnts->type = type;
    pnts->count = numPoints;  /* set again later */
    pnts->point = NULL;


    /* empty list head */
    switch (type) {
	case RT_PNT_TYPE_PNT:
	    BU_GETSTRUCT(headPoint, pnt);
	    BU_LIST_INIT(&(((struct pnt *)headPoint)->l));
            num_doubles_per_point = 3;
	    break;
	case RT_PNT_TYPE_COL:
	    BU_GETSTRUCT(headPoint, pnt_color);
	    BU_LIST_INIT(&(((struct pnt_color *)headPoint)->l));
            num_doubles_per_point = 6;
	    break;
	case RT_PNT_TYPE_SCA:
	    BU_GETSTRUCT(headPoint, pnt_scale);
	    BU_LIST_INIT(&(((struct pnt_scale *)headPoint)->l));
            num_doubles_per_point = 4;
	    break;
	case RT_PNT_TYPE_NRM:
	    BU_GETSTRUCT(headPoint, pnt_normal);
	    BU_LIST_INIT(&(((struct pnt_normal *)headPoint)->l));
            num_doubles_per_point = 6;
	    break;
	case RT_PNT_TYPE_COL_SCA:
	    BU_GETSTRUCT(headPoint, pnt_color_scale);
	    BU_LIST_INIT(&(((struct pnt_color_scale *)headPoint)->l));
            num_doubles_per_point = 7;
	    break;
	case RT_PNT_TYPE_COL_NRM:
	    BU_GETSTRUCT(headPoint, pnt_color_normal);
	    BU_LIST_INIT(&(((struct pnt_color_normal *)headPoint)->l));
            num_doubles_per_point = 9;
	    break;
	case RT_PNT_TYPE_SCA_NRM:
	    BU_GETSTRUCT(headPoint, pnt_scale_normal);
	    BU_LIST_INIT(&(((struct pnt_scale_normal *)headPoint)->l));
            num_doubles_per_point = 7;
	    break;
	case RT_PNT_TYPE_COL_SCA_NRM:
	    BU_GETSTRUCT(headPoint, pnt_color_scale_normal);
	    BU_LIST_INIT(&(((struct pnt_color_scale_normal *)headPoint)->l));
            num_doubles_per_point = 10;
	    break;
    }    
    pnts->point = headPoint;

    bu_log("About to execute 'open file'.\n");

    if ((fp=fopen(filename, "rb")) == NULL) {
        bu_log("ERROR: Could not open file '%s'\n", filename);
        /* may need to do some cleanup before exit here on error */
        return GED_ERROR;
    }

    while( !found_eof ) {  /* points_loop */
        /* allocate memory for single point structure for current point-cloud type */
        switch (type) {
            case RT_PNT_TYPE_PNT:
                BU_GETSTRUCT(point, pnt);
                break;
            case RT_PNT_TYPE_COL:
                BU_GETSTRUCT(point, pnt_color);
                break;
            case RT_PNT_TYPE_SCA:
                BU_GETSTRUCT(point, pnt_scale);
                break;
            case RT_PNT_TYPE_NRM:
                BU_GETSTRUCT(point, pnt_normal);
                break;
            case RT_PNT_TYPE_COL_SCA:
                BU_GETSTRUCT(point, pnt_color_scale);
                break;
            case RT_PNT_TYPE_COL_NRM:
                BU_GETSTRUCT(point, pnt_color_normal);
                break;
            case RT_PNT_TYPE_SCA_NRM:
                BU_GETSTRUCT(point, pnt_scale_normal);
                break;
            case RT_PNT_TYPE_COL_SCA_NRM:
                BU_GETSTRUCT(point, pnt_color_scale_normal);
                break;
        }    

        while( !found_eof && !done_processing_format_string ) {   /* format_string_loop */

            while( !found_eof  && !found_double ) {  /* find_doubles_loop */

                buf = fgetc(fp);

                num_characters_read_from_file++;

                if (feof(fp)) {
                    if (ferror(fp)) {
                        perror("ERROR: Problem reading file, system error message");
                        bu_log("ERROR: Unable to read file at byte '%d'.\n", num_characters_read_from_file);
                        fclose(fp);
                        /* do structure deallocate? */
                        return GED_ERROR;
                    } else {
                        found_eof = 1;
                        bu_log("End-of-file encountered.\n");
                    }
                }

                if (found_eof) {
                    fclose(fp);
                    current_character_double = 0;
                    if (num_doubles_read == 0) {
                        bu_log("ERROR: No data in file.\n");
                        /* do structure deallocate? */
                        return GED_ERROR;
                    }
                } else {
                    if ((temp_char_ptr = strchr("0123456789.+-eE", buf)) != NULL) {
                        /* character read is part of a double */
                        current_character_double = 1;
                    } else {
                        current_character_double = 0;
                    }
                }

                /* case logic 1 */
                if (previous_character_double && current_character_double) {
#if 0
                    bu_log("processing case logic 1 --- previous_character_double='%i' current_character_double='%i'\n",
                           previous_character_double, current_character_double);
#endif
                    if (temp_string_index >= temp_string_size) {
                        bu_log("ERROR: String representing double too large, exceeds '%d' character limit.\n", temp_string_size - 1);
                        report_import_error_location(num_doubles_read, num_doubles_per_point, start_offset_of_current_double,
                                                          format_string[format_string_index]);
                        /* do structure deallocate? */
                        return GED_ERROR;
                    }
                    temp_string[temp_string_index] = (char)buf;
                    temp_string_index++;
                }

                /* case logic 2 */
                if (previous_character_double && !current_character_double) {
#if 0
                    bu_log("processing case logic 2 --- previous_character_double='%i' current_character_double='%i'\n",
                           previous_character_double, current_character_double);
#endif
                    if (temp_string_index >= temp_string_size) {
                        bu_log("ERROR: String representing double too large, exceeds '%d' character limit.\n", temp_string_size - 1);
                        report_import_error_location(num_doubles_read, num_doubles_per_point, start_offset_of_current_double,
                                                          format_string[format_string_index]);
                        /* do structure deallocate? */
                        return GED_ERROR;
                    }
                    temp_string[temp_string_index] = (char)NULL;

                    temp_double = strtod(temp_string, &endp);
                    if (!((temp_string != endp) && (*endp == '\0'))) {
                        bu_log("ERROR: Unable to convert string '%s' to double.\n", temp_string);
                        report_import_error_location(num_doubles_read, num_doubles_per_point, start_offset_of_current_double,
                                                          format_string[format_string_index]);
                        /* do structure deallocate? */
                        return GED_ERROR;
                    }
                    num_doubles_read++;
                    temp_string_index = 0;
                    found_double = 1;
                    previous_character_double = current_character_double;
                }

#if 0
                /* logic not needed, 'case logic 2' handles this case */
                /* case logic 3 */
                if (previous_character_double && found_eof) {
                    bu_log("processing case logic 3 --- previous_character_double='%i' found_eof='%i'\n",
                           previous_character_double, found_eof);
                    if (temp_string_index >= temp_string_size) {
                        bu_log("ERROR: String representing double too large, exceeds '%d' character limit.\n", temp_string_size - 1);
                        report_import_error_location(num_doubles_read, num_doubles_per_point, start_offset_of_current_double);
                        /* do structure deallocate? */
                        return GED_ERROR;
                    }
                    temp_string[temp_string_index] = (char)NULL;
                    num_doubles_read++;

                    temp_double = strtod(temp_string, &endp);
                    if (!((temp_string != endp) && (*endp == '\0'))) {
                        bu_log("ERROR: Unable to convert string '%s' to double.\n", temp_string);
                        report_import_error_location(num_doubles_read, num_doubles_per_point, start_offset_of_current_double);
                        /* do structure deallocate? */
                        return GED_ERROR;
                    }
                    found_double = 1;
                }
#endif

                /* case logic 5 */
                if (!previous_character_double && current_character_double) {
#if 0
                    bu_log("processing case logic 5 --- previous_character_double='%i' current_character_double='%i'\n",
                           previous_character_double, current_character_double);
#endif
                    temp_string[temp_string_index] = (char)buf;
                    temp_string_index++;
                    start_offset_of_current_double = num_characters_read_from_file;
                    previous_character_double = current_character_double;
                }

            } /* loop exits when eof encounted (and/or) double found */

            if (found_double) {
                /* insert double into point structure for current point-cloud type */
                switch (type) {
                    case RT_PNT_TYPE_PNT:
                        INSERT_COORDINATE_INTO_STRUCTURE(pnt, format_string[format_string_index], (temp_double * local2base))
                        break;
                    case RT_PNT_TYPE_COL:
                        INSERT_COORDINATE_INTO_STRUCTURE(pnt_color, format_string[format_string_index], (temp_double * local2base))
                        INSERT_COLOR_INTO_STRUCTURE(pnt_color, format_string[format_string_index], temp_double)
                        break;
                    case RT_PNT_TYPE_SCA:
                        INSERT_COORDINATE_INTO_STRUCTURE(pnt_scale, format_string[format_string_index], (temp_double * local2base))
                        INSERT_SCALE_INTO_STRUCTURE(pnt_scale, format_string[format_string_index], (temp_double * local2base))
                        break;
                    case RT_PNT_TYPE_NRM:
                        INSERT_COORDINATE_INTO_STRUCTURE(pnt_normal, format_string[format_string_index], (temp_double * local2base))
                        INSERT_NORMAL_INTO_STRUCTURE(pnt_normal, format_string[format_string_index], (temp_double * local2base))
                        break;
                    case RT_PNT_TYPE_COL_SCA:
                        INSERT_COORDINATE_INTO_STRUCTURE(pnt_color_scale, format_string[format_string_index], (temp_double * local2base))
                        INSERT_COLOR_INTO_STRUCTURE(pnt_color_scale, format_string[format_string_index], temp_double)
                        INSERT_SCALE_INTO_STRUCTURE(pnt_color_scale, format_string[format_string_index], (temp_double * local2base))
                        break;
                    case RT_PNT_TYPE_COL_NRM:
                        INSERT_COORDINATE_INTO_STRUCTURE(pnt_color_normal, format_string[format_string_index], (temp_double * local2base))
                        INSERT_COLOR_INTO_STRUCTURE(pnt_color_normal, format_string[format_string_index], temp_double)
                        INSERT_NORMAL_INTO_STRUCTURE(pnt_color_normal, format_string[format_string_index], (temp_double * local2base))
                        break;
                    case RT_PNT_TYPE_SCA_NRM:
                        INSERT_COORDINATE_INTO_STRUCTURE(pnt_scale_normal, format_string[format_string_index], (temp_double * local2base))
                        INSERT_SCALE_INTO_STRUCTURE(pnt_scale_normal, format_string[format_string_index], (temp_double * local2base))
                        INSERT_NORMAL_INTO_STRUCTURE(pnt_scale_normal, format_string[format_string_index], (temp_double * local2base))
                        break;
                    case RT_PNT_TYPE_COL_SCA_NRM:
                        INSERT_COORDINATE_INTO_STRUCTURE(pnt_color_scale_normal, format_string[format_string_index], (temp_double * local2base))
                        INSERT_COLOR_INTO_STRUCTURE(pnt_color_scale_normal, format_string[format_string_index], temp_double)
                        INSERT_SCALE_INTO_STRUCTURE(pnt_color_scale_normal, format_string[format_string_index], (temp_double * local2base))
                        INSERT_NORMAL_INTO_STRUCTURE(pnt_color_scale_normal, format_string[format_string_index], (temp_double * local2base))
                        break;
                }    
                found_double = 0;  /* allows loop to continue */
                format_string_index++;
                if (format_string_index >= format_string_length) {
                    done_processing_format_string = 1;
                }
            }

        } /* loop exits when eof encountered (and/or) all doubles for a single point are stored in point structure */

        if (done_processing_format_string) {
            /* push single point structure onto linked-list of points which makeup the point-cloud */
            switch (type) {
                case RT_PNT_TYPE_PNT:
                    BU_LIST_PUSH(&(((struct pnt *)headPoint)->l), &((struct pnt *)point)->l);
                    break;
                case RT_PNT_TYPE_COL:
                    BU_LIST_PUSH(&(((struct pnt_color *)headPoint)->l), &((struct pnt_color *)point)->l);
                    break;
                case RT_PNT_TYPE_SCA:
                    BU_LIST_PUSH(&(((struct pnt_scale *)headPoint)->l), &((struct pnt_scale *)point)->l);
                    break;
                case RT_PNT_TYPE_NRM:
                    BU_LIST_PUSH(&(((struct pnt_normal *)headPoint)->l), &((struct pnt_normal *)point)->l);
                    break;
                case RT_PNT_TYPE_COL_SCA:
                    BU_LIST_PUSH(&(((struct pnt_color_scale *)headPoint)->l), &((struct pnt_color_scale *)point)->l);
                    break;
                case RT_PNT_TYPE_COL_NRM:
                    BU_LIST_PUSH(&(((struct pnt_color_normal *)headPoint)->l), &((struct pnt_color_normal *)point)->l);
                    break;
                case RT_PNT_TYPE_SCA_NRM:
                    BU_LIST_PUSH(&(((struct pnt_scale_normal *)headPoint)->l), &((struct pnt_scale_normal *)point)->l);
                    break;
                case RT_PNT_TYPE_COL_SCA_NRM:
                    BU_LIST_PUSH(&(((struct pnt_color_scale_normal *)headPoint)->l), &((struct pnt_color_scale_normal *)point)->l);
                    break;
            }    
            numPoints++;
            format_string_index = 0;
            done_processing_format_string = 0;
        }

    } /* loop exits when eof encountered */

    if (num_doubles_read < num_doubles_per_point) {
        bu_log("ERROR: Number of values read inconsistent with point-cloud type.\n");
        /* do structure deallocate? */
        return GED_ERROR;  /* aborts point-cloud creation */
    }

    if (num_doubles_read % num_doubles_per_point) {
        bu_log("ERROR: Number of values read inconsistent with point-cloud type.\n");
        /* do structure deallocate? */
        return GED_ERROR;  /* aborts point-cloud creation */
    }

    bu_log("Number of doubles read: '%lu'\n", num_doubles_read);

    pnts->count = numPoints;

    GED_DB_DIRADD(gedp, dp, argv[1], -1L, 0, DIR_SOLID, (genptr_t)&internal.idb_type, GED_ERROR);
    GED_DB_PUT_INTERNAL(gedp, dp, &internal, &rt_uniresource, GED_ERROR);

    bu_log("About to exit function 'ged_make_pnts'.\n");

    return GED_OK;
}


/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */


