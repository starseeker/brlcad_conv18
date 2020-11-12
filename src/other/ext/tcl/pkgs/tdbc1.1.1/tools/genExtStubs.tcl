# genExtStubs.tcl --
#
#	Generates an import table for one or more external dynamic
#	link libraries.
#
# Usage:
#
#	tclsh genExtStubs.tcl stubDefs.txt stubStruct.h stubInit.c
#
# Parameters:
#
#	stubsDefs.txt --
#		Name of a file containing declarations of functions
#		to be stubbed. The functions are expected to be in
#		stylized C where exach appears on a single line, and
#		has the form 'returnType name(param,param,...);'
#		In addition, comments of the following forms
#		are expected to precede the function declarations.
#			/* LIBRARY: name1 name2... */
#		These comments give the rootnames of dynamic link
#		libraries that are expected to contain the functions,
#		in order of preference.
#			/* STUBSTRUCT: prefix */
#		String to be prepended to the function name that translates
#		to its reference in the stub table.
#	stubStruct.h --
#		Name of a file that will contain (a) the declaration
#		of a structure that contains pointers to the stubbed
#		functions, and (b) #defines replacing the function name
#		with references into the stub table

# parseImports --
#
#	Parse the import declarations in a given file
#
# Parameters:
#	stubDefs -- Name of the file to parse
#
# Results:
#
#	Returns a list of tuples. The possible tuples are:
#
#	    libraries NAME NAME...
#		Sets the names of the
#	    prefix NAME
#	        Sets the name of the stub structure to NAME and prefixes
#		all the definitions of the stubbed routines with NAME
#	    import TYPE NAME PARAMS
#		Declares the imported routine NAME to return data of type
#		TYPE and accept parmeters PARAMS.

