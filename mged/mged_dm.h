/*
 *			D M . H
 *
 * Header file for communication with the display manager.
 *  
 * Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 *
 *  $Header$
 */

#include "rtstring.h"		/* for vls string support */

struct device_values  {
	struct rt_vls	dv_string;	/* newline-separated "commands" from dm */
};
extern struct device_values dm_values;

/* defined in ged.c */
extern int dm_pipe[];

/* defined in dm-ps.c */
extern FILE *ps_fp;

#define SL_TOL 0.031265266  /* size of dead spot - 64/2047 */

#define VIRTUAL_TRACKBALL_OFF 0
#define VIRTUAL_TRACKBALL_ON 1
#define VIRTUAL_TRACKBALL_ROTATE 2 
#define VIRTUAL_TRACKBALL_TRANSLATE 3
#define VIRTUAL_TRACKBALL_ZOOM 4

/* Interface to a specific Display Manager */
struct dm {
	int	(*dmr_open)();
	void	(*dmr_close)();
	void	(*dmr_input)MGED_ARGS((fd_set *input, int noblock));
	void	(*dmr_prolog)();
	void	(*dmr_epilog)();
	void	(*dmr_normal)();
	void	(*dmr_newrot)();
	void	(*dmr_update)();
	void	(*dmr_puts)();
	void	(*dmr_2d_line)();
	void	(*dmr_light)();
	int	(*dmr_object)();	/* Invoke an object subroutine */
	unsigned (*dmr_cvtvecs)();	/* returns size requirement of subr */
	unsigned (*dmr_load)();		/* DMA the subr to device */
	void	(*dmr_statechange)();	/* called on editor state change */
	void	(*dmr_viewchange)();	/* add/drop solids from view */
	void	(*dmr_colorchange)();	/* called when color table changes */
	void	(*dmr_window)();	/* Change window boundry */
	void	(*dmr_debug)();		/* Set DM debug level */
	int	dmr_displaylist;	/* !0 means device has displaylist */
	int	dmr_releasedisplay;	/* !0 release for other programs */
	double	dmr_bound;		/* zoom-in limit */
	char	*dmr_name;		/* short name of device */
	char	*dmr_lname;		/* long name of device */
	struct mem_map *dmr_map;	/* displaylist mem map */
	int	(*dmr_cmd)();		/* dm-specific cmds to perform */
};

struct shared_info {
  fastf_t _Viewscale;
  fastf_t _i_Viewscale;
  mat_t   _Viewrot;
  mat_t   _toViewcenter;
  mat_t   _model2view;
  mat_t   _view2model;
  mat_t   _model2objview;
  mat_t   _objview2model;
  struct _mged_variables _mged_variables;

/* Angle/distance cursor stuff */
  int     _dv_xadc;
  int     _dv_yadc;
  int     _dv_1adc;
  int     _dv_2adc;
  int     _dv_distadc;
  fastf_t _curs_x;
  fastf_t _curs_y;
  fastf_t _c_tdist;
  fastf_t _angle1;
  fastf_t _angle2;

/* Rate stuff */
  int     _rateflag_slew;
  vect_t  _rate_slew;

  int     _rateflag_rotate;
  vect_t  _rate_rotate;
	
  int     _rateflag_zoom;
  fastf_t  _rate_zoom;

/* Absolute stuff */
  vect_t  _absolute_slew;
  vect_t  _absolute_rotate;
  fastf_t  _absolute_zoom;

/* Virtual trackball stuff */
  point_t _orig_pos;

/* Slider stuff */
  int _scroll_top;
  int _scroll_active;
  int _scroll_y;
  struct scroll_item *_scroll_array[6];

  int _rot_set;
  int _tran_set;
  int _dmaflag;
  int _rc;         /* reference count */
};


struct dm_list {
  struct rt_list l;
  struct dm *_dmp;

/* New stuff to allow more than one active display manager */
  struct shared_info *s_info;  /* info that can be used by display managers that are tied */
  char *_dm_vars;   /* pointer to dependant display manager variables */
  struct rt_vls _pathName; /* full name of drawing window */
  int _dirty;      /* true if received an expose or configuration event */
  int _owner;      /* true if owner of the shared info */
  struct cmd_list *aim;
  char _dname[80];  /* Display name */
  void (*_knob_offset_hook)();
  void (*_axis_color_hook)();
};

extern int update_views;   /* from dm-X.h */
extern struct dm_list head_dm_list;  /* list of active display managers */
extern struct dm_list *curr_dm_list;

#define DM_LIST_NULL ((struct dm_list *)NULL)
#define dmp curr_dm_list->_dmp
#define dm_vars curr_dm_list->_dm_vars
#define pathName curr_dm_list->_pathName
#define dirty curr_dm_list->_dirty
#define owner curr_dm_list->_owner
#define dname curr_dm_list->_dname
#define knob_offset_hook curr_dm_list->_knob_offset_hook
#define axis_color_hook curr_dm_list->_axis_color_hook

#define mged_variables curr_dm_list->s_info->_mged_variables

