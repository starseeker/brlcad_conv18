#==============================================================================
#
# TCL versions of MGED "helpdevel", "?devel", and "aproposdevel" commands
#
#==============================================================================

set mged_helpdevel_data(aip)		{{[fb]}	{advance illumination pointer or path position forward or backward}}
set mged_helpdevel_data(cmd_close)	{{id}	{close the command window represented by id}}
set mged_helpdevel_data(cmd_get)	{{}	{returns a list of ids associated with the current command window}}
set mged_helpdevel_data(cmd_init)	{{id}	{initialize a new command window whose name will be id}}
set mged_helpdevel_data(cmd_set)	{{id}	{set the current command window to id}}
set mged_helpdevel_data(get_comb)	{{comb_name}	{get information about combination}}
set mged_helpdevel_data(get_dm_list)	{{}	{returns a list of all display managers}}
set mged_helpdevel_data(get_edit_solid)	{{[-c]}	{get the solid parameters for the solid currently
        being edited}}
set mged_helpdevel_data(get_more_default)	{{}	{get the current default input value}}
set mged_helpdevel_data(grid2model_lu)	{{gx gy}	{given a point in grid coordinates (local units),
        convert it to model coordinates (local units).}}
set mged_helpdevel_data(grid2view_lu)	{{gx gy}	{given a point in grid coordinates (local units),
        convert it to view coordinates (local units).}}
set mged_helpdevel_data(gui_destroy)	{{id}	{destroy display/command window pair}}
set mged_helpdevel_data(hist_prev)	{{}	{returns previous command in history}}
set mged_helpdevel_data(hist_next)	{{}	{returns next command in history}}
set mged_helpdevel_data(hist_add)	{{[command]}	{adds command to the history (without executing it)}}
set mged_helpdevel_data(jcs)		{{id}	{join collaborative session}}
set mged_helpdevel_data(make_name)	{{template}	{make an object name not occuring in database}}
set mged_helpdevel_data(mged_update)	{{}	{handle outstanding events and refresh}}
set mged_helpdevel_data(mmenu_get)	{{index}	{get menu corresponding to index}}
set mged_helpdevel_data(mmenu_set)	{{w id i menu}	{this Tcl proc is used to set/install menu "i"}}
set mged_helpdevel_data(model2grid_lu)	{{mx my mz}	{convert point in model coords (local units)
        to grid coords (local units)}}
set mged_helpdevel_data(model2view)	{{mx my mz}	{convert point in model coords (mm) to view coords}}
set mged_helpdevel_data(model2view_lu)	{{mx my mz}	{convert point in model coords (local units) to view coords (local units)}}
set mged_helpdevel_data(output_hook)	{{[hook_cmd]}	{set up to have output from bu_log sent to hook_cmd}}
set mged_helpdevel_data(pcs)		{{}	{print collaborative participants}}
set mged_helpdevel_data(pmp)		{{}	{print mged players}}
set mged_helpdevel_data(put_comb)	{{comb_name is_Region id air material los color shader inherit boolean_expr} {set combination}}
set mged_helpdevel_data(put_edit_solid)	{{solid parameters}	{put the solid parameters into the in-memory (i.e. es_int)
        solid currently being edited.}}
set mged_helpdevel_data(qcs)		{{id}	{quit collaborative session}}
set mged_helpdevel_data(reset_edit_solid)	{{}	{reset the parameters for the currently edited solid (i.e. es_int)}}
set mged_helpdevel_data(rset)		{{[res_type [res [vals]]]}	{provides a mechanism to get/set resource values.}}
set mged_helpdevel_data(set_more_default)	{{more_default}	{set the current default input value}}
set mged_helpdevel_data(share)		{{resource dm1 dm2}	{dm1 (display manager 1) shares its resource with dm2}}
set mged_helpdevel_data(solids_on_ray)	{{h v}	{list all displayed solids along a ray}}
set mged_helpdevel_data(stuff_str)	{{string}	{sends a string to stdout while leaving the current command-line alone}}
set mged_helpdevel_data(svb)		{{}	{set view reference base}}
set mged_helpdevel_data(tie)		{{[[-u] cw [dm]]}	{ties/associates a command window (cw) with a display manager (dm)}}
set mged_helpdevel_data(view_ring)	{{cmd [i]}	{manipulates the view_ring for the current display manager}}
set mged_helpdevel_data(view2grid_lu)	{{vx vy vz}	{given a point in view coordinates (local units),
        convert it to grid coordinates (local units).}}
set mged_helpdevel_data(view2model)	{{vx vy vz}	{given a point in view (screen) space coordinates,
        convert it to model coordinates (in mm).}}
set mged_helpdevel_data(view2model_vec)	{{vx vy vz}	{given a vector in view coordinates,
        convert it to model coordinates.}}
set mged_helpdevel_data(view2model_lu)	{{vx vy vz}	{given a point in view coordinates (local units),
        convert it to model coordinates (local units).}}
set mged_helpdevel_data(viewget)	{{center|size|eye|ypr|quat|aet}	{Experimental - return high-precision view parameters.}}
set mged_helpdevel_data(viewset)	{{center|eye|size|ypr|quat|aet}	{Experimental - set several view parameters at once.}}
set mged_helpdevel_data(winset)		{{[pathname]}	{sets the current display manager to pathname}}

proc helpdevel {args} {
    global mged_helpdevel_data

    if {[llength $args] > 0} {
	return [help_comm mged_helpdevel_data $args]
    } else {
	return [help_comm mged_helpdevel_data]
    }
}

proc ?devel {} {
    global mged_helpdevel_data

    return [?_comm mged_helpdevel_data 20 4]
}

proc aproposdevel key {
    global mged_helpdevel_data

    return [apropos_comm mged_helpdevel_data $key]
}
