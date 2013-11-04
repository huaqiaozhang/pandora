/**
 *  @file   PandoraPfaNew/Algoirthms/src/KinkPfoAlgorithm.cc
 * 
 *  @brief  Analyse kink pfos to identify decays to muons and neutral hadrons
 * 
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "PfoConstruction/V0PfoCreationAlgorithm.h"

using namespace pandora;

StatusCode V0PfoCreationAlgorithm::Run()
{
    const PfoList *pPfoList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentPfoList(*this, pPfoList));

    for (PfoList::const_iterator iter = pPfoList->begin(), iterEnd = pPfoList->end(); iter != iterEnd; ++iter)
    {
        ParticleFlowObject *pPfo = *iter;

        const TrackList &trackList(pPfo->GetTrackList());

        if (trackList.size() != 2)
            continue;

        const Track *pTrack1 = *(trackList.begin());

        const TrackList &siblingTrackList(pTrack1->GetSiblingTrackList());

        if (siblingTrackList.size() != 1)
            continue;

        const Track *pTrack2 = *(siblingTrackList.begin());

        float mass = 0.f;
        const CartesianVector &momentumAtStart1(pTrack1->GetTrackStateAtStart().GetMomentum());
        const CartesianVector &momentumAtStart2(pTrack2->GetTrackStateAtStart().GetMomentum());

        if (std::abs(pTrack1->GetParticleId()) == std::abs(E_MINUS) && std::abs(pTrack2->GetParticleId()) == std::abs(E_MINUS))
        {
            if (STATUS_CODE_SUCCESS == this->GetV0Mass(momentumAtStart1, momentumAtStart2, PdgTable::GetParticleMass(E_MINUS),
                PdgTable::GetParticleMass(E_MINUS), mass))
            {
                pPfo->SetParticleId(PHOTON);
            }
        }

        if (std::abs(pTrack1->GetParticleId()) == std::abs(PI_PLUS) && std::abs(pTrack2->GetParticleId()) == std::abs(PI_PLUS))
        {
            if (STATUS_CODE_SUCCESS == this->GetV0Mass(momentumAtStart1, momentumAtStart2, PdgTable::GetParticleMass(PI_MINUS),
                PdgTable::GetParticleMass(PI_MINUS), mass))
            {
                pPfo->SetParticleId(K_SHORT);
            }
        }

        if (std::abs(pTrack1->GetParticleId()) == std::abs(PROTON) && std::abs(pTrack2->GetParticleId()) == std::abs(PI_MINUS))
        {
            if (STATUS_CODE_SUCCESS == this->GetV0Mass(momentumAtStart1, momentumAtStart2, PdgTable::GetParticleMass(PROTON),
                PdgTable::GetParticleMass(PI_MINUS), mass))
            {
                (pTrack1->GetParticleId() == PROTON) ? pPfo->SetParticleId(LAMBDA) : pPfo->SetParticleId(LAMBDA_BAR);
            }
        }

        if (std::abs(pTrack1->GetParticleId()) == std::abs(PI_MINUS) && std::abs(pTrack2->GetParticleId()) == std::abs(PROTON))
        {
            if (STATUS_CODE_SUCCESS == this->GetV0Mass(momentumAtStart1, momentumAtStart2, PdgTable::GetParticleMass(PI_MINUS),
                PdgTable::GetParticleMass(PROTON), mass))
            {
                (pTrack2->GetParticleId() == PROTON) ? pPfo->SetParticleId(LAMBDA) : pPfo->SetParticleId(LAMBDA_BAR);
            }
        }

        if (m_mcMonitoring)
        {
            const MCParticle *pMCParent1 = NULL;
            const MCParticle *pMCParent2 = NULL;

            try
            {
                const MCParticle *pMCParticle1 = NULL;
                const MCParticle *pMCParticle2 = NULL;

                PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, pTrack1->GetMCParticle(pMCParticle1));
                PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, pTrack2->GetMCParticle(pMCParticle2));

                const int ipdg1 =  pMCParticle1->GetParticleId();
                std::cout << " Track 1 : " << " ipdg = " << ipdg1 << " E = " << pMCParticle1->GetEnergy() << std::endl;

                const int ipdg2 =  pMCParticle2->GetParticleId();
                std::cout << " Track 2 : " << " ipdg = " << ipdg2 << " E = " << pMCParticle2->GetEnergy() << std::endl;

                const MCParticleList &parentList1(pMCParticle1->GetParentList());
                const MCParticleList &parentList2(pMCParticle2->GetParentList());

                if (parentList1.empty() || parentList2.empty())
                    throw StatusCodeException(STATUS_CODE_NOT_INITIALIZED);

                pMCParent1 = *(parentList1.begin());
                pMCParent2 = *(parentList2.begin());

                for (MCParticleList::const_iterator diter = parentList1.begin(), iterEnd = parentList1.end(); diter != iterEnd; ++diter)
                {
                    int dipdg = (*diter)->GetParticleId();
                    const float energyD = (*diter)->GetEnergy();

                    if(energyD > 0.1f)
                        std::cout << "    decay parent " << dipdg <<  " E = " << energyD << std::endl;
                }

                for (MCParticleList::const_iterator diter = parentList2.begin(), iterEnd = parentList2.end(); diter != iterEnd; ++diter)
                {
                    int dipdg = (*diter)->GetParticleId();
                    const float energyD = (*diter)->GetEnergy();

                    if (energyD > 0.1f)
                        std::cout << "    decay parent " << dipdg <<  " E = " << energyD << std::endl;
                }
            }
            catch (StatusCodeException &statusCodeException)
            {
                std::cout << "Failed to find MC particle for track " << statusCodeException.ToString() << std::endl;
                continue;
            }

            bool goodV0 = true;

            if (pMCParent1 != pMCParent2)
                goodV0 = false;

            if (pMCParent1->GetParticleId() != pPfo->GetParticleId())
                goodV0 = false;

            // Fast electron id
            bool isElectron1 = false;

            if (pTrack1->HasAssociatedCluster())
            {
                Cluster *pCluster1 = NULL;
                PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, pTrack1->GetAssociatedCluster(pCluster1));
                isElectron1 = ParticleIdHelper::IsElectronFast(pCluster1);
            }

            bool isElectron2 = false;

            if (pTrack2->HasAssociatedCluster())
            {
                Cluster *pCluster2 = NULL;
                PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, pTrack2->GetAssociatedCluster(pCluster2));
                isElectron2 = ParticleIdHelper::IsElectronFast(pCluster2);
            }

            // Print debug information
            if (goodV0)
            {
                std::cout << " Good V0 : " << pPfo->GetParticleId() << " " << pTrack1->GetEnergyAtDca() << " "
                          << pTrack2->GetEnergyAtDca() << " eID " << isElectron1 << " " << isElectron2 << " Mass = " << mass << std::endl;
            }
            else
            {
                std::cout << " Bad  V0 : " << pPfo->GetParticleId() << " " << pTrack1->GetEnergyAtDca() << " "
                          << pTrack2->GetEnergyAtDca() << " eID " << isElectron1 << " " << isElectron2 << " Mass = " << mass << std::endl;
            }

            const TrackState &trackState1(pTrack1->GetTrackStateAtStart());
            const TrackState &trackState2(pTrack2->GetTrackStateAtStart());
            const CartesianVector &position1(trackState1.GetPosition());
            const CartesianVector &position2(trackState2.GetPosition());

            const float rxy1(std::sqrt(position1.GetX() * position1.GetX() + position1.GetY() * position1.GetY()));
            const float rxy2(std::sqrt(position2.GetX() * position2.GetX() + position2.GetY() * position2.GetY()));

            std::cout << "     " << " r = " << position1.GetMagnitude() << " rxy = " << rxy1 << std::endl;
            std::cout << "     " << " r = " << position2.GetMagnitude() << " rxy = " << rxy2 << std::endl;
        }
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode V0PfoCreationAlgorithm::GetV0Mass(const CartesianVector &momentum1, const CartesianVector &momentum2, float mass1, float mass2,
    float &v0mass) const
{
    // Calculate the invariant mass for a decaying charged particle
    v0mass = 0.f;

    const float energy1(std::sqrt(momentum1.GetMagnitudeSquared() + mass1 * mass1));
    const float energy2(std::sqrt(momentum2.GetMagnitudeSquared() + mass2 * mass2));

    const CartesianVector totalMomentum(momentum1 + momentum2);

    const float totalEnergy(energy1 + energy2);
    const float massSquared(totalEnergy * totalEnergy - totalMomentum.GetMagnitudeSquared());

    if (massSquared <= 0.f)
        return STATUS_CODE_FAILURE;

    v0mass = std::sqrt(massSquared);
    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode V0PfoCreationAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    m_mcMonitoring = false;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "McMonitoring", m_mcMonitoring));

    return STATUS_CODE_SUCCESS;
}
