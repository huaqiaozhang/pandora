/**
 *  @file   PandoraPFANew/FineGranularityContent/src/TopologicalAssociation/ShowerMipMerging2Algorithm.cc
 * 
 *  @brief  Implementation of the shower mip merging 2 algorithm class.
 * 
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "TopologicalAssociation/ShowerMipMerging2Algorithm.h"

using namespace pandora;

StatusCode ShowerMipMerging2Algorithm::Run()
{
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

    // Loop over all candidate parent clusters
    for (ClusterVector::const_iterator iterI = clusterVector.begin(); iterI != clusterVector.end(); ++iterI)
    {
        Cluster *pParentCluster = *iterI;

        // Check to see if cluster has already been changed
        if (NULL == pParentCluster)
            continue;

        if ((pParentCluster->GetNCaloHits() < m_minHitsInCluster) || (pParentCluster->GetOrderedCaloHitList().size() < m_minOccupiedLayersInCluster))
            continue;

        ClusterHelper::ClusterFitResult parentClusterFitResult;
        if (STATUS_CODE_SUCCESS != ClusterHelper::FitEnd(pParentCluster, m_nPointsToFit, parentClusterFitResult))
            continue;

        if (!parentClusterFitResult.IsFitSuccessful() || (parentClusterFitResult.GetChi2() > m_fitToAllHitsChi2Cut))
            continue;

        const PseudoLayer parentOuterLayer(pParentCluster->GetOuterPseudoLayer());
        const CartesianVector parentOuterCentroid(pParentCluster->GetCentroid(parentOuterLayer));

        float minPerpendicularDistance(std::numeric_limits<float>::max());
        ClusterVector::iterator bestDaughterClusterIter(clusterVector.end());
        float bestDaughterClusterEnergy(std::numeric_limits<float>::max());

        // Compare this successfully fitted cluster with all others
        for (ClusterVector::iterator iterJ = clusterVector.begin(); iterJ != clusterVector.end(); ++iterJ)
        {
            Cluster *pDaughterCluster = *iterJ;

            // Check to see if cluster has already been changed
            if ((NULL == pDaughterCluster) || (pParentCluster == pDaughterCluster))
                continue;

            // Cut on layer separation between the two clusters
            const PseudoLayer daughterInnerLayer(pDaughterCluster->GetInnerPseudoLayer());

            if ((daughterInnerLayer <= parentOuterLayer) || ((daughterInnerLayer - parentOuterLayer) > m_maxLayerDifference))
                continue;

            // Also cut on physical separation between the two clusters
            const CartesianVector daughterInnerCentroid(pDaughterCluster->GetCentroid(daughterInnerLayer));
            const CartesianVector centroidDifference(parentOuterCentroid - daughterInnerCentroid);

            if (centroidDifference.GetMagnitude() > m_maxCentroidDifference)
                continue;

            // Require clusters to point at one another
            if (centroidDifference.GetUnitVector().GetDotProduct(parentClusterFitResult.GetDirection()) > m_maxFitDirectionDotProduct)
                continue;

            // Cut on perpendicular distance between fit direction and centroid difference vector.
            const CartesianVector parentCrossProduct(parentClusterFitResult.GetDirection().GetCrossProduct(centroidDifference));
            const float perpendicularDistance(parentCrossProduct.GetMagnitude());

            const float perpendicularDistanceCut((GeometryHelper::GetHitTypeGranularity(pDaughterCluster->GetInnerLayerHitType()) <= FINE) ?
                m_perpendicularDistanceCutFine : m_perpendicularDistanceCutCoarse);

            if (perpendicularDistance > perpendicularDistanceCut)
                continue;

            const float daughterClusterEnergy(pDaughterCluster->GetHadronicEnergy());

            if ((perpendicularDistance < minPerpendicularDistance) ||
                ((perpendicularDistance == minPerpendicularDistance) && (daughterClusterEnergy < bestDaughterClusterEnergy)))
            {
                bestDaughterClusterIter = iterJ;
                minPerpendicularDistance = perpendicularDistance;
                bestDaughterClusterEnergy = daughterClusterEnergy;
            }
        }

        if (bestDaughterClusterIter != clusterVector.end())
        {
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::MergeAndDeleteClusters(*this, pParentCluster, *bestDaughterClusterIter));
            *bestDaughterClusterIter = NULL;
            --iterI;
        }
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ShowerMipMerging2Algorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
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

    m_fitToAllHitsChi2Cut = 5.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "FitToAllHitsChi2Cut", m_fitToAllHitsChi2Cut));

    m_nPointsToFit = 8;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "NPointsToFit", m_nPointsToFit));

    m_maxLayerDifference = 6;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxLayerDifference", m_maxLayerDifference));

    m_maxCentroidDifference = 2000.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxCentroidDifference", m_maxCentroidDifference));

    m_maxFitDirectionDotProduct = -0.8f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxFitDirectionDotProduct", m_maxFitDirectionDotProduct));

    m_perpendicularDistanceCutFine = 50.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "PerpendicularDistanceCutFine", m_perpendicularDistanceCutFine));

    m_perpendicularDistanceCutCoarse = 75.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "PerpendicularDistanceCutCoarse", m_perpendicularDistanceCutCoarse));

    return STATUS_CODE_SUCCESS;
}
