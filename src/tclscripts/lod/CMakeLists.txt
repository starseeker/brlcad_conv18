set(lod_TCLSCRIPTS
  LODDialog.tcl
  )
BRLCAD_ADDDATA(lod_TCLSCRIPTS tclscripts/lod)
tclIndex_BUILD(tclscripts/lod)

CMAKEFILES(CMakeLists.txt)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
