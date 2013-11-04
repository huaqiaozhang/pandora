/**
 *  @file   PandoraPFANew/FineGranularityContent/src/FineGranularityShowerProfileCalculator.cc
 * 
 *  @brief  Implementation of the fine granularity pseudo layer calculator class.
 * 
 *  $Log: $
 */

#include "Helpers/GeometryHelper.h"
#include "Helpers/XmlHelper.h"

#include "Objects/Cluster.h"

#include "FineGranularityShowerProfileCalculator.h"

#include <cmath>
#include <limits>

using namespace pandora;

void FineGranularityShowerProfileCalculator::CalculateLongitudinalProfile(const Cluster *const pCluster, float &profileStart, float &profileDiscrepancy) const
{
    // 1. Construct cluster profile.
    const float clusterEnergy(pCluster->GetElectromagneticEnergy() - pCluster->GetIsolatedElectromagneticEnergy());

    if (clusterEnergy <= 0.f || (pCluster->GetNCaloHits() < 1))
        throw StatusCodeException(STATUS_CODE_INVALID_PARAMETER);

    // Extract information from the cluster
    if (GeometryHelper::GetHitTypeGranularity(pCluster->GetInnerLayerHitType()) > FINE)
        return;

    const CartesianVector &clusterDirection(pCluster->GetFitToAllHitsResult().IsFitSuccessful() ?
        pCluster->GetFitToAllHitsResult().GetDirection() : pCluster->GetInitialDirection());

    // Initialize profile
    FloatVector profile;
    for (unsigned int iBin = 0; iBin < m_longProfileNBins; ++iBin)
    {
        profile.push_back(0.f);
    }

    // Examine layers to construct profile
    float eCalEnergy(0.f), nRadiationLengths(0.f), nRadiationLengthsInLastLayer(0.f);
    unsigned int profileEndBin(0);

    const OrderedCaloHitList &orderedCaloHitList(pCluster->GetOrderedCaloHitList());
    const PseudoLayer innerPseudoLayer(pCluster->GetInnerPseudoLayer());
    static const PseudoLayer firstPseudoLayer(GeometryHelper::GetPseudoLayerAtIp());

    for (PseudoLayer iLayer = innerPseudoLayer, outerPseudoLayer = pCluster->GetOuterPseudoLayer(); iLayer <= outerPseudoLayer; ++iLayer)
    {
        OrderedCaloHitList::const_iterator iter = orderedCaloHitList.find(iLayer);

        if ((orderedCaloHitList.end() == iter) || (iter->second->empty()))
        {
            nRadiationLengths += nRadiationLengthsInLastLayer;
            continue;
        }

        // Extract information from calo hits
        bool isFineGranularity(true);
        float energyInLayer(0.f);
        float nRadiationLengthsInLayer(0.f);

        for (CaloHitList::const_iterator hitIter = iter->second->begin(), hitIterEnd = iter->second->end(); hitIter != hitIterEnd; ++hitIter)
        {
            if (GeometryHelper::GetHitTypeGranularity((*hitIter)->GetHitType()) > FINE)
            {
                isFineGranularity = false;
                break;
            }

            float cosOpeningAngle(std::fabs((*hitIter)->GetCellNormalVector().GetCosOpeningAngle(clusterDirection)));
            cosOpeningAngle = std::max(cosOpeningAngle, m_longProfileMinCosAngle);

            energyInLayer += (*hitIter)->GetElectromagneticEnergy();
            nRadiationLengthsInLayer += (*hitIter)->GetNCellRadiationLengths() / cosOpeningAngle;
        }

        if (!isFineGranularity)
            break;

        eCalEnergy += energyInLayer;
        nRadiationLengthsInLayer /= static_cast<float>(iter->second->size());
        nRadiationLengthsInLastLayer = nRadiationLengthsInLayer;
        nRadiationLengths += nRadiationLengthsInLayer;

        // Account for layers before start of cluster
        if (innerPseudoLayer == iLayer)
            nRadiationLengths *= static_cast<float>(innerPseudoLayer + 1 - firstPseudoLayer);

        // Finally, create the profile
        const float endPosition(nRadiationLengths / m_longProfileBinWidth);
        const unsigned int endBin(std::min(static_cast<unsigned int>(endPosition), m_longProfileNBins - 1));

        const float deltaPosition(nRadiationLengthsInLayer / m_longProfileBinWidth);

        const float startPosition(endPosition - deltaPosition);
        const unsigned int startBin(static_cast<unsigned int>(startPosition));

        for (unsigned int iBin = startBin; iBin <= endBin; ++iBin)
        {
            float delta(1.f);

            if (startBin == iBin)
            {
                delta -= startPosition - startBin;
            }
            else if (endBin == iBin)
            {
                delta -= 1.f - endPosition + endBin;
            }

            profile[iBin] += energyInLayer * (delta / deltaPosition);
        }
    }

    profileEndBin = std::min(static_cast<unsigned int>(nRadiationLengths / m_longProfileBinWidth), m_longProfileNBins);

    if (eCalEnergy <= 0.f)
        throw StatusCodeException(STATUS_CODE_FAILURE);

    // 2. Construct expected cluster profile
    const double a(m_longProfileParameter0 + m_longProfileParameter1 * std::log(clusterEnergy / m_longProfileCriticalEnergy));
#ifdef __GNUC__
    const double gammaA(std::exp(lgamma(a)));
#else
    const double gammaA(0.);
    std::cout << " PandoraPFANew/FineGranularityContent/ShowerProfileCalculator: TODO - implement gamma function " << std::endl;
    throw StatusCodeException(STATUS_CODE_FAILURE);
#endif

    float t(0.f);
    FloatVector expectedProfile;
    for (unsigned int iBin = 0; iBin < m_longProfileNBins; ++iBin)
    {
        t += m_longProfileBinWidth;
        expectedProfile.push_back(static_cast<float>(clusterEnergy / 2. * std::pow(t / 2.f, static_cast<float>(a - 1.)) *
            std::exp(-t / 2.) * m_longProfileBinWidth / gammaA));
    }

    // 3. Compare the cluster profile with the expected profile
    unsigned int binOffsetAtMinDifference(0);
    float minProfileDifference(std::numeric_limits<float>::max());

    for (unsigned int iBinOffset = 0; iBinOffset < profileEndBin; ++iBinOffset)
    {
        float profileDifference(0.);

        for (unsigned int iBin = 0; iBin < profileEndBin; ++iBin)
        {
            if (iBin < iBinOffset)
            {
                profileDifference += profile[iBin];
            }
            else
            {
                profileDifference += std::fabs(expectedProfile[iBin - iBinOffset] - profile[iBin]);
            }
        }

        if (profileDifference < minProfileDifference)
        {
            minProfileDifference = profileDifference;
            binOffsetAtMinDifference = iBinOffset;
        }

        if (profileDifference - minProfileDifference > m_longProfileMaxDifference)
            break;
    }

    profileStart =  binOffsetAtMinDifference * m_longProfileBinWidth;
    profileDiscrepancy = minProfileDifference / eCalEnergy;
}

