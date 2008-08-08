/*                       P C M A T H V M . C P P
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
/** @addtogroup pcsolver */
/** @{ */
/** @file pcMathVM.cpp
 *
 * Math VM Method implementation
 *
 * @author Dawn Thomas
 */

#include "pcMathVM.h"
#include <cassert>

/**
 * 				Stack Object Methods
 */

/** Copy constructor */
Stack::Stack(Stack const & s)
{
    copy(s.data);
}

/** Private copy function used by the copy constructor */
void Stack::copy(Stack::container_t const & a)
{
    data.clear();
    Stack::container_t::const_iterator i = a.begin();
    Stack::container_t::const_iterator const end = a.end();
    for (; i != end; ++i)
	data.push_back(*i);
}

/** Size access method */
Stack::size_type Stack::size() const
{
    return data.size();
}

/** Stack empty check method */
bool Stack::empty() const
{
    return data.empty();
}

/** Node addition method */
void Stack::push_back(Node * n)
{
    /** assert that node exists */
    assert(n);
    data.push_back(boost::shared_ptr<Node>(n));
}

/** Stack clear */
void Stack::clear()
{
    data.clear();
}

/**
 * 			     Math Virtual Machine methods
 */

void MathVM::display()
{

}
    
/** 
 * 				  MathFunction methods
 */

MathFunction::MathFunction(std::string const & n) :
	name(n)
{}

/** getName() returns the name of the function */
std::string const & MathFunction::getName() const
{
    return name;
}

/** 
 * eval() is effectively a wrapper function over the private evaluation
 * function . It does an arity check to match the number of provided
 * arguments with the expected.
 */

double MathFunction::eval(std::vector<double> const & args) const
{
    assert(args.size() == arity());
    return evalp(args);
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
