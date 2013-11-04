//  Empty skelecton to run pandora
//  Author: Huaqiao ZHANG (IHEP, Beijing)
//  Email:  Huaqiao.Zhang@cern.ch
//  first version: Oct 2013

// EVENT /afs/cern.ch/eng/clic/software/x86_64-slc5-gcc41/ILCSOFT/v01-16/lcio/v02-03-01/include/EVENT/Track.h
#include "runPandora.h"
#include "Api/PandoraApi.h"
#include "TLorentzVector.h"

pandora::Pandora * runPandora::m_pPandora = NULL;

runPandora::runPandora()
{
}

void runPandora::init(){
	m_pPandora = new pandora::Pandora();
	prepareGeometry();
	
}

void runPandora::loopEvent(){
	int nEvents = 2;
	for(int i=0; i<nEvents; i++){
		prepareTrack(i);
		prepareHits(i);
		//preparemcParticle(i);
		PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,PandoraApi::ProcessEvent(*m_pPandora));
		std::cout << "test event " << i << std::endl;
		preparePFO(i);
		PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,PandoraApi::Reset(*m_pPandora));

	}
}

void runPandora::finalize(){
}

void runPandora::prepareTrack(int iEvent){ // function to setup tracks in an event for pandora
	PandoraApi::Track::Parameters trackParameters;
	//    PandoraPFANew/v00-09/include/Api/PandoraApi.h
	//    class TrackParameters
	//    {
	//    public:
	//        pandora::InputFloat             m_d0;                       ///< The 2D impact parameter wrt (0,0), units mm
	//        pandora::InputFloat             m_z0;                       ///< The z coordinate at the 2D distance of closest approach, units mm
	//        pandora::InputInt               m_particleId;               ///< The PDG code of the tracked particle
	//        pandora::InputInt               m_charge;                   ///< The charge of the tracked particle
	//        pandora::InputFloat             m_mass;                     ///< The mass of the tracked particle, units GeV
	//        pandora::InputCartesianVector   m_momentumAtDca;            ///< Track momentum at the 2D distance of closest approach, units GeV
	//        pandora::InputTrackState        m_trackStateAtStart;        ///< Track state at the start of the track, units mm and GeV
	//        pandora::InputTrackState        m_trackStateAtEnd;          ///< Track state at the end of the track, units mm and GeV
	//        pandora::InputTrackState        m_trackStateAtCalorimeter;  ///< The (sometimes projected) track state at the calorimeter, units mm and GeV
	//        pandora::InputFloat             m_timeAtCalorimeter;        ///< The (sometimes projected) time at the calorimeter, units ns
	//        pandora::InputBool              m_reachesCalorimeter;       ///< Whether the track actually reaches the calorimeter
	//        pandora::InputBool              m_isProjectedToEndCap;      ///< Whether the calorimeter projection is to an endcap
	//        pandora::InputBool              m_canFormPfo;               ///< Whether track should form a pfo, if it has an associated cluster
	//        pandora::InputBool              m_canFormClusterlessPfo;    ///< Whether track should form a pfo, even if it has no associated cluster
	//        pandora::InputAddress           m_pParentAddress;           ///< Address of the parent track in the user framework
	//    };
	trackParameters.m_d0 = 0.+iEvent;
	trackParameters.m_z0 = 0.+iEvent;
	trackParameters.m_particleId = 11;
	trackParameters.m_charge = 1;
	trackParameters.m_mass = 0.;
	double trackAtIP_Phi = 0.1;
	double TanLambda = 0.1;
	double pt = 5;
	trackParameters.m_momentumAtDca = pandora::CartesianVector(std::cos(trackAtIP_Phi), std::sin(trackAtIP_Phi), TanLambda) * pt;
	//pandora::TrackState(xs, ys, zs, px, py, pz)
	trackParameters.m_trackStateAtStart 	  = pandora::TrackState(0.1,0.1,0.1,10,12,15);
	trackParameters.m_trackStateAtEnd 	  = pandora::TrackState(0.1,0.1,0.1,10,12,15);
	trackParameters.m_trackStateAtCalorimeter = pandora::TrackState(0.1,0.1,0.1,10,12,15);
	trackParameters.m_timeAtCalorimeter = 0.f;
	trackParameters.m_reachesCalorimeter = true;
	trackParameters.m_isProjectedToEndCap = false;
    	trackParameters.m_canFormPfo = true;
	trackParameters.m_canFormClusterlessPfo = true;
	TLorentzVector thisTrack(5,4,2,1);
	//trackParameters.m_pParentAddress = &thisTrack;

	//PandoraApi::SetTrackParentDaughterRelationship(*m_pPandora, pTrack, trackVec[jTrack]);
	//PandoraApi::SetTrackSiblingRelationship(*m_pPandora, pTrack, trackVec[jTrack]);
	
	PandoraApi::Track::Create(*m_pPandora, trackParameters);
	
}

