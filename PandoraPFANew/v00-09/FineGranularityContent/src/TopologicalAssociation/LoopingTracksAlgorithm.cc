/**
 *  @file   PandoraPFANew/FineGranularityContent/src/TopologicalAssociation/LoopingTracksAlgorithm.cc
 * 
 *  @brief  Implementation of the looping tracks algorithm class.
 * 
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "TopologicalAssociation/LoopingTracksAlgorithm.h"

using namespace pandora;

StatusCode LoopingTracksAlgorithm::Run()
{
    const ClusterList *pClusterList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentClusterList(*this, pClusterList));

    ClusterVector clusterVector(pClusterList->begin(), pClusterList->end());
    std::sort(clusterVector.begin(), clusterVector.end(), Cluster::SortByInnerLayer);

    // Fit a straight line to the last n occupied pseudo layers in each cluster and store results
    ClusterFitRelationList clusterFitRelationList;

    for (ClusterVector::const_iterator iter = clusterVector.begin(), iterEnd = clusterVector.end(); iter != iterEnd; ++iter)
    {
        Cluster *pCluster = *iter;

        if (!ClusterHelper::CanMergeCluster(pCluster, m_canMergeMinMipFraction, m_canMergeMaxRms))
            continue;

        if ((pCluster->GetNCaloHits() < m_minHitsInCluster) || (pCluster->GetOrderedCaloHitList().size() < m_minOccupiedLayersInCluster))
            continue;

        ClusterFitResult clusterFitResult;
        (void) ClusterHelper::FitEnd(pCluster, m_nLayersToFit, clusterFitResult);

        if (clusterFitResult.IsFitSuccessful() && (clusterFitResult.GetChi2() < m_fitChi2Cut))
            clusterFitRelationList.push_back(new ClusterFitRelation(pCluster, clusterFitResult));
    }

    // Loop over cluster combinations, comparing fit results to determine whether clusters should be merged
    for (ClusterFitRelationList::const_iterator iterI = clusterFitRelationList.begin(), iterIEnd = clusterFitRelationList.end(); iterI != iterIEnd; ++iterI)
    {
        if ((*iterI)->IsDefunct())
            continue;

        Cluster *pParentCluster((*iterI)->GetCluster());
        const ClusterHelper::ClusterFitResult &parentClusterFitResult((*iterI)->GetClusterFitResult());

        const PseudoLayer parentOuterLayer(pParentCluster->GetOuterPseudoLayer());
        const bool isParentFineGranularity(GeometryHelper::GetHitTypeGranularity(pParentCluster->GetOuterLayerHitType()) <= FINE);

        ClusterFitRelation *pBestClusterFitRelation(NULL);
        float minFitResultsApproach(std::numeric_limits<float>::max());
        ClusterFitRelationList::const_iterator iterJ(iterI);

        for (++iterJ; iterJ != clusterFitRelationList.end(); ++iterJ)
        {
            // Check to see if cluster has already been changed
            if ((*iterJ)->IsDefunct())
                continue;

            Cluster *pDaughterCluster((*iterJ)->GetCluster());
            const ClusterHelper::ClusterFitResult &daughterClusterFitResult((*iterJ)->GetClusterFitResult());

            // Apply loose cuts to examine suitability of merging clusters before proceeding
            const PseudoLayer daughterOuterLayer(pDaughterCluster->GetOuterPseudoLayer());

            const PseudoLayer outerLayerDifference((parentOuterLayer > daughterOuterLayer) ? (parentOuterLayer - daughterOuterLayer) :
                (daughterOuterLayer - parentOuterLayer));

            if (outerLayerDifference > m_maxOuterLayerDifference)
                continue;

            const CartesianVector centroidDifference(pParentCluster->GetCentroid(parentOuterLayer) - pDaughterCluster->GetCentroid(daughterOuterLayer));

            if (centroidDifference.GetMagnitude() > m_maxCentroidDifference)
                continue;

            // Are both clusters contained within fine granularity region? If not, relax cluster compatibility checks.
            const bool isDaughterFineGranularity(GeometryHelper::GetHitTypeGranularity(pDaughterCluster->GetOuterLayerHitType()) <= FINE);
            const bool isFineGranularity(isParentFineGranularity && isDaughterFineGranularity);

            // Check that cluster fit directions are compatible with looping track hypothesis
            const float fitDirectionDotProductCut(isFineGranularity ? m_fitDirectionDotProductCutFine : m_fitDirectionDotProductCutCoarse);
            const float fitDirectionDotProduct(parentClusterFitResult.GetDirection().GetDotProduct(daughterClusterFitResult.GetDirection()));

            if (fitDirectionDotProduct > fitDirectionDotProductCut)
                continue;

            if (centroidDifference.GetDotProduct(daughterClusterFitResult.GetDirection() - parentClusterFitResult.GetDirection()) <= 0.)
                continue;

            // Cut on distance of closest approach between hits in outer layers of the two clusters
            const float closestHitDistance(this->GetClosestDistanceBetweenOuterLayerHits(pParentCluster, pDaughterCluster));
            const float closestHitDistanceCut(isFineGranularity ? m_closestHitDistanceCutFine : m_closestHitDistanceCutCoarse);

            if (closestHitDistance > closestHitDistanceCut)
                continue;

            // Cut on distance of closest approach between fit extrapolations
            const float fitResultsClosestApproachCut(isFineGranularity ? m_fitResultsClosestApproachCutFine : m_fitResultsClosestApproachCutCoarse);
            float fitResultsClosestApproach(std::numeric_limits<float>::max());

            if (STATUS_CODE_SUCCESS != ClusterHelper::GetFitResultsClosestApproach(parentClusterFitResult, daughterClusterFitResult, fitResultsClosestApproach))
                continue;

            if ((fitResultsClosestApproach > fitResultsClosestApproachCut) || (fitResultsClosestApproach > minFitResultsApproach))
                continue;

            // Merge clusters if they are in region of coarse granularity, otherwise look for "good" features (bit ad hoc) ...
            unsigned int nGoodFeatures(0);

            if (isFineGranularity)
            {
                if (fitDirectionDotProduct < m_goodFeaturesMaxFitDotProduct)
                    nGoodFeatures++;

                if (fitResultsClosestApproach < m_goodFeaturesMaxFitApproach)
                    nGoodFeatures++;

                if (outerLayerDifference < m_goodFeaturesMaxLayerDifference)
                    nGoodFeatures++;

                if ((pParentCluster->GetMipFraction() > m_goodFeaturesMinMipFraction) && (pDaughterCluster->GetMipFraction() > m_goodFeaturesMinMipFraction))
                    nGoodFeatures++;
            }

            // Now have sufficient information to decide whether to join clusters
            if (!isFineGranularity || (nGoodFeatures >= m_nGoodFeaturesForClusterMerge))
            {
                pBestClusterFitRelation = *iterJ;
                minFitResultsApproach = fitResultsClosestApproach;
            }
        }

        if (NULL != pBestClusterFitRelation)
        {
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::MergeAndDeleteClusters(*this, pParentCluster, pBestClusterFitRelation->GetCluster()));
            pBestClusterFitRelation->SetAsDefunct();

            // Re-fit and re-use modified parent cluster
            ClusterFitResult clusterFitResult;
            (void) ClusterHelper::FitEnd(pParentCluster, m_nLayersToFit, clusterFitResult);

            if (clusterFitResult.IsFitSuccessful() && (clusterFitResult.GetChi2() < m_fitChi2Cut))
            {
                (*iterI)->SetClusterFitResult(clusterFitResult);
                --iterI;
            }
        }
    }

    // Tidy up
    for (ClusterFitRelationList::const_iterator iter = clusterFitRelationList.begin(), iterEnd = clusterFitRelationList.end(); iter != iterEnd; ++iter)
        delete (*iter);

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

float LoopingTracksAlgorithm::GetClosestDistanceBetweenOuterLayerHits(const Cluster *const pClusterI, const Cluster *const pClusterJ) const
{
    float closestDistance(std::numeric_limits<float>::max());

    const PseudoLayer outerLayerI(pClusterI->GetOuterPseudoLayer());
    const PseudoLayer outerLayerJ(pClusterJ->GetOuterPseudoLayer());

    CaloHitList *pCaloHitListI = NULL;
    PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, pClusterI->GetOrderedCaloHitList().GetCaloHitsInPseudoLayer(outerLayerI, pCaloHitListI));

    CaloHitList *pCaloHitListJ = NULL;
    PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, pClusterJ->GetOrderedCaloHitList().GetCaloHitsInPseudoLayer(outerLayerJ, pCaloHitListJ));

    for (CaloHitList::const_iterator iterI = pCaloHitListI->begin(), iterIEnd = pCaloHitListI->end(); iterI != iterIEnd; ++iterI)
    {
        CaloHit *pCaloHitI = *iterI;

        for (CaloHitList::const_iterator iterJ = pCaloHitListJ->begin(), iterIEnd = pCaloHitListJ->end(); iterJ != iterIEnd; ++iterJ)
        {
            CaloHit *pCaloHitJ = *iterJ;

            const float distance((pCaloHitI->GetPositionVector() - pCaloHitJ->GetPositionVector()).GetMagnitude());

            if (distance < closestDistance)
                closestDistance = distance;
        }
    }

    return closestDistance;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode LoopingTracksAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    m_nLayersToFit = 5;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "NLayersToFit", m_nLayersToFit));

    m_fitChi2Cut = 100.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "FitChi2Cut", m_fitChi2Cut));

    m_canMergeMinMipFraction = 0.7f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "CanMergeMinMipFraction", m_canMergeMinMipFraction));

    m_canMergeMaxRms = 5.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "CanMergeMaxRms", m_canMergeMaxRms));

    m_minHitsInCluster = 4;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinHitsInCluster", m_minHitsInCluster));

    m_minOccupiedLayersInCluster = 2;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinOccupiedLayersInCluster", m_minOccupiedLayersInCluster));

    m_maxOuterLayerDifference = 6;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxOuterLayerDifference", m_maxOuterLayerDifference));

    m_maxCentroidDifference = 2000.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxCentroidDifference", m_maxCentroidDifference));

    m_fitDirectionDotProductCutFine = -0.1f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "FitDirectionDotProductCutFine", m_fitDirectionDotProductCutFine));

    m_fitDirectionDotProductCutCoarse = 0.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "FitDirectionDotProductCutCoarse", m_fitDirectionDotProductCutCoarse));

    m_closestHitDistanceCutFine = 250.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ClosestHitDistanceCutFine", m_closestHitDistanceCutFine));

    m_closestHitDistanceCutCoarse = 500.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ClosestHitDistanceCutCoarse", m_closestHitDistanceCutCoarse));

    m_fitResultsClosestApproachCutFine = 50.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "FitResultsClosestApproachCutFine", m_fitResultsClosestApproachCutFine));

    m_fitResultsClosestApproachCutCoarse = 200.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "FitResultsClosestApproachCutCoarse", m_fitResultsClosestApproachCutCoarse));

    m_nGoodFeaturesForClusterMerge = 2;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "NGoodFeaturesForClusterMerge", m_nGoodFeaturesForClusterMerge));

    m_goodFeaturesMaxFitDotProduct = -0.5f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "NGoodFeaturesForClusterMerge", m_nGoodFeaturesForClusterMerge));

    m_goodFeaturesMaxFitApproach = 50.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "GoodFeaturesMaxFitApproach", m_goodFeaturesMaxFitApproach));

    m_goodFeaturesMaxLayerDifference = 4;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "GoodFeaturesMaxLayerDifference", m_goodFeaturesMaxLayerDifference));

    m_goodFeaturesMinMipFraction = 0.9f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "GoodFeaturesMinMipFraction", m_goodFeaturesMinMipFraction));

    return STATUS_CODE_SUCCESS;
}
