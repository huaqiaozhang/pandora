/**
 *  @file   PandoraPFANew/FineGranularityContent/src/FragmentRemoval/MergeSplitPhotonsAlgorithm.cc
 * 
 *  @brief  Implementation of the merge split photons algorithm class.
 * 
 *  $Log: $
 */

#include "FragmentRemoval/MergeSplitPhotonsAlgorithm.h"

#include "Pandora/AlgorithmHeaders.h"

using namespace pandora;

StatusCode MergeSplitPhotonsAlgorithm::Run()
{
    const ClusterList *pClusterList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentClusterList(*this, pClusterList));

    // Create a vector of input clusters, ordered by inner layer
    ClusterVector clusterVector(pClusterList->begin(), pClusterList->end());
    std::sort(clusterVector.begin(), clusterVector.end(), Cluster::SortByInnerLayer);

    // Loop over photon candidate clusters
    for (ClusterVector::iterator iterI = clusterVector.begin(), iterIEnd = clusterVector.end(); iterI != iterIEnd; ++iterI)
    {
        Cluster *pParentCluster = *iterI;

        if (NULL == pParentCluster)
            continue;

        if (!pParentCluster->GetAssociatedTrackList().empty())
            continue;

        if (GeometryHelper::GetHitTypeGranularity(pParentCluster->GetOuterLayerHitType()) > FINE)
            continue;

        const CartesianVector parentShowerMaxCentroid(pParentCluster->GetCentroid(this->GetShowerMaxLayer(pParentCluster)));
        const bool isParentPhoton(pParentCluster->IsPhotonFast());

        // Find daughter photon candidate clusters
        for (ClusterVector::iterator iterJ = iterI + 1, iterJEnd = clusterVector.end(); iterJ != iterJEnd; ++iterJ)
        {
            Cluster *pDaughterCluster = *iterJ;

            if (NULL == pDaughterCluster)
                continue;

            if (!pDaughterCluster->GetAssociatedTrackList().empty())
                continue;

            if (GeometryHelper::GetHitTypeGranularity(pDaughterCluster->GetOuterLayerHitType()) > FINE)
                continue;

            const CartesianVector daughterShowerMaxCentroid(pDaughterCluster->GetCentroid(this->GetShowerMaxLayer(pDaughterCluster)));
            const bool isDaughterPhoton(pDaughterCluster->IsPhotonFast());

            // Look for compatible parent/daughter pairings
            if (!isParentPhoton && !isDaughterPhoton)
                continue;

            if (parentShowerMaxCentroid.GetCosOpeningAngle(daughterShowerMaxCentroid) <= m_minShowerMaxCosAngle)
                continue;

            unsigned int nContactLayers(0);
            float contactFraction(0.f);

            const StatusCode statusCode(FragmentRemovalHelper::GetClusterContactDetails(pParentCluster, pDaughterCluster, 
                m_contactDistanceThreshold, nContactLayers, contactFraction));

            if ((STATUS_CODE_SUCCESS == statusCode) && (nContactLayers >= m_minContactLayers) && (contactFraction > m_minContactFraction))
            {
                // Initialize fragmentation to compare merged cluster with original
                ClusterList clusterList;
                clusterList.insert(pParentCluster); clusterList.insert(pDaughterCluster);

                std::string originalClusterListName, mergedClusterListName;
                PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::InitializeFragmentation(*this, clusterList,
                    originalClusterListName, mergedClusterListName));

                CaloHitList caloHitList;
                pParentCluster->GetOrderedCaloHitList().GetCaloHitList(caloHitList);
                pDaughterCluster->GetOrderedCaloHitList().GetCaloHitList(caloHitList);

                Cluster *pMergedCluster = NULL;
                PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::Cluster::Create(*this, &caloHitList, pMergedCluster));

                // Look for peaks in cluster transverse shower profile
                ParticleIdHelper::ShowerPeakList showerPeakList;
                ParticleIdHelper::CalculateTransverseProfile(pMergedCluster, m_transProfileMaxLayer, showerPeakList);

                const float subsidiaryPeakEnergy((showerPeakList.size() > 1) ? showerPeakList[1].GetPeakEnergy() : 0.f);
                const float smallFragmentEnergy(std::min(pDaughterCluster->GetElectromagneticEnergy(), pParentCluster->GetElectromagneticEnergy()));
                const float largeFragmentEnergy(std::max(pDaughterCluster->GetElectromagneticEnergy(), pParentCluster->GetElectromagneticEnergy()));

                // Decide whether merged cluster is better than individual fragments
                bool acceptMerge(false);

                if (smallFragmentEnergy < m_acceptMaxSmallFragmentEnergy)
                {
                    acceptMerge = true;
                }
                else if (subsidiaryPeakEnergy < m_acceptMaxSubsidiaryPeakEnergy)
                {
                    if (smallFragmentEnergy < m_acceptFragmentEnergyRatio * largeFragmentEnergy)
                    {
                        acceptMerge = true;
                    }
                    else if (subsidiaryPeakEnergy < m_acceptSubsidiaryPeakEnergyRatio * smallFragmentEnergy)
                    {
                        acceptMerge = true;
                    }
                }

                // If merging hard photons, check for early peaks in transverse profile
                if (acceptMerge && (smallFragmentEnergy > m_acceptMaxSmallFragmentEnergy))
                {
                    ParticleIdHelper::ShowerPeakList earlyShowerPeakList;
                    ParticleIdHelper::CalculateTransverseProfile(pMergedCluster, m_earlyTransProfileMaxLayer, earlyShowerPeakList);

                    const float earlySubsidiaryPeakEnergy((earlyShowerPeakList.size() > 1) ? earlyShowerPeakList[1].GetPeakEnergy() : 0.f);

                    if (earlySubsidiaryPeakEnergy > m_acceptMaxSubsidiaryPeakEnergy)
                        acceptMerge = false;
                }

                // Tidy up
                const std::string clusterListToSaveName(acceptMerge ? mergedClusterListName : originalClusterListName);
                const std::string clusterListToDeleteName(acceptMerge ? originalClusterListName : mergedClusterListName);

                PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::EndFragmentation(*this, clusterListToSaveName,
                    clusterListToDeleteName));

                if (acceptMerge)
                {
                    *iterI = NULL;
                    *iterJ = NULL;
                    break;
                }
            }
        }
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

