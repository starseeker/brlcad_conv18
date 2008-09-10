/*                       S E A R C H . C
 * BRL-CAD
 *
 * Copyright (c) 2008 United States Government as represented by
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
 *
 * Includes code from OpenBSD's and NetBSD's find commands:
 *
 * OpenBSD:
 * 
 * Copyright (c) 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * NetBSD:
 * 
 * Copyright (c) 1990, 1993, 1994
 * The Regents of the University of California.  All rights reserved.
 *   
 * This code is derived from software contributed to Berkeley by
 * Cimarron D. Taylor of the University of California, Berkeley.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.                             
 *
 * 
 */
/** @file search.c
 *
 * The search command.
 *
 */

#include "common.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <regex.h>
#include "bio.h"

#include "cmd.h"
#include "ged_private.h"

#include "search.h"

/*
 * D B _ F U L L P A T H _ T R A V E R S E _ S U B T R E E
 *
 * A generic traversal function maintaining awareness of
 * the full path to a given object.
 */
void
db_fullpath_traverse_subtree(union tree *tp,
		    void (*traverse_func) ( struct rt_wdb *, struct db_full_path *,
									void (*) (struct rt_wdb *, struct db_full_path *, genptr_t),
									void (*) (struct rt_wdb *, struct db_full_path *, genptr_t),
									struct resource *,
									genptr_t),
		    struct rt_wdb *wdbp,
			struct db_full_path *dfp,
			void (*comb_func) (struct rt_wdb *, struct db_full_path *, genptr_t),
			void (*leaf_func) (struct rt_wdb *, struct db_full_path *, genptr_t),
			struct resource *resp,
			genptr_t client_data)
{
    struct directory *dp;

    if ( !tp )
	return;

    RT_CK_FULL_PATH( dfp );
    RT_CHECK_DBI( wdbp->dbip );
    RT_CK_TREE( tp );
    RT_CK_RESOURCE( resp );

    switch ( tp->tr_op )  {

	case OP_DB_LEAF:
	    if ( (dp=db_lookup( wdbp->dbip, tp->tr_l.tl_name, LOOKUP_NOISY )) == DIR_NULL ) {
			return;
		} else {
			db_add_node_to_full_path( dfp, dp);
			traverse_func( wdbp, dfp, comb_func, leaf_func, resp, client_data );
			DB_FULL_PATH_POP(dfp);
			break;
		}
	case OP_UNION:
	case OP_INTERSECT:
	case OP_SUBTRACT:
	case OP_XOR:
		db_fullpath_traverse_subtree( tp->tr_b.tb_left, traverse_func, wdbp, dfp, comb_func, leaf_func, resp, client_data );
		db_fullpath_traverse_subtree( tp->tr_b.tb_right, traverse_func, wdbp, dfp, comb_func, leaf_func, resp, client_data );
		break;
	default:
	    bu_log( "db_functree_subtree: unrecognized operator %d\n", tp->tr_op );
	    bu_bomb( "db_functree_subtree: unrecognized operator\n" );
    }
}

/*
 *     D B _ F U L L P A T H _ T R A V E R S E
 *
 *  This subroutine is called for a no-frills tree-walk,
 *  with the provided subroutines being called when entering and
 *  exiting combinations and at leaf (solid) nodes.
 *
 *  This routine is recursive, so no variables may be declared static.
 *
 *  Unlike db_preorder_traverse, this routine and its subroutines
 *  use db_full_path structures instead of directory structures.
 */
void
db_fullpath_traverse( struct rt_wdb *wdbp,
	    struct db_full_path *dfp,
	    void (*comb_func) (struct rt_wdb *, struct db_full_path *, genptr_t),
	    void (*leaf_func) (struct rt_wdb *, struct db_full_path *, genptr_t),
	    struct resource *resp,
	    genptr_t client_data )
{
	struct directory *dp;
    register int i;
    RT_CK_FULL_PATH(dfp);
    RT_CK_DBI(wdbp->dbip);

	dp = DB_FULL_PATH_CUR_DIR(dfp);

	if ( dp->d_flags & DIR_COMB )  {
		/* entering region */
		if ( comb_func )
			comb_func( wdbp, dfp, client_data );
		if ( wdbp->dbip->dbi_version < 5 ) {
			register union record   *rp;
			register struct directory *mdp;
			/*
			* Load the combination into local record buffer
			* This is in external v4 format.
			*/
			if ( (rp = db_getmrec( wdbp->dbip, dp )) == (union record *)0 )
				return;
			/* recurse */
			for ( i=1; i < dp->d_len; i++ )  {
			if ( (mdp = db_lookup( wdbp->dbip, rp[i].M.m_instname,
						LOOKUP_NOISY )) == DIR_NULL ) {
					continue;
				} else {
					db_add_node_to_full_path(dfp, mdp);
					db_fullpath_traverse(wdbp, dfp, comb_func, leaf_func, resp, client_data);
					DB_FULL_PATH_POP(dfp);
				}
			}
			bu_free( (char *)rp, "db_preorder_traverse[]" );
		} else {
			struct rt_db_internal in;
			struct rt_comb_internal *comb;
			struct directory *ndp;

			if ( rt_db_get_internal5( &in, dp, wdbp->dbip, NULL, resp ) < 0 )
				return;

			comb = (struct rt_comb_internal *)in.idb_ptr;

			db_fullpath_traverse_subtree( comb->tree, db_fullpath_traverse, wdbp, dfp, comb_func, leaf_func, resp, client_data );

			rt_db_free_internal( &in, resp );
		}
	}
	if ( dp->d_flags & DIR_SOLID || dp->d_major_type & DB5_MAJORTYPE_BINARY_MASK )  {
	   /* at leaf */
	   if ( leaf_func )
	       leaf_func( wdbp, dfp, client_data );
    }
}


