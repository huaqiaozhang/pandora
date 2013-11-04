/**
 *  @file   PandoraPFANew/Framework/include/Pandora/PandoraImpl.h
 * 
 *  @brief  Header file for the pandora impl class.
 * 
 *  $Log: $
 */
#ifndef PANDORA_IMPL_H
#define PANDORA_IMPL_H 1

namespace pandora
{

/**
 *  @brief  PandoraImpl class
 */
class PandoraImpl
{
private:
    /**
     *  @brief  Prepare mc particles: select mc pfo targets, match tracks and calo hits to the correct mc
     *          particles for particle flow
     */
    StatusCode PrepareMCParticles() const;

    /**
     *  @brief  Prepare tracks: add track associations (parent-daughter and sibling)
     */
    StatusCode PrepareTracks() const;

    /**
     *  @brief  Prepare calo hits: order the hits by pseudo layer, calculate density weights, identify
     *          isolated hits, identify possible mip hits and calculate surrounding energy values.
     */
    StatusCode PrepareCaloHits() const;

    /**
     *  @brief  Initialize pandora algorithms
     * 
     *  @param  pXmlHandle address of the relevant xml handle
     */
    StatusCode InitializeAlgorithms(const TiXmlHandle *const pXmlHandle) const;

    /**
     *  @brief  Initialize pandora plugins
     * 
     *  @param  pXmlHandle address of the relevant xml handle
     */
    StatusCode InitializePlugins(const TiXmlHandle *const pXmlHandle) const;

    /**
     *  @brief  Run an algorithm registered with pandora
     * 
     *  @param  algorithmName the name of the algorithm instance to run
     */
    StatusCode RunAlgorithm(const std::string &algorithmName) const;

private:
    /**
     *  @brief  Constructor
     * 
     *  @param  pPandora address of the pandora object to interface
     */
    PandoraImpl(Pandora *pPandora);

    Pandora    *m_pPandora;    ///< The pandora object to provide an interface to

    friend class Pandora;
};

} // namespace pandora

#endif // #ifndef PANDORA_IMPL_H