PseudoLayer MergeSplitPhotonsAlgorithm::GetShowerMaxLayer(const Cluster *const pCluster) const
{
    const OrderedCaloHitList &orderedCaloHitList(pCluster->GetOrderedCaloHitList());

    float maxEnergyInLayer(0.f);
    PseudoLayer showerMaxLayer(0);
    bool isLayerFound(false);

    for (OrderedCaloHitList::const_iterator iter = orderedCaloHitList.begin(), iterEnd = orderedCaloHitList.end(); iter != iterEnd; ++iter)
    {
        float energyInLayer(0.f);

        for (CaloHitList::const_iterator hitIter = iter->second->begin(), hitIterEnd = iter->second->end(); hitIter != hitIterEnd; ++hitIter)
        {
            energyInLayer += (*hitIter)->GetElectromagneticEnergy();
        }

        if (energyInLayer > maxEnergyInLayer)
        {
            maxEnergyInLayer = energyInLayer;
            showerMaxLayer = iter->first;
            isLayerFound = true;
        }
    }

    if (!isLayerFound)
        throw StatusCodeException(STATUS_CODE_NOT_FOUND);

    return showerMaxLayer;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode MergeSplitPhotonsAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    m_minShowerMaxCosAngle = 0.98f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinShowerMaxCosAngle", m_minShowerMaxCosAngle));

    m_contactDistanceThreshold = 2.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ContactDistanceThreshold", m_contactDistanceThreshold));

    m_minContactLayers = 3;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinContactLayers", m_minContactLayers));

    m_minContactFraction = 0.5f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinContactFraction", m_minContactFraction));

    m_transProfileMaxLayer = 30;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "TransProfileMaxLayer", m_transProfileMaxLayer));

    m_acceptMaxSmallFragmentEnergy = 0.2f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "AcceptMaxSmallFragmentEnergy", m_acceptMaxSmallFragmentEnergy));

    m_acceptMaxSubsidiaryPeakEnergy = 0.5f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "AcceptMaxSubsidiaryPeakEnergy", m_acceptMaxSubsidiaryPeakEnergy));

    m_acceptFragmentEnergyRatio = 0.05f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "AcceptFragmentEnergyRatio", m_acceptFragmentEnergyRatio));

    m_acceptSubsidiaryPeakEnergyRatio = 0.1f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "AcceptSubsidiaryPeakEnergyRatio", m_acceptSubsidiaryPeakEnergyRatio));

    m_earlyTransProfileMaxLayer = 20;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "EarlyTransProfileMaxLayer", m_earlyTransProfileMaxLayer));

    return STATUS_CODE_SUCCESS;
}
