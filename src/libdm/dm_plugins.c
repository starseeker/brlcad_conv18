#include "common.h"
#include <string.h>

#include "bu/app.h"
#include "bu/dylib.h"
#include "bu/file.h"
#include "bu/log.h"
#include "bu/ptbl.h"
#include "bu/vls.h"

#include "dm.h"

int
dm_load_backends(struct bu_ptbl *plugins, struct bu_ptbl *handles)
{
    const char *ppath = bu_dir(NULL, 0, BU_DIR_LIBEXEC, "dm", NULL);
    char **filenames;
    const char *psymbol = "dm_plugin_info";
    struct bu_vls plugin_pattern = BU_VLS_INIT_ZERO;
    bu_vls_sprintf(&plugin_pattern, "*%s", DM_PLUGIN_SUFFIX);
    size_t nfiles = bu_file_list(ppath, bu_vls_cstr(&plugin_pattern), &filenames);
    for (size_t i = 0; i < nfiles; i++) {
	char pfile[MAXPATHLEN] = {0};
	bu_dir(pfile, MAXPATHLEN, BU_DIR_LIBEXEC, "dm", filenames[i], NULL);
	void *dl_handle, *info_val;
	if (!(dl_handle = bu_dlopen(pfile, BU_RTLD_NOW))) {
	    const char * const error_msg = bu_dlerror();
	    if (error_msg)
		bu_log("%s\n", error_msg);

	    bu_log("Unable to dynamically load '%s' (skipping)\n", pfile);
	    continue;
	}
	if (handles) {
	    bu_ptbl_ins(handles, (long *)dl_handle);
	}
	info_val = bu_dlsym(dl_handle, psymbol);
	const struct dm_plugin *(*plugin_info)() = (const struct dm_plugin *(*)())(intptr_t)info_val;
	if (!plugin_info) {
	    const char * const error_msg = bu_dlerror();

	    if (error_msg)
		bu_log("%s\n", error_msg);

	    bu_log("Unable to load symbols from '%s' (skipping)\n", pfile);
	    bu_log("Could not find '%s' symbol in plugin\n", psymbol);
	    continue;
	}

	const struct dm_plugin *plugin = plugin_info();

	if (!plugin || !plugin->p) {
	    bu_log("Invalid plugin encountered from '%s' (skipping)\n", pfile);
	    continue;
	}

	const struct dm *d = plugin->p;
	bu_ptbl_ins(plugins, (long *)d);
    }

    return BU_PTBL_LEN(plugins);
}

int
dm_close_backends(struct bu_ptbl *handles)
{
    int ret = 0;
    for (size_t i = 0; i < BU_PTBL_LEN(handles); i++) {
	if (bu_dlclose((void *)BU_PTBL_GET(handles, i))) {
	    ret = 1;
	}
    }

    return ret;
}

void
dm_list_backends()
{
    struct bu_ptbl plugins = BU_PTBL_INIT_ZERO;
    struct bu_ptbl handles = BU_PTBL_INIT_ZERO;
    int dm_cnt = dm_load_backends(&plugins, &handles);
    if (!dm_cnt) {
	bu_log("No display manager implementations found!\n");
	return;
    }

    for (size_t i = 0; i < BU_PTBL_LEN(&plugins); i++) {
	const struct dm *d = (const struct dm *)BU_PTBL_GET(&plugins, i);
	bu_log("Have backend: %s\n", dm_get_name(d));
    }
    bu_ptbl_free(&plugins);

    if (dm_close_backends(&handles)) {
	bu_log("bu_dlclose failed to unload plugins.\n");
    }
    bu_ptbl_free(&handles);
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
