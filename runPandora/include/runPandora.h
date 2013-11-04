//  Empty skelecton to run pandora
//  Author: Huaqiao ZHANG (IHEP, Beijing)
//  Email:  Huaqiao.Zhang@cern.ch
//  first version: Oct 2013
#ifndef runPandora_h
#define runPandora_h

#include <vector>
#include <map>
#include <iostream>
#include <string>

namespace pandora {class Pandora;}
class prepareGeometry;

class runPandora {

	public :
		runPandora();
		~runPandora(){};
		void init();
		void loopEvent();
		void finalize();
		void prepareGeometry();
		void preparemcParticle(int iEvent);
		void preparemcAssociation(int iEvent);
		void prepareTrack(int iEvent);
		void prepareHits(int iEvent);
		void preparePFO(int iEvent);

		static pandora::Pandora        *m_pPandora;

};

#endif

