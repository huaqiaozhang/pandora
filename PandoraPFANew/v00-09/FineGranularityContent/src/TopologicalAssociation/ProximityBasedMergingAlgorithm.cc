/**
 *  @file   PandoraPFANew/FineGranularityContent/src/TopologicalAssociation/ProximityBasedMergingAlgorithm.cc
 * 
 *  @brief  Implementation of the proximity based merging algorithm class.
 * 
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "TopologicalAssociation/ProximityBasedMergingAlgorithm.h"

using namespace pandora;

StatusCode ProximityBasedMergingAlgorithm::Run()
{
    // Begin by recalculating track-cluster associations
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::RunDaughterAlgorithm(*this, m_trackClusterAssociationAlgName));

    const ClusterList *pClusterList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentClusterList(*this, pClusterList));

    // Apply preselection and order clusters by inner layer
    ClusterVector clusterVector;
    for (ClusterList::const_iterator iter = pClusterList->begin(), iterEnd = pClusterList->end(); iter != iterEnd; ++iter)
    {
        if (ClusterHelper::CanMergeCluster(*iter, m_canMergeMinMipFraction, m_canMergeMaxRms))
            clusterVector.push_back(*iter);
    }

    std::sort(clusterVector.begin(), clusterVector.end(), Cluster::SortByInnerLayer);

    // Examine pairs of clusters to evaluate merging suitability. Begin by comparing clusters in highest layers with those in lowest layers.
    for (ClusterVector::reverse_iterator iterI = clusterVector.rbegin(), iterIEnd = clusterVector.rend(); iterI != iterIEnd; ++iterI)
    {
        Cluster *pDaughterCluster = *iterI;

        // Check to see if cluster has already been changed
        if (NULL == pDaughterCluster)
            continue;

        if (!pDaughterCluster->GetAssociatedTrackList().empty())
            continue;

        if (!ClusterHelper::CanMergeCluster(pDaughterCluster, m_canMergeMinMipFraction, m_canMergeMaxRms))
            continue;

        const PseudoLayer daughterInnerLayer(pDaughterCluster->GetInnerPseudoLayer());
        const PseudoLayer daughterOuterLayer(pDaughterCluster->GetOuterPseudoLayer());
        const float daughterHadronicEnergy(pDaughterCluster->GetHadronicEnergy());

        Cluster *pBestParentCluster(NULL);
        float bestParentHadronicEnergy(0.);
        float minGenericDistance(m_maxGenericDistance);

        for (ClusterVector::const_iterator iterJ = clusterVector.begin(), iterJEnd = clusterVector.end(); iterJ != iterJEnd; ++iterJ)
        {
            Cluster *pParentCluster = *iterJ;

            // Check to see if cluster has already been changed
            if ((NULL == pParentCluster) || (pDaughterCluster == pParentCluster))
                continue;

            if (!ClusterHelper::CanMergeCluster(pParentCluster, m_canMergeMinMipFraction, m_canMergeMaxRms))
                continue;

            // Check level of overlap between clusters
            const PseudoLayer parentInnerLayer(pParentCluster->GetInnerPseudoLayer());
            const PseudoLayer parentOuterLayer(pParentCluster->GetOuterPseudoLayer());

            if ((daughterInnerLayer < m_minClusterInnerLayer) && (parentInnerLayer < m_minClusterInnerLayer))
                continue;

            const int layerSpan1(static_cast<int>(parentOuterLayer) - static_cast<int>(daughterInnerLayer));
            const int layerSpan2(static_cast<int>(daughterOuterLayer) - static_cast<int>(parentInnerLayer));
            const int showerLayerSpan(static_cast<int>(daughterInnerLayer) - static_cast<int>(pParentCluster->GetShowerStartLayer()));
            const int layerSpan(std::min(layerSpan1, layerSpan2));

            if ((layerSpan < m_minLayerSpan) || (showerLayerSpan < m_minShowerLayerSpan))
                continue;

            // Check consistency of cluster energy and energy of associated tracks
            float trackEnergySum(0.);
            const TrackList &trackList(pParentCluster->GetAssociatedTrackList());

            for (TrackList::const_iterator trackIter = trackList.begin(), trackIterEnd = trackList.end(); trackIter != trackIterEnd; ++trackIter)
                trackEnergySum += (*trackIter)->GetEnergyAtDca();

            const float parentHadronicEnergy(pParentCluster->GetHadronicEnergy());

            if (trackEnergySum > 0.)
            {
                static const float hadronicEnergyResolution(PandoraSettings::GetHadronicEnergyResolution());
                const float sigmaE(hadronicEnergyResolution * trackEnergySum / std::sqrt(trackEnergySum));

                if (0. == sigmaE)
                    return STATUS_CODE_FAILURE;

                const float clusterEnergySum = (daughterHadronicEnergy + parentHadronicEnergy);

                const float chi((clusterEnergySum - trackEnergySum) / sigmaE);
                const float chi0((parentHadronicEnergy - trackEnergySum) / sigmaE);

                if ((chi > m_maxTrackClusterChi) || ((chi * chi - chi0 * chi0) > m_maxTrackClusterDChi2))
                    continue;
            }

            // Identify the best parent cluster candidate, which has the smallest generic distance
            float genericDistance(std::numeric_limits<float>::max());
            const PseudoLayer startLayer(daughterInnerLayer);
            const PseudoLayer endLayer(daughterInnerLayer + m_nGenericDistanceLayers);

            if (STATUS_CODE_SUCCESS != this->GetGenericDistanceBetweenClusters(pParentCluster, pDaughterCluster, startLayer, endLayer, genericDistance))
                continue;

            if ((genericDistance < minGenericDistance) ||
                ((genericDistance == minGenericDistance) && (parentHadronicEnergy > bestParentHadronicEnergy)))
            {
                minGenericDistance = genericDistance;
                pBestParentCluster = pParentCluster;
                bestParentHadronicEnergy = parentHadronicEnergy;
            }
        }

        // Check distance and inner layer separation for best parent candidate cluster and daughter cluster
        if (NULL == pBestParentCluster)
            continue;

        const CartesianVector parentInnerLayerCentroid(pBestParentCluster->GetCentroid(pBestParentCluster->GetInnerPseudoLayer()));
        const CartesianVector daughterInnerLayerCentroid(pDaughterCluster->GetCentroid(daughterInnerLayer));

        const float innerLayerSeparation((parentInnerLayerCentroid - daughterInnerLayerCentroid).GetMagnitude());

        if (innerLayerSeparation > m_maxInnerLayerSeparation)
            continue;

        // Finally, check to see if daughter cluster is likely to be a fragment of the parent cluster
        if (this->IsClusterFragment(pBestParentCluster, pDaughterCluster))
        {
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::MergeAndDeleteClusters(*this, pBestParentCluster, pDaughterCluster));
            *iterI = NULL;
        }
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ProximityBasedMergingAlgorithm::GetGenericDistanceBetweenClusters(const Cluster *const pParentCluster, const Cluster *const pDaughterCluster,
    const PseudoLayer startLayer, const PseudoLayer endLayer, float &distance) const
{
    bool distanceFound(false);
    float minDistance(std::numeric_limits<float>::max());

    const OrderedCaloHitList &orderedCaloHitListP(pParentCluster->GetOrderedCaloHitList());
    const OrderedCaloHitList &orderedCaloHitListD(pDaughterCluster->GetOrderedCaloHitList());

    for(PseudoLayer iLayer = startLayer; iLayer <= endLayer; ++iLayer)
    {
        OrderedCaloHitList::const_iterator iterP = orderedCaloHitListP.find(iLayer);

        if (orderedCaloHitListP.end() == iterP)
            continue;

        // Loop over hits in parent cluster that fall between specified layers
        for (CaloHitList::const_iterator hitIterP = iterP->second->begin(), hitIterPEnd = iterP->second->end(); hitIterP != hitIterPEnd; ++hitIterP)
        {
            const CartesianVector &positionP((*hitIterP)->GetPositionVector());
            const CartesianVector &directionP((*hitIterP)->GetExpectedDirection());

            // For each hit, consider distance to all hits in daughter cluster that lie within +/-m_nLayersToExamine
            const PseudoLayer firstExaminationLayer((iLayer > m_nAdjacentLayersToExamine) ? iLayer - m_nAdjacentLayersToExamine : 0);
            const PseudoLayer lastExaminationLayer(iLayer + m_nAdjacentLayersToExamine);

            for (PseudoLayer iExaminationLayer = firstExaminationLayer; iExaminationLayer <= lastExaminationLayer; ++iExaminationLayer)
            {
                OrderedCaloHitList::const_iterator iterD = orderedCaloHitListD.find(iExaminationLayer);

                if (orderedCaloHitListD.end() == iterD)
                    continue;

                for (CaloHitList::const_iterator hitIterD = iterD->second->begin(), hitIterDEnd = iterD->second->end(); hitIterD != hitIterDEnd; ++hitIterD)
                {
                    const CartesianVector positionDifference(positionP - (*hitIterD)->GetPositionVector());

                    const float perpendicularDistance((directionP.GetCrossProduct(positionDifference)).GetMagnitude());
                    const float parallelDistance(std::fabs(directionP.GetDotProduct(positionDifference)));

                    if ((perpendicularDistance < minDistance) && (parallelDistance < m_maxParallelDistance))
                    {
                        distanceFound = true;
                        minDistance = perpendicularDistance;
                    }
                }
            }
        }
    }

    if (!distanceFound)
        return STATUS_CODE_NOT_FOUND;

    distance = minDistance;
    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

bool ProximityBasedMergingAlgorithm::IsClusterFragment(const Cluster *const pParentCluster, const Cluster *const pDaughterCluster) const
{
    // Must meet one of following criteria if we are to identify daughter cluster as a fragment of parent:

    // 1. Large fraction of hits in clusters that are deemed to be "close"
    const float closeHitFraction(FragmentRemovalHelper::GetFractionOfCloseHits(pDaughterCluster, pParentCluster, m_closeHitThreshold));

    if ((closeHitFraction - m_minCloseHitFraction) > std::numeric_limits<float>::epsilon())
    {
        return true;
    }

    // 2. Large fraction of cluster overlap layers that are deemed to be "in contact"
    float contactFraction(0.);
    unsigned int nContactLayers(0);

    StatusCode statusCode = FragmentRemovalHelper::GetClusterContactDetails(pDaughterCluster, pParentCluster, m_clusterContactThreshold,
        nContactLayers, contactFraction);

    if ((STATUS_CODE_SUCCESS == statusCode) && ((contactFraction - m_minContactFraction) > std::numeric_limits<float>::epsilon()))
    {
        return true;
    }

    // 3. Small distance between extrapolation of parent-associated track helices and daughter cluster
    const PseudoLayer daughterInnerLayer(pDaughterCluster->GetInnerPseudoLayer());
    const float daughterZCoordinate(pDaughterCluster->GetCentroid(daughterInnerLayer).GetZ());
    const TrackList &parentTrackList(pParentCluster->GetAssociatedTrackList());

    for (TrackList::const_iterator trackIter = parentTrackList.begin(), trackIterEnd = parentTrackList.end(); trackIter != trackIterEnd; ++trackIter)
    {
        // First sanity check helix pathlength from calorimeter surface to daughter cluster candidate
        const Helix *const pHelix((*trackIter)->GetHelixFitAtCalorimeter());
        const CartesianVector &momentum(pHelix->GetMomentum());
        const float deltaZ(std::fabs(pHelix->GetReferencePoint().GetZ() - daughterZCoordinate));

        if((0. == momentum.GetZ()) || ((momentum.GetMagnitude() / momentum.GetZ()) * deltaZ > m_maxHelixPathlengthToDaughter))
        {
            continue;
        }

        // Then check average distance between helix projection and daughter cluster
        float closestDistanceToHit(std::numeric_limits<float>::max()), meanDistanceToHits(std::numeric_limits<float>::max());

        if (STATUS_CODE_SUCCESS != FragmentRemovalHelper::GetClusterHelixDistance(pDaughterCluster, pHelix, daughterInnerLayer,
            daughterInnerLayer + m_helixDistanceNLayers, m_helixDistanceMaxOccupiedLayers, closestDistanceToHit, meanDistanceToHits))
        {
            continue;
        }

        if (meanDistanceToHits < m_maxClusterHelixDistance)
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ProximityBasedMergingAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ProcessFirstAlgorithm(*this, xmlHandle, m_trackClusterAssociationAlgName));

    m_canMergeMinMipFraction = 0.7f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "CanMergeMinMipFraction", m_canMergeMinMipFraction));

    m_canMergeMaxRms = 5.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "CanMergeMaxRms", m_canMergeMaxRms));

    m_minClusterInnerLayer = 6;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinClusterInnerLayer", m_minClusterInnerLayer));

    m_minLayerSpan = -2;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinLayerSpan", m_minLayerSpan));

    m_minShowerLayerSpan = -4;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinShowerLayerSpan", m_minShowerLayerSpan));

    m_maxTrackClusterChi = 2.5f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxTrackClusterChi", m_maxTrackClusterChi));

    m_maxTrackClusterDChi2 = 1.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxTrackClusterDChi2", m_maxTrackClusterDChi2));

    m_nGenericDistanceLayers = 5;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "NGenericDistanceLayers", m_nGenericDistanceLayers));

    m_maxGenericDistance = 50.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxGenericDistance", m_maxGenericDistance));

    m_nAdjacentLayersToExamine = 2;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "NAdjacentLayersToExamine", m_nAdjacentLayersToExamine));

    m_maxParallelDistance = 1000.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxParallelDistance", m_maxParallelDistance));

    m_maxInnerLayerSeparation = 500.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxInnerLayerSeparation", m_maxInnerLayerSeparation));

    m_clusterContactThreshold = 2.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ClusterContactThreshold", m_clusterContactThreshold));

    m_minContactFraction = 0.3f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinContactFraction", m_minContactFraction));

    m_closeHitThreshold = 50.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "CloseHitThreshold", m_closeHitThreshold));

    m_minCloseHitFraction = 0.2f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinCloseHitFraction", m_minCloseHitFraction));

    m_maxHelixPathlengthToDaughter = 300.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxHelixPathlengthToDaughter", m_maxHelixPathlengthToDaughter));

    m_helixDistanceNLayers = 20;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "HelixDistanceNLayers", m_helixDistanceNLayers));

    m_helixDistanceMaxOccupiedLayers = 10;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "HelixDistanceMaxOccupiedLayers", m_helixDistanceMaxOccupiedLayers));

    m_maxClusterHelixDistance = 50.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxClusterHelixDistance", m_maxClusterHelixDistance));

    return STATUS_CODE_SUCCESS;
}