int typecompare(const void *, const void *);

/* NB: the following table must be sorted lexically. */
static OPTION options[] = {
        { "!",          N_NOT,          c_not,          O_ZERO },
        { "(",          N_OPENPAREN,    c_openparen,    O_ZERO },
        { ")",          N_CLOSEPAREN,   c_closeparen,   O_ZERO },
		{ "-a",         N_AND,          NULL,           O_NONE },
		{ "-and",       N_AND,          NULL,           O_NONE },
		{ "-attr",	N_ATTR,		c_attr,		O_ARGV },
		{ "-iregex",   N_IREGEX,      c_iregex,      O_ARGV },
		{ "-name",      N_NAME,         c_name,         O_ARGV },
        { "-not",       N_NOT,          c_not,          O_ZERO },
		{ "-o",         N_OR,           c_or,		O_ZERO },
		{ "-or", 	N_OR, 		c_or, 		O_ZERO },
		{ "-print",     N_PRINT,        c_print,        O_ZERO },
		{ "-print0",    N_PRINT0,       c_print0,       O_ZERO },
		{ "-regex",   N_REGEX,      c_regex,      O_ARGV },
		{ "-stdattr",   N_STDATTR,      c_stdattr,      O_ZERO },
		{ "-type",   	N_TYPE,     	c_type,		O_ARGV },
};

static PLAN *
palloc(enum ntype t, int (*f)(PLAN *, struct db_full_path *, struct rt_wdb *))
{
        PLAN *new;

        if ((new = bu_calloc(1, sizeof(PLAN), "Allocate PLAN structure"))) {
                new->type = t; 
                new->eval = f; 
                return (new);
        }
        bu_exit(1, NULL);
        /* NOTREACHED */ 
}

/*
 * ( expression ) functions --
 *
 *      True if expression is true.
 */
int
f_expr(PLAN *plan, struct db_full_path *entry, struct rt_wdb *wdbp)
{
        PLAN *p;
        int state;

        for (p = plan->p_data[0];
            p && (state = (p->eval)(p, entry, wdbp)); p = p->next);
        return (state);
}


/*
 * N_OPENPAREN and N_CLOSEPAREN nodes are temporary place markers.  They are
 * eliminated during phase 2 of find_formplan() --- the '(' node is converted
 * to a N_EXPR node containing the expression and the ')' node is discarded.
 */
int
c_openparen(char *ignore, char ***ignored, int unused, PLAN **resultplan)
{
        (*resultplan) = (palloc(N_OPENPAREN, (int (*)(PLAN *, struct db_full_path *, struct rt_wdb *))-1));
	return BRLCAD_OK;
}
 
int
c_closeparen(char *ignore, char ***ignored, int unused, PLAN **resultplan)
{
        (*resultplan) = (palloc(N_CLOSEPAREN, (int (*)(PLAN *, struct db_full_path *, struct rt_wdb *))-1));
	return BRLCAD_OK;
}


/*
 * ! expression functions --
 *
 *      Negation of a primary; the unary NOT operator.
 */
int
f_not(PLAN *plan, struct db_full_path *entry, struct rt_wdb *wdbp)
{
        PLAN *p;
        int state;

        for (p = plan->p_data[0];
            p && (state = (p->eval)(p, entry, wdbp)); p = p->next);
        return (!state);
}
 
int
c_not(char *ignore, char ***ignored, int unused, PLAN **resultplan)
{
        (*resultplan) =  (palloc(N_NOT, f_not));
	return BRLCAD_OK;
}
 
/*
 * expression -o expression functions --
 *
 *      Alternation of primaries; the OR operator.  The second expression is
 * not evaluated if the first expression is true.
 */
int
f_or(PLAN *plan, struct db_full_path *entry, struct rt_wdb *wdbp)
{
        PLAN *p;
        int state;

        for (p = plan->p_data[0];
            p && (state = (p->eval)(p, entry, wdbp)); p = p->next);

        if (state)
                return (1);

        for (p = plan->p_data[1];
            p && (state = (p->eval)(p, entry, wdbp)); p = p->next);
        return (state); 
}

int 
c_or(char *ignore, char ***ignored, int unused, PLAN **resultplan)
{
        (*resultplan) = (palloc(N_OR, f_or));
	return BRLCAD_OK;
}


