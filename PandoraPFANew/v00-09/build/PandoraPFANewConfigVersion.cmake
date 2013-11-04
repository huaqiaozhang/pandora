##############################################################################
# this file is parsed when FIND_PACKAGE is called with version argument
#
# @author Jan Engels, Desy IT
##############################################################################


SET( ${PACKAGE_FIND_NAME}_VERSION_MAJOR 0 )
SET( ${PACKAGE_FIND_NAME}_VERSION_MINOR 8 )
SET( ${PACKAGE_FIND_NAME}_VERSION_PATCH 0 )


INCLUDE( "/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/ilcutil/v01-00/cmakemodules/MacroCheckPackageVersion.cmake" )
CHECK_PACKAGE_VERSION( ${PACKAGE_FIND_NAME} 0.8.0 )

