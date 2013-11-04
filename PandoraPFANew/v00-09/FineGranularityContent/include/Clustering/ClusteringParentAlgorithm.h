/**
 *  @file   PandoraPFANew/FineGranularityContent/include/Clustering/ClusteringParentAlgorithm.h
 * 
 *  @brief  Header file for the clustering parent algorithm class.
 * 
 *  $Log: $
 */
#ifndef CLUSTERING_PARENT_ALGORITHM_H
#define CLUSTERING_PARENT_ALGORITHM_H 1

#include "Pandora/Algorithm.h"

/**
 *  @brief  ClusteringParentAlgorithm class
 */
class ClusteringParentAlgorithm : public pandora::Algorithm
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

    std::string     m_clusteringAlgorithmName;      ///< The name of the clustering algorithm to run
    std::string     m_associationAlgorithmName;     ///< The name of the topological association algorithm to run

    std::string     m_inputCaloHitListName;         ///< The name of the input calo hit list, containing the hits to be clustered
    bool            m_restoreOriginalCaloHitList;   ///< Whether to restore the original calo hit list as the "current" list upon completion

    std::string     m_clusterListName;              ///< The name under which to save the new cluster list
    bool            m_replaceCurrentClusterList;    ///< Whether to subsequently use the new cluster list as the "current" list
};

//------------------------------------------------------------------------------------------------------------------------------------------

inline pandora::Algorithm *ClusteringParentAlgorithm::Factory::CreateAlgorithm() const
{
    return new ClusteringParentAlgorithm();
}

#endif // #ifndef CLUSTERING_PARENT_ALGORITHM_H