/*
 * -name functions --
 *
 *      True if the basename of the filename being examined
 *      matches pattern using Pattern Matching Notation S3.14
 */
int
f_name(PLAN *plan, struct db_full_path *entry, struct rt_wdb *wdbp)
{
    	return (!bu_fnmatch(plan->c_data, DB_FULL_PATH_CUR_DIR(entry)->d_namep, 0));
}

int
c_name(char *pattern, char ***ignored, int unused, PLAN **resultplan)
{
        PLAN *new;

        new = palloc(N_NAME, f_name);
        new->c_data = pattern;
	(*resultplan) = new;
        return BRLCAD_OK;
}



/*
 * -regex regexp (and related) functions --
 *
 *	True if the complete file path matches the regular expression regexp.
 *	For -regex, regexp is a case-sensitive (basic) regular expression.
 *	For -iregex, regexp is a case-insensitive (basic) regular expression.
 */
int
f_regex(PLAN *plan, struct db_full_path *entry, struct rt_wdb *wdbp)
{
	return (!(regexec(&plan->regexp_data, db_path_to_string(entry), 0, NULL, 0)));
}

int
c_regex_common(enum ntype type, char *regexp, int icase, PLAN **resultplan)
{
	regex_t reg;
	PLAN *new;
	int rv;
	size_t len;
	bu_log("Matching extened regular expression: %s\n", regexp);
	if (icase = 1 ) {
		rv = regcomp(&reg, regexp, REG_NOSUB|REG_EXTENDED|REG_ICASE);
	} else {
		rv = regcomp(&reg, regexp, REG_NOSUB|REG_EXTENDED);
	}	
	if (rv != 0) {
		bu_log("Error - regex compile did not succeed: %s\n", regexp);
		return BRLCAD_ERROR;
	}
	new = palloc(type, f_regex);
	new->regexp_data = reg;
	(*resultplan) = new;
	regfree(&reg);
	return BRLCAD_OK;
}

int
c_regex(char *pattern, char ***ignored, int unused, PLAN **resultplan)
{
	return (c_regex_common(N_REGEX, pattern, 0, resultplan));
}

int
c_iregex(char *pattern, char ***ignored, int unused, PLAN **resultplan)
{

	return (c_regex_common(N_IREGEX, pattern, 1, resultplan));
}




/*
 * -attr functions --
 *
 *      True if the database object being examined has the attribute
 *      supplied to the attr option
 */
int
f_attr(PLAN *plan, struct db_full_path *entry, struct rt_wdb *wdbp)
{
	struct bu_vls attribname;
	struct bu_vls value;
	struct bu_attribute_value_set avs;
	struct bu_attribute_value_pair *avpp;
	int equalpos = 0;
	int checkval = 0;
	int i;
	bu_vls_init(&attribname);
	bu_vls_init(&value);
	
	    
	/* Check for unescaped equal sign - if present, the
	 * attribute must not only be present but have the
 	 * value indicated.  Escaping is done with the "/" 
	 * character.
	 */

	while ((equalpos < strlen(plan->attr_data)) && (plan->attr_data[equalpos] != '=')) {
	    if ((plan->attr_data[equalpos] == '/') && (plan->attr_data[equalpos + 1] == '=')) {equalpos++;}
	    equalpos++;
	}

	if (equalpos == strlen(plan->attr_data)){
	    bu_vls_strcpy(&attribname, plan->attr_data);
	} else {
	    checkval = 1;
	    bu_vls_strncpy(&attribname, plan->attr_data, equalpos);
	    bu_vls_strncpy(&value, &(plan->attr_data[equalpos+1]), strlen(plan->attr_data) - equalpos - 1);
	}
	
	/* Get attributes for object and check all of
	 * them to see if there is a match to the requested
	 * attribute.  If a value is supplied, check the
	 * value of any matches to the attribute name before
	 * returning success.
	 */
	
	bu_avs_init_empty(&avs);
	db5_get_attributes( wdbp->dbip, &avs, DB_FULL_PATH_CUR_DIR(entry));
        avpp = avs.avp;
	for (i = 0; i < avs.count; i++, avpp++) {
	    if (!bu_fnmatch(bu_vls_addr(&attribname), avpp->name, 0)) {
		if ( checkval == 1 ) {
		    if (!bu_fnmatch(bu_vls_addr(&value), avpp->value, 0)) {
			bu_avs_free( &avs);
			bu_vls_free( &attribname);
			bu_vls_free( &value);
			return (1);
		    }
		} else {
		    bu_avs_free( &avs);
		    bu_vls_free( &attribname);
		    bu_vls_free( &value);
		    return (1);
		}
	    }
	 }
	bu_avs_free( &avs);
	bu_vls_free( &attribname);
	bu_vls_free( &value);
	return (0);
}

int
c_attr(char *pattern, char ***ignored, int unused, PLAN **resultplan)
{
            PLAN *new;

            new = palloc(N_ATTR, f_attr);
            new->attr_data = pattern;
            (*resultplan) = new;
            return BRLCAD_OK; 
}


