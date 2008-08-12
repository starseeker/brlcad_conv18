/*                    P C P A R S E R . C P P
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
 */
/** @addtogroup pcparser */
/** @{ */
/** @file pcParser.cpp
 *
 * Method implementation for Parser Class as well as the associated
 * grammar classes
 *
 * @author Dawn Thomas
 */
#include "pcParser.h"

/* Grammar Classes */

Variable_grammar::Variable_grammar(VCSet &vcs) :
vcset(vcs)
{
}

Variable_grammar::~Variable_grammar()
{
}

Constraint_grammar::Constraint_grammar(VCSet &vcs) :
vcset(vcs)
{
}
Constraint_grammar::~Constraint_grammar()
{
}

/* Parser Class */

Parser::Parser(VCSet &vcs): vcset(vcs), var_gram(NULL), con_gram(NULL)
{
    var_gram = new Variable_grammar(vcset);
    con_gram = new Constraint_grammar(vcset);
}

Parser::~Parser()
{
    if (var_gram)
        delete var_gram;
    if (con_gram)
        delete con_gram;
}

void Parser::parse(struct pc_pc_set * pcs)
{
    /*Iterate through the parameter set first*/
    struct pc_param * par;
    struct pc_constrnt * con;
    while (BU_LIST_WHILE(par, pc_param, &(pcs->ps->l))) {
	name.clear();
	//std::cout<<"Parameter expression Input: "<<(char *) bu_vls_addr(&(par->name))<<std::endl;
	if (par->ctype == PC_DB_BYEXPR) {
	    boost::spirit::classic::parse_info<> p_info = \
	    boost::spirit::classic::parse(\
			    (char *) bu_vls_addr(&(par->data.expression)),\
			    *var_gram, boost::spirit::classic::space_p);
	    if (p_info.full) {
		//vcset.pushVar();
	    } else {
		std::cout << "Error during Variable expression parsing\n";
	    }
	    bu_vls_free(&(par->data.expression));
	} else {
	    if (par->dtype == PC_DB_FASTF_T) {
		std::cout << *(par->data.pval.valuep) << std::endl;
	    } else if (par->dtype == PC_DB_VECTOR_T) {
		std::cout << "Vector ( " <<
			    *(par->data.pval.vectorp) << ", " <<
			    *(par->data.pval.vectorp + 1) << ", " <<
			    *(par->data.pval.vectorp + 2) << ")" <<
			    std::endl;
		vcset.addParameter((char *) bu_vls_addr(&(par->name)),\
				    PC_DB_VECTOR_T,par->data.pval.vectorp);
	    } else if (par->dtype == PC_DB_POINT_T) {
		std::cout << "Point ( " <<
			    *(par->data.pval.pointp) << ", " <<
			    *(par->data.pval.pointp + 1) << ", " <<
			    *(par->data.pval.pointp + 2) << ")" <<
			    std::endl;
	    }
	}
	bu_vls_free(&(par->name));
	BU_LIST_DEQUEUE(&(par->l));
	bu_free(par, "free parameter");
    }
    while (BU_LIST_WHILE(con, pc_constrnt, &(pcs->cs->l))) {
	std::cout<<"Constraint: "<<(char *) bu_vls_addr(&(con->name))<<std::endl;
	/*boost::spirit::parse((char *) bu_vls_addr(&(con->name)), *con_gram, boost::spirit::space_p);*/
        bu_vls_free(&(con->name));
        bu_vls_free(&(con->data.expression));	
	BU_LIST_DEQUEUE(&(con->l));
	bu_free(con, "free constraint");
    }
}

/** @} */
/*
 * Local Variables:
 * mode: C++
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
