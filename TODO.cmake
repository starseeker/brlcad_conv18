Remaining items:

2.  Do a diff of all the generated scripts using configure_file - make sure what autotools produces 
	 is what is being produced by CMake, and make sure no variable definitions in support of scripts 
	 are nuking variables used by CMake

6.  Review the dist logic in the toplevel Makefile.am.  Gonna have to study up on CPack and CTest -
	 figure out the process for checking permissions, install results, etc. in order to provide
	 the same robustness for CMake generated tarballs that we have for autotools.

7.  Build flags - we supply a lot via several options in autotools - express that in CMake

8.  Review and test binaries - get regression testing working, check mged and archer, etc.

12. Try enabling the Aqua compile logic - it still won't work, but get the build logic to the point
    where the autotools logic is.

13. Enable any remaining things present in autotools but not CMake - RTGL and libpc come to mind.

14. Apply lessons learned to the SCL build logic

15. Try and get archer working in the build directory - this is going to involve carefully studying
    what bu_brlcad_data and friends are up to, and what we need to have in place. Right now mged
	 will crash if either a share dir is created in the build toplevel (empty data dir) or there
	 is an installed BRL-CAD in the target install directory (not sure what the issue is there) but
	 will run in isolation. Even a full copy of the installed share dir in the toplevel build causes
	 a crash, and Archer can't find its files even with the full share dir present (?)

Done (to first order, all this needs testing)

1.  Detect OpenGL properly on Apple - choose X11 vs Aqua, and get the ogl code working

3.  Our tcl autopath function for adding paths to the package search list needs fixing - CMake breaks
    assumptions it was using.

4.  Scrub the third party logic and clean up/simplify - try to get away from using BRLCAD_ variables
    when they aren't needed.

5.  Break logic out of the toplevel into src and src/other dirs - among other things, we want to
    be able to cd in to src and type make to avoid the doc subdirectory.

9.  Multiplatform testing.  Specifically, find a Windows box and conditionalize everything which
    doesn't work out-of-box (lex/yacc and sh based logic are obvious, other probables)

11. Find out why make package from CPack isn't including much of anything, fix it.

10. Convert -D options that need spaces in args (mostly pathnames) to config.h header files - mostly
    this is an issue for Tcl/Tk and packages, but CMake->Visual C++ solutions doesn't tolerate the
	 spaces.


