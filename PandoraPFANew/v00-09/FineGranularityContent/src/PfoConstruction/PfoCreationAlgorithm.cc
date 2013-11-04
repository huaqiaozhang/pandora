/**
 *  @file   PandoraPFANew/FineGranularityContent/src/PfoConstruction/PfoCreationAlgorithm.cc
 * 
 *  @brief  Implementation of the pfo creation algorithm class.
 * 
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "PfoConstruction/PfoCreationAlgorithm.h"

using namespace pandora;

StatusCode PfoCreationAlgorithm::Run()
{
    const PfoList *pPfoList = NULL; std::string pfoListName;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::CreateTemporaryPfoListAndSetCurrent(*this, pPfoList, pfoListName));

    if (m_shouldCreateTrackBasedPfos)
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->CreateTrackBasedPfos());

    if (m_shouldCreateNeutralPfos)
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->CreateNeutralPfos());

    if (!pPfoList->empty())
    {
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::SavePfoList(*this, m_outputPfoListName));
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::ReplaceCurrentPfoList(*this, m_outputPfoListName));
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode PfoCreationAlgorithm::CreateTrackBasedPfos() const
{
    // Current track list should contain those tracks selected as "good" by the track preparation algorithm
    const TrackList *pTrackList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentTrackList(*this, pTrackList));

    for (TrackList::const_iterator iter = pTrackList->begin(), iterEnd = pTrackList->end(); iter != iterEnd; ++iter)
    {
        Track *pTrack = *iter;
        PandoraContentApi::ParticleFlowObject::Parameters pfoParameters;

        // Walk along list of associated daughter/sibling tracks and their cluster associations
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->PopulateTrackBasedPfo(pTrack, pfoParameters));

        // Specify the pfo parameters
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->SetTrackBasedPfoParameters(pTrack, pfoParameters));

        // Create the pfo
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::ParticleFlowObject::Create(*this, pfoParameters));
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode PfoCreationAlgorithm::PopulateTrackBasedPfo(Track *const pTrack, PfoParameters &pfoParameters, const bool readSiblingInfo) const
{
    // Add track to the pfo
    pfoParameters.m_trackList.insert(pTrack);

    // Add any cluster associated with this track to the pfo
    Cluster *pAssociatedCluster = NULL;

    if (STATUS_CODE_SUCCESS == pTrack->GetAssociatedCluster(pAssociatedCluster))
    {
        if (NULL == pAssociatedCluster)
            return STATUS_CODE_FAILURE;

        pfoParameters.m_clusterList.insert(pAssociatedCluster);
    }

    // Consider any sibling tracks
    if (readSiblingInfo)
    {
        const TrackList &siblingTrackList(pTrack->GetSiblingTrackList());

        for (TrackList::const_iterator iter = siblingTrackList.begin(), iterEnd = siblingTrackList.end(); iter != iterEnd; ++iter)
        {
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->PopulateTrackBasedPfo(*iter, pfoParameters, false));
        }
    }

    // Consider any daughter tracks
    const TrackList &daughterTrackList(pTrack->GetDaughterTrackList());

    for (TrackList::const_iterator iter = daughterTrackList.begin(), iterEnd = daughterTrackList.end(); iter != iterEnd; ++iter)
    {
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->PopulateTrackBasedPfo(*iter, pfoParameters));
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode PfoCreationAlgorithm::SetTrackBasedPfoParameters(Track *const pTrack, PfoParameters &pfoParameters) const
{
    const bool hasParent(!pTrack->GetParentTrackList().empty());

    if (hasParent)
        return STATUS_CODE_NOT_ALLOWED;

    const bool hasSibling(!pTrack->GetSiblingTrackList().empty());
    const bool hasDaughter(!pTrack->GetDaughterTrackList().empty());

    if (hasSibling && hasDaughter)
        return STATUS_CODE_NOT_ALLOWED;

    if (hasSibling)
        return this->SetSiblingTrackBasedPfoParameters(pTrack, pfoParameters);

    if (hasDaughter)
        return this->SetDaughterTrackBasedPfoParameters(pTrack, pfoParameters);

    return this->SetSimpleTrackBasedPfoParameters(pTrack, pfoParameters);
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode PfoCreationAlgorithm::SetSiblingTrackBasedPfoParameters(Track *const pTrack, PfoParameters &pfoParameters) const
{
    int charge(0);
    float energy(0.f);
    CartesianVector momentum(0.f, 0.f, 0.f);

    TrackList fullSiblingTrackList(pTrack->GetSiblingTrackList());
    fullSiblingTrackList.insert(pTrack);

    for (TrackList::const_iterator iter = fullSiblingTrackList.begin(), iterEnd = fullSiblingTrackList.end(); iter != iterEnd; ++iter)
    {
        Track *pSiblingTrack = *iter;
        charge += pSiblingTrack->GetCharge();

        if (!pSiblingTrack->CanFormPfo() && !pSiblingTrack->CanFormClusterlessPfo())
            continue;

        energy += pSiblingTrack->GetEnergyAtDca();
        momentum += pSiblingTrack->GetMomentumAtDca();
    }

    pfoParameters.m_energy = energy;
    pfoParameters.m_momentum = momentum;
    pfoParameters.m_mass = std::sqrt(std::max(energy * energy - momentum.GetDotProduct(momentum), 0.f));
    pfoParameters.m_charge = charge;
    pfoParameters.m_particleId = PHOTON;

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode PfoCreationAlgorithm::SetDaughterTrackBasedPfoParameters(Track *const pTrack, PfoParameters &pfoParameters) const
{
    int daughterCharge(0);
    float energy(0.f);
    CartesianVector momentum(0.f, 0.f, 0.f);

    const TrackList &daughterTrackList(pTrack->GetDaughterTrackList());
    const unsigned int nDaughters(daughterTrackList.size());

    for (TrackList::const_iterator iter = daughterTrackList.begin(), iterEnd = daughterTrackList.end(); iter != iterEnd; ++iter)
    {
        Track *pDaughterTrack = *iter;

        if (!pDaughterTrack->CanFormPfo() && !pDaughterTrack->CanFormClusterlessPfo())
            continue;

        daughterCharge += pDaughterTrack->GetCharge();
        energy += pDaughterTrack->GetEnergyAtDca();
        momentum += pDaughterTrack->GetMomentumAtDca();
    }

    pfoParameters.m_energy = energy;
    pfoParameters.m_momentum = momentum;
    pfoParameters.m_mass = std::sqrt(std::max(energy * energy - momentum.GetDotProduct(momentum), 0.f));
    pfoParameters.m_charge = (nDaughters > 1) ? pTrack->GetCharge() : daughterCharge;
    pfoParameters.m_particleId = (pfoParameters.m_charge.Get() > 0) ? PI_PLUS : PI_MINUS;

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode PfoCreationAlgorithm::SetSimpleTrackBasedPfoParameters(Track *const pTrack, PfoParameters &pfoParameters) const
{
    pfoParameters.m_energy = pTrack->GetEnergyAtDca();
    pfoParameters.m_momentum = pTrack->GetMomentumAtDca();
    pfoParameters.m_mass = pTrack->GetMass();
    pfoParameters.m_charge = pTrack->GetCharge();
    pfoParameters.m_particleId = (pTrack->GetCharge() > 0) ? PI_PLUS : PI_MINUS;

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode PfoCreationAlgorithm::CreateNeutralPfos() const
{
    const ClusterList *pClusterList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentClusterList(*this, pClusterList));

    // Examine clusters with no associated tracks to form neutral pfos
    for (ClusterList::const_iterator iter = pClusterList->begin(), iterEnd = pClusterList->end(); iter != iterEnd; ++iter)
    {
        Cluster *pCluster = *iter;

        if (!pCluster->GetAssociatedTrackList().empty())
            continue;

        if (pCluster->GetNCaloHits() < m_minHitsInCluster)
            continue;

        const bool isPhoton(pCluster->IsPhotonFast());
        float clusterEnergy(isPhoton ? pCluster->GetCorrectedElectromagneticEnergy() : pCluster->GetCorrectedHadronicEnergy());

        // Veto non-photon clusters below hadronic energy threshold and those occupying a single layer
        if (!isPhoton)
        {
            if (clusterEnergy < m_minClusterHadronicEnergy)
                continue;

            if (!m_allowSingleLayerClusters && (pCluster->GetInnerPseudoLayer() == pCluster->GetOuterPseudoLayer()))
                continue;
        }
        else
        {
            if (clusterEnergy < m_minClusterElectromagneticEnergy)
                continue;
        }

        // Specify the pfo parameters
        PandoraContentApi::ParticleFlowObject::Parameters pfoParameters;
        pfoParameters.m_particleId = (isPhoton ? PHOTON : NEUTRON);
        pfoParameters.m_charge = 0;
        pfoParameters.m_mass = (isPhoton ? PdgTable::GetParticleMass(PHOTON) : PdgTable::GetParticleMass(NEUTRON));
        pfoParameters.m_energy = clusterEnergy;
        pfoParameters.m_clusterList.insert(pCluster);

        // Photon position: 0) unweighted inner centroid, 1) energy-weighted inner centroid, 2+) energy-weighted centroid for all layers
        CartesianVector positionUnitVector(0.f, 0.f, 0.f);
        const PseudoLayer clusterInnerLayer(pCluster->GetInnerPseudoLayer());

        if (!isPhoton || (0 == m_photonPositionAlgorithm))
        {
            positionUnitVector = pCluster->GetCentroid(clusterInnerLayer).GetUnitVector();
        }
        else if (1 == m_photonPositionAlgorithm)
        {
            positionUnitVector = this->GetEnergyWeightedCentroid(pCluster, clusterInnerLayer, clusterInnerLayer).GetUnitVector();
        }
        else
        {
            positionUnitVector = this->GetEnergyWeightedCentroid(pCluster, clusterInnerLayer, pCluster->GetOuterPseudoLayer()).GetUnitVector();
        }

        pfoParameters.m_momentum = CartesianVector(clusterEnergy * positionUnitVector.GetX(),
            clusterEnergy * positionUnitVector.GetY(),
            clusterEnergy * positionUnitVector.GetZ());

        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::ParticleFlowObject::Create(*this, pfoParameters));
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

const CartesianVector PfoCreationAlgorithm::GetEnergyWeightedCentroid(const Cluster *const pCluster, const PseudoLayer innerPseudoLayer,
    const PseudoLayer outerPseudoLayer) const
{
    float energySum(0.f);
    CartesianVector energyPositionSum(0.f, 0.f, 0.f);
    const OrderedCaloHitList &orderedCaloHitList(pCluster->GetOrderedCaloHitList());

    for (OrderedCaloHitList::const_iterator iter = orderedCaloHitList.begin(), iterEnd = orderedCaloHitList.end(); iter != iterEnd; ++iter)
    {
        if (iter->first > outerPseudoLayer)
            break;

        if (iter->first < innerPseudoLayer)
            continue;

        for (CaloHitList::const_iterator hitIter = iter->second->begin(), hitIterEnd = iter->second->end(); hitIter != hitIterEnd; ++hitIter)
        {
            const float electromagneticEnergy((*hitIter)->GetElectromagneticEnergy());
            energySum += electromagneticEnergy;
            energyPositionSum += ((*hitIter)->GetPositionVector() * electromagneticEnergy);
        }
    }

    if (energySum < std::numeric_limits<float>::epsilon())
        throw StatusCodeException(STATUS_CODE_NOT_INITIALIZED);

    return (energyPositionSum * (1.f / energySum));
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode PfoCreationAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    m_outputPfoListName = "PrimaryPfoList";
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "OutputPfoListName", m_outputPfoListName));

    m_shouldCreateTrackBasedPfos = true;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ShouldCreateTrackBasedPfos", m_shouldCreateTrackBasedPfos));

    m_shouldCreateNeutralPfos = true;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ShouldCreateNeutralPfos", m_shouldCreateNeutralPfos));

    m_minClusterHadronicEnergy = 0.25f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinClusterHadronicEnergy", m_minClusterHadronicEnergy));

    m_minClusterElectromagneticEnergy = 0.f;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinClusterElectromagneticEnergy", m_minClusterElectromagneticEnergy));

    m_minHitsInCluster = 5;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinHitsInCluster", m_minHitsInCluster));

    m_allowSingleLayerClusters = false;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "AllowSingleLayerClusters", m_allowSingleLayerClusters));

    m_photonPositionAlgorithm = 2;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "PhotonPositionAlgorithm", m_photonPositionAlgorithm));

    return STATUS_CODE_SUCCESS;
}