/*	 
  * -stdattr function --	 
  *	 
  *    Search based on the presence of the	 
  *    "standard" attributes - matches when there
  *	   are ONLY "standard" attributes
  *    associated with an object.
  */	 
 int	 
 f_stdattr(PLAN *plan, struct db_full_path *entry, struct rt_wdb *wdbp)	 
 {	 
         struct bu_attribute_value_set avs;	 
         struct bu_attribute_value_pair *avpp;	 
         int i;	 
         int found_nonstd_attr = 0;	 
         int found_attr = 0;	 
 	 
         /* Get attributes for object and check all of	 
          * them to see if there is not a match to the	 
          * standard attributes.  If any is found return	 
          * failure, otherwise success.	 
          */	 
 	 
         bu_avs_init_empty(&avs);	 
         db5_get_attributes( wdbp->dbip, &avs, DB_FULL_PATH_CUR_DIR(entry));	 
         avpp = avs.avp;	 
         for (i = 0; i < avs.count; i++, avpp++) {	 
           found_attr = 1;	 
            if (strcmp(avpp->name, "GIFTmater") != 0 &&	 
                strcmp(avpp->name, "aircode") != 0 &&	 
                strcmp(avpp->name, "inherit") != 0 &&	 
                strcmp(avpp->name, "los") != 0 &&	 
                strcmp(avpp->name, "material_id") != 0 &&	 
                strcmp(avpp->name, "oshader") != 0 &&	 
                strcmp(avpp->name, "region") != 0 &&	 
                strcmp(avpp->name, "region_id") != 0 &&	 
                strcmp(avpp->name, "rgb") != 0){	 
 	 
               found_nonstd_attr = 1;	 
           }	 
         }	 
 	 
         bu_avs_free( &avs);	 
	 
         if (!found_nonstd_attr && found_attr)  return 1;	 
         return 0;	 
 }	 
 	 
 int	 
 c_stdattr(char *pattern, char ***ignored, int unused, PLAN **resultplan)	 
 {	 
             PLAN *new;	 
 	 
             new = palloc(N_STDATTR, f_stdattr);	 
             (*resultplan) = new;	 
             return BRLCAD_OK;	 
 }	

