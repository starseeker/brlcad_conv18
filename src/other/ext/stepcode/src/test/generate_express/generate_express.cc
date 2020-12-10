/*
 * generate_express.cc
 *
 *
 */

/* Since generate_express doesn't use any schema-specific things except the
   init function, we don't need to include the schema's header file inside
   tests.h  */
#define DONT_NEED_HEADER
#include <tests.h>

/********************  main()  ****************************/

main()
{
    // This has to be done before anything else.  This initializes
    // all of the registry information for the schema you are using.
    // The SchemaInit() function is generated by exp2cxx... see
    // extern statement above.

    Registry *registry = new Registry(SchemaInit);

    // "Reset" has tables for browsing
    registry->ResetSchemas();

    const SchemaDescriptor *schema = 0;

    SchemaDescriptor *schema2 = 0;
    schema = (SchemaDescriptor *)registry->FindSchema("Example_Schema");
    EntityDescriptor *ed = (EntityDescriptor *)registry->FindEntity("Circle");
    Uniqueness_rule_ptr ur = new Uniqueness_rule;
    ur->comment_("(* Hi Dave *)\n");
    if(ed->_uniqueness_rules) {
        ed->_uniqueness_rules->Append(ur);
    } else {
        ed->uniqueness_rules_(new Uniqueness_rule__set);
        ed->_uniqueness_rules->Append(ur);
    }

    registry->ResetSchemas();
    schema = registry->NextSchema();
    ofstream *efile;
    std::string str, tmp;

    while(schema != 0) {
        str = "";
        str.Append(StrToLower(schema->Name(), tmp));
        str.Append(".exp");
        efile = new ofstream(str.c_str());
        cout << "Generating: " << str << endl;
        schema->GenerateExpress(*efile);
        efile->close();
        delete efile;

        schema = registry->NextSchema();
    }
    cout << "Done!" << endl;
}