proc parseImports {stubDefs} {

    set defsFile [open $stubDefs r]
    set imports {}
    set lineNo 0
    while {[gets $defsFile line] >= 0} {
	incr lineNo
	if {[string is space $line]} {
	    # do nothing
	} elseif {[regexp -expanded -- {
	    ^\s*\*\s*LIBRARY:\s+
	    ([a-zA-Z0-9_]+(?:\s+[a-zA-Z0-9_]+)*) # List of library names
	} $line -> m]} {
	    set libNames $m
	    lappend imports [linsert $libNames 0 libraries]
	} elseif {[regexp {^\s*\*\s*STUBSTRUCT:\s*(.*)} $line -> m]} {
	    set stubPrefix $m
	    lappend imports [list prefix $m]
	} elseif {[regexp {^\s*\*\s*CONVENTION:\s*(.*)} $line -> c]} {
	    lappend imports [list convention $c]
	} elseif {[regexp -nocase -- {^\s*#} $line]} {
	    # do nothing
	} elseif {[regexp -nocase -expanded -- {
	    \s*(.*)\s+			# Return type
	    ([[:alpha:]_][[:alnum:]_]+)	# Function name
	    \s*\((.*)\);		# Parameters
	} $line -> type name params]} {
	    lappend imports [list import $type $name $params]
	} else {
	    puts stderr "$stubDefs:$lineNo: unrecognized syntax"
	}
    }
    close $defsFile

    return $imports
}

# writeStructHeader --
#
#	Writes the header of the stubs structure to the '.h' file
#
# Parameters:
#	stubDefs   -- Name of the input file from which stubs are being
#		      generated
#	stubStruct -- Name of the file .h being written
#	structFile -- Channel ID of the .h file being written
#
# Results:
#	None.
#
# Side effects:
#	Writes the 'struct' header to the .h file

proc writeStructHeader {stubDefs stubStruct structFile} {

    chan puts $structFile "/*"
    chan puts $structFile " *[string repeat - 77]"
    chan puts $structFile " *"
    chan puts $structFile " * $stubStruct --"
    chan puts $structFile " *"
    chan puts $structFile " *\tStubs for procedures in [file tail $stubDefs]"
    chan puts $structFile " *"
    chan puts $structFile " * Generated by [file tail $::argv0]: DO NOT EDIT"
    chan puts $structFile " * [clock format [clock seconds] \
				-format {%Y-%m-%d %H:%M:%SZ} -gmt true]"
    chan puts $structFile " *"
    chan puts $structFile " *[string repeat - 77]"
    chan puts $structFile " */"
    chan puts $structFile ""
    chan puts $structFile "typedef struct [file rootname [file tail $stubDefs]] \{"

    return
}

# writeStubDeclarations --
#
#	Writes the declarations of the stubs in the table to the .h file.
#
# Parameters:
#	structFile -- Channel ID of the .h file
#	imports -- List of tuples returned from 'parseImports'
#
# Results:
#	None.
#
# Side effects:
#	C pointer-to-function declarations are written to the given file.

proc writeStubDeclarations {structFile imports} {

    set convention {}
    foreach i $imports {
	set key [lindex $i 0]
	switch -exact -- $key {
	    convention {
		set convention [lindex $i 1]
	    }
	    import {
		lassign $i key type name params
		chan puts $structFile \
		    "    $type (${convention}*${name}Ptr)($params);"
	    }
	    libraries {
		chan puts $structFile {}
		chan puts $structFile \
		    "    /* Functions from libraries: [lrange $i 1 end] */"
		chan puts $structFile {}
	    }
	    default {
	    }
	}
    }

    return
}

# writeStructFooter --
#
#	Writes the close of the 'struct' declaration to the .h file
#
# Parameters:
#	stubDefs   -- Name of the struct
#	structFile -- Channel handle of the .h file
#
# Results:
#	None
#
# Side effects:
#	Structure declaration is closed.

proc writeStructFooter {stubDefs structFile} {
    chan puts $structFile "\} [file rootname [file tail $stubDefs]]\;"
    return
}

# writeStubDefines --
#
#	Write the #define directives that replace stub function calls with
#	indirections through the stubs table.
#
# Parameters:
#	structFile -- Channel id of the .h file
#	imports    -- Table of imports from parseImports

proc writeStubDefines {structFile imports} {

    set stubPrefix {}
    foreach i $imports {
	switch -exact -- [lindex $i 0] {
	    prefix {
		lassign $i -> stubPrefix
	    }
	    import {
		lassign $i -> type name params
		chan puts $structFile "#define $name ($stubPrefix->${name}Ptr)"
	    }
	}
    }
    return $stubPrefix
}

# accumulateLibNames --
#
#	Accumulates the list of library names into the Stub initialization
#
# Parameters:
#	codeVar - Name of variable in caller's scope containing the code
#		  under construction
#	imports - Import definitions from 'parseImports'
#
# Results:
#	Returns the code burst for the initialization file.

proc accumulateLibNames {codeVar imports} {
    upvar 1 $codeVar code
    set sep "\n    "
    foreach i $imports {
	if {[lindex $i 0] eq {libraries}} {
	    foreach lib [lrange $i 1 end] {
		append code $sep \" $lib \"
		set sep ", "
	    }
	}
    }
    append code $sep "NULL"
}

# accumulateSymNames --
#
#	Accumulates the list of import symbols into the Stub initialization
#
# Parameters:
#	codeVar - Name of variable in caller's scope containing the code
#		  under construction
#	imports - Import definitions from 'parseImports'
#
# Results:
#	Returns the code burst for the initialization file.

proc accumulateSymNames {codeVar imports} {
    upvar 1 $codeVar code
    set inLibrary 0
    set sep {}
    foreach i $imports {
	switch -exact -- [lindex $i 0] {
	    import {
		lassign $i key type name args
		append code $sep \n {    } \" $name \"
		set sep ,
	    }
	}
    }
    append code $sep \n {    NULL}
}

# rewriteInitProgram --
#
#	Rewrite the 'stubInit.c' program to contain new definitions
#	of imported routines
#
# Parameters:
#	oldProgram -- Previous content of the 'stubInit.c' file
#	imports    -- Import definitions from 'parseImports'
#
# Results:
#	Returns the new import program

proc rewriteInitProgram {stubDefs oldProgram imports} {
    set newProgram {}
    set sep {}
    set state {}
    foreach piece [split $oldProgram \n] {
	switch -exact -- $state {
	    {} {
		switch -regexp -- $piece {
		    @CREATED@ {
			regsub @CREATED@.* $piece {@CREATED@ } piece
			append piece [clock format [clock seconds] \
					  -format {%Y-%m-%d %H:%M:%SZ} \
					  -gmt 1]
			append piece " by " [file tail $::argv0]
			append piece " from " $stubDefs
		    }
		    @LIBNAMES@ {
			set state ignoring
			accumulateLibNames piece $imports
		    }
		    @SYMNAMES@ {
			set state ignoring
			accumulateSymNames piece $imports
		    }
		}
		append newProgram $sep $piece
		set sep \n

	    }
	    ignoring {
		if {[regexp -- @END@ $piece]} {
		    set state {}
		    append newProgram $sep $piece
		    set sep \n
		}
	    }
	}
    }
    return $newProgram
}

# MAIN PROGRAM - see file header for calling sequence

proc main {stubDefs stubStruct stubInit} {

    # Parse the import definitions

    set imports [parseImports $stubDefs]

    # Write the Stub structure declarations

    set structFile [open $stubStruct w]
    chan configure $structFile -translation lf
    writeStructHeader $stubDefs $stubStruct $structFile
    writeStubDeclarations $structFile $imports
    writeStructFooter $stubDefs $structFile
    set stubPrefix [writeStubDefines $structFile $imports]
    chan puts $structFile "MODULE_SCOPE const [file rootname [file tail $stubDefs]]\
                           *${stubPrefix};"
    close $structFile

    # Write the initializations of the function names to import

    set initFile [open $stubInit r+]
    set initProgram [chan read $initFile]
    set initProgram [rewriteInitProgram $stubDefs $initProgram $imports]
    chan seek $initFile 0
    chan truncate $initFile
    chan configure $initFile -translation lf
    chan puts -nonewline $initFile $initProgram
    close $initFile

}
main {*}$argv