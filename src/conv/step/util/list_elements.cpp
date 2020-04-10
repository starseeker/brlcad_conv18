/*               L I S T _ E L E M E N T S . C P P
 * BRL-CAD
 *
 * Published in 2020 by the United States Government.
 * This work is in the public domain.
 *
 */
/** @file list_elements.cpp
 * list_elements.cpp
 *
 * List elements and their attributes based on the compiled-in schema.
 * Based on treg.cc from stepcode.  Public Domain.
 */

#include "common.h"

/* C++ Stuff */
#include <iostream>

/* General SCL stuff */
#include <ExpDict.h>
#include <STEPfile.h>
#include <STEPattribute.h>
#include <STEPcomplex.h>
#include <SdaiHeaderSchema.h>
#include <sdai.h>
#include "schema.h"

#include "ap_schema.h"

class SEarrIterator
{
private:
    const STEPentity ** SEarr; // Array of pointers to STEPentity's
    int index;           // current index
    int size;            // size of array
public:
    // Construct the iterator with a given array and its size
    SEarrIterator(const STEPentity ** entarr, int sz)
    {
	SEarr = entarr;
	index = 0;
	size = sz;
    }

    // set the value of the index: SEitr = 3
    void operator= (int newindex)
    {
	index = newindex;
    }

    // check if we're out of range: if (!SEitr)...
    int operator!()
    {
	return (index < size);
    }

    // return current element: SEptr = SEitr()
    STEPentity * operator()()
    {
	return (STEPentity *)SEarr[index];
    }

    // PREFIX increment operator: ++SEitr
    int operator++ ()
    {
	index++;
	return operator!();
    }
};

void PrintEntity(STEPentity * ent)
{
    EntityDescriptor * entDesc = 0;
    cout << ent->EntityName() << " :\n";
    //cout << "Populating " << ent->EntityName() << " which has ";
    //cout << attrCount << " attributes." << endl;


    const EntityDescriptorList * supertypeList = &(ent->eDesc->Supertypes());
    EntityDescLinkNode * supertypePtr = (EntityDescLinkNode *)supertypeList->GetHead();
    entDesc = 0;
    while (supertypePtr != 0) {
	entDesc = supertypePtr->EntityDesc();
	bool logi = entDesc->AbstractEntity().asInt();
	if (logi == false) {
	    cout << "     Subtype of: " << entDesc->Name() << "\n";
	}
	supertypePtr = (EntityDescLinkNode *)supertypePtr->NextNode();
    }


    const EntityDescriptorList * subtypeList = &(ent->eDesc->Subtypes());
    EntityDescLinkNode * subtypePtr = (EntityDescLinkNode *)subtypeList->GetHead();
    entDesc = 0;
    while (subtypePtr != 0) {
	entDesc = subtypePtr->EntityDesc();
	bool logi = entDesc->AbstractEntity().asInt();
	if (logi == false) {
	    cout << "     Supertype of:  " << entDesc->Name() << "\n";
	}
	subtypePtr = (EntityDescLinkNode *)subtypePtr->NextNode();
    }

    ent->ResetAttributes();    // start us walking at the top of the list

    STEPattribute * attr = ent->NextAttribute();
    while (attr != 0) {
	const AttrDescriptor * attrDesc = attr->aDesc;
	cout << "     " << attrDesc->Name() << "[" << attrDesc->TypeName() << "]\n";
	attr = ent->NextAttribute();
    }
    cout << "\n";
}


int main()
{

    // This has to be done before anything else.  This initializes
    // all of the registry information for the schema you are using.
    // The SchemaInit() function is generated by exp2cxx... see
    // extern statement above.

    Registry * registry = new Registry(SchemaInit);

    // The nifty thing about the Registry is that it basically keeps a list
    // of everything in your schema.  What this means is that we can go
    // through the Registry and instantiate, say, one of everything, without
    // knowing at coding-time what entities there are to instantiate.  So,
    // this test could be linked with other class libraries produced from
    // other schema, rather than the example, and run happily.

    InstMgr instance_list;

    // Here's what's going to happen below: we're going to figure out
    // how many different entities there are, instantiate one of each and
    // keep an array of pointers to each.  We'll stick some random data in
    // them as we go.  When we're done, we'll print out everything by walking
    // the array, and then write out the STEPfile information to the screen.

    // Figure outhow many entities there are, then allocate an array
    // to store a pointer to one of each.

    int num_ents = registry->GetEntityCnt();
    STEPentity ** SEarray = new STEPentity*[num_ents];

    // "Reset" the Schema and Entity hash tables... this sets things up
    // so we can walk through the table using registry->NextEntity()

    registry->ResetSchemas();
    registry->ResetEntities();

    // Print out what schema we're running through.

    const SchemaDescriptor * schema = registry->NextSchema();
    cout << "Entities in schema " << schema->Name() << ":\n";

    // "Loop" through the schema, building one of each entity type.

    const EntityDescriptor * ent;  // needs to be declared const...
    for (int i = 0; i < num_ents; i++) {
	ent = registry->NextEntity();

	// Build object, using its name, through the registry
	SEarray[i] = registry->ObjCreate(ent->Name());

	// Add each realized entity to the instance list
	instance_list.Append(SEarray[i], completeSE);

	// Put some data into each instance
	PrintEntity(SEarray[i]);
    }

    // Print out all entities

    SEarrIterator SEitr((const STEPentity **) SEarray, num_ents);
    exit(0);
}


// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