/*	 
  * -type function --	 
  *	 
  *    Search based on the type of the object - primitives are matched
  *    based on their primitive type (tor, tgc, arb4, etc.) and combinations
  *    are matched based on whether they are a combination or region.
  */	 
 int	 
 f_type(PLAN *plan, struct db_full_path *entry, struct rt_wdb *wdbp)	 
 {	 
         struct rt_db_internal intern;	 
		 int type_match = 0;
         int type;	 

		 rt_db_get_internal(&intern, DB_FULL_PATH_CUR_DIR(entry), wdbp->dbip, (fastf_t *)NULL, &rt_uniresource);

		 if(intern.idb_major_type != DB5_MAJORTYPE_BRLCAD) return 0; 
 
		 /* Eventually this whole switch statement needs to go away
		  * in favor of a function to query the primitive's short name
		  * and use that for the comparison - will be MUCH shorter and
		  * simpler.
		  */
		 switch (intern.idb_minor_type) {
			case DB5_MINORTYPE_BRLCAD_TOR:
				type_match = (!bu_fnmatch(plan->type_data, "tor", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_TGC:
				type_match = (!bu_fnmatch(plan->type_data, "tgc", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_ELL:
				type_match = (!bu_fnmatch(plan->type_data, "ell", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_ARB8:
				type = rt_arb_std_type(&intern, &wdbp->wdb_tol);

				switch (type) {
				case 4:
					type_match = (!bu_fnmatch(plan->type_data, "arb4", 0));
					break;
				case 5:
					type_match = (!bu_fnmatch(plan->type_data, "arb5", 0));
					break;
				case 6:
					type_match = (!bu_fnmatch(plan->type_data, "arb6", 0));
					break;
				case 7:
					type_match = (!bu_fnmatch(plan->type_data, "arb7", 0));
					break;
				case 8:
					type_match = (!bu_fnmatch(plan->type_data, "arb8", 0));
					break;
				default:
					type_match = (!bu_fnmatch(plan->type_data, "invalid", 0));
					break;
				}

				break;
			case DB5_MINORTYPE_BRLCAD_ARS:
				type_match = (!bu_fnmatch(plan->type_data, "ars", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_HALF:
				type_match = (!bu_fnmatch(plan->type_data, "half", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_REC:
				type_match = (!bu_fnmatch(plan->type_data, "rec", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_POLY:
				type_match = (!bu_fnmatch(plan->type_data, "poly", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_BSPLINE:
				type_match = (!bu_fnmatch(plan->type_data, "spline", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_SPH:
				type_match = (!bu_fnmatch(plan->type_data, "sph", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_NMG:
				type_match = (!bu_fnmatch(plan->type_data, "nmg", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_EBM:
				type_match = (!bu_fnmatch(plan->type_data, "ebm", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_VOL:
				type_match = (!bu_fnmatch(plan->type_data, "vol", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_ARBN:
				type_match = (!bu_fnmatch(plan->type_data, "arbn", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_PIPE:
				type_match = (!bu_fnmatch(plan->type_data, "pipe", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_PARTICLE:
				type_match = (!bu_fnmatch(plan->type_data, "part", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_RPC:
				type_match = (!bu_fnmatch(plan->type_data, "rpc", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_RHC:
				type_match = (!bu_fnmatch(plan->type_data, "rhc", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_EPA:
				type_match = (!bu_fnmatch(plan->type_data, "epa", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_EHY:
				type_match = (!bu_fnmatch(plan->type_data, "ehy", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_ETO:
				type_match = (!bu_fnmatch(plan->type_data, "eto", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_GRIP:
				type_match = (!bu_fnmatch(plan->type_data, "grip", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_JOINT:
				type_match = (!bu_fnmatch(plan->type_data, "joint", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_HF:
				type_match = (!bu_fnmatch(plan->type_data, "hf", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_DSP:
				type_match = (!bu_fnmatch(plan->type_data, "dsp", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_SKETCH:
				type_match = (!bu_fnmatch(plan->type_data, "sketch", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_EXTRUDE:
				type_match = (!bu_fnmatch(plan->type_data, "extrude", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_SUBMODEL:
				type_match = (!bu_fnmatch(plan->type_data, "submodel", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_CLINE:
				type_match = (!bu_fnmatch(plan->type_data, "cline", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_BOT:
				type_match = (!bu_fnmatch(plan->type_data, "bot", 0));
				break;
			case DB5_MINORTYPE_BRLCAD_COMBINATION:
				if (DB_FULL_PATH_CUR_DIR(entry)->d_flags & DIR_REGION) {
					if ((!bu_fnmatch(plan->type_data, "r", 0)) || (!bu_fnmatch(plan->type_data, "region", 0))) {
						type_match = 1;
					}
				} else {
					if ((!bu_fnmatch(plan->type_data, "comb", 0)) || (!bu_fnmatch(plan->type_data, "combination", 0))) {
						type_match = 1;
					}
				}
				break;
			default:
				type_match = (!bu_fnmatch(plan->type_data, "other", 0));
				break;
    }

    rt_db_free_internal(&intern, &rt_uniresource);
	return (type_match);
 }	 
 	 
 int	 
 c_type(char *pattern, char ***ignored, int unused, PLAN **resultplan)	 
 {	 
             PLAN *new;	 
 	 
             new = palloc(N_TYPE, f_type);
			 new->type_data = pattern;
             (*resultplan) = new;	 
             return BRLCAD_OK;	 
 }	



/*
 * -print functions --
 *
 *      Always true, causes the current pathame to be written to
 *      standard output.
 */
int
f_print(PLAN *plan, struct db_full_path *entry, struct rt_wdb *wdbp)
{
        bu_log("%s\n", db_path_to_string(entry));
		isoutput = 0;
        return(1);
}

/* ARGSUSED */
int
f_print0(PLAN *plan, struct db_full_path *entry, struct rt_wdb *wdbp)
{
        (void)fputs(db_path_to_string(entry), stdout);
        (void)fputc('\0', stdout);
        return(1);
}

int
c_print(char *ignore, char ***ignored, int unused, PLAN **resultplan)
{
        isoutput = 1;
	
	(*resultplan) = palloc(N_PRINT, f_print);
	return BRLCAD_OK;
}

int
c_print0(char *ignore, char ***ignored, int unused, PLAN **resultplan)
{
        isoutput = 1;

	(*resultplan) = palloc(N_PRINT0, f_print0);
	return BRLCAD_OK;
}




/*
 * find_create --
 *      create a node corresponding to a command line argument.
 *
 * TODO:
 *      add create/process function pointers to node, so we can skip
 *      this switch stuff.
 */
int
find_create(char ***argvp, PLAN **resultplan)
{
        OPTION *p;
        PLAN *new;
        char **argv;

        argv = *argvp;

        if ((p = option(*argv)) == NULL) {        
	    	bu_log("%s: unknown option passed to find_create\n", *argv);
		return BRLCAD_ERROR;
	}
        ++argv;
        if (p->flags & (O_ARGV|O_ARGVP) && !*argv) {
                bu_log("%s: requires additional arguments\n", *--argv);
		return BRLCAD_ERROR;
	}
        switch(p->flags) {
        case O_NONE:
                new = NULL;
                break;
        case O_ZERO:
                (p->create)(NULL, NULL, 0, &new);
                break;
        case O_ARGV:
                (p->create)(*argv++, NULL, 0, &new);
                break;
        case O_ARGVP: 
                (p->create)(NULL, &argv, p->token == N_OK, &new);
                break;
        default:
                return BRLCAD_OK;
        }
        *argvp = argv;
	(*resultplan) = new;
        return BRLCAD_OK;
}

OPTION *
option(char *name)
{
        OPTION tmp;

        tmp.name = name;
        return ((OPTION *)bsearch(&tmp, options,
            sizeof(options)/sizeof(OPTION), sizeof(OPTION), typecompare));
}

int
typecompare(const void *a, const void *b)
{
        return (strcmp(((OPTION *)a)->name, ((OPTION *)b)->name));
}




/*
 * yanknode -- 
 *      destructively removes the top from the plan
 */
static PLAN *
yanknode(PLAN **planp)          /* pointer to top of plan (modified) */
{
        PLAN *node;             /* top node removed from the plan */
     
        if ((node = (*planp)) == NULL)
                return (NULL);
        (*planp) = (*planp)->next;
        node->next = NULL;
        return (node);  
}

/*
 * yankexpr --
 *      Removes one expression from the plan.  This is used mainly by
 *      paren_squish.  In comments below, an expression is either a
 *      simple node or a N_EXPR node containing a list of simple nodes.
 */
int
yankexpr(PLAN **planp, PLAN **resultplan)          /* pointer to top of plan (modified) */
{
        PLAN *next;     /* temp node holding subexpression results */
        PLAN *node;             /* pointer to returned node or expression */
        PLAN *tail;             /* pointer to tail of subplan */
        PLAN *subplan;          /* pointer to head of ( ) expression */
        extern int f_expr(PLAN *, struct db_full_path *, struct rt_wdb *);
        int error_return = BRLCAD_OK;       
	
        /* first pull the top node from the plan */
        if ((node = yanknode(planp)) == NULL) {
	    	(*resultplan) = NULL;
                return BRLCAD_OK;
 	}
        /*
         * If the node is an '(' then we recursively slurp up expressions
         * until we find its associated ')'.  If it's a closing paren we
         * just return it and unwind our recursion; all other nodes are
         * complete expressions, so just return them.
         */
        if (node->type == N_OPENPAREN) 
                for (tail = subplan = NULL;;) {
		    	if ((error_return = yankexpr(planp, &next)) != BRLCAD_OK) return BRLCAD_ERROR;
                        if (next == NULL) {
                                bu_log("(: missing closing ')'\n");
				return BRLCAD_ERROR;
			}
                        /* 
                         * If we find a closing ')' we store the collected
                         * subplan in our '(' node and convert the node to
                         * a N_EXPR.  The ')' we found is ignored.  Otherwise,
                         * we just continue to add whatever we get to our
                         * subplan.
                         */
                        if (next->type == N_CLOSEPAREN) {
                                if (subplan == NULL) {
                                        bu_log("(): empty inner expression");
					return BRLCAD_ERROR;
				}
                                node->p_data[0] = subplan;
                                node->type = N_EXPR;
                                node->eval = f_expr;
                                break;
                        } else {
                                if (subplan == NULL)
                                        tail = subplan = next;
                                else {
                                        tail->next = next;
                                        tail = next;
                                }
                                tail->next = NULL;
                        }
                }
	(*resultplan) = node;
	if (!(error_return == BRLCAD_OK)) {
	    return BRLCAD_ERROR;
	} else {
	    return BRLCAD_OK;
	}
}


/*
 * paren_squish --
 *      replaces "parentheisized" plans in our search plan with "expr" nodes.
 */
int
paren_squish(PLAN *plan, PLAN **resultplan)                /* plan with ( ) nodes */
{
        PLAN *expr;     /* pointer to next expression */
        PLAN *tail;     /* pointer to tail of result plan */
        PLAN *result;           /* pointer to head of result plan */

        result = tail = NULL;

        /*
         * the basic idea is to have yankexpr do all our work and just
         * collect it's results together.
         */
	if (yankexpr(&plan, &expr) != BRLCAD_OK) return BRLCAD_ERROR;
        while (expr != NULL) {
                /*
                 * if we find an unclaimed ')' it means there is a missing
                 * '(' someplace.
                 */
                if (expr->type == N_CLOSEPAREN) {
                        bu_log("): no beginning '('");
						return BRLCAD_ERROR;
				}

                /* add the expression to our result plan */
                if (result == NULL)
                        tail = result = expr;
                else {
                        tail->next = expr;
                        tail = expr;
                }
                tail->next = NULL;
		if (yankexpr(&plan, &expr) != BRLCAD_OK) return BRLCAD_ERROR;
        }
 	(*resultplan) = result;
 	return BRLCAD_OK;
}
 
/*
 * not_squish --
 *      compresses "!" expressions in our search plan.
 */
int
not_squish(PLAN *plan, PLAN **resultplan)          /* plan to process */
{
        PLAN *next;     /* next node being processed */
        PLAN *node;     /* temporary node used in N_NOT processing */
        PLAN *tail;     /* pointer to tail of result plan */
        PLAN *result;           /* pointer to head of result plan */

        tail = result = next = NULL;

        while ((next = yanknode(&plan)) != NULL) {
                /*
                 * if we encounter a ( expression ) then look for nots in
                 * the expr subplan.
                 */
                if (next->type == N_EXPR) 
                        if (not_squish(next->p_data[0], &(next->p_data[0])) != BRLCAD_OK) return BRLCAD_ERROR;

                /*
                 * if we encounter a not, then snag the next node and place
                 * it in the not's subplan.  As an optimization we compress
                 * several not's to zero or one not.
                 */
                if (next->type == N_NOT) {
                        int notlevel = 1;

                        node = yanknode(&plan);
                        while (node != NULL && node->type == N_NOT) {
                                ++notlevel;
                                node = yanknode(&plan);
                        }
                        if (node == NULL) {
                                bu_log("!: no following expression");
								return BRLCAD_ERROR;
						}
                        if (node->type == N_OR) {
                                bu_log("!: nothing between ! and -o");
								return BRLCAD_ERROR;
						}
                        if (node->type == N_EXPR)
                                if (not_squish(node, &node) != BRLCAD_OK) return BRLCAD_ERROR;
                        if (notlevel % 2 != 1)
                                next = node;
                        else
                                next->p_data[0] = node;
                }

                /* add the node to our result plan */
                if (result == NULL)
                        tail = result = next;
                else {
                        tail->next = next;
                        tail = next;
                }
                tail->next = NULL;
        }
	(*resultplan) = result;
        return BRLCAD_OK;
}


/*
 * or_squish --
 *      compresses -o expressions in our search plan.
 */
int
or_squish(PLAN *plan, PLAN **resultplan)           /* plan with ors to be squished */
{
        PLAN *next;     /* next node being processed */
        PLAN *tail;     /* pointer to tail of result plan */
        PLAN *result;           /* pointer to head of result plan */

        tail = result = next = NULL;

        while ((next = yanknode(&plan)) != NULL) {
                /*
                 * if we encounter a ( expression ) then look for or's in
                 * the expr subplan.
                 */
                if (next->type == N_EXPR) 
                       if(or_squish(next->p_data[0], &(next->p_data[0])) != BRLCAD_OK) return BRLCAD_ERROR;

                /* if we encounter a not then look for not's in the subplan */
                if (next->type == N_NOT)
						if(or_squish(next->p_data[0], &(next->p_data[0])) != BRLCAD_OK) return BRLCAD_ERROR;

                /*
                 * if we encounter an or, then place our collected plan in the
                 * or's first subplan and then recursively collect the
                 * remaining stuff into the second subplan and return the or.
                 */
                if (next->type == N_OR) {
                        if (result == NULL) {
                                bu_log("-o: no expression before -o");
								return BRLCAD_ERROR;
						}
                        next->p_data[0] = result;
                        if(or_squish(plan, &(next->p_data[1]))  != BRLCAD_OK) return BRLCAD_ERROR;
                        if (next->p_data[1] == NULL) {
                                bu_log("-o: no expression after -o");
								return BRLCAD_ERROR;
						}
                        (*resultplan) = next;
			return BRLCAD_OK;
                }

                /* add the node to our result plan */
                if (result == NULL)
                        tail = result = next;
                else {
                        tail->next = next;
                        tail = next;
                }
                tail->next = NULL;
        }
	(*resultplan) = result;
        return BRLCAD_OK;
}




/*
 * find_formplan --
 *      process the command line and create a "plan" corresponding to the
 *      command arguments.
 */
int
find_formplan(char **argv, PLAN **resultplan)
{
        PLAN *plan, *tail, *new;

        /*
         * for each argument in the command line, determine what kind of node
         * it is, create the appropriate node type and add the new plan node
         * to the end of the existing plan.  The resulting plan is a linked
         * list of plan nodes.  For example, the string:
         *
         *      % find . -name foo -newer bar -print
         *
         * results in the plan:
         *
         *      [-name foo]--> [-newer bar]--> [-print]
         *
         * in this diagram, `[-name foo]' represents the plan node generated
         * by c_name() with an argument of foo and `-->' represents the
         * plan->next pointer.
         */
        for (plan = tail = NULL; *argv;) {
	    	if (find_create(&argv, &new) != BRLCAD_OK) return BRLCAD_ERROR;
                if (!(new))
                        continue;
                if (plan == NULL)
                        tail = plan = new;
                else {
                        tail->next = new;
                        tail = new;
                }
        }

        /*
         * if the user didn't specify one of -print, -ok or -exec, then -print
         * is assumed so we bracket the current expression with parens, if
         * necessary, and add a -print node on the end.
         */
        if (!isoutput) {
                if (plan == NULL) {
                        c_print(NULL, NULL, 0, &new);
                        tail = plan = new;
                } else {
                        c_openparen(NULL, NULL, 0, &new);
                        new->next = plan;
                        plan = new;
                        c_closeparen(NULL, NULL, 0, &new);
                        tail->next = new;
                        tail = new;
                        c_print(NULL, NULL, 0, &new);
                        tail->next = new;
                        tail = new;
                }
        }

        /*
         * the command line has been completely processed into a search plan
         * except for the (, ), !, and -o operators.  Rearrange the plan so
         * that the portions of the plan which are affected by the operators
         * are moved into operator nodes themselves.  For example:
         *
         *      [!]--> [-name foo]--> [-print]
         *
         * becomes
         *
         *      [! [-name foo] ]--> [-print]
         *
         * and
         *
         *      [(]--> [-depth]--> [-name foo]--> [)]--> [-print]
         *
         * becomes
         *
         *      [expr [-depth]-->[-name foo] ]--> [-print]
         *
         * operators are handled in order of precedence.
         */

        if(paren_squish(plan, &plan) != BRLCAD_OK) return BRLCAD_ERROR;              /* ()'s */
        if(not_squish(plan, &plan) != BRLCAD_OK) return BRLCAD_ERROR;                /* !'s */
        if(or_squish(plan, &plan) != BRLCAD_OK) return BRLCAD_ERROR;                 /* -o's */
	(*resultplan) = plan;
        return BRLCAD_OK;
}

void
find_execute_plans(struct rt_wdb *wdbp, struct db_full_path *dfp, genptr_t inputplan) {
	PLAN *p;
	PLAN *plan = (PLAN *)inputplan;
	for (p = plan; p && (p->eval)(p, dfp, wdbp); p = p->next) 
		    ;

}

void
find_execute(PLAN *plan,        /* search plan */
	struct db_full_path *pathname,               /* array of pathnames to traverse */
	struct rt_wdb *wdbp,
	int execute_style)
{
    		struct directory *dp;
		struct db_full_path fullname;
		int i;
		db_full_path_init(&fullname);
		switch (execute_style) {
		    case 0:
			db_fullpath_traverse(wdbp, pathname, find_execute_plans, find_execute_plans, wdbp->wdb_resp, plan);
			break;
		    case 1:
			for (i = 0; i < RT_DBNHASH; i++) {
			    for (dp = wdbp->dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw) {
					if (!(dp->d_flags & DIR_HIDDEN) && (dp->d_addr != RT_DIR_PHONY_ADDR)) {
						db_string_to_path(&fullname, wdbp->dbip, dp->d_namep);
						find_execute_plans(wdbp, &fullname, plan);
					}
			    }
			}
			break;
		    default:
			db_fullpath_traverse(wdbp, pathname, find_execute_plans, find_execute_plans, wdbp->wdb_resp, plan);
			break;
		}
		db_free_full_path(&fullname);
}


int isoutput;
   
int
wdb_search_cmd(struct rt_wdb      *wdbp,
             Tcl_Interp         *interp,
             int                argc,
             char               *argv[])
{
    register int                                i, k;
    register struct directory           *dp;
    struct rt_db_internal                       intern;
    register struct rt_comb_internal    *comb=(struct rt_comb_internal *)NULL;
    struct bu_vls vls;
    int aflag = 0;              /* look at all objects */
    PLAN *dbplan;
    struct db_full_path dfp;
    
    if (argc < 2) {
	Tcl_AppendResult(interp, "search [path] [expressions...]\n", (char *)NULL);
    } else {
        db_full_path_init(&dfp);
        db_update_nref(wdbp->dbip, &rt_uniresource);

   
	if ( !( (argv[1][0] == '-') || (argv[1][0] == '!')  || (argv[1][0] == '(') ) && (strcmp(argv[1],"/") != 0) && (strcmp(argv[1],".") != 0) ) {
		db_string_to_path(&dfp, wdbp->dbip, argv[1]);	
	        isoutput = 0;
		if (find_formplan(&argv[2], &dbplan) != BRLCAD_OK) {
		    Tcl_AppendResult(interp, "Failed to build find plan.\n", (char *)NULL);
		    return TCL_ERROR;
		} else {
		   find_execute(dbplan, &dfp, wdbp, 0);
		}
	} else {
	    if (strcmp(argv[1],".") == 0) {
		   isoutput = 0;
		   if (find_formplan(&argv[2], &dbplan) != BRLCAD_OK) {
		       Tcl_AppendResult(interp, "Failed to build find plan.\n", (char *)NULL);
		       return TCL_ERROR;
		   } else {
	               find_execute(dbplan, NULL, wdbp, 1);
                   } 
	    } else {
                for (i = 0; i < RT_DBNHASH; i++) {
                      for (dp = wdbp->dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw) {
                              if (dp->d_nref == 0 && !(dp->d_flags & DIR_HIDDEN) && (dp->d_addr != RT_DIR_PHONY_ADDR)) {
				  db_string_to_path(&dfp, wdbp->dbip, dp->d_namep);
				  isoutput = 0;
				  if ( (argv[1][0] == '-') || (argv[1][0] == '!')  || (argv[1][0] == '(') ) {
				      	if (find_formplan(&argv[1], &dbplan) != BRLCAD_OK) {
		    				Tcl_AppendResult(interp, "Failed to build find plan.\n", (char *)NULL);
		    				return TCL_ERROR;
					} else {
		   				find_execute(dbplan, &dfp, wdbp, 0);
					}
				  } else {
			      		if (find_formplan(&argv[2], &dbplan) != BRLCAD_OK) {
					        Tcl_AppendResult(interp, "Failed to build find plan.\n", (char *)NULL);
		    				return TCL_ERROR;
					} else {
		   				find_execute(dbplan, &dfp, wdbp, 0);
					}
				  }
			      }
		      }
		}
	    }
	}
       db_free_full_path(&dfp);
    }
    return TCL_OK;
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
