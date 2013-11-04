/**
 *  @file   PandoraPFANew/FineGranularityContent/src/TrackClusterAssociation/TrackRecoveryAlgorithm.cc
 * 
 *  @brief  Implementation of the track recovery algorithm class.
 * 
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "TrackClusterAssociation/TrackRecoveryAlgorithm.h"

using namespace pandora;

StatusCode TrackRecoveryAlgorithm::Run()
{
    const TrackList *pTrackList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentTrackList(*this, pTrackList));

    TrackVector trackVector(pTrackList->begin(), pTrackList->end());
    std::sort(trackVector.begin(), trackVector.end(), Track::SortByEnergy);

    const ClusterList *pClusterList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentClusterList(*this, pClusterList));

    // Loop over all unassociated tracks in the current track list
    for (TrackVector::const_iterator iterT = trackVector.begin(), iterTEnd = trackVector.end(); iterT != iterTEnd; ++iterT)
    {
        Track *pTrack = *iterT;

        // Use only unassociated tracks that can be used to form a pfo
        if (pTrack->HasAssociatedCluster() || !pTrack->CanFormPfo())
            continue;

        // To avoid tracks split along main track z-axis, examine number of parent/daughter tracks and start z coordinate
        const float zStart(std::fabs(pTrack->GetTrackStateAtStart().GetPosition().GetZ()));

        if (!pTrack->GetDaughterTrackList().empty())
            continue;

        if ((zStart > m_maxTrackZStart) && pTrack->GetParentTrackList().empty())
            continue;

        // Extract track energy resolution information
        const float trackEnergy(pTrack->GetEnergyAtDca());
        static const float hadronicEnergyResolution(PandoraSettings::GetHadronicEnergyResolution());

        if ((0. == trackEnergy) || (0. == hadronicEnergyResolution))
            return STATUS_CODE_FAILURE;

        const float sigmaE(hadronicEnergyResolution * trackEnergy / std::sqrt(trackEnergy));

        // Identify best cluster to be associated with this track, based on energy consistency and proximity
        Cluster *pBestCluster(NULL);
        float minEnergyDifference(std::numeric_limits<float>::max());
        float smallestTrackClusterDistance(std::numeric_limits<float>::max());

        for (ClusterList::const_iterator iterC = pClusterList->begin(), iterCEnd = pClusterList->end(); iterC != iterCEnd; ++iterC)
        {
            Cluster *pCluster = *iterC;

            if (!pCluster->GetAssociatedTrackList().empty() || (0 == pCluster->GetNCaloHits()))
                continue;

            const bool isLeavingCluster(ClusterHelper::IsClusterLeavingDetector(pCluster));

            const float deltaE(pCluster->GetTrackComparisonEnergy() - trackEnergy);
            const float chi(deltaE / sigmaE);

            if ((std::fabs(chi) < m_maxAbsoluteTrackClusterChi) || (isLeavingCluster && (chi < 0.f)))
            {
                float trackClusterDistance(std::numeric_limits<float>::max());

                if (STATUS_CODE_SUCCESS != ClusterHelper::GetTrackClusterDistance(pTrack, pCluster, m_maxSearchLayer, m_parallelDistanceCut,
                    trackClusterDistance))
                {
                    continue;
                }

                const float energyDifference(std::fabs(pCluster->GetHadronicEnergy() - pTrack->GetEnergyAtDca()));

                if ((trackClusterDistance < smallestTrackClusterDistance) ||
                    ((trackClusterDistance == smallestTrackClusterDistance) && (energyDifference < minEnergyDifference)))
                {
                    smallestTrackClusterDistance = trackClusterDistance;
                    pBestCluster = pCluster;
                    minEnergyDifference = energyDifference;
                }
            }
        }

        if (NULL == pBestCluster)
            continue;

        // Should track be associated with "best" cluster? Depends on whether track reaches EndCap or Barrel:
        if (pTrack->IsProjectedToEndCap())
        {
            if ( (smallestTrackClusterDistance < m_endCapMaxTrackClusterDistance1) ||
                ((smallestTrackClusterDistance < m_endCapMaxTrackClusterDistance2) && (pBestCluster->GetTrackComparisonEnergy() < trackEnergy)) )
            {
                PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::AddTrackClusterAssociation(*this, pTrack, pBestCluster));
            }
        }
        else
        {
            if (smallestTrackClusterDistance < m_barrelMaxTrackClusterDistance)
            {
                PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::AddTrackClusterAssociation(*this, pTrack, pBestCluster));
            }
        }
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackRecoveryAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    m_maxTrackZStart = 100.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxTrackZStart", m_maxTrackZStart));

    m_maxAbsoluteTrackClusterChi = 2.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxAbsoluteTrackClusterChi", m_maxAbsoluteTrackClusterChi));

    m_endCapMaxTrackClusterDistance1 = 100.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "EndCapMaxTrackClusterDistance1", m_endCapMaxTrackClusterDistance1));

    m_endCapMaxTrackClusterDistance2 = 250.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "EndCapMaxTrackClusterDistance2", m_endCapMaxTrackClusterDistance2));

    m_barrelMaxTrackClusterDistance = 10.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "BarrelMaxTrackClusterDistance", m_barrelMaxTrackClusterDistance));

    m_maxSearchLayer = 19;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxSearchLayer", m_maxSearchLayer));

    m_parallelDistanceCut = 100.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ParallelDistanceCut", m_parallelDistanceCut));

    return STATUS_CODE_SUCCESS;
}
