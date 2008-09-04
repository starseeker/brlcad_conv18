/*                         F I N D . C
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
 * Includes code from OpenBSD's find command:
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
 */
/** @file find.c
 *
 * The find command.
 *
 */

#include "common.h"

#include <string.h>
#include <stdlib.h>

#include "bio.h"
#include "cmd.h"
#include "ged_private.h"

#include "find.h"

int typecompare(const void *, const void *);

/* NB: the following table must be sorted lexically. */
static OPTION options[] = {
        { "!",          N_NOT,          c_not,          O_ZERO },
        { "(",          N_OPENPAREN,    c_openparen,    O_ZERO },
        { ")",          N_CLOSEPAREN,   c_closeparen,   O_ZERO },
        { "-name",      N_NAME,         c_name,         O_ARGV },
		{ "-o",         N_OR,           c_or,           O_ZERO },
		{ "-print",     N_PRINT,        c_print,        O_ZERO },
		{ "-print0",    N_PRINT0,       c_print0,       O_ZERO },
};

static PLAN *
palloc(enum ntype t, int (*f)(PLAN *, struct db_full_path *))
{
        PLAN *new;

        if ((new = bu_calloc(1, sizeof(PLAN), "Allocate PLAN structure"))) {
                new->type = t; 
                new->eval = f; 
                return (new);
        }
        err(1, NULL);
        /* NOTREACHED */ 
}

/*
 * ( expression ) functions --
 *
 *      True if expression is true.
 */
int
f_expr(PLAN *plan, struct db_full_path *entry)
{
        PLAN *p;
        int state;

        for (p = plan->p_data[0];
            p && (state = (p->eval)(p, entry)); p = p->next);
        return (state);
}


/*
 * N_OPENPAREN and N_CLOSEPAREN nodes are temporary place markers.  They are
 * eliminated during phase 2 of find_formplan() --- the '(' node is converted
 * to a N_EXPR node containing the expression and the ')' node is discarded.
 */
PLAN *
c_openparen(char *ignore, char ***ignored, int unused)
{
        return (palloc(N_OPENPAREN, (int (*)(PLAN *, struct db_full_path *))-1));
}
 
PLAN *
c_closeparen(char *ignore, char ***ignored, int unused)
{
        return (palloc(N_CLOSEPAREN, (int (*)(PLAN *, struct db_full_path *))-1));
}


/*
 * ! expression functions --
 *
 *      Negation of a primary; the unary NOT operator.
 */
int
f_not(PLAN *plan, struct db_full_path *entry)
{
        PLAN *p;
        int state;

        for (p = plan->p_data[0];
            p && (state = (p->eval)(p, entry)); p = p->next);
        return (!state);
}
 
PLAN *
c_not(char *ignore, char ***ignored, int unused)
{
        return (palloc(N_NOT, f_not));
}
 
/*
 * expression -o expression functions --
 *
 *      Alternation of primaries; the OR operator.  The second expression is
 * not evaluated if the first expression is true.
 */
int
f_or(PLAN *plan, struct db_full_path *entry)
{
        PLAN *p;
        int state;

        for (p = plan->p_data[0];
            p && (state = (p->eval)(p, entry)); p = p->next);

        if (state)
                return (1);

        for (p = plan->p_data[1];
            p && (state = (p->eval)(p, entry)); p = p->next);
        return (state); 
}

PLAN *  
c_or(char *ignore, char ***ignored, int unused)
{
        return (palloc(N_OR, f_or));
}


/*
 * -name functions --
 *
 *      True if the basename of the filename being examined
 *      matches pattern using Pattern Matching Notation S3.14
 */
int
f_name(PLAN *plan, struct db_full_path *entry)
{
    	return (!fnmatch(plan->c_data, DB_FULL_PATH_CUR_DIR(entry)->d_namep, 0));
}

PLAN *
c_name(char *pattern, char ***ignored, int unused)
{
        PLAN *new;

        new = palloc(N_NAME, f_name);
        new->c_data = pattern;
        return (new);
}


/*
 * -print functions --
 *
 *      Always true, causes the current pathame to be written to
 *      standard output.
 */
