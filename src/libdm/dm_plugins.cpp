#include "common.h"
#include <string.h>

#include "bu/app.h"
#include "bu/dylib.h"
#include "bu/file.h"
#include "bu/log.h"
#include "bu/ptbl.h"
#include "bu/vls.h"

#include "dm.h"
#include "./include/private.h"

extern "C" struct dm *
dm_open(void *interp, const char *type, int argc, const char *argv[])
{
    if (BU_STR_EQUIV(type, "nu")) {
	return dm_null.i->dm_open(interp, argc, argv);
    }
    if (BU_STR_EQUIV(type, "null")) {
	return dm_null.i->dm_open(interp, argc, argv);
    }
    std::map<std::string, const struct dm *> *dmb = (std::map<std::string, const struct dm *> *)dm_backends;
    std::map<std::string, const struct dm *>::iterator d_it = dmb->find(std::string(type));
    if (d_it == dmb->end()) {
	return DM_NULL;
    }
    const struct dm *d = d_it->second;
    struct dm *dmp = d->i->dm_open(interp, argc, argv);
    return dmp;
}

extern "C" struct bu_vls *
dm_list_types(const char *separator)
{
    struct bu_vls *list;
    BU_GET(list, struct bu_vls);
    bu_vls_init(list);
    bu_vls_trunc(list, 0);

    // We've got something, and may need a separator
    struct bu_vls sep = BU_VLS_INIT_ZERO;
    if (!separator) {
	bu_vls_sprintf(&sep, " ");
    } else {
	bu_vls_sprintf(&sep, "%s", separator);
    }

    std::map<std::string, const struct dm *> *dmb = (std::map<std::string, const struct dm *> *)dm_backends;
    std::map<std::string, const struct dm *>::iterator d_it;
    for (d_it = dmb->begin(); d_it != dmb->end(); d_it++) {
	std::string key = d_it->first;
	const struct dm *d = d_it->second;
	if (strlen(bu_vls_cstr(list)) > 0) bu_vls_printf(list, "%s", bu_vls_cstr(&sep));
	bu_vls_printf(list, "%s", dm_get_name(d));
    }
    if (strlen(bu_vls_cstr(list)) > 0) bu_vls_printf(list, "%s", bu_vls_cstr(&sep));
    bu_vls_printf(list, "nu");

    return list;
}

extern "C" int
dm_validXType(const char *dpy_string, const char *name)
{
    if (BU_STR_EQUIV(name, "nu")) {
	return 1;
    }
    if (BU_STR_EQUIV(name, "null")) {
	return 1;
    }
    std::map<std::string, const struct dm *> *dmb = (std::map<std::string, const struct dm *> *)dm_backends;
    std::map<std::string, const struct dm *>::iterator d_it = dmb->find(std::string(name));
    if (d_it == dmb->end()) {
	return 0;
    }
    const struct dm *d = d_it->second;
    int is_valid = d->i->dm_viable(dpy_string);
    return is_valid;
}
extern "C" int
dm_valid_type(const char *name, const char *dpy_string)
{
    return dm_validXType(dpy_string, name);
}


/**
 * dm_bestXType determines what mged will normally use as the default display
 * manager.  Checks if the display manager backend can work at runtime, if the
 * backend supports that check, and will report the "best" available WORKING
 * backend rather than simply the first one present in the list that is also
 * in the plugins directory.
  */
extern "C" const char *
dm_bestXType(const char *dpy_string)
{
    static const char *priority_list[] = {"osgl", "wgl", "ogl", "X", "tk", "nu"};
    std::map<std::string, const struct dm *> *dmb = (std::map<std::string, const struct dm *> *)dm_backends;
    const char *ret = NULL;

    int i = 0;
    const char *b = priority_list[i];
    while (!BU_STR_EQUAL(b, "nu")) {
	std::map<std::string, const struct dm *>::iterator d_it = dmb->find(std::string(b));
	if (d_it == dmb->end()) {
	    i++;
	    b = priority_list[i];
	    continue;
	}
	const struct dm *d = d_it->second;
	if (d->i->dm_viable(dpy_string) == 1) {
	    ret = b;
	    break;
	}
	i++;
	b = priority_list[i];
    }

    return (ret) ? ret : b;
}

