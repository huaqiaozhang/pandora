/**
 *  @file   PandoraPFANew/FineGranularityContent/src/Clustering/ConeClusteringAlgorithm.cc
 * 
 *  @brief  Implementation of the clustering algorithm class.
 * 
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "Clustering/ConeClusteringAlgorithm.h"

using namespace pandora;

unsigned int CustomHitOrder::m_hitSortingStrategy = 0;

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ConeClusteringAlgorithm::Run()
{
    const CaloHitList *pCaloHitList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentCaloHitList(*this, pCaloHitList));

    if (pCaloHitList->empty())
        return STATUS_CODE_SUCCESS;

    OrderedCaloHitList orderedCaloHitList;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, orderedCaloHitList.Add(*pCaloHitList));

    ClusterVector clusterVector;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->SeedClustersWithTracks(clusterVector));

    for (OrderedCaloHitList::const_iterator iter = orderedCaloHitList.begin(), iterEnd = orderedCaloHitList.end(); iter != iterEnd; ++iter)
    {
        const PseudoLayer pseudoLayer(iter->first);
        CustomSortedCaloHitList customSortedCaloHitList;

        for (CaloHitList::const_iterator hitIter = iter->second->begin(), hitIterEnd = iter->second->end(); hitIter != hitIterEnd; ++hitIter)
        {
            CaloHit *pCaloHit = *hitIter;

            if ((m_shouldUseIsolatedHits || !pCaloHit->IsIsolated()) &&
                (!m_shouldUseOnlyECalHits || (ECAL == pCaloHit->GetHitType())) &&
                (PandoraContentApi::IsCaloHitAvailable(*this, pCaloHit)))
            {
                customSortedCaloHitList.insert(pCaloHit);
            }
        }

        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->FindHitsInPreviousLayers(pseudoLayer, &customSortedCaloHitList, clusterVector));
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->FindHitsInSameLayer(pseudoLayer, &customSortedCaloHitList, clusterVector));
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->UpdateClusterProperties(clusterVector));
    }

    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->RemoveEmptyClusters(clusterVector));

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ConeClusteringAlgorithm::SeedClustersWithTracks(ClusterVector &clusterVector) const
{
    if (0 == m_clusterSeedStrategy)
        return STATUS_CODE_SUCCESS;

    const TrackList *pTrackList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentTrackList(*this, pTrackList));

    for (TrackList::const_iterator iter = pTrackList->begin(), iterEnd = pTrackList->end(); iter != iterEnd; ++iter)
    {
        Track *pTrack = *iter;

        if (!pTrack->CanFormPfo())
            continue;

        bool useTrack(false);

        if (2 == m_clusterSeedStrategy)
        {
            useTrack = true;
        }
        else if ((1 == m_clusterSeedStrategy) && pTrack->IsProjectedToEndCap())
        {
            useTrack = true;
        }

        if (useTrack)
        {
            Cluster *pCluster = NULL;
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::Cluster::Create(*this, pTrack, pCluster));
            clusterVector.push_back(pCluster);
        }
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ConeClusteringAlgorithm::FindHitsInPreviousLayers(PseudoLayer pseudoLayer, CustomSortedCaloHitList *const pCustomSortedCaloHitList,
    ClusterVector &clusterVector) const
{
    for (CustomSortedCaloHitList::iterator iter = pCustomSortedCaloHitList->begin(), iterEnd = pCustomSortedCaloHitList->end();
        iter != iterEnd;)
    {
        CaloHit *pCaloHit = *iter;

        Cluster *pBestCluster = NULL;
        float bestClusterEnergy(0.f);
        float smallestGenericDistance(m_genericDistanceCut);
        const PseudoLayer layersToStepBack((GeometryHelper::GetHitTypeGranularity(pCaloHit->GetHitType()) <= FINE) ?
            m_layersToStepBackFine : m_layersToStepBackCoarse);

        // Associate with existing clusters in stepBack layers. If stepBackLayer == pseudoLayer, will examine track projections
        for (PseudoLayer stepBackLayer = 1; (stepBackLayer <= layersToStepBack) && (stepBackLayer <= pseudoLayer); ++stepBackLayer)
        {
            const PseudoLayer searchLayer(pseudoLayer - stepBackLayer);

            // See if hit should be associated with any existing clusters
            for (ClusterVector::iterator clusterIter = clusterVector.begin(), clusterIterEnd = clusterVector.end();
                clusterIter != clusterIterEnd; ++clusterIter)
            {
                Cluster *pCluster = *clusterIter;
                float genericDistance(std::numeric_limits<float>::max());
                const float clusterEnergy(pCluster->GetHadronicEnergy());

                PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_UNCHANGED, !=, this->GetGenericDistanceToHit(pCluster,
                    pCaloHit, searchLayer, genericDistance));

                if ((genericDistance < smallestGenericDistance) || ((genericDistance == smallestGenericDistance) && (clusterEnergy > bestClusterEnergy)))
                {
                    pBestCluster = pCluster;
                    bestClusterEnergy = clusterEnergy;
                    smallestGenericDistance = genericDistance;
                }
            }

            // Add best hit found after completing examination of a stepback layer
            if ((0 == m_clusterFormationStrategy) && (NULL != pBestCluster))
            {
                PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::AddCaloHitToCluster(*this, pBestCluster, pCaloHit));
                break;
            }
        }

        // Add best hit found after examining all stepback layers
        if ((1 == m_clusterFormationStrategy) && (NULL != pBestCluster))
        {
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::AddCaloHitToCluster(*this, pBestCluster, pCaloHit));
        }

        // Tidy the energy sorted calo hit list
        if (!PandoraContentApi::IsCaloHitAvailable(*this, pCaloHit))
        {
            pCustomSortedCaloHitList->erase(iter++);
        }
        else
        {
            iter++;
        }
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ConeClusteringAlgorithm::FindHitsInSameLayer(PseudoLayer pseudoLayer, CustomSortedCaloHitList *const pCustomSortedCaloHitList,
    ClusterVector &clusterVector) const
{
    while (!pCustomSortedCaloHitList->empty())
    {
        bool clustersModified = true;

        while (clustersModified)
        {
            clustersModified = false;

            for (CustomSortedCaloHitList::iterator iter = pCustomSortedCaloHitList->begin(), iterEnd = pCustomSortedCaloHitList->end();
                iter != iterEnd;)
            {
                CaloHit *pCaloHit = *iter;

                Cluster *pBestCluster = NULL;
                float bestClusterEnergy(0.f);
                float smallestGenericDistance(m_genericDistanceCut);

                // See if hit should be associated with any existing clusters
                for (ClusterVector::iterator clusterIter = clusterVector.begin(), clusterIterEnd = clusterVector.end();
                    clusterIter != clusterIterEnd; ++clusterIter)
                {
                    Cluster *pCluster = *clusterIter;
                    float genericDistance(std::numeric_limits<float>::max());
                    const float clusterEnergy(pCluster->GetHadronicEnergy());

                    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_UNCHANGED, !=, this->GetGenericDistanceToHit(pCluster,
                        pCaloHit, pseudoLayer, genericDistance));

                    if ((genericDistance < smallestGenericDistance) || ((genericDistance == smallestGenericDistance) && (clusterEnergy > bestClusterEnergy)))
                    {
                        pBestCluster = pCluster;
                        bestClusterEnergy = clusterEnergy;
                        smallestGenericDistance = genericDistance;
                    }
                }

                if (NULL != pBestCluster)
                {
                    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::AddCaloHitToCluster(*this, pBestCluster, pCaloHit));
                    pCustomSortedCaloHitList->erase(iter++);
                    clustersModified = true;
                }
                else
                {
                    iter++;
                }
            }
        }

        // Seed a new cluster
        if (!pCustomSortedCaloHitList->empty())
        {
            CaloHit *pCaloHit = *(pCustomSortedCaloHitList->begin());
            pCustomSortedCaloHitList->erase(pCaloHit);

            Cluster *pCluster = NULL;
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::Cluster::Create(*this, pCaloHit, pCluster));
            clusterVector.push_back(pCluster);
        }
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ConeClusteringAlgorithm::UpdateClusterProperties(ClusterVector &clusterVector) const
{
    // TODO replace this eventually - it remains only to reproduce old pandora results
    for (ClusterVector::iterator iter = clusterVector.begin(), iterEnd = clusterVector.end(); iter != iterEnd; ++iter)
    {
        Cluster *pCluster = *iter;

        if (pCluster->GetNCaloHits() < 2)
            continue;

        ClusterHelper::ClusterFitPointList clusterFitPointList;
        ClusterHelper::ClusterFitResult clusterFitResult;

        const PseudoLayer innerLayer(pCluster->GetInnerPseudoLayer());
        const PseudoLayer outerLayer(pCluster->GetOuterPseudoLayer());
        const PseudoLayer nLayersSpanned(outerLayer - innerLayer);

        if (nLayersSpanned > m_nLayersSpannedForFit)
        {
            PseudoLayer nLayersToFit(m_nLayersToFit);

            if (pCluster->GetMipFraction() - m_nLayersToFitLowMipCut < std::numeric_limits<float>::epsilon())
                nLayersToFit *= m_nLayersToFitLowMipMultiplier;

            const PseudoLayer startLayer( (nLayersSpanned > nLayersToFit) ? (outerLayer - nLayersToFit) : innerLayer);
            (void) ClusterHelper::FitLayerCentroids(pCluster, startLayer, outerLayer, clusterFitResult);

            if (clusterFitResult.IsFitSuccessful())
            {
                const float dotProduct(clusterFitResult.GetDirection().GetDotProduct(pCluster->GetInitialDirection()));
                const float chi2(clusterFitResult.GetChi2());

                if (((dotProduct < m_fitSuccessDotProductCut1) && (chi2 > m_fitSuccessChi2Cut1)) ||
                    ((dotProduct < m_fitSuccessDotProductCut2) && (chi2 > m_fitSuccessChi2Cut2)) )
                {
                    clusterFitResult.SetSuccessFlag(false);
                }

                if ((chi2 > m_mipTrackChi2Cut) && pCluster->IsMipTrack())
                    pCluster->SetIsMipTrackFlag(false);
            }
        }
        else if (nLayersSpanned > m_nLayersSpannedForApproxFit)
        {
            const CartesianVector centroidChange(pCluster->GetCentroid(outerLayer) - pCluster->GetCentroid(innerLayer));
            clusterFitResult.Reset();
            clusterFitResult.SetDirection(centroidChange.GetUnitVector());
            clusterFitResult.SetSuccessFlag(true);
        }

        pCluster->SetCurrentFitResult(clusterFitResult);
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ConeClusteringAlgorithm::GetGenericDistanceToHit(Cluster *const pCluster, CaloHit *const pCaloHit, const PseudoLayer searchLayer,
    float &genericDistance) const
{
    static const PseudoLayer firstLayer(GeometryHelper::GetPseudoLayerAtIp());

    // Use position of track projection at calorimeter. Proceed only if projection is reasonably compatible with calo hit
    if (((searchLayer == 0) || (searchLayer < firstLayer)) && pCluster->IsTrackSeeded())
    {
        const TrackState &trackState(pCluster->GetTrackSeed()->GetTrackStateAtCalorimeter());
        const CartesianVector trackDirection(trackState.GetMomentum().GetUnitVector());

        if (pCaloHit->GetExpectedDirection().GetCosOpeningAngle(trackDirection) < m_minHitTrackCosAngle)
            return STATUS_CODE_UNCHANGED;

        return this->GetConeApproachDistanceToHit(pCaloHit, trackState.GetPosition(), trackDirection, genericDistance);
    }

    // Check that cluster is occupied in the searchlayer and is reasonably compatible with calo hit
    OrderedCaloHitList::const_iterator clusterHitListIter = pCluster->GetOrderedCaloHitList().find(searchLayer);

    if (pCluster->GetOrderedCaloHitList().end() == clusterHitListIter)
        return STATUS_CODE_UNCHANGED;

    const ClusterHelper::ClusterFitResult &clusterFitResult(pCluster->GetCurrentFitResult());
    const CartesianVector &clusterDirection(clusterFitResult.IsFitSuccessful() ?  clusterFitResult.GetDirection() : pCluster->GetInitialDirection());

    if (pCaloHit->GetExpectedDirection().GetCosOpeningAngle(clusterDirection) < m_minHitClusterCosAngle)
        return STATUS_CODE_UNCHANGED;

    // Cone approach measurements
    float initialDirectionDistance(std::numeric_limits<float>::max());
    float currentDirectionDistance(std::numeric_limits<float>::max());
    float trackSeedDistance(std::numeric_limits<float>::max());

    const bool useTrackSeed(m_shouldUseTrackSeed && pCluster->IsTrackSeeded());
    const bool followInitialDirection(m_shouldFollowInitialDirection && pCluster->IsTrackSeeded() && (searchLayer > m_trackSeedCutOffLayer));

    if (!useTrackSeed || (searchLayer > m_trackSeedCutOffLayer))
    {
        const CaloHitList *pClusterCaloHitList = clusterHitListIter->second;

        if (searchLayer == pCaloHit->GetPseudoLayer())
        {
            return this->GetDistanceToHitInSameLayer(pCaloHit, pClusterCaloHitList, genericDistance);
        }

        // Measurement using initial cluster direction
        StatusCode statusCode = this->GetConeApproachDistanceToHit(pCaloHit, pClusterCaloHitList, pCluster->GetInitialDirection(),
            initialDirectionDistance);

        if (STATUS_CODE_SUCCESS == statusCode)
        {
            if (followInitialDirection)
                initialDirectionDistance /= 5.;
        }
        else if (STATUS_CODE_UNCHANGED != statusCode)
        {
            return statusCode;
        }

        // Measurement using current cluster direction
        if (clusterFitResult.IsFitSuccessful())
        {
            StatusCode statusCode = this->GetConeApproachDistanceToHit(pCaloHit, pClusterCaloHitList, clusterFitResult.GetDirection(),
                currentDirectionDistance);

            if (STATUS_CODE_SUCCESS == statusCode)
            {
                if ((currentDirectionDistance < m_genericDistanceCut) && pCluster->IsMipTrack())
                    currentDirectionDistance /= 5.;
            }
            else if (STATUS_CODE_UNCHANGED != statusCode)
            {
                return statusCode;
            }
        }
    }

    // Seed track distance measurements
    if (useTrackSeed && !followInitialDirection)
    {
        StatusCode statusCode = this->GetDistanceToTrackSeed(pCluster, pCaloHit, searchLayer, trackSeedDistance);

        if (STATUS_CODE_SUCCESS == statusCode)
        {
            if (trackSeedDistance < m_genericDistanceCut)
                trackSeedDistance /= 5.;
        }
        else if (STATUS_CODE_UNCHANGED != statusCode)
        {
            return statusCode;
        }
    }

    // Identify best measurement of generic distance
    const float smallestDistance(std::min(trackSeedDistance, std::min(initialDirectionDistance, currentDirectionDistance)));
    if (smallestDistance < genericDistance)
    {
        genericDistance = smallestDistance;

        if (std::numeric_limits<float>::max() != genericDistance)
            return STATUS_CODE_SUCCESS;
    }

    return STATUS_CODE_UNCHANGED;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ConeClusteringAlgorithm::GetDistanceToHitInSameLayer(CaloHit *const pCaloHit, const CaloHitList *const pCaloHitList,
    float &distance) const
{
    const float dCut ((GeometryHelper::GetHitTypeGranularity(pCaloHit->GetHitType()) <= FINE) ?
        (m_sameLayerPadWidthsFine * pCaloHit->GetCellLengthScale()) :
        (m_sameLayerPadWidthsCoarse * pCaloHit->GetCellLengthScale()) );

    if (0.f == dCut)
        return STATUS_CODE_FAILURE;

    const CartesianVector &hitPosition(pCaloHit->GetPositionVector());

    bool hitFound(false);
    float smallestDistanceSquared(std::numeric_limits<float>::max());
    const float rDCutSquared(1.f / (dCut * dCut));

    for (CaloHitList::const_iterator iter = pCaloHitList->begin(), iterEnd = pCaloHitList->end(); iter != iterEnd; ++iter)
    {
        CaloHit *pHitInCluster = *iter;
        const CartesianVector &hitInClusterPosition(pHitInCluster->GetPositionVector());
        const float separationSquared((hitPosition - hitInClusterPosition).GetMagnitudeSquared());
        const float hitDistanceSquared(separationSquared * rDCutSquared);

        if (hitDistanceSquared < smallestDistanceSquared)
        {
            smallestDistanceSquared = hitDistanceSquared;
            hitFound = true;
        }
    }

    if (!hitFound)
        return STATUS_CODE_UNCHANGED;

    distance = std::sqrt(smallestDistanceSquared);
    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ConeClusteringAlgorithm::GetConeApproachDistanceToHit(CaloHit *const pCaloHit, const CaloHitList *const pCaloHitList,
    const CartesianVector &clusterDirection, float &distance) const
{
    bool hitFound(false);
    float smallestDistance(std::numeric_limits<float>::max());

    for (CaloHitList::const_iterator iter = pCaloHitList->begin(), iterEnd = pCaloHitList->end(); iter != iterEnd; ++iter)
    {
        CaloHit *pHitInCluster = *iter;
        float hitDistance(std::numeric_limits<float>::max());

        PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_UNCHANGED, !=, this->GetConeApproachDistanceToHit(pCaloHit,
            pHitInCluster->GetPositionVector(), clusterDirection, hitDistance));

        if (hitDistance < smallestDistance)
        {
            smallestDistance = hitDistance;
            hitFound = true;
        }
    }

    if (!hitFound)
        return STATUS_CODE_UNCHANGED;

    distance = smallestDistance;
    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ConeClusteringAlgorithm::GetConeApproachDistanceToHit(CaloHit *const pCaloHit, const CartesianVector &clusterPosition,
    const CartesianVector &clusterDirection, float &distance) const
{
    const CartesianVector &hitPosition(pCaloHit->GetPositionVector());
    const CartesianVector positionDifference(hitPosition - clusterPosition);

    if (positionDifference.GetMagnitudeSquared() > m_coneApproachMaxSeparation2)
        return STATUS_CODE_UNCHANGED;

    const float dAlong(clusterDirection.GetDotProduct(positionDifference));

    if ((dAlong < m_maxClusterDirProjection) && (dAlong > m_minClusterDirProjection))
    {
        const float dCut ((GeometryHelper::GetHitTypeGranularity(pCaloHit->GetHitType()) <= FINE) ?
            (std::fabs(dAlong) * m_tanConeAngleFine) + (m_additionalPadWidthsFine * pCaloHit->GetCellLengthScale()) :
            (std::fabs(dAlong) * m_tanConeAngleCoarse) + (m_additionalPadWidthsCoarse * pCaloHit->GetCellLengthScale()) );

        if (0.f == dCut)
            return STATUS_CODE_FAILURE;

        const float dPerp (clusterDirection.GetCrossProduct(positionDifference).GetMagnitude());

        distance = dPerp / dCut;
        return STATUS_CODE_SUCCESS;
    }

    return STATUS_CODE_UNCHANGED;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ConeClusteringAlgorithm::GetDistanceToTrackSeed(Cluster *const pCluster, CaloHit *const pCaloHit, PseudoLayer searchLayer,
    float &distance) const
{
    if (searchLayer < m_maxLayersToTrackSeed)
        return this->GetDistanceToTrackSeed(pCluster, pCaloHit, distance);

    const int searchLayerInt(static_cast<int>(searchLayer));
    const int startLayer(std::max(0, searchLayerInt - static_cast<int>(m_maxLayersToTrackLikeHit)));

    const OrderedCaloHitList &orderedCaloHitList = pCluster->GetOrderedCaloHitList();

    for (int iLayer = startLayer; iLayer < searchLayerInt; ++iLayer)
    {
        OrderedCaloHitList::const_iterator listIter = orderedCaloHitList.find(iLayer);
        if (orderedCaloHitList.end() == listIter)
            continue;

        for (CaloHitList::const_iterator iter = listIter->second->begin(), iterEnd = listIter->second->end(); iter != iterEnd; ++iter)
        {
            float tempDistance(std::numeric_limits<float>::max());
            PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_UNCHANGED, !=, this->GetDistanceToTrackSeed(pCluster, *iter,
                tempDistance));

            if (tempDistance < m_genericDistanceCut)
                return this->GetDistanceToTrackSeed(pCluster, pCaloHit, distance);
        }
    }

    return STATUS_CODE_UNCHANGED;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ConeClusteringAlgorithm::GetDistanceToTrackSeed(Cluster *const pCluster, CaloHit *const pCaloHit, float &distance) const
{
    const CartesianVector &hitPosition(pCaloHit->GetPositionVector());
    const CartesianVector &trackSeedPosition(pCluster->GetTrackSeed()->GetTrackStateAtCalorimeter().GetPosition());

    const CartesianVector positionDifference(hitPosition - trackSeedPosition);
    const float separationSquared(positionDifference.GetMagnitudeSquared());

    if (separationSquared < m_maxTrackSeedSeparation2)
    {
        const float flexibility(1.f + (m_trackPathWidth * std::sqrt(separationSquared / m_maxTrackSeedSeparation2)));

        const float dCut ((GeometryHelper::GetHitTypeGranularity(pCaloHit->GetHitType()) <= FINE) ?
            flexibility * (m_additionalPadWidthsFine * pCaloHit->GetCellLengthScale()) :
            flexibility * (m_additionalPadWidthsCoarse * pCaloHit->GetCellLengthScale()) );

        if (0.f == dCut)
            return STATUS_CODE_FAILURE;

        const float dPerp((pCluster->GetInitialDirection().GetCrossProduct(positionDifference)).GetMagnitude());

        distance = dPerp / dCut;
        return STATUS_CODE_SUCCESS;
    }

    return STATUS_CODE_UNCHANGED;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ConeClusteringAlgorithm::RemoveEmptyClusters(ClusterVector &clusterVector) const
{
    ClusterList clusterDeletionList;

    for (ClusterVector::iterator iter = clusterVector.begin(), iterEnd = clusterVector.end(); iter != iterEnd; ++iter)
    {
        if (0 == (*iter)->GetNCaloHits())
        {
            clusterDeletionList.insert(*iter);
            (*iter) = NULL;
        }
    }

    if (!clusterDeletionList.empty())
    {
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::DeleteClusters(*this, clusterDeletionList));
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ConeClusteringAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    // Track seeding parameters
    m_clusterSeedStrategy = 2;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ClusterSeedStrategy", m_clusterSeedStrategy));

    // High level clustering parameters
    CustomHitOrder::m_hitSortingStrategy = 0;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "HitSortingStrategy", CustomHitOrder::m_hitSortingStrategy));

    m_shouldUseOnlyECalHits = false;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ShouldUseOnlyECalHits", m_shouldUseOnlyECalHits));

    m_shouldUseIsolatedHits = false;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ShouldUseIsolatedHits", m_shouldUseIsolatedHits));

    m_layersToStepBackFine = 3;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "LayersToStepBackFine", m_layersToStepBackFine));

    m_layersToStepBackCoarse = 3;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "LayersToStepBackCoarse", m_layersToStepBackCoarse));

    m_clusterFormationStrategy = 0;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ClusterFormationStrategy", m_clusterFormationStrategy));

    m_genericDistanceCut = 1.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "GenericDistanceCut", m_genericDistanceCut));

    m_minHitTrackCosAngle = 0.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinHitTrackCosAngle", m_minHitTrackCosAngle));

    m_minHitClusterCosAngle = 0.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinHitClusterCosAngle", m_minHitClusterCosAngle));

    m_shouldUseTrackSeed = true;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ShouldUseTrackSeed", m_shouldUseTrackSeed));

    m_trackSeedCutOffLayer = 0;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "TrackSeedCutOffLayer", m_trackSeedCutOffLayer));

    m_shouldFollowInitialDirection = false;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ShouldFollowInitialDirection", m_shouldFollowInitialDirection));

    // Same layer distance parameters
    m_sameLayerPadWidthsFine = 2.8f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "SameLayerPadWidthsFine", m_sameLayerPadWidthsFine));

    m_sameLayerPadWidthsCoarse = 1.8f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "SameLayerPadWidthsCoarse", m_sameLayerPadWidthsCoarse));

    // Cone approach distance parameters
    float coneApproachMaxSeparation = 1000.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ConeApproachMaxSeparation", coneApproachMaxSeparation));

    m_coneApproachMaxSeparation2 = coneApproachMaxSeparation * coneApproachMaxSeparation;

    m_tanConeAngleFine = 0.3f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "TanConeAngleFine", m_tanConeAngleFine));

    m_tanConeAngleCoarse = 0.5f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "TanConeAngleCoarse", m_tanConeAngleCoarse));

    m_additionalPadWidthsFine = 2.5f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "AdditionalPadWidthsFine", m_additionalPadWidthsFine));

    m_additionalPadWidthsCoarse = 2.5f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "AdditionalPadWidthsCoarse", m_additionalPadWidthsCoarse));

    m_maxClusterDirProjection = 200.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxClusterDirProjection", m_maxClusterDirProjection));

    m_minClusterDirProjection = -10.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinClusterDirProjection", m_minClusterDirProjection));

    // Track seed distance parameters
    m_trackPathWidth = 2.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "TrackPathWidth", m_trackPathWidth));

    float maxTrackSeedSeparation = 250.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxTrackSeedSeparation", maxTrackSeedSeparation));

    m_maxTrackSeedSeparation2 = maxTrackSeedSeparation * maxTrackSeedSeparation;

    if (m_shouldUseTrackSeed && (0.f == m_maxTrackSeedSeparation2))
        return STATUS_CODE_INVALID_PARAMETER;

    m_maxLayersToTrackSeed = 3;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxLayersToTrackSeed", m_maxLayersToTrackSeed));

    m_maxLayersToTrackLikeHit = 3;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxLayersToTrackLikeHit", m_maxLayersToTrackLikeHit));

    // Cluster current direction and mip track parameters
    m_nLayersSpannedForFit = 6;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "NLayersSpannedForFit", m_nLayersSpannedForFit));

    m_nLayersSpannedForApproxFit = 10;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "NLayersSpannedForApproxFit", m_nLayersSpannedForApproxFit));

    m_nLayersToFit = 8;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "NLayersToFit", m_nLayersToFit));

    m_nLayersToFitLowMipCut = 0.5f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "NLayersToFitLowMipCut", m_nLayersToFitLowMipCut));

    m_nLayersToFitLowMipMultiplier = 2;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "NLayersToFitLowMipMultiplier", m_nLayersToFitLowMipMultiplier));

    m_fitSuccessDotProductCut1 = 0.75f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "FitSuccessDotProductCut1", m_fitSuccessDotProductCut1));

    m_fitSuccessChi2Cut1 = 5.0f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "FitSuccessChi2Cut1", m_fitSuccessChi2Cut1));

    m_fitSuccessDotProductCut2 = 0.50f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "FitSuccessDotProductCut2", m_fitSuccessDotProductCut2));

    m_fitSuccessChi2Cut2 = 2.5f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "FitSuccessChi2Cut2", m_fitSuccessChi2Cut2));

    m_mipTrackChi2Cut = 2.5f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MipTrackChi2Cut", m_mipTrackChi2Cut));

    return STATUS_CODE_SUCCESS;
}
