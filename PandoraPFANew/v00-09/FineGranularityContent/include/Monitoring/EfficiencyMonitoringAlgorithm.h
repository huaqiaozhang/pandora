/**
 *  @file   PandoraPFANew/FineGranularityContent/include/Monitoring/EfficiencyMonitoringAlgorithm.h
 * 
 *  @brief  Header file for the efficiency monitoring algorithm class.
 * 
 *  $Log: $
 */
#ifndef EFFICIENCY_MONITORING_ALGORITHM_H
#define EFFICIENCY_MONITORING_ALGORITHM_H 1

#include "Pandora/Algorithm.h"

/**
 *  @brief EfficiencyMonitoringAlgorithm class
 */
class EfficiencyMonitoringAlgorithm : public pandora::Algorithm
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
     *  @brief  Destructor
     */
    ~EfficiencyMonitoringAlgorithm();

private:
    pandora::StatusCode Run();
    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

    std::string     m_monitoringFileName;       ///< The name of the file in which to save the monitoring tree
    float           m_mcThresholdEnergy;        ///< MC particle threshold energy, units GeV
};

//------------------------------------------------------------------------------------------------------------------------------------------

inline pandora::Algorithm *EfficiencyMonitoringAlgorithm::Factory::CreateAlgorithm() const
{
    return new EfficiencyMonitoringAlgorithm();
}

#endif // #ifndef EFFICIENCY_MONITORING_ALGORITHM_H