int
f_print(PLAN *plan, struct db_full_path *entry)
{
        bu_log("%s\n", db_path_to_string(entry));
		isoutput = 0;
        return(1);
}

/* ARGSUSED */
int
f_print0(PLAN *plan, struct db_full_path *entry)
{
        (void)fputs(db_path_to_string(entry), stdout);
        (void)fputc('\0', stdout);
        return(1);
}

PLAN *
c_print(char *ignore, char ***ignored, int unused)
{
        isoutput = 1;

        return(palloc(N_PRINT, f_print));
}

PLAN *
c_print0(char *ignore, char ***ignored, int unused)
{
        isoutput = 1;

        return(palloc(N_PRINT0, f_print0));
}




/*
 * find_create --
 *      create a node corresponding to a command line argument.
 *
 * TODO:
 *      add create/process function pointers to node, so we can skip
 *      this switch stuff.
 */
PLAN *
find_create(char ***argvp)
{
        OPTION *p;
        PLAN *new;
        char **argv;

        argv = *argvp;

        if ((p = option(*argv)) == NULL)
                errx(1, "%s: unknown option", *argv);
        ++argv;
        if (p->flags & (O_ARGV|O_ARGVP) && !*argv)
                errx(1, "%s: requires additional arguments", *--argv);

        switch(p->flags) {
        case O_NONE:
                new = NULL;
                break;
        case O_ZERO:
                new = (p->create)(NULL, NULL, 0);
                break;
        case O_ARGV:
                new = (p->create)(*argv++, NULL, 0);
                break;
        case O_ARGVP: 
                new = (p->create)(NULL, &argv, p->token == N_OK);
                break;
        default:
                return NULL;
        }
        *argvp = argv;
        return (new);
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
static PLAN *
yankexpr(PLAN **planp)          /* pointer to top of plan (modified) */
{
        PLAN *next;     /* temp node holding subexpression results */
        PLAN *node;             /* pointer to returned node or expression */
        PLAN *tail;             /* pointer to tail of subplan */
        PLAN *subplan;          /* pointer to head of ( ) expression */
        extern int f_expr(PLAN *, struct db_full_path *);
    
        /* first pull the top node from the plan */
        if ((node = yanknode(planp)) == NULL)
                return (NULL);

        /*
         * If the node is an '(' then we recursively slurp up expressions
         * until we find its associated ')'.  If it's a closing paren we
         * just return it and unwind our recursion; all other nodes are
         * complete expressions, so just return them.
         */
        if (node->type == N_OPENPAREN) 
                for (tail = subplan = NULL;;) {
                        if ((next = yankexpr(planp)) == NULL)
                                errx(1, "(: missing closing ')'");
                        /* 
                         * If we find a closing ')' we store the collected
                         * subplan in our '(' node and convert the node to
                         * a N_EXPR.  The ')' we found is ignored.  Otherwise,
                         * we just continue to add whatever we get to our
                         * subplan.
                         */
                        if (next->type == N_CLOSEPAREN) {
                                if (subplan == NULL)
                                        errx(1, "(): empty inner expression");
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
        return (node);
}


/*
 * paren_squish --
 *      replaces "parentheisized" plans in our search plan with "expr" nodes.
 */
PLAN *
paren_squish(PLAN *plan)                /* plan with ( ) nodes */
{
        PLAN *expr;     /* pointer to next expression */
        PLAN *tail;     /* pointer to tail of result plan */
        PLAN *result;           /* pointer to head of result plan */

        result = tail = NULL;

        /*
         * the basic idea is to have yankexpr do all our work and just
         * collect it's results together.
         */
        while ((expr = yankexpr(&plan)) != NULL) {
                /*
                 * if we find an unclaimed ')' it means there is a missing
                 * '(' someplace.
                 */
                if (expr->type == N_CLOSEPAREN)
                        errx(1, "): no beginning '('");

                /* add the expression to our result plan */
                if (result == NULL)
                        tail = result = expr;
                else {
                        tail->next = expr;
                        tail = expr;
                }
                tail->next = NULL;
        }
        return (result);
}
 
/*
 * not_squish --
 *      compresses "!" expressions in our search plan.
 */
PLAN *
not_squish(PLAN *plan)          /* plan to process */
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
                        next->p_data[0] = not_squish(next->p_data[0]);

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
                        if (node == NULL)
                                errx(1, "!: no following expression");
                        if (node->type == N_OR)
                                errx(1, "!: nothing between ! and -o");
                        if (node->type == N_EXPR)
                                node = not_squish(node);
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
        return (result);
}


/*
 * or_squish --
 *      compresses -o expressions in our search plan.
 */
PLAN *
or_squish(PLAN *plan)           /* plan with ors to be squished */
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
                        next->p_data[0] = or_squish(next->p_data[0]);

                /* if we encounter a not then look for not's in the subplan */
                if (next->type == N_NOT)
                        next->p_data[0] = or_squish(next->p_data[0]);

                /*
                 * if we encounter an or, then place our collected plan in the
                 * or's first subplan and then recursively collect the
                 * remaining stuff into the second subplan and return the or.
                 */
                if (next->type == N_OR) {
                        if (result == NULL)
                                errx(1, "-o: no expression before -o");
                        next->p_data[0] = result;
                        next->p_data[1] = or_squish(plan);
                        if (next->p_data[1] == NULL)
                                errx(1, "-o: no expression after -o");
                        return (next);
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
        return (result);
}




/*
 * find_formplan --
 *      process the command line and create a "plan" corresponding to the
 *      command arguments.
 */
PLAN *
find_formplan(char **argv)
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
                if (!(new = find_create(&argv)))
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
                        new = c_print(NULL, NULL, 0);
                        tail = plan = new;
                } else {
                        new = c_openparen(NULL, NULL, 0);
                        new->next = plan;
                        plan = new;
                        new = c_closeparen(NULL, NULL, 0);
                        tail->next = new;
                        tail = new;
                        new = c_print(NULL, NULL, 0);
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

        plan = paren_squish(plan);              /* ()'s */
        /*plan = not_squish(plan);*/                /* !'s */
        /*plan = or_squish(plan);*/                 /* -o's */
        return (plan);
}

void
find_execute_plans(struct db_i *dbip, struct db_full_path *dfp, genptr_t inputplan) {
	PLAN *p;
	PLAN *plan = (PLAN *)inputplan;
	for (p = plan; p && (p->eval)(p, dfp); p = p->next) 
		    ;

}

void
find_execute(PLAN *plan,        /* search plan */
	struct db_full_path *pathname,               /* array of pathnames to traverse */
	struct rt_wdb *wdbp)
{
    		struct directory *dp;
		int i;
		db_fullpath_traverse(wdbp->dbip, pathname, find_execute_plans, find_execute_plans, wdbp->wdb_resp, plan);
}


int isoutput;
   
int
wdb_find_cmd2(struct rt_wdb      *wdbp,
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
    struct db_full_path dfp;
    db_full_path_init(&dfp);
    db_update_nref(wdbp->dbip, &rt_uniresource);
	if (!(argv[1][0] == '-') && (strcmp(argv[1],"/") != 0)) {
    	        db_string_to_path(&dfp, wdbp->dbip, argv[1]);	
	        isoutput = 0;
		find_execute(find_formplan(&argv[2]), &dfp, wdbp);
	} else {
                for (i = 0; i < RT_DBNHASH; i++) {
                      for (dp = wdbp->dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw) {
                              if (dp->d_nref == 0 && !(dp->d_flags & DIR_HIDDEN) && (dp->d_addr != RT_DIR_PHONY_ADDR)) {
				  db_string_to_path(&dfp, wdbp->dbip, dp->d_namep);
				  isoutput = 0;
				  if (argv[1][0] == '-') {
				      find_execute(find_formplan(&argv[1]), &dfp, wdbp);
				  } else {
				      find_execute(find_formplan(&argv[2]), &dfp, wdbp);
				  }
			      }
		      }
		}
	}
    db_free_full_path(&dfp);
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
