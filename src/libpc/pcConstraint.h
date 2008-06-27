/*                       P C C O N S T R A I N T . H
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
/** @addtogroup soln */
/** @{ */
/** @file pcConstraint.h
 *
 * Constraint Class for Parametrics and Constraints Library
 *
 * @author Dawn Thomas
 */
#ifndef __PCCONSTRAINT_H__
#define __PCCONSTRAINT_H__

#include "pcVariable.h"

template<class T>
class Constraint {
private:
    int status;
    std::string id;
    std::string expression;
    bool (*funct) (std::vector<T>);
public:

    typename std::list<std::string> Variables;

    Constraint() { status = 0; } 
    Constraint(std::string Cid, std::string Cexpression) { 
	id = Cid;
	expression = Cexpression;
	status = 0;
    } 
    void function(bool (*pf) (std::vector<T>)) { funct = pf; };
    bool solved() { 
	if (status == 0) return false;
	else return true;
	}
    /* TODO: Take a functor as an argument? */
    bool check(std::vector<T> V) {
	return *funct(V);
    }
    std::string getID() const { return id; }
    std::string getExp() const { return expression; }

};

#endif
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
