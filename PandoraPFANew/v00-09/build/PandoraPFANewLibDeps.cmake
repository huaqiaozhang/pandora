# Generated by CMake 2.8.5

IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 2.4)
  # Information for CMake 2.6 and above.
  SET("PandoraFineGranularityContent_LIB_DEPENDS" "general;PandoraMonitoring;general;PandoraFramework;")
  SET("PandoraKMeansContent_LIB_DEPENDS" "general;PandoraMonitoring;general;PandoraFramework;")
  SET("PandoraMonitoring_LIB_DEPENDS" "general;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libCore.so;general;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libCint.so;general;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libRIO.so;general;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libNet.so;general;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libHist.so;general;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libGraf.so;general;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libGraf3d.so;general;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libGpad.so;general;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libTree.so;general;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libRint.so;general;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libPostscript.so;general;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libMatrix.so;general;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libPhysics.so;general;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libMathCore.so;general;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libThread.so;general;/usr/lib64/libdl.so;general;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libEve.so;general;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libGeom.so;general;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libRGL.so;general;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libEG.so;general;PandoraFramework;")
ELSE("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 2.4)
  # Information for CMake 2.4 and lower.
  SET("PandoraFineGranularityContent_LIB_DEPENDS" "PandoraMonitoring;PandoraFramework;")
  SET("PandoraKMeansContent_LIB_DEPENDS" "PandoraMonitoring;PandoraFramework;")
  SET("PandoraMonitoring_LIB_DEPENDS" "/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libCore.so;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libCint.so;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libRIO.so;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libNet.so;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libHist.so;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libGraf.so;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libGraf3d.so;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libGpad.so;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libTree.so;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libRint.so;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libPostscript.so;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libMatrix.so;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libPhysics.so;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libMathCore.so;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libThread.so;/usr/lib64/libdl.so;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libEve.so;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libGeom.so;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libRGL.so;/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/root/5.28.00f/lib/libEG.so;PandoraFramework;")
ENDIF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 2.4)
