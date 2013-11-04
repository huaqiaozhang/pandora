#!/bin/tcsh
## setup root for slc6 system as pcbeijing

cd /afs/cern.ch/user/h/huaqiao/public/CMS/slc6/CMSSW_6_2_0_pre3
cmsenv
cd -

### setup the lib path
setenv LD_LIBRARY_PATH /afs/cern.ch/work/h/huaqiao/CMS/ILCSoftwareSetup/PandoraPFANew/v00-09/lib:${LD_LIBRARY_PATH}

