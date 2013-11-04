/**
 *  @file   PandoraPFANew/Framework/src/Managers/TrackManager.cc
 * 
 *  @brief  Implementation of the track manager class.
 * 
 *  $Log: $
 */

#include "Managers/TrackManager.h"

#include "Objects/Track.h"

namespace pandora
{

TrackManager::TrackManager() :
    InputObjectManager<Track>()
{
    PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->CreateInitialLists());
}

//------------------------------------------------------------------------------------------------------------------------------------------

TrackManager::~TrackManager()
{
    (void) this->EraseAllContent();
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::CreateTrack(const PandoraApi::TrackParameters &trackParameters)
{
    try
    {
        Track *pTrack = NULL;
        pTrack = new Track(trackParameters);

        NameToListMap::iterator inputIter = m_nameToListMap.find(INPUT_LIST_NAME);

        if ((NULL == pTrack) || (m_nameToListMap.end() == inputIter) || (!inputIter->second->insert(pTrack).second))
            throw StatusCodeException(STATUS_CODE_FAILURE);

        if (!m_uidToTrackMap.insert(UidToTrackMap::value_type(pTrack->GetParentTrackAddress(), pTrack)).second)
            throw StatusCodeException(STATUS_CODE_FAILURE);

        return STATUS_CODE_SUCCESS;
    }
    catch (StatusCodeException &statusCodeException)
    {
        std::cout << "Failed to create track: " << statusCodeException.ToString() << std::endl;
        return statusCodeException.GetStatusCode();
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::EraseAllContent()
{
    m_uidToTrackMap.clear();
    m_parentDaughterRelationMap.clear();
    m_siblingRelationMap.clear();

    return InputObjectManager<Track>::EraseAllContent();
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::MatchTracksToMCPfoTargets(const UidToMCParticleMap &trackToPfoTargetMap)
{
    if (trackToPfoTargetMap.empty())
        return STATUS_CODE_SUCCESS;

    NameToListMap::const_iterator inputIter = m_nameToListMap.find(INPUT_LIST_NAME);

    if (m_nameToListMap.end() == inputIter)
        return STATUS_CODE_FAILURE;

    for (TrackList::const_iterator iter = inputIter->second->begin(), iterEnd = inputIter->second->end(); iter != iterEnd; ++iter)
    {
        UidToMCParticleMap::const_iterator pfoTargetIter = trackToPfoTargetMap.find((*iter)->GetParentTrackAddress());

        if (trackToPfoTargetMap.end() != pfoTargetIter)
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, (*iter)->SetMCParticle(pfoTargetIter->second));
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::SetTrackParentDaughterRelationship(const Uid parentUid, const Uid daughterUid)
{
    m_parentDaughterRelationMap.insert(TrackRelationMap::value_type(parentUid, daughterUid));

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::SetTrackSiblingRelationship(const Uid firstSiblingUid, const Uid secondSiblingUid)
{
    m_siblingRelationMap.insert(TrackRelationMap::value_type(firstSiblingUid, secondSiblingUid));

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::AssociateTracks() const
{
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->AddParentDaughterAssociations());
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->AddSiblingAssociations());

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::AddParentDaughterAssociations() const
{
    for (TrackRelationMap::const_iterator uidIter = m_parentDaughterRelationMap.begin(), uidIterEnd = m_parentDaughterRelationMap.end();
        uidIter != uidIterEnd; ++uidIter)
    {
        UidToTrackMap::const_iterator parentIter = m_uidToTrackMap.find(uidIter->first);
        UidToTrackMap::const_iterator daughterIter = m_uidToTrackMap.find(uidIter->second);

        if ((m_uidToTrackMap.end() == parentIter) || (m_uidToTrackMap.end() == daughterIter))
            continue;

        const StatusCode firstStatusCode(parentIter->second->AddDaughter(daughterIter->second));
        const StatusCode secondStatusCode(daughterIter->second->AddParent(parentIter->second));

        if (firstStatusCode != secondStatusCode)
            return STATUS_CODE_FAILURE;

        if ((firstStatusCode != STATUS_CODE_SUCCESS) && (firstStatusCode != STATUS_CODE_ALREADY_PRESENT))
            return firstStatusCode;
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::AddSiblingAssociations() const
{
    for (TrackRelationMap::const_iterator uidIter = m_siblingRelationMap.begin(), uidIterEnd = m_siblingRelationMap.end();
        uidIter != uidIterEnd; ++uidIter)
    {
        UidToTrackMap::const_iterator firstSiblingIter = m_uidToTrackMap.find(uidIter->first);
        UidToTrackMap::const_iterator secondSiblingIter = m_uidToTrackMap.find(uidIter->second);

        if ((m_uidToTrackMap.end() == firstSiblingIter) || (m_uidToTrackMap.end() == secondSiblingIter))
            continue;

        const StatusCode firstStatusCode(firstSiblingIter->second->AddSibling(secondSiblingIter->second));
        const StatusCode secondStatusCode(secondSiblingIter->second->AddSibling(firstSiblingIter->second));

        if (firstStatusCode != secondStatusCode)
            return STATUS_CODE_FAILURE;

        if ((firstStatusCode != STATUS_CODE_SUCCESS) && (firstStatusCode != STATUS_CODE_ALREADY_PRESENT))
            return firstStatusCode;
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::RemoveAllClusterAssociations() const
{
    NameToListMap::const_iterator inputIter = m_nameToListMap.find(INPUT_LIST_NAME);

    if (m_nameToListMap.end() == inputIter)
        return STATUS_CODE_FAILURE;

    for (TrackList::const_iterator iter = inputIter->second->begin(), iterEnd = inputIter->second->end(); iter != iterEnd; ++iter)
    {
        (*iter)->m_pAssociatedCluster = NULL;
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::RemoveCurrentClusterAssociations(TrackToClusterMap &danglingClusters) const
{
    NameToListMap::const_iterator listIter = m_nameToListMap.find(m_currentListName);

    if (m_nameToListMap.end() == listIter)
        return STATUS_CODE_FAILURE;

    for (TrackList::iterator iter = listIter->second->begin(), iterEnd = listIter->second->end(); iter != iterEnd; ++iter)
    {
        Cluster *&pAssociatedCluster = (*iter)->m_pAssociatedCluster;

        if (NULL == pAssociatedCluster)
            continue;

        if (!danglingClusters.insert(TrackToClusterMap::value_type(*iter, pAssociatedCluster)).second)
            return STATUS_CODE_FAILURE;

        pAssociatedCluster = NULL;
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::RemoveClusterAssociations(const TrackList &trackList) const
{
    for (TrackList::const_iterator iter = trackList.begin(), iterEnd = trackList.end(); iter != iterEnd; ++iter)
    {
        (*iter)->m_pAssociatedCluster = NULL;
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::InitializeReclustering(const Algorithm *const pAlgorithm, const TrackList &trackList, const std::string &originalReclusterListName)
{
    std::string temporaryListName;
    return this->CreateTemporaryListAndSetCurrent(pAlgorithm, trackList, temporaryListName);
}

} // namespace pandora