/**
 * dm_default_type suggests a display manager.  Checks if a plugin supplies the
 * specified backend type before reporting it, but does NOT perform a runtime
 * test to verify its suggestion will work (unlike dm_bestXType) before
 * reporting back.
  */
extern "C" const char *
dm_default_type()
{
    static const char *priority_list[] = {"osgl", "wgl", "ogl", "X", "tk", "nu"};
    std::map<std::string, const struct dm *> *dmb = (std::map<std::string, const struct dm *> *)dm_backends;
    const char *ret = NULL;

    int i = 0;
    const char *b = priority_list[i];
    while (!BU_STR_EQUAL(b, "nu")) {
	std::map<std::string, const struct dm *>::iterator d_it = dmb->find(std::string(b));
	if (d_it == dmb->end()) {
	    i++;
	    b = priority_list[i];
	    continue;
	}
	ret = b;
    }

    return (ret) ? ret : b;
}


extern "C" void
fb_set_interface(struct fb *ifp, const char *interface_type)
{
    if (!ifp) return;
    std::map<std::string, const struct fb *> *fmb = (std::map<std::string, const struct fb *> *)fb_backends;
    std::map<std::string, const struct fb *>::iterator f_it;
    for (f_it = fmb->begin(); f_it != fmb->end(); f_it++) {
	const struct fb *f = f_it->second;
        if (bu_strncmp(interface_type, f->i->if_name+5, strlen(interface_type)) == 0) {
	    /* found it, copy its struct in */
            *ifp->i = *(f->i);
            return;
        }
    }
}

extern "C" struct fb_platform_specific *
fb_get_platform_specific(uint32_t magic)
{
    std::map<std::string, const struct fb *> *fmb = (std::map<std::string, const struct fb *> *)fb_backends;
    std::map<std::string, const struct fb *>::iterator f_it;
    for (f_it = fmb->begin(); f_it != fmb->end(); f_it++) {
	const struct fb *f = f_it->second;
        if (magic == f->i->type_magic) {
            /* found it, get its specific struct */
            return f->i->if_existing_get(magic);
        }
    }

    return NULL;
}

extern "C" void
fb_put_platform_specific(struct fb_platform_specific *fb_p)
{
    if (!fb_p) return;

    std::map<std::string, const struct fb *> *fmb = (std::map<std::string, const struct fb *> *)fb_backends;
    std::map<std::string, const struct fb *>::iterator f_it;
    for (f_it = fmb->begin(); f_it != fmb->end(); f_it++) {
	const struct fb *f = f_it->second;
	if (fb_p->magic == f->i->type_magic) {
	    f->i->if_existing_put(fb_p);
	    return;
	}
    }
    return;
}

#define Malloc_Bomb(_bytes_)                                    \
    fb_log("\"%s\"(%d) : allocation of %lu bytes failed.\n",    \
           __FILE__, __LINE__, _bytes_)

#ifdef IF_REMOTE
/**
 * True if the non-null string s is all digits
 */
static int
fb_totally_numeric(const char *s)
{
    if (s == (char *)0 || *s == 0)
        return 0;

    while (*s) {
        if (*s < '0' || *s > '9')
            return 0;
        s++;
    }

    return 1;
}
#endif

