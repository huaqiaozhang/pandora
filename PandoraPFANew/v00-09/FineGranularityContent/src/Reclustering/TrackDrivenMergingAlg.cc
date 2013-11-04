/**
 *  @file   PandoraPFANew/FineGranularityContent/src/Reclustering/TrackDrivenMergingAlg.cc
 * 
 *  @brief  Implementation of the track driven merging algorithm class.
 * 
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "Reclustering/TrackDrivenMergingAlg.h"

using namespace pandora;

StatusCode TrackDrivenMergingAlg::Run()
{
    // Begin by recalculating track-cluster associations
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::RunDaughterAlgorithm(*this, m_trackClusterAssociationAlgName));

    // Examine each cluster in the input list
    const ClusterList *pClusterList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentClusterList(*this, pClusterList));

    ClusterVector clusterVector(pClusterList->begin(), pClusterList->end());
    std::sort(clusterVector.begin(), clusterVector.end(), Cluster::SortByInnerLayer);

    const unsigned int nClusters(clusterVector.size());

    for (unsigned int i = 0; i < nClusters; ++i)
    {
        Cluster *pParentCluster = clusterVector[i];

        if (NULL == pParentCluster)
            continue;

        // Only consider contained clusters with specific range of track associations
        const TrackList &parentTrackList(pParentCluster->GetAssociatedTrackList());
        const unsigned int nTrackAssociations(parentTrackList.size());

        if ((nTrackAssociations < m_minTrackAssociations) || (nTrackAssociations > m_maxTrackAssociations))
            continue;

        // Check compatibility of cluster with its associated tracks
        float trackEnergySum(0.);
        for (TrackList::const_iterator trackIter = parentTrackList.begin(), trackIterEnd = parentTrackList.end(); trackIter != trackIterEnd; ++trackIter)
        {
            trackEnergySum += (*trackIter)->GetEnergyAtDca();
        }

        static const float hadronicEnergyResolution(PandoraSettings::GetHadronicEnergyResolution());

        if ((0. == trackEnergySum) || (0. == hadronicEnergyResolution))
            return STATUS_CODE_FAILURE;

        const float sigmaTrackEnergy(hadronicEnergyResolution * trackEnergySum / std::sqrt(trackEnergySum));
        const float parentClusterEnergy(pParentCluster->GetTrackComparisonEnergy());

        const float originalChi((parentClusterEnergy - trackEnergySum) / sigmaTrackEnergy);

        if (originalChi > m_chiToAttemptMerging)
            continue;

        if (ClusterHelper::IsClusterLeavingDetector(pParentCluster))
            continue;

        // Examine pairs of clusters to evaluate merging suitability.
        float highestConeFraction(0.);
        Cluster *pBestDaughterCluster(NULL);

        static const unsigned int MAX_INDEX(std::numeric_limits<unsigned int>::max());
        unsigned int bestDaughterClusterIndex(MAX_INDEX);

        ClusterConeFractionList clusterConeFractionList;
        const PseudoLayer parentOuterLayer(pParentCluster->GetOuterPseudoLayer());

        for (unsigned int j = 0; j < nClusters; ++j)
        {
            Cluster *pDaughterCluster = clusterVector[j];

            // Check to see if cluster has already been changed
            if ((NULL == pDaughterCluster) || (pParentCluster == pDaughterCluster))
                continue;

            if (!pDaughterCluster->GetAssociatedTrackList().empty())
                continue;

            const float newChi((parentClusterEnergy + pDaughterCluster->GetTrackComparisonEnergy() - trackEnergySum) / sigmaTrackEnergy);

            if (std::fabs(newChi) > m_coarseDaughterChiCut)
                continue;

            // Identify best daughter candidate: that with highest cone fraction
            const float coneFraction(FragmentRemovalHelper::GetFractionOfHitsInCone(pDaughterCluster, pParentCluster, m_coneCosineHalfAngle));

            if ((std::fabs(newChi) < std::fabs(m_chiToAttemptMerging)) && (coneFraction > highestConeFraction))
            {
                highestConeFraction = coneFraction;
                pBestDaughterCluster = pDaughterCluster;
                bestDaughterClusterIndex = j;
            }

            // Store all cone fraction values in order to consider multiple merges
            const PseudoLayer daughterInnerLayer(pDaughterCluster->GetInnerPseudoLayer());

            if (coneFraction > m_minConeFractionMultiple)
            {
                if ((daughterInnerLayer < parentOuterLayer) || (daughterInnerLayer - parentOuterLayer < m_maxLayerSeparationMultiple))
                {
                    clusterConeFractionList.insert(ClusterConeFraction(coneFraction, pDaughterCluster, j));
                }
            }
        }

        // If able to find match to a single cluster do so
        if ((highestConeFraction > m_minConeFractionSingle) && (NULL != pBestDaughterCluster) && (MAX_INDEX != bestDaughterClusterIndex))
        {
            clusterVector[bestDaughterClusterIndex] = NULL;
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::MergeAndDeleteClusters(*this, pParentCluster, pBestDaughterCluster));
            continue;
        }

        // If single merge unsuccessful, consider multiple merges: work through the cone fraction list, starting with highest fractions
        float lastChi(originalChi);
        float daughterClusterEnergySum(0.);
        ClusterConeFractionList daughterClusters;
        bool performMultipleMerge(false);

        for (ClusterConeFractionList::const_iterator iter = clusterConeFractionList.begin(), iterEnd = clusterConeFractionList.end();
            iter != iterEnd; ++iter)
        {
            daughterClusterEnergySum += iter->GetCluster()->GetTrackComparisonEnergy();
            const float newChi((parentClusterEnergy + daughterClusterEnergySum - trackEnergySum) / sigmaTrackEnergy);

            if ((std::fabs(newChi) > std::fabs(lastChi)) || (newChi > std::fabs(m_chiToAttemptMerging)))
                break;

            if (std::fabs(newChi) < std::fabs(m_chiToAttemptMerging))
                performMultipleMerge = true;

            lastChi = newChi;
            daughterClusters.insert(*iter);
        }

        if (!performMultipleMerge)
            continue;

        // Finally, merge parent with the selected daughter clusters
        for (ClusterConeFractionList::const_iterator iter = daughterClusters.begin(), iterEnd = daughterClusters.end(); iter != iterEnd; ++iter)
        {
            clusterVector[iter->GetClusterIndex()] = NULL;
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::MergeAndDeleteClusters(*this, pParentCluster, iter->GetCluster()));
        }

        // ATTN: haven't implemented the "dubious partial merge" present in old Pandora. This would merge daughter cluster with highest
        // fraction above 0.75, in situation where std::fabs(newChi) > 2.5 AND (newChi > 2.5 OR magnitude of chi is increasing).
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackDrivenMergingAlg::ReadSettings(const TiXmlHandle xmlHandle)
{
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ProcessAlgorithm(*this, xmlHandle, "TrackClusterAssociation",
        m_trackClusterAssociationAlgName));

    m_minTrackAssociations = 1;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinTrackAssociations", m_minTrackAssociations));

    if (m_minTrackAssociations < 1)
        return STATUS_CODE_INVALID_PARAMETER;

    m_maxTrackAssociations = std::numeric_limits<unsigned int>::max();
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxTrackAssociations", m_maxTrackAssociations));

    m_chiToAttemptMerging = -2.5f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ChiToAttemptMerging", m_chiToAttemptMerging));

    m_coarseDaughterChiCut = 200.5f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "CoarseDaughterChiCut", m_coarseDaughterChiCut));

    m_coneCosineHalfAngle = 0.9f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ConeCosineHalfAngle", m_coneCosineHalfAngle));

    m_minConeFractionSingle = 0.5f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinConeFractionSingle", m_minConeFractionSingle));

    m_minConeFractionMultiple = 0.4f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinConeFractionMultiple", m_minConeFractionMultiple));

    m_maxLayerSeparationMultiple = 5;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxLayerSeparationMultiple", m_maxLayerSeparationMultiple));

    return STATUS_CODE_SUCCESS;
}