#define curs_x curr_dm_list->s_info->_curs_x
#define curs_y curr_dm_list->s_info->_curs_y
#define c_tdist curr_dm_list->s_info->_c_tdist
#define angle1 curr_dm_list->s_info->_angle1
#define angle2 curr_dm_list->s_info->_angle2
#define dv_xadc curr_dm_list->s_info->_dv_xadc
#define dv_yadc curr_dm_list->s_info->_dv_yadc
#define dv_1adc curr_dm_list->s_info->_dv_1adc
#define dv_2adc curr_dm_list->s_info->_dv_2adc
#define dv_distadc curr_dm_list->s_info->_dv_distadc

#define rateflag_slew curr_dm_list->s_info->_rateflag_slew
#define rate_slew curr_dm_list->s_info->_rate_slew
#define absolute_slew curr_dm_list->s_info->_absolute_slew
#define rateflag_rotate curr_dm_list->s_info->_rateflag_rotate
#define rate_rotate curr_dm_list->s_info->_rate_rotate
#define absolute_rotate curr_dm_list->s_info->_absolute_rotate
#define rateflag_zoom curr_dm_list->s_info->_rateflag_zoom
#define rate_zoom curr_dm_list->s_info->_rate_zoom
#define absolute_zoom curr_dm_list->s_info->_absolute_zoom

#define Viewscale curr_dm_list->s_info->_Viewscale
#define i_Viewscale curr_dm_list->s_info->_i_Viewscale
#define Viewrot curr_dm_list->s_info->_Viewrot
#define toViewcenter curr_dm_list->s_info->_toViewcenter
#define model2view curr_dm_list->s_info->_model2view
#define view2model curr_dm_list->s_info->_view2model
#define model2objview curr_dm_list->s_info->_model2objview
#define objview2model curr_dm_list->s_info->_objview2model

#define rot_x curr_dm_list->s_info->_rot_x
#define rot_y curr_dm_list->s_info->_rot_y
#define rot_z curr_dm_list->s_info->_rot_z
#define tran_x curr_dm_list->s_info->_tran_x
#define tran_y curr_dm_list->s_info->_tran_y
#define tran_z curr_dm_list->s_info->_tran_z
#define orig_pos curr_dm_list->s_info->_orig_pos

#define rot_set curr_dm_list->s_info->_rot_set
#define tran_set curr_dm_list->s_info->_tran_set
#define dmaflag curr_dm_list->s_info->_dmaflag
#define rc curr_dm_list->s_info->_rc

#define scroll_top curr_dm_list->s_info->_scroll_top
#define scroll_active curr_dm_list->s_info->_scroll_active
#define scroll_y curr_dm_list->s_info->_scroll_y
#define scroll_array curr_dm_list->s_info->_scroll_array

#define VIRTUAL_TRACKBALL_NOT_ACTIVE(_type,_name)\
  ((_type)dm_vars)->_name == VIRTUAL_TRACKBALL_OFF ||\
  ((_type)dm_vars)->_name == VIRTUAL_TRACKBALL_ON

/*
 * Definitions for dealing with the buttons and lights.
 * BV are for viewing, and BE are for editing functions.
 */
#define LIGHT_OFF	0
#define LIGHT_ON	1
#define LIGHT_RESET	2		/* all lights out */

/* Function button/light codes.  Note that code 0 is reserved */
#define BV_TOP		15+16
#define BV_BOTTOM	14+16
#define BV_RIGHT	13+16
#define BV_LEFT		12+16
#define BV_FRONT	11+16
#define BV_REAR		10+16
#define BV_VRESTORE	9+16
#define BV_VSAVE	8+16
#define BE_O_ILLUMINATE	7+16
#define BE_O_SCALE	6+16
#define BE_O_X		5+16
#define BE_O_Y		4+16
#define BE_O_XY		3+16
#define BE_O_ROTATE	2+16
#define BE_ACCEPT	1+16
#define BE_REJECT	0+16

#define BV_SLICEMODE	15
#define BE_S_EDIT	14
#define BE_S_ROTATE	13
#define BE_S_TRANS	12
#define BE_S_SCALE	11
#define BE_MENU		10
#define BV_ADCURSOR	9
#define BV_RESET	8
#define BE_S_ILLUMINATE	7
#define BE_O_XSCALE	6
#define BE_O_YSCALE	5
#define BE_O_ZSCALE	4
#define BV_ZOOM_IN	3
#define BV_ZOOM_OUT	2
#define BV_45_45	1
#define BV_35_25	0+32

#define BV_RATE_TOGGLE	1+32

#define BV_MAXFUNC	64	/* largest code used */

/*  Colors */

#define DM_BLACK	0
#define DM_RED		1
#define DM_BLUE		2
#define DM_YELLOW	3
#define DM_WHITE	4

/* Command parameter to dmr_viewchange() */
#define DM_CHGV_REDO	0	/* Display has changed substantially */
#define DM_CHGV_ADD	1	/* Add an object to the display */
#define DM_CHGV_DEL	2	/* Delete an object from the display */
#define DM_CHGV_REPL	3	/* Replace an object */
#define DM_CHGV_ILLUM	4	/* Make new object the illuminated object */