void runPandora::prepareHits(int iEvent){
	PandoraApi::CaloHit::Parameters caloHitParameters;
    	//class CaloHitBaseParameters
    	//{
    	//public:
    	//    pandora::InputCartesianVector   m_positionVector;           ///< Position vector of center of calorimeter cell, units mm
    	//    pandora::InputCartesianVector   m_expectedDirection;        ///< Unit vector in direction of expected hit propagation
    	//    pandora::InputCartesianVector   m_cellNormalVector;         ///< Unit normal to sampling layer, pointing outwards from the origin
    	//    pandora::InputFloat             m_cellThickness;            ///< Thickness of cell, units mm
    	//    pandora::InputFloat             m_nCellRadiationLengths;    ///< Absorber material in front of cell, units radiation lengths
    	//    pandora::InputFloat             m_nCellInteractionLengths;  ///< Absorber material in front of cell, units interaction lengths
    	//    pandora::InputFloat             m_time;                     ///< Time of (earliest) energy deposition in this cell, units ns
    	//    pandora::InputFloat             m_inputEnergy;              ///< Corrected energy of calorimeter cell in user framework, units GeV
    	//    pandora::InputFloat             m_mipEquivalentEnergy;      ///< The calibrated mip equivalent energy, units mip
    	//    pandora::InputFloat             m_electromagneticEnergy;    ///< The calibrated electromagnetic energy measure, units GeV
    	//    pandora::InputFloat             m_hadronicEnergy;           ///< The calibrated hadronic energy measure, units GeV
    	//    pandora::InputBool              m_isDigital;                ///< Whether cell should be treated as digital
    	//    pandora::InputHitType           m_hitType;                  ///< The type of calorimeter hit
    	//    pandora::InputDetectorRegion    m_detectorRegion;           ///< Region of the detector in which the calo hit is located
    	//    pandora::InputUInt              m_layer;                    ///< The subdetector readout layer number
    	//    pandora::InputBool              m_isInOuterSamplingLayer;   ///< Whether cell is in one of the outermost detector sampling layers
    	//    pandora::InputAddress           m_pParentAddress;           ///< Address of the parent calo hit in the user framework
    	//};
	PandoraApi::CaloHit::Create(*m_pPandora, caloHitParameters);
}

void runPandora::preparePFO(int iEvent){
	const pandora::PfoList *pPfoList = NULL;
	// PandoraPFANew/v00-09/include/Pandora/PandoraInternal.h
	// typedef std::set<ParticleFlowObject *> PfoList;  
	PandoraApi::GetCurrentPfoList(*m_pPandora, pPfoList);
	// 	PandoraPFANew/v00-09/include/Api/PandoraContentApi.h
    	//	class ParticleFlowObject
    	//	{
    	//	public:
    	//	    /**
    	//	     *  @brief  Parameters class
    	//	     */
    	//	    class Parameters
    	//	    {
    	//	    public:
    	//	        pandora::InputInt               m_particleId;       ///< The particle flow object id (PDG code)
    	//	        pandora::InputInt               m_charge;           ///< The particle flow object charge
    	//	        pandora::InputFloat             m_mass;             ///< The particle flow object mass
    	//	        pandora::InputFloat             m_energy;           ///< The particle flow object energy
    	//	        pandora::InputCartesianVector   m_momentum;         ///< The particle flow object momentum
    	//	        pandora::ClusterList            m_clusterList;      ///< The clusters in the particle flow object
    	//	        pandora::TrackList              m_trackList;        ///< The tracks in the particle flow object
    	//	    };
    	//	    /**
    	//	     *  @brief  Create a particle flow object
    	//	     * 
    	//	     *  @param  algorithm the algorithm creating the particle flow object
    	//	     *  @param  particleFlowObjectParameters the particle flow object parameters
    	//	     */
    	//	    static pandora::StatusCode Create(const pandora::Algorithm &algorithm, const Parameters &parameters);
    	//	};
    	//	typedef ParticleFlowObject::Parameters ParticleFlowObjectParameters;
	
}

