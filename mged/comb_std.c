/*
 *			C O M B _ S T D . C
 *
 * Functions -
 *	bt_create_leaf		Create a leaf node for a Boolean tree
 *	bt_create_internal	Create an internal node for a Boolean tree
 *	f_comb_std		Create or extend a combination from
 *				a Boolean expression in unrestricted
 *				(standard) form
 *	bool_input_from_vls	input function for lex(1) scanner
 *	bool_unput_from_vls	output function for lex(1) scanner
 *
 *  Author -
 *	Paul Tanenbaum
 *
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *
 *  Distribution Notice -
 *	Re-distribution of this software is restricted, as described in
 *	your "Statement of Terms and Conditions for the Release of
 *	The BRL-CAD Package" agreement.
 *
 *  Copyright Notice -
 *	This software is Copyright (C) 1995 by the United States Army
 *	in all countries except the USA.  All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "conf.h"

#include <stdio.h>

#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "raytrace.h"
#include "rtgeom.h"
#include "./ged.h"
#include "rtstring.h"
#include "./comb_bool.h"

#define	PRINT_USAGE	rt_log("c: usage 'c [-gr] comb_name [bool_expr]'\n")

static struct rt_vls	vp;			/* The Boolean expression */
char			*bool_bufp = 0;
struct bool_tree_node	*comb_bool_tree;

/*
 *		    B T _ C R E A T E _ L E A F ( )
 *
 *		Create a leaf node for a Boolean tree
 *
 */
struct bool_tree_node *bt_create_leaf (char *object_name)
{
    struct bool_tree_node	*b;
    char			*sp;

    b = (struct bool_tree_node *)
	    rt_malloc(sizeof(struct bool_tree_node), "Bool-tree leaf");

    b -> btn_magic = BOOL_TREE_NODE_MAGIC;
    bt_opn(b) = OPN_NULL;
    sp = rt_malloc(strlen(object_name) + 1, "Bool-tree leaf name");

    RES_ACQUIRE( &rt_g.res_syscall );		/* lock */
    sprintf(sp, "%s", object_name);
    RES_RELEASE( &rt_g.res_syscall );		/* unlock */

    bt_leaf_name(b) = sp;

    return(b);
}

/*
 *		B T _ C R E A T E _ I N T E R N A L ( )
 *
 *		Create a leaf node for a Boolean tree
 *
 */
struct bool_tree_node *bt_create_internal (int opn,
		    struct bool_tree_node *opd1,
		    struct bool_tree_node *opd2)
{
    struct bool_tree_node	*b;

    RT_CKMAG(opd1, BOOL_TREE_NODE_MAGIC, "Boolean tree node");
    RT_CKMAG(opd2, BOOL_TREE_NODE_MAGIC, "Boolean tree node");

    b = (struct bool_tree_node *)
	    rt_malloc(sizeof(struct bool_tree_node), "Bool-tree leaf");

    b -> btn_magic = BOOL_TREE_NODE_MAGIC;
    switch (opn)
    {
	case OPN_UNION:
	case OPN_DIFFERENCE:
	case OPN_INTERSECTION:
	    bt_opn(b) = opn;
	    break;
	default:
	    rt_log("%s:%d:bt_create_internal: Illegal operation '%d'\n",
		__FILE__, __LINE__, opn);
	    exit (1);
    }
    bt_opd(b, BT_LEFT) = opd1;
    bt_opd(b, BT_RIGHT) = opd2;

    return (b);
}


/*
 *		    F _ C O M B _ S T D ( )
 *
 *	Input a combination in standard set-theoetic notation
 *
 *	Syntax: c [-gr] comb_name [boolean_expr]
 */
int f_comb_std (argc, argv)

int	argc;
char	**argv;

{
    char			*comb_name;
    int				ch;
    int				region_flag = -1;
    register struct directory	*dp;
    union record		record;

    extern int			optind;
    extern char			*optarg;

    /* Parse options */
    optind = 1;	/* re-init getopt() */
    while ((ch = getopt(argc, argv, "gr?")) != EOF)
    {
	switch (ch)
	{
	    case 'g':
		region_flag = 0;
		break;
	    case 'r':
		region_flag = 1;
		break;
	    case '?':
	    default:
		PRINT_USAGE;
		return (CMD_OK);
	}
    }
    argc -= (optind + 1);
    argv += optind;

    comb_name = *argv++;
    if (argc == -1)
    {
	PRINT_USAGE;
	return (CMD_OK);
    }

    if ((region_flag != -1) && (argc == 0))
    {
	/*
	 *	Set/Reset the REGION flag of an existing combination
	 */
	if ((dp = db_lookup(dbip, comb_name, LOOKUP_NOISY)) == DIR_NULL)
	    return (CMD_BAD);
	if (db_get(dbip, dp, &record, 0, 1) < 0)
	{
	    READ_ERR;
	    return (CMD_BAD);
	}
	if (record.u_id != ID_COMB )
	{
	    rt_log("%s:  not a combination\n", comb_name );
	    return (CMD_OK);
	}
	record.c.c_flags = region_flag ? 'R' : ' ';
	if (db_put(dbip, dp, &record, 0, 1) < 0)
	{
	    WRITE_ERR;
	    return (CMD_BAD);
	}
	return (CMD_OK);
    }
    /*
     *	At this point, we know we have a Boolean expression.
     *	If the combination already existed and region_flag is -1,
     *	then leave its c_flags alone.
     *	If the combination didn't exist yet,
     *	then pretend region_flag was 0.
     *	Otherwise, make sure to set its c_flags according to region_flag.
     */

    if (bool_bufp == 0)
    {
	rt_vls_init(&vp);
	bool_bufp = rt_vls_addr(&vp);
    }
    else
	rt_vls_trunc(&vp, 0);
    rt_vls_from_argv(&vp, argc, argv);
    rt_log("Will define %s '%s' as '%s'\n",
	(region_flag ? "region" : "group"), comb_name, bool_bufp);
    if (yyparse() != 0)
    {
	rt_log("Invalid Boolean expression\n");
	return (CMD_BAD);
    }
    show_gift_bool(comb_bool_tree, 1);

    return (CMD_OK);
}

/*
 *		B O O L _ I N P U T _ F R O M _ V L S ( )
 *
 *	Input function for LEX(1) scanner to read Boolean expressions
 *	from a variable-length string.
 */
int bool_input_from_vls ()
{
    if (*bool_bufp == '\0')
	return (0);
    else
	return (*bool_bufp++);
}

/*
 *		B O O L _ U N P U T _ F R O M _ V L S ( )
 *
 *	Unput function for LEX(1) scanner to read Boolean expressions
 *	from a variable-length string.
 */
void bool_unput_from_vls (ch)

int	ch;

{
    *--bool_bufp = ch;
}
