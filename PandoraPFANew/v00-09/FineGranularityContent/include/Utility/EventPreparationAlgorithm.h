/**
 *  @file   PandoraPFANew/FineGranularityContent/include/Utility/EventPreparationAlgorithm.h
 * 
 *  @brief  Header file for the track selection algorithm class.
 * 
 *  $Log: $
 */
#ifndef EVENT_PREPARATION_ALGORITHM_H
#define EVENT_PREPARATION_ALGORITHM_H 1

#include "Pandora/Algorithm.h"

/**
 *  @brief  EventPreparationAlgorithm class
 */
class EventPreparationAlgorithm : public pandora::Algorithm
{
public:
    /**
     *  @brief  Factory class for instantiating algorithm
     */
    class Factory : public pandora::AlgorithmFactory
    {
    public:
        pandora::Algorithm *CreateAlgorithm() const;
    };

private:
    pandora::StatusCode Run();
    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

    std::string     m_outputTrackListName;          ///< The output track list name
    std::string     m_outputMuonCaloHitListName;    ///< The output muon calo hit list name
    std::string     m_outputCaloHitListName;        ///< The output calo hit list name
};

//------------------------------------------------------------------------------------------------------------------------------------------

inline pandora::Algorithm *EventPreparationAlgorithm::Factory::CreateAlgorithm() const
{
    return new EventPreparationAlgorithm();
}

#endif // #ifndef EVENT_PREPARATION_ALGORITHM_H
