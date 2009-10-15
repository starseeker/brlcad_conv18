/*                 ToroidalSurface.cpp
 * BRL-CAD
 *
 * Copyright (c) 1994-2009 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file ToroidalSurface.cpp
 *
 * Routines to interface to STEP "ToroidalSurface".
 *
 */
#include "STEPWrapper.h"
#include "Factory.h"

#include "ToroidalSurface.h"

#define CLASSNAME "ToroidalSurface"
#define ENTITYNAME "Toroidal_Surface"
string ToroidalSurface::entityname = Factory::RegisterClass(ENTITYNAME,(FactoryMethod)ToroidalSurface::Create);

ToroidalSurface::ToroidalSurface() {
	step = NULL;
	id = 0;
}

ToroidalSurface::ToroidalSurface(STEPWrapper *sw,int STEPid) {
	step=sw;
	id = STEPid;
}

ToroidalSurface::~ToroidalSurface() {
}

bool
ToroidalSurface::Load(STEPWrapper *sw, SCLP23(Application_instance) *sse) {
	step=sw;
	id = sse->STEPfile_id;

	if ( !ElementarySurface::Load(step,sse) ) {
		cout << CLASSNAME << ":Error loading base class ::Surface." << endl;
		return false;
	}

	// need to do this for local attributes to makes sure we have
	// the actual entity and not a complex/supertype parent
	sse = step->getEntity(sse,ENTITYNAME);

	major_radius = step->getRealAttribute(sse,"major_radius");
	minor_radius = step->getRealAttribute(sse,"minor_radius");

	return true;
}

void
ToroidalSurface::Print(int level) {
	TAB(level); cout << CLASSNAME << ":" << name << "(";
	cout << "ID:" << STEPid() << ")" << endl;

	TAB(level+1); cout << "major_radius: " << major_radius << endl;
	TAB(level+1); cout << "minor_radius: " << minor_radius << endl;

	ElementarySurface::Print(level+1);
}

STEPEntity *
ToroidalSurface::Create(STEPWrapper *sw, SCLP23(Application_instance) *sse) {
	Factory::OBJECTS::iterator i;
	if ((i = Factory::FindObject(sse->STEPfile_id)) == Factory::objects.end()) {
		ToroidalSurface *object = new ToroidalSurface(sw,sse->STEPfile_id);

		Factory::AddObject(object);

		if (!object->Load(sw, sse)) {
			cerr << CLASSNAME << ":Error loading class in ::Create() method." << endl;
			delete object;
			return NULL;
		}
		return static_cast<STEPEntity *>(object);
	} else {
		return (*i).second;
	}
}

bool
ToroidalSurface::LoadONBrep(ON_Brep *brep)
{
	cerr << "Error: ::LoadONBrep(ON_Brep *brep) not implemented for " << entityname << endl;
	return false;
}

