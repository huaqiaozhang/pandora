/**
 *  @file   PandoraPFANew/FineGranularityContent/src/TopologicalAssociation/IsolatedHitMergingAlgorithm.cc
 * 
 *  @brief  Implementation of the isolated hit merging algorithm class.
 * 
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "TopologicalAssociation/IsolatedHitMergingAlgorithm.h"

using namespace pandora;

StatusCode IsolatedHitMergingAlgorithm::Run()
{
    // Read specified lists of input clusters
    ClusterList clusterList;

    if (m_shouldUseCurrentClusterList)
    {
        const ClusterList *pClusterList = NULL;
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentClusterList(*this, pClusterList));

        clusterList.insert(pClusterList->begin(), pClusterList->end());
    }

    for (StringVector::const_iterator iter = m_additionalClusterListNames.begin(), iterEnd = m_additionalClusterListNames.end(); iter != iterEnd; ++iter)
    {
        const ClusterList *pClusterList = NULL;

        if (STATUS_CODE_SUCCESS == PandoraContentApi::GetClusterList(*this, *iter, pClusterList))
        {
            clusterList.insert(pClusterList->begin(), pClusterList->end());
        }
        else
        {
            std::cout << "IsolatedHitMergingAlgorithm: Failed to obtain cluster list " << *iter << std::endl;
        }
    }

    // Create a vector of input clusters, ordered by inner layer
    ClusterVector clusterVector(clusterList.begin(), clusterList.end());
    std::sort(clusterVector.begin(), clusterVector.end(), Cluster::SortByInnerLayer);


    // FIRST PART - find "small" clusters, below threshold number of calo hits, delete them and associate hits with other clusters
    for (ClusterVector::iterator iterI = clusterVector.begin(), iterIEnd = clusterVector.end(); iterI != iterIEnd; ++iterI)
    {
        Cluster *pClusterToDelete = *iterI;

        if (NULL == pClusterToDelete)
            continue;

        const unsigned int nCaloHits(pClusterToDelete->GetNCaloHits());

        if (nCaloHits > m_minHitsInCluster)
            continue;

        CaloHitList caloHitList;
        pClusterToDelete->GetOrderedCaloHitList().GetCaloHitList(caloHitList);

        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::DeleteCluster(*this, pClusterToDelete));
        *iterI = NULL;

        // Redistribute hits that used to be in cluster I amongst other clusters
        for (CaloHitList::const_iterator hitIter = caloHitList.begin(), hitIterEnd = caloHitList.end(); hitIter != hitIterEnd; ++hitIter)
        {
            CaloHit *pCaloHit = *hitIter;

            Cluster *pBestHostCluster(NULL);
            float bestHostClusterEnergy(0.);
            float minDistance(m_maxRecombinationDistance);

            // Find the most appropriate cluster for this newly-available hit
            for (ClusterVector::const_iterator iterJ = clusterVector.begin(), iterJEnd = clusterVector.end(); iterJ != iterJEnd; ++iterJ)
            {
                Cluster *pNewHostCluster = *iterJ;

                if (NULL == pNewHostCluster)
                    continue;

                if (pNewHostCluster->GetNCaloHits() < nCaloHits)
                    continue;

                const float distance(this->GetDistanceToHit(pNewHostCluster, pCaloHit));
                const float hostClusterEnergy(pNewHostCluster->GetHadronicEnergy());

                // In event of equidistant host candidates, choose highest energy cluster
                if ((distance < minDistance) || ((distance == minDistance) && (hostClusterEnergy > bestHostClusterEnergy)))
                {
                    minDistance = distance;
                    pBestHostCluster = pNewHostCluster;
                    bestHostClusterEnergy = hostClusterEnergy;
                }
            }

            if (NULL != pBestHostCluster)
            {
                PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::AddIsolatedCaloHitToCluster(*this, pBestHostCluster, pCaloHit));
            }
        }
    }


    // SECOND PART - loop over the remaining available isolated hits, and associate them with other clusters
    const CaloHitList *pCaloHitList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentCaloHitList(*this, pCaloHitList));

    for (CaloHitList::const_iterator hitIterI = pCaloHitList->begin(); hitIterI != pCaloHitList->end(); ++hitIterI)
    {
        CaloHit *pCaloHit = *hitIterI;

        if (!pCaloHit->IsIsolated() || !PandoraContentApi::IsCaloHitAvailable(*this, pCaloHit))
            continue;

        Cluster *pBestHostCluster(NULL);
        float bestHostClusterEnergy(0.);
        float minDistance(m_maxRecombinationDistance);

        // Find most appropriate cluster for this isolated hit
        for (ClusterVector::const_iterator iterJ = clusterVector.begin(), iterJEnd = clusterVector.end(); iterJ != iterJEnd; ++iterJ)
        {
            Cluster *pCluster = *iterJ;

            if (NULL == pCluster)
                continue;

            const float distance(this->GetDistanceToHit(pCluster, pCaloHit));
            const float hostClusterEnergy(pCluster->GetHadronicEnergy());

            if ((distance < minDistance) || ((distance == minDistance) && (hostClusterEnergy > bestHostClusterEnergy)))
            {
                minDistance = distance;
                pBestHostCluster = pCluster;
                bestHostClusterEnergy = hostClusterEnergy;
            }
        }

        if (NULL != pBestHostCluster)
        {
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::AddIsolatedCaloHitToCluster(*this, pBestHostCluster, pCaloHit));
        }
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

float IsolatedHitMergingAlgorithm::GetDistanceToHit(const Cluster *const pCluster, const CaloHit *const pCaloHit) const
{
    // Apply simple preselection using cosine of opening angle between the hit and cluster directions
    if (pCaloHit->GetExpectedDirection().GetCosOpeningAngle(pCluster->GetInitialDirection()) < m_minCosOpeningAngle)
        return std::numeric_limits<float>::max();

    float minDistanceSquared(std::numeric_limits<float>::max());
    const CartesianVector &hitPosition(pCaloHit->GetPositionVector());
    const OrderedCaloHitList &orderedCaloHitList(pCluster->GetOrderedCaloHitList());

    for (OrderedCaloHitList::const_iterator iter = orderedCaloHitList.begin(), iterEnd = orderedCaloHitList.end(); iter != iterEnd; ++iter)
    {
        const float distanceSquared((pCluster->GetCentroid(iter->first) - hitPosition).GetMagnitudeSquared());

        if (distanceSquared < minDistanceSquared)
            minDistanceSquared = distanceSquared;
    }

    if (minDistanceSquared < std::numeric_limits<float>::max())
        return std::sqrt(minDistanceSquared);

    return std::numeric_limits<float>::max();
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode IsolatedHitMergingAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    m_shouldUseCurrentClusterList = true;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ShouldUseCurrentClusterList", m_shouldUseCurrentClusterList));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadVectorOfValues(xmlHandle,
        "AdditionalClusterListNames", m_additionalClusterListNames));

    m_minHitsInCluster = 4;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinHitsInCluster", m_minHitsInCluster));

    m_maxRecombinationDistance = 250.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxRecombinationDistance", m_maxRecombinationDistance));

    m_minCosOpeningAngle = 0.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinCosOpeningAngle", m_minCosOpeningAngle));

    return STATUS_CODE_SUCCESS;
}