//------------------------------------------------------------------------------------------------------------------------------------------

void FineGranularityShowerProfileCalculator::CalculateTransverseProfile(const Cluster *const pCluster, const PseudoLayer maxPseudoLayer,
    ParticleIdHelper::ShowerPeakList &showerPeakList) const
{
    // Set up coordinate system
    const CartesianVector innerLayerCentroid(pCluster->GetCentroid(pCluster->GetInnerPseudoLayer()));
    const CartesianVector uAxis(CartesianVector(innerLayerCentroid.GetY(), -innerLayerCentroid.GetX(), 0.f).GetUnitVector());
    const CartesianVector vAxis(uAxis.GetCrossProduct(innerLayerCentroid).GetUnitVector());

    // Create 2d shower profile
    TwoDShowerProfile showerProfile(m_transProfileNBins, ShowerProfile(m_transProfileNBins, ShowerProfileEntry()));
    const OrderedCaloHitList &orderedCaloHitList(pCluster->GetOrderedCaloHitList());
    static const int nOffsetBins(m_transProfileNBins / 2);

    for (OrderedCaloHitList::const_iterator iter = orderedCaloHitList.begin(), iterEnd = orderedCaloHitList.end(); iter != iterEnd; ++iter)
    {
        if (iter->first > maxPseudoLayer)
            break;

        for (CaloHitList::const_iterator hitIter = iter->second->begin(), hitIterEnd = iter->second->end(); hitIter != hitIterEnd; ++hitIter)
        {
            CaloHit *pCaloHit = *hitIter;
            const float cellLengthScale(pCaloHit->GetCellLengthScale());

            if (std::fabs(cellLengthScale) < std::numeric_limits<float>::epsilon())
                continue;

            const CartesianVector hitCoordinates((pCaloHit->GetPositionVector() - innerLayerCentroid) * (1.f / cellLengthScale));
            const float uValue(hitCoordinates.GetDotProduct(uAxis));
            const float vValue(hitCoordinates.GetDotProduct(vAxis));

            const int uBin(static_cast<int>(uValue + 0.5f + nOffsetBins));
            const int vBin(static_cast<int>(vValue + 0.5f + nOffsetBins));

            if ((uBin >= 0) && (vBin >= 0) && (uBin < m_transProfileNBins) && (vBin < m_transProfileNBins))
            {
                showerProfile[uBin][vBin].m_energy += pCaloHit->GetElectromagneticEnergy();
                showerProfile[uBin][vBin].m_caloHitList.insert(pCaloHit);
            }
        }
    }

    // Mask low pulse-height regions
    for (int uBin = 0; uBin < m_transProfileNBins; ++uBin)
    {
        for (int vBin = 0; vBin < m_transProfileNBins; ++vBin)
        {
            if (showerProfile[uBin][vBin].m_energy < m_transProfilePeakThreshold)
                showerProfile[uBin][vBin].m_isAvailable = false;
        }
    }

    // Search for peaks in profile
    unsigned int nPeaks(0);

    while (true)
    {
        // Identify current peak
        float peakEnergy(0.f);
        CaloHitList peakCaloHitList;

        int peakUBin(0);
        int peakVBin(0);

        for (int uBin = 0; uBin < m_transProfileNBins; ++uBin)
        {
            for (int vBin = 0; vBin < m_transProfileNBins; ++vBin)
            {
                if (showerProfile[uBin][vBin].m_isAvailable && (showerProfile[uBin][vBin].m_energy > peakEnergy))
                {
                    peakEnergy = showerProfile[uBin][vBin].m_energy;
                    peakUBin = uBin;
                    peakVBin = vBin;
                }
            }
        }

        if (peakEnergy < m_transProfilePeakThreshold)
            break;

        // Begin to extract peak properties
        nPeaks++;
        peakCaloHitList = showerProfile[peakUBin][peakVBin].m_caloHitList;

        const float uPeakBinDifference(peakUBin - nOffsetBins);
        const float vPeakBinDifference(peakVBin - nOffsetBins);
        float uBar(uPeakBinDifference * peakEnergy);
        float vBar(vPeakBinDifference * peakEnergy);
        float uuBar(uPeakBinDifference * uPeakBinDifference * peakEnergy);
        float vvBar(vPeakBinDifference * vPeakBinDifference * peakEnergy);

        // Examine region around the peak
        int peakStartBin(0);
        int peakEndBin(0);
        int peakCurrentBin(peakEndBin);
        const int maxBinCount(m_transProfileNBins * m_transProfileNBins);

        typedef std::pair<int, int> BinCoordinates;
        typedef std::vector<BinCoordinates> BinCoordinatesList;

        BinCoordinatesList binCoordinatesList(maxBinCount, BinCoordinates(0, 0));
        binCoordinatesList[0] = BinCoordinates(peakUBin, peakVBin);

        while (true)
        {
            for (int peakBin = peakStartBin; peakBin <= peakEndBin; ++peakBin)
            {
                const int uCentralBin(binCoordinatesList[peakBin].first);
                const int vCentralBin(binCoordinatesList[peakBin].second);

                if ((uCentralBin < 0) || (vCentralBin < 0) || (uCentralBin >= m_transProfileNBins) || (vCentralBin >= m_transProfileNBins))
                    continue;

                const float centralEnergy(showerProfile[uCentralBin][vCentralBin].m_energy);
                showerProfile[uCentralBin][vCentralBin].m_isAvailable = false;

                const int uStartBin((uCentralBin > 0) ? uCentralBin - 1 : 0);
                const int uEndBin((uCentralBin < m_transProfileNBins - 1) ? uCentralBin + 1 : m_transProfileNBins -1);

                for (int uBin = uStartBin; uBin <= uEndBin; ++uBin)
                {
                    const int vStartBin((vCentralBin > 0) ? vCentralBin - 1 : 0);
                    const int vEndBin((vCentralBin < m_transProfileNBins - 1) ? vCentralBin + 1 : m_transProfileNBins -1);

                    for (int vBin = vStartBin; vBin <= vEndBin; ++vBin)
                    {
                        ShowerProfileEntry &showerProfileEntry(showerProfile[uBin][vBin]);

                        if (showerProfileEntry.m_isAvailable && (showerProfileEntry.m_energy < m_transProfileNearbyEnergyRatio * centralEnergy))
                        {
                            const float energy(showerProfileEntry.m_energy);
                            peakEnergy += energy;

                            const float uBinDifference(uBin - nOffsetBins);
                            const float vBinDifference(vBin - nOffsetBins);
                            uBar += uBinDifference * energy;
                            vBar += vBinDifference * energy;
                            uuBar += uBinDifference * uBinDifference * energy;
                            vvBar += vBinDifference * vBinDifference * energy;

                            peakCaloHitList.insert(showerProfileEntry.m_caloHitList.begin(), showerProfileEntry.m_caloHitList.end());
                            showerProfile[uBin][vBin].m_isAvailable = false;
                            peakCurrentBin++;

                            if (peakCurrentBin > maxBinCount)
                                throw StatusCodeException(STATUS_CODE_FAILURE);

                            binCoordinatesList[peakCurrentBin] = BinCoordinates(uBin, vBin);
                        }
                    }
                }
            }

            if (peakCurrentBin == peakEndBin)
                break;

            peakStartBin = peakEndBin + 1;
            peakEndBin = peakCurrentBin;
        }

        // Store peak properties
        uBar /= peakEnergy;
        vBar /= peakEnergy;
        uuBar /= peakEnergy;
        vvBar /= peakEnergy;
        const float meanSquareDeviation(uuBar + vvBar - uBar * uBar - vBar * vBar);
        const float peakRms((meanSquareDeviation > 0.f) ? std::sqrt(meanSquareDeviation) : std::numeric_limits<float>::max());

        showerPeakList.push_back(ParticleIdHelper::ShowerPeak(peakEnergy, peakRms, peakCaloHitList));

        if (nPeaks > m_transProfileMaxPeaksToFind)
            break;
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

// Parameter default values
float FineGranularityShowerProfileCalculator::m_longProfileBinWidth = 0.5f;
unsigned int FineGranularityShowerProfileCalculator::m_longProfileNBins = 100;
float FineGranularityShowerProfileCalculator::m_longProfileMinCosAngle = 0.3f;
float FineGranularityShowerProfileCalculator::m_longProfileCriticalEnergy = 0.08f;
float FineGranularityShowerProfileCalculator::m_longProfileParameter0 = 1.25f;
float FineGranularityShowerProfileCalculator::m_longProfileParameter1 = 0.5f;
float FineGranularityShowerProfileCalculator::m_longProfileMaxDifference = 0.1f;
int FineGranularityShowerProfileCalculator::m_transProfileNBins = 41;
float FineGranularityShowerProfileCalculator::m_transProfilePeakThreshold = 0.025f;
float FineGranularityShowerProfileCalculator::m_transProfileNearbyEnergyRatio = 2.f;
unsigned int FineGranularityShowerProfileCalculator::m_transProfileMaxPeaksToFind = 3;

//------------------------------------------------------------------------------------------------------------------------------------------

void FineGranularityShowerProfileCalculator::ReadSettings(const TiXmlHandle *const pXmlHandle)
{
    PANDORA_THROW_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(*pXmlHandle,
        "LongProfileBinWidth", m_longProfileBinWidth));

    if (0.f == m_longProfileBinWidth)
        throw StatusCodeException(STATUS_CODE_INVALID_PARAMETER);

    PANDORA_THROW_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(*pXmlHandle,
        "LongProfileNBins", m_longProfileNBins));

    if (0 == m_longProfileNBins)
        throw StatusCodeException(STATUS_CODE_INVALID_PARAMETER);

    PANDORA_THROW_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(*pXmlHandle,
        "LongProfileMinCosAngle", m_longProfileMinCosAngle));

    PANDORA_THROW_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(*pXmlHandle,
        "LongProfileCriticalEnergy", m_longProfileCriticalEnergy));

    PANDORA_THROW_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(*pXmlHandle,
        "LongProfileParameter0", m_longProfileParameter0));

    PANDORA_THROW_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(*pXmlHandle,
        "LongProfileParameter1", m_longProfileParameter1));

    PANDORA_THROW_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(*pXmlHandle,
        "LongProfileMaxDifference", m_longProfileMaxDifference));

    PANDORA_THROW_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(*pXmlHandle,
        "TransProfileNBins", m_transProfileNBins));

    PANDORA_THROW_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(*pXmlHandle,
        "TransProfilePeakThreshold", m_transProfilePeakThreshold));

    PANDORA_THROW_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(*pXmlHandle,
        "TransProfileNearbyEnergyRatio", m_transProfileNearbyEnergyRatio));

    PANDORA_THROW_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(*pXmlHandle,
        "TransProfileMaxPeaksToFind", m_transProfileMaxPeaksToFind));
}
