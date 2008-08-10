/*                    P C P C S E T . C P P
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
/** @addtogroup libpc */
/** @{ */
/** @file pcVCSet.cpp
 *
 * VCSet Class Methods
 * 
 * @author Dawn Thomas
 */
#include "pcVCSet.h"
#include "pc.h"

VCSet::~VCSet()
{
    std::list<VariableAbstract *>::iterator i;
    for (i = Vars.begin(); i != Vars.end(); i++) {
	switch ((**i).getType()) {
	    case VAR_INT:
		delete (Variable<int> *) *i;
		break;
	    case VAR_DBL:
		delete (Variable<double> *) *i;
		break;
	    case VAR_ABS:
	    default:
		delete *i;
	}
    }
    std::list<Constraint *>::iterator j;
    for (j = Constraints.begin(); j != Constraints.end(); j++) {
	delete *j;
    }
    std::list<Parameter *>::iterator k = ParTable.begin();
    std::list<Parameter *>::iterator p_end = ParTable.end();
    for (; k != p_end; k++) {
	switch ((**k).getType()) {
	    case PC_DB_FASTF_T:
		break;
	    case PC_DB_POINT_T:
		break;
	    case PC_DB_VECTOR_T:
		delete (Vector *) *k;
		break;
	    default:;
	}
    }
}

#if 0
void VCSet::pushVar()
{
    Variable<double> *v = new Variable<double>(name,value);
    v->addInterval(Interval<double>( -std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), 0.00001));
    Vars.push_back(v);
    /*addVariable<double>(name, value);*/
    name.clear();
} 
#endif

void VCSet::addConstraint(std::string cid, std::string cexpr, functor f,int count,...)
{
    va_list args;
    va_start(args,count);
    Constraint *c = new Constraint(*this, cid, cexpr, f, count, &args);
    va_end(args);
    Constraints.push_back(c);
}

VariableAbstract * VCSet::getVariablebyID(std::string vid)
{
    std::list<VariableAbstract *>::iterator i;
    for (i = Vars.begin(); i != Vars.end(); i++) {
	if (vid.compare((**i).getID()) == 0)
	    return *i;
    }
    return NULL;
}

void VCSet::addParameter(std::string pname, int type, void * ptr)
{
    switch (type) {
	case PC_DB_VECTOR_T:
	    {
		Vector * v = new Vector(*this,pname,ptr); 
		ParTable.push_back(v);
		std::cout << "!-- Pushed " << pname << std::endl;
		break;
	    }
	case PC_DB_FASTF_T:
	    break;
	case PC_DB_POINT_T:
	    break;
	default:
	    std::cerr << " Invalid parameter type detected"
		      << pname << std::endl;
    }
}

void VCSet::display()
{
    using std::endl;
    std::list<VariableAbstract *>::iterator i;
    std::list<Constraint *>::iterator j;
    std::cout << "----------------------------------------------" << endl;
    std::cout << "-----------Variable - Constraint Set----------" << endl;
    if (!Vars.empty()) {
	std::cout<< endl << "Variables:" << endl << endl;
	for (i = Vars.begin(); i != Vars.end(); i++)
	    (**i).display();
    }	    
    if (!Constraints.empty()) {
	std::cout<< endl << "Constraints:" << endl << endl;
	for (j = Constraints.begin(); j != Constraints.end(); j++)
	    (**j).display();
    }
}

bool VCSet::check()
{
    std::list<Constraint *>::iterator i;
    for (i = Constraints.begin(); i != Constraints.end(); ++i) {
	if (! (**i).check()) {
	    return false;
	}
    }
    return true;
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