void runPandora::prepareGeometry(){ // function to setup a geometry for pandora
	PandoraApi::Geometry::Parameters geometryParameters;
	//    PandoraPFANew/v00-09/include/Api/PandoraApi.h
	//    class GeometryParameters
	//    {
	//    public:
	//        class LayerParameters
	//        {
	//        public:
	//            pandora::InputFloat         m_closestDistanceToIp;      ///< Closest distance of the layer from the interaction point, units mm
	//            pandora::InputFloat         m_nRadiationLengths;        ///< Absorber material in front of layer, units radiation lengths
	//            pandora::InputFloat         m_nInteractionLengths;      ///< Absorber material in front of layer, units interaction lengths
	//        };
	//
	//        typedef std::vector<LayerParameters> LayerParametersList;
	//        class SubDetectorParameters
	//        {
	//        public:
	//            pandora::InputFloat         m_innerRCoordinate;         ///< Inner cylindrical polar r coordinate, origin interaction point, units mm
	//            pandora::InputFloat         m_innerZCoordinate;         ///< Inner cylindrical polar z coordinate, origin interaction point, units mm
	//            pandora::InputFloat         m_innerPhiCoordinate;       ///< Inner cylindrical polar phi coordinate (angle wrt cartesian x axis)
	//            pandora::InputUInt          m_innerSymmetryOrder;       ///< Order of symmetry of the innermost edge of subdetector
	//            pandora::InputFloat         m_outerRCoordinate;         ///< Outer cylindrical polar r coordinate, origin interaction point, units mm
	//            pandora::InputFloat         m_outerZCoordinate;         ///< Outer cylindrical polar z coordinate, origin interaction point, units mm
	//            pandora::InputFloat         m_outerPhiCoordinate;       ///< Outer cylindrical polar phi coordinate (angle wrt cartesian x axis)
	//            pandora::InputUInt          m_outerSymmetryOrder;       ///< Order of symmetry of the outermost edge of subdetector
	//            pandora::InputBool          m_isMirroredInZ;            ///< Whether to construct a second subdetector, via reflection in z=0 plane
	//            pandora::InputUInt          m_nLayers;                  ///< The number of layers in the detector section
	//            LayerParametersList         m_layerParametersList;      ///< The list of layer parameters for the detector section
	//        };
	//
	//        typedef std::map<std::string, SubDetectorParameters> SubDetectorParametersMap;
	//
	//        SubDetectorParameters           m_inDetBarrelParameters;    ///< The inner detector barrel parameters
	//        SubDetectorParameters           m_inDetEndCapParameters;    ///< The inner detector end cap parameters
	//        SubDetectorParameters           m_eCalBarrelParameters;     ///< The ecal barrel parameters
	//        SubDetectorParameters           m_eCalEndCapParameters;     ///< The ecal end cap parameters
	//        SubDetectorParameters           m_hCalBarrelParameters;     ///< The hcal barrel parameters
	//        SubDetectorParameters           m_hCalEndCapParameters;     ///< The hcal end cap parameters
	//        SubDetectorParameters           m_muonBarrelParameters;     ///< The muon detector barrel parameters
	//        SubDetectorParameters           m_muonEndCapParameters;     ///< The muon detector end cap parameters
	//        pandora::InputFloat             m_mainTrackerInnerRadius;   ///< The main tracker inner radius, units mm
	//        pandora::InputFloat             m_mainTrackerOuterRadius;   ///< The main tracker outer radius, units mm
	//        pandora::InputFloat             m_mainTrackerZExtent;       ///< The main tracker z extent, units mm
	//        pandora::InputFloat             m_coilInnerRadius;          ///< The coil inner radius, units mm
	//        pandora::InputFloat             m_coilOuterRadius;          ///< The coil outer radius, units mm
	//        }
	geometryParameters.m_mainTrackerInnerRadius = 1;
	geometryParameters.m_mainTrackerOuterRadius = 2;
	geometryParameters.m_mainTrackerZExtent     = 3;
	geometryParameters.m_coilInnerRadius        = 4;
	geometryParameters.m_coilOuterRadius        = 5;
	geometryParameters.m_coilZExtent            = 2;
	std::cout << "before set GEO" << std::endl;
	PandoraApi::Geometry::Create(*m_pPandora, geometryParameters);
	std::cout << "after set GEO" << std::endl;

}


void runPandora::preparemcParticle(int iEvent){ // function to setup a mcParticle for pandora
	PandoraApi::MCParticle::Parameters mcParticleParameters;
	// PandoraPFANew/v00-09/include/Api/PandoraApi.h
    	//class MCParticleParameters
    	//{
    	//public:
    	//    pandora::InputFloat             m_energy;                   ///< The energy of the MC particle, units GeV
    	//    pandora::InputCartesianVector   m_momentum;                 ///< The momentum of the MC particle, units GeV
    	//    pandora::InputCartesianVector   m_vertex;                   ///< The production vertex of the MC particle, units mm
    	//    pandora::InputCartesianVector   m_endpoint;                 ///< The endpoint of the MC particle, units mm
    	//    pandora::InputInt               m_particleId;               ///< The MC particle's ID (PDG code)
    	//    pandora::InputAddress           m_pParentAddress;           ///< Address of the parent MC particle in the user framework
    	//};
	PandoraApi::MCParticle::Create(*m_pPandora, mcParticleParameters);
	//PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::MCParticle::Create(*m_pPandora, mcParticleParameters));
	//PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::SetMCParentDaughterRelationship(*m_pPandora,pMcParticle, *itDaughter));
}
void runPandora::preparemcAssociation(int iEvent){ // function to setup a association information for pandora
	//PandoraPFANew/v00-09/include/Api/PandoraApi.h 
	//
	//PandoraApi::SetCaloHitToMCParticleRelationship
	//PandoraApi::SetTrackToMCParticleRelationship
}
