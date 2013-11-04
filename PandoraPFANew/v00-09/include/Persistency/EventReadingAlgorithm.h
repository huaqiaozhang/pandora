/**
 *  @file   PandoraPFANew/FineGranularityContent/include/Persistency/EventReadingAlgorithm.h
 * 
 *  @brief  Header file for the event reading algorithm class.
 * 
 *  $Log: $
 */
#ifndef EVENT_READING_ALGORITHM_H
#define EVENT_READING_ALGORITHM_H 1

#include "Pandora/Algorithm.h"

namespace pandora {class FileReader;}

//------------------------------------------------------------------------------------------------------------------------------------------

/**
 *  @brief  EventReadingAlgorithm class
 */
class EventReadingAlgorithm : public pandora::Algorithm
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

    /**
     *  @brief  Default constructor
     */
    EventReadingAlgorithm();

    /**
     *  @brief  Destructor
     */
    ~EventReadingAlgorithm();

private:
    pandora::StatusCode Initialize();
    pandora::StatusCode Run();
    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

    bool                    m_shouldReadGeometry;           ///< Whether to read geometry from a specified file
    std::string             m_geometryFileName;             ///< Name of the file containing geometry information

    bool                    m_shouldReadEvents;             ///< Whether to read events from a specified file
    std::string             m_eventFileName;                ///< Name of the file containing event information
    unsigned int            m_skipToEvent;                  ///< Index of first event to consider in input file
    pandora::FileReader    *m_pEventFileReader;             ///< Address of the event file reader
};

//------------------------------------------------------------------------------------------------------------------------------------------

inline pandora::Algorithm *EventReadingAlgorithm::Factory::CreateAlgorithm() const
{
    return new EventReadingAlgorithm();
}

#endif // #ifndef EVENT_READING_ALGORITHM_H