struct fb *
fb_open(const char *file, int width, int height)
{
    static const char *priority_list[] = {"/dev/osgl", "/dev/wgl", "/dev/ogl", "/dev/X", "/dev/tk", "nu"};
    register struct fb *ifp;
    int i;
    const char *b;
    std::map<std::string, const struct fb *> *fmb = (std::map<std::string, const struct fb *> *)fb_backends;
    std::map<std::string, const struct fb *>::iterator f_it;

    if (width < 0 || height < 0)
        return FB_NULL;

    ifp = (struct fb *) calloc(sizeof(struct fb), 1);
    if (ifp == FB_NULL) {
        Malloc_Bomb(sizeof(struct fb));
        return FB_NULL;
    }
    ifp->i = (struct fb_impl *) calloc(sizeof(struct fb_impl), 1);
    if (file == NULL || *file == '\0') {
        /* No name given, check environment variable first.     */
        if ((file = (const char *)getenv("FB_FILE")) == NULL || *file == '\0') {
            /* None set, use first valid device in priority order as default */
	    i = 0;
	    b = priority_list[i];
	    while (!BU_STR_EQUAL(b, "nu")) {
		f_it = fmb->find(std::string(b));
		if (f_it == fmb->end()) {
		    i++;
		    b = priority_list[i];
		    continue;
		}
		const struct fb *f = f_it->second;
		*ifp->i = *(f->i);        /* struct copy */
		file = ifp->i->if_name;
		goto found_interface;
	    }

            *ifp->i = *fb_null_interface.i;        /* struct copy */
            file = ifp->i->if_name;
            goto found_interface;
        }
    }
    /*
     * Determine what type of hardware the device name refers to.
     *
     * "file" can in general look like: hostname:/pathname/devname#
     *
     * If we have a ':' assume the remote interface
     * (We don't check to see if it's us. Good for debugging.)
     * else strip out "/path/devname" and try to look it up in the
     * device array.  If we don't find it assume it's a file.
     */
    f_it = fmb->find(std::string(file));
    if (f_it != fmb->end()) {
	const struct fb *f = f_it->second;
	*ifp->i = *(f->i);        /* struct copy */
	goto found_interface;
    }


    /* Not in list, check special interfaces or disk files */
    /* "/dev/" protection! */
    if (bu_strncmp(file, "/dev/", 5) == 0) {
        fb_log("fb_open: no such device \"%s\".\n", file);
        free((void *) ifp);
        return FB_NULL;
    }

#ifdef IF_REMOTE
    if (fb_totally_numeric(file) || strchr(file, ':') != NULL) {
        /* We have a remote file name of the form <host>:<file>
         * or a port number (which assumes localhost) */
        *ifp->i = *remote_interface.i;
        goto found_interface;
    }
#endif /* IF_REMOTE */

    /* Assume it's a disk file */
    if (_fb_disk_enable) {
        *ifp->i = *disk_interface.i;
    } else {
        fb_log("fb_open: no such device \"%s\".\n", file);
        free((void *) ifp);
        return FB_NULL;
    }

found_interface:
    /* Copy over the name it was opened by. */
    ifp->i->if_name = (char*)malloc((unsigned) strlen(file) + 1);
    if (ifp->i->if_name == (char *)NULL) {
        Malloc_Bomb(strlen(file) + 1);
        free((void *) ifp);
        return FB_NULL;
    }
    bu_strlcpy(ifp->i->if_name, file, strlen(file)+1);

    /* Mark OK by filling in magic number */
    ifp->i->if_magic = FB_MAGIC;

    i=(*ifp->i->if_open)(ifp, file, width, height);
    if (i != 0) {
        ifp->i->if_magic = 0;           /* sanity */
        free((void *) ifp->i->if_name);
        free((void *) ifp);

        if (i < 0)
            fb_log("fb_open: can't open device \"%s\", ret=%d.\n", file, i);
        else
            bu_exit(0, "Terminating early by request\n"); /* e.g., zap memory */

        return FB_NULL;
    }
    return ifp;
}


/**
 * Generic Help.
 * Print out the list of available frame buffers.
 */
extern "C" int
fb_genhelp(void)
{
    std::map<std::string, const struct fb *> *fmb = (std::map<std::string, const struct fb *> *)fb_backends;
    std::map<std::string, const struct fb *>::iterator f_it;
    for (f_it = fmb->begin(); f_it != fmb->end(); f_it++) {
	const struct fb *f = f_it->second;
	fb_log("%-12s  %s\n", f->i->if_name, f->i->if_type);
    }

    /* Print the ones not in the device list */
#ifdef IF_REMOTE
    fb_log("%-12s  %s\n",
           remote_interface.i->if_name,
           remote_interface.i->if_type);
#endif
    if (_fb_disk_enable) {
        fb_log("%-12s  %s\n",
               disk_interface.i->if_name,
               disk_interface.i->if_type);
    }

    return 0;
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
