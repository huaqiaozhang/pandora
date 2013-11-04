/**
 *  @file   PandoraPFANew/FineGranularityContent/include/ParticleId/MuonReconstructionAlgorithm.h
 * 
 *  @brief  Header file for the muon reconstruction algorithm class.
 * 
 *  $Log: $
 */
#ifndef MUON_RECONSTRUCTION_ALGORITHM_H
#define MUON_RECONSTRUCTION_ALGORITHM_H 1

#include "Pandora/Algorithm.h"

/**
 *  @brief  MuonReconstructionAlgorithm class
 */
class MuonReconstructionAlgorithm : public pandora::Algorithm
{
public:
    /**
     *  @brief  Factory class for instantiating algorithm
     */
    class Factory : public pandora::AlgorithmFactory
    {
    public:
        pandora::Algorithm *CreateAlgorithm() const;
    };

private:
    typedef const void *Uid;

    pandora::StatusCode Run();
    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

    /**
     *  @brief  Associate muon clusters with appropriate tracks
     * 
     *  @param  pMuonClusterList address of the muon cluster list
     */
    pandora::StatusCode AssociateMuonTracks(const pandora::ClusterList *const pMuonClusterList) const;

    /**
     *  @brief  Get the coordinates of the point at which a helix enters the muon detectors
     * 
     *  @param  pHelix address of the helix
     *  @param  isPositiveZ whether to project the helix to the muon endcap with positive or negative z coordiante
     *  @param  muonEntryPoint to receive the muon entry point
     */
    pandora::StatusCode GetMuonEntryPoint(const pandora::Helix *const pHelix, const bool isPositiveZ, pandora::CartesianVector &muonEntryPoint) const;

    /**
     *  @brief  Add appropriate calo hits in the ecal/hcal to the muon clusters
     * 
     *  @param  pMuonClusterList address of the muon cluster list
     *  @param  inputCaloHitListName the name of the algorithm input calo hit list
     */
    pandora::StatusCode AddCaloHits(const pandora::ClusterList *const pMuonClusterList, const std::string &inputCaloHitListName) const;

    /**
     *  @brief  Create the muon pfos
     * 
     *  @param  pMuonClusterList address of the muon cluster list
     */
    pandora::StatusCode CreateMuonPfos(const pandora::ClusterList *const pMuonClusterList) const;

    /**
     *  @brief  Tidy all relevant pandora lists, saving the muon clusters and saving muon-removed track and calo hit lists
     * 
     *  @param  inputTrackListName the name of the algorithm input track list
     *  @param  inputCaloHitListName the name of the algorithm input calo hit list
     *  @param  muonClusterListName the name of the list containing the initial muon clusters
     */
    pandora::StatusCode TidyLists(const std::string &inputTrackListName, const std::string &inputCaloHitListName, const std::string &muonClusterListName) const;

    typedef std::pair<pandora::CaloHit *, float> TrackDistanceInfo;
    typedef std::vector<TrackDistanceInfo> TrackDistanceInfoVector;

    /**
     *  @brief  Sort TrackDistanceInfo objects by increasing distance from track
     * 
     *  @param  lhs the first calo hit distance pair
     *  @param  rhs the second calo hit distance pair
     */
    static bool SortByDistanceToTrack(const TrackDistanceInfo &lhs, const TrackDistanceInfo &rhs);

    /**
     *  @brief  Get lists of the components used to build pfos
     * 
     *  @param  pfoTrackList to receive the list of tracks in pfos
     *  @param  pfoCaloHitList to receive the list of calo hits in pfos
     *  @param  pfoClusterList to receive the list of clusters in pfos
     */
    pandora::StatusCode GetPfoComponents(pandora::TrackList &pfoTrackList, pandora::CaloHitList &pfoCaloHitList, pandora::ClusterList &pfoClusterList) const;

    std::string     m_muonCaloHitListName;          ///< The name of the original calo hit list containing muon hits
    std::string     m_muonClusteringAlgName;        ///< The name of the muon clustering algorithm to run

    unsigned int    m_maxClusterCaloHits;           ///< The maximum number of calo hits in a muon cluster
    unsigned int    m_minClusterOccupiedLayers;     ///< The minimum number of occupied layers in a muon cluster
    unsigned int    m_minClusterLayerSpan;          ///< The minimum layer span for a muon cluster
    unsigned int    m_nClusterLayersToFit;          ///< The number of layers to use in the fit to the muon cluster
    float           m_maxClusterFitChi2;            ///< The maximum fit chi2 for a muon cluster

    float           m_maxDistanceToTrack;           ///< The maximum distance from track helix to muon cluster
    float           m_minTrackCandidateEnergy;      ///< The minimum energy for a muon candidate track
    float           m_minHelixClusterCosAngle;      ///< The minimum cosine of the angle between muon candidate cluster and helix

    unsigned int    m_nExpectedTracksPerCluster;    ///< Expected number of tracks associated to each muon cluster
    unsigned int    m_nExpectedParentTracks;        ///< Expected number of parent tracks for muon-associated track

    float           m_minHelixCaloHitCosAngle;      ///< The minimum cosine of the angle between muon candidate ecal/hcal hits and helix
    float           m_region1GenericDistance;       ///< Generic distance (helix->hit distance / pad-width) value to define region 1
    float           m_region2GenericDistance;       ///< Generic distance (helix->hit distance / pad-width) value to define region 2
    unsigned int    m_isolatedMinRegion1Hits;       ///< Min number of hits in region 1 for an isolated muon
    unsigned int    m_isolatedMaxRegion2Hits;       ///< Max number of hits in region 2 for an isolated muon
    float           m_maxGenericDistance;           ///< Max generic distance to add ecal/hcal hit to muon
    float           m_isolatedMaxGenericDistance;   ///< Max generic distance to add ecal/hcal hit to isolated muon

    std::string     m_outputMuonPfoListName;        ///< The name of the output muon pfo list
    std::string     m_outputMuonClusterListName;    ///< The name of the output muon cluster list
    std::string     m_outputTrackListName;          ///< The name of the output muon-removed track list
    std::string     m_outputCaloHitListName;        ///< The name of the output muon-removed calo hit list
    std::string     m_outputMuonCaloHitListName;    ///< The name of the output muon calo hit list, after removal of hits in muon pfos

    bool            m_shouldClusterIsolatedHits;    ///< Whether to directly include isolated hits in newly formed clusters
};

//------------------------------------------------------------------------------------------------------------------------------------------

inline bool MuonReconstructionAlgorithm::SortByDistanceToTrack(const TrackDistanceInfo &lhs, const TrackDistanceInfo &rhs)
{
    return (lhs.second < rhs.second);
}

//------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------

inline pandora::Algorithm *MuonReconstructionAlgorithm::Factory::CreateAlgorithm() const
{
    return new MuonReconstructionAlgorithm();
}

#endif // #ifndef MUON_RECONSTRUCTION_ALGORITHM_H
