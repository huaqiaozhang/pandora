/**
 *  @file   PandoraPFANew/FineGranularityContent/src/TrackClusterAssociation/TrackRecoveryHelixAlgorithm.cc
 * 
 *  @brief  Implementation of the track recovery helix algorithm class.
 * 
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "TrackClusterAssociation/TrackRecoveryHelixAlgorithm.h"

using namespace pandora;

StatusCode TrackRecoveryHelixAlgorithm::Run()
{
    TrackAssociationInfoMap trackAssociationInfoMap;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->GetTrackAssociationInfoMap(trackAssociationInfoMap));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->MakeTrackClusterAssociations(trackAssociationInfoMap));

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackRecoveryHelixAlgorithm::GetTrackAssociationInfoMap(TrackAssociationInfoMap &trackAssociationInfoMap) const
{
    const TrackList *pTrackList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentTrackList(*this, pTrackList));

    const ClusterList *pClusterList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentClusterList(*this, pClusterList));

    // Loop over all unassociated tracks in the current track list
    for (TrackList::const_iterator iterT = pTrackList->begin(), iterTEnd = pTrackList->end(); iterT != iterTEnd; ++iterT)
    {
        Track *pTrack = *iterT;

        // Use only unassociated tracks that can be used to form a pfo
        if (pTrack->HasAssociatedCluster() || !pTrack->CanFormPfo())
            continue;

        if (!pTrack->GetDaughterTrackList().empty())
            continue;

        // Extract track information
        const Helix *const pHelix(pTrack->GetHelixFitAtCalorimeter());
        const float trackEnergy(pTrack->GetEnergyAtDca());
        const float trackCalorimeterZPosition(pTrack->GetTrackStateAtCalorimeter().GetPosition().GetZ());

        for (ClusterList::const_iterator iterC = pClusterList->begin(), iterCEnd = pClusterList->end(); iterC != iterCEnd; ++iterC)
        {
            Cluster *pCluster = *iterC;

            if (!pCluster->GetAssociatedTrackList().empty() || (0 == pCluster->GetNCaloHits()) || pCluster->IsPhotonFast())
                continue;

            // Cut on z-coordinate separation between track calorimeter projection and the cluster
            const PseudoLayer innerLayer(pCluster->GetInnerPseudoLayer());
            const float clusterZPosition(pCluster->GetCentroid(innerLayer).GetZ());

            if ((std::fabs(trackCalorimeterZPosition) > (std::fabs(clusterZPosition) + m_maxTrackClusterDeltaZ)) ||
                (trackCalorimeterZPosition * clusterZPosition < 0.f))
            {
                continue;
            }

            // Check consistency of track momentum and cluster energy
            const float chi(ReclusterHelper::GetTrackClusterCompatibility(pCluster->GetTrackComparisonEnergy(), trackEnergy));

            if (std::fabs(chi) > m_maxAbsoluteTrackClusterChi)
                continue;

            // Cut on number of layers crossed by track helix in its motion between calorimeter projection and the cluster
            const PseudoLayer nLayersCrossed(FragmentRemovalHelper::GetNLayersCrossed(pHelix, trackCalorimeterZPosition, clusterZPosition));

            if (nLayersCrossed > m_maxLayersCrossed)
                continue;

            // Get distance of closest approach between track projected direction and cluster
            float trackClusterDistance(std::numeric_limits<float>::max());

            if (STATUS_CODE_SUCCESS != ClusterHelper::GetTrackClusterDistance(pTrack, pCluster, m_maxSearchLayer, m_parallelDistanceCut,
                trackClusterDistance))
            {
                trackClusterDistance = std::numeric_limits<float>::max();
            }

            // Get distance of closest approach between track helix projection and cluster
            float closestDistanceToHit(std::numeric_limits<float>::max());
            float meanDistanceToHits(std::numeric_limits<float>::max());

            if (STATUS_CODE_SUCCESS != FragmentRemovalHelper::GetClusterHelixDistance(pCluster, pHelix, innerLayer,
                innerLayer + m_helixComparisonNLayers, m_helixComparisonMaxOccupiedLayers, closestDistanceToHit, meanDistanceToHits))
            {
                closestDistanceToHit = std::numeric_limits<float>::max();
                meanDistanceToHits = std::numeric_limits<float>::max();
            }

            // Cut on closest distance of approach between track and cluster
            if ( (trackClusterDistance > m_maxTrackClusterDistance) &&
                ((closestDistanceToHit > m_maxClosestHelixClusterDistance) || (meanDistanceToHits > m_maxMeanHelixClusterDistance)) )
            {
                continue;
            }

            AssociationInfo associationInfo(pCluster, std::min(closestDistanceToHit, trackClusterDistance));

            if (!(trackAssociationInfoMap[pTrack].insert(AssociationInfoSet::value_type(associationInfo)).second))
                return STATUS_CODE_FAILURE;
        }
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackRecoveryHelixAlgorithm::MakeTrackClusterAssociations(TrackAssociationInfoMap &trackAssociationInfoMap) const
{
    bool shouldContinue(true);

    while (shouldContinue)
    {
        shouldContinue = false;

        Track *pBestTrack(NULL);
        Cluster *pBestCluster(NULL);
        float minEnergyDifference(std::numeric_limits<float>::max());
        float closestApproach(std::numeric_limits<float>::max());

        // Find the closest track-cluster pairing
        for (TrackAssociationInfoMap::const_iterator iter = trackAssociationInfoMap.begin(), iterEnd = trackAssociationInfoMap.end();
            iter != iterEnd; ++iter)
        {
            for (AssociationInfoSet::const_iterator infoIter = iter->second.begin(), infoIterEnd = iter->second.end();
                infoIter != infoIterEnd; ++infoIter)
            {
                const float approach(infoIter->GetClosestApproach());
                const float energyDifference(std::fabs(infoIter->GetCluster()->GetHadronicEnergy() - iter->first->GetEnergyAtDca()));

                if ((approach < closestApproach) || ((approach == closestApproach) && (energyDifference < minEnergyDifference)))
                {
                    closestApproach = approach;
                    pBestTrack = iter->first;
                    pBestCluster = infoIter->GetCluster();
                    minEnergyDifference = energyDifference;
                }
            }
        }

        // Make the track-cluster association
        if ((NULL != pBestTrack) && (NULL != pBestCluster))
        {
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::AddTrackClusterAssociation(*this, pBestTrack, pBestCluster));

            // Clear information to prevent multiple associations to same track/cluster
            trackAssociationInfoMap.erase(trackAssociationInfoMap.find(pBestTrack));

            for (TrackAssociationInfoMap::iterator iter = trackAssociationInfoMap.begin(), iterEnd = trackAssociationInfoMap.end(); iter != iterEnd; ++iter)
            {
                for (AssociationInfoSet::iterator infoIter = iter->second.begin(), infoIterEnd = iter->second.end(); infoIter != infoIterEnd;)
                {
                    if (infoIter->GetCluster() == pBestCluster)
                    {
                        iter->second.erase(infoIter++);
                    }
                    else
                    {
                        infoIter++;
                    }
                }
            }

            shouldContinue = true;
        }
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackRecoveryHelixAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    m_maxTrackClusterDeltaZ = 250.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxTrackClusterDeltaZ", m_maxTrackClusterDeltaZ));

    m_maxAbsoluteTrackClusterChi = 2.5f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxAbsoluteTrackClusterChi", m_maxAbsoluteTrackClusterChi));

    m_maxLayersCrossed = 50;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxLayersCrossed", m_maxLayersCrossed));

    m_maxSearchLayer = 19;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxSearchLayer", m_maxSearchLayer));

    m_parallelDistanceCut = 100.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ParallelDistanceCut", m_parallelDistanceCut));

    m_helixComparisonNLayers = 20;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "HelixComparisonNLayers", m_helixComparisonNLayers));

    m_helixComparisonMaxOccupiedLayers = 9;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "HelixComparisonMaxOccupiedLayers", m_helixComparisonMaxOccupiedLayers));

    m_maxTrackClusterDistance = 100.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxTrackClusterDistance", m_maxTrackClusterDistance));

    m_maxClosestHelixClusterDistance = 100.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxClosestHelixClusterDistance", m_maxClosestHelixClusterDistance));

    m_maxMeanHelixClusterDistance = 150.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxMeanHelixClusterDistance", m_maxMeanHelixClusterDistance));

    return STATUS_CODE_SUCCESS;
}
