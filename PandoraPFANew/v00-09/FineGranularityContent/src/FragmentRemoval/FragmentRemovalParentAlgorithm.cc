/**
 *  @file   PandoraPFANew/FineGranularityContent/src/FragmentRemoval/FragmentRemovalParentAlgorithm.cc
 * 
 *  @brief  Implementation of the fragment removal parent algorithm class.
 * 
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "FragmentRemoval/FragmentRemovalParentAlgorithm.h"

using namespace pandora;

StatusCode FragmentRemovalParentAlgorithm::Run()
{
    // Run fragment removal daughter algorithms
    for (StringVector::const_iterator iter = m_FragmentRemovalParentAlgorithms.begin(), iterEnd = m_FragmentRemovalParentAlgorithms.end(); iter != iterEnd; ++iter)
    {
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::RunDaughterAlgorithm(*this, *iter));
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode FragmentRemovalParentAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ProcessAlgorithmList(*this, xmlHandle, "fragmentRemovalAlgorithms",
        m_FragmentRemovalParentAlgorithms));

    return STATUS_CODE_SUCCESS;
}
