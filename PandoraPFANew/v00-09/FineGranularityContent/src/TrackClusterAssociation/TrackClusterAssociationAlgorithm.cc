/**
 *  @file   PandoraPFANew/FineGranularityContent/src/TrackClusterAssociation/TrackClusterAssociationAlgorithm.cc
 * 
 *  @brief  Implementation of the track-cluster association algorithm class.
 * 
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "TrackClusterAssociation/TrackClusterAssociationAlgorithm.h"

using namespace pandora;

StatusCode TrackClusterAssociationAlgorithm::Run()
{
    const TrackList *pTrackList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentTrackList(*this, pTrackList));

    TrackVector trackVector(pTrackList->begin(), pTrackList->end());
    std::sort(trackVector.begin(), trackVector.end(), Track::SortByEnergy);

    const ClusterList *pClusterList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentClusterList(*this, pClusterList));

    // Clear any existing track - cluster associations
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::RemoveCurrentTrackClusterAssociations(*this));

    // Look to make new associations
    for (TrackVector::const_iterator trackIter = trackVector.begin(), trackIterEnd = trackVector.end(); trackIter != trackIterEnd; ++trackIter)
    {
        Track *pTrack = *trackIter;

        // Use only tracks that can be used to form a pfo
        if (!pTrack->CanFormPfo())
            continue;

        if (!pTrack->GetDaughterTrackList().empty())
            continue;

        Cluster *pBestCluster = NULL;
        Cluster *pBestLowEnergyCluster = NULL;

        float minDistance(m_maxTrackClusterDistance);
        float minLowEnergyDistance(m_maxTrackClusterDistance);

        float minEnergyDifference(std::numeric_limits<float>::max());
        float minLowEnergyDifference(std::numeric_limits<float>::max());

        // Identify the closest cluster and also the closest cluster below a specified hadronic energy threshold
        for (ClusterList::const_iterator clusterIter = pClusterList->begin(), clusterIterEnd = pClusterList->end();
            clusterIter != clusterIterEnd; ++clusterIter)
        {
            Cluster *pCluster = *clusterIter;

            if (0 == pCluster->GetNCaloHits())
                continue;

            float trackClusterDistance(std::numeric_limits<float>::max());
            if (STATUS_CODE_SUCCESS != ClusterHelper::GetTrackClusterDistance(pTrack, pCluster, m_maxSearchLayer, m_parallelDistanceCut, trackClusterDistance))
                continue;

            const float energyDifference(std::fabs(pCluster->GetHadronicEnergy() - pTrack->GetEnergyAtDca()));

            if (pCluster->GetHadronicEnergy() > m_lowEnergyCut)
            {
                if ((trackClusterDistance < minDistance) || ((trackClusterDistance == minDistance) && (energyDifference < minEnergyDifference)))
                {
                    minDistance = trackClusterDistance;
                    pBestCluster = pCluster;
                    minEnergyDifference = energyDifference;
                }
            }
            else
            {
                if ((trackClusterDistance < minLowEnergyDistance) || ((trackClusterDistance == minLowEnergyDistance) && (energyDifference < minLowEnergyDifference)))
                {
                    minLowEnergyDistance = trackClusterDistance;
                    pBestLowEnergyCluster = pCluster;
                    minLowEnergyDifference = energyDifference;
                }
            }
        }

        // Apply a final track-cluster association distance cut
        Cluster *pMatchedCluster = NULL;

        if (NULL != pBestCluster)
        {
            pMatchedCluster = pBestCluster;
        }
        else if (NULL != pBestLowEnergyCluster)
        {
            pMatchedCluster = pBestLowEnergyCluster;
        }

        // Now make the association
        if (NULL != pMatchedCluster)
        {
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::AddTrackClusterAssociation(*this, pTrack, pMatchedCluster));
        }
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackClusterAssociationAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    m_lowEnergyCut = 0.2f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "LowEnergyCut", m_lowEnergyCut));

    m_maxTrackClusterDistance = 10.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxTrackClusterDistance", m_maxTrackClusterDistance));

    m_maxSearchLayer = 9;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxSearchLayer", m_maxSearchLayer));

    m_parallelDistanceCut = 100.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ParallelDistanceCut", m_parallelDistanceCut));

    return STATUS_CODE_SUCCESS;
}
