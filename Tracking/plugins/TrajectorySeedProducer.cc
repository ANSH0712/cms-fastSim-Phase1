/*!

  NOTE: what is called here 'FastTrackPreCandidate' is currently still known as 'FastTrackerRecHitCombination'
  
  TrajectorySeedProducer emulates the reconstruction of TrajectorySeeds in FastSim.

  The input data is a list of FastTrackPreCandidates.
  (input data are configured through the parameter 'src')
  In each given FastTrackPreCandidate,
  TrajectorySeedProducer searches for one combination of hits that matches all given seed requirements,
  (see parameters 'layerList','regionFactoryPSet')
  In that process it respects the order of the hits in the FastTrackPreCandidate.
  When such combination is found, a TrajectorySeed is reconstructed.
  Optionally, one can specify a list of hits to be ignored by TrajectorySeedProducer.
  (see parameter 'hitMasks')
  
  The output data is the list of reconstructed TrajectorySeeds.

*/

// system
#include <memory>
#include <vector>
#include <string>

// framework
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

// data formats 
#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/Common/interface/OwnVector.h"
#include "DataFormats/TrackerRecHit2D/interface/FastTrackerRecHitCollection.h"
#include "DataFormats/TrajectorySeed/interface/TrajectorySeedCollection.h"

// reco track classes
#include "RecoTracker/TkTrackingRegions/interface/TrackingRegion.h"
#include "RecoTracker/TkSeedingLayers/interface/SeedingHitSet.h"
#include "RecoTracker/TkSeedGenerator/interface/SeedCreator.h"
#include "RecoTracker/TkSeedGenerator/interface/SeedCreatorFactory.h"

// geometry
#include "Geometry/Records/interface/TrackerTopologyRcd.h"
#include "DataFormats/TrackerCommon/interface/TrackerTopology.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
// fastsim
#include "FastSimulation/Tracking/interface/SeedingTree.h"
#include "FastSimulation/Tracking/interface/TrackingLayer.h"
#include "FastSimulation/Tracking/interface/FastTrackingUtilities.h"
#include "FastSimulation/Tracking/interface/SeedFinder.h"
#include "FastSimulation/Tracking/interface/SeedFinderSelector.h"

class MeasurementTracker;

class TrajectorySeedProducer:
    public edm::stream::EDProducer<>
{
private:
    
    // tokens
    
    edm::EDGetTokenT<FastTrackerRecHitCombinationCollection> recHitCombinationsToken;
    edm::EDGetTokenT<std::vector<bool> > hitMasksToken;
    edm::EDGetTokenT<edm::OwnVector<TrackingRegion> > trackingRegionToken;

    // other data members
    unsigned int nHitsPerSeed_;

    std::vector<std::vector<TrackingLayer>> seedingLayers;
    SeedingTree<TrackingLayer> _seedingTree;
    
    std::unique_ptr<SeedCreator> seedCreator;
    std::string measurementTrackerLabel;
    
    std::unique_ptr<SeedFinderSelector> seedFinderSelector;
public:
    TrajectorySeedProducer(const edm::ParameterSet& conf);

    virtual void produce(edm::Event& e, const edm::EventSetup& es);


};


template class SeedingTree<TrackingLayer>;
template class SeedingNode<TrackingLayer>;

TrajectorySeedProducer::TrajectorySeedProducer(const edm::ParameterSet& conf)
{
    // products
    produces<TrajectorySeedCollection>();

    // consumes
    recHitCombinationsToken = consumes<FastTrackerRecHitCombinationCollection>(conf.getParameter<edm::InputTag>("recHitCombinations"));
    if (conf.exists("hitMasks"))
    {
        hitMasksToken = consumes<std::vector<bool> >(conf.getParameter<edm::InputTag>("hitMasks"));
    }

    // read Layers
    std::vector<std::string> layerStringList = conf.getParameter<std::vector<std::string>>("layerList");
    std::string layerBegin = *(layerStringList.cbegin());
    nHitsPerSeed_ = 0;
    for(auto it=layerStringList.cbegin(); it < layerStringList.cend(); ++it) 
    {
        std::vector<TrackingLayer> trackingLayerList;
        std::string line = *it;
        std::string::size_type pos=0;
	unsigned int nHitsPerSeed = 0;
        while (pos != std::string::npos)
        {
            pos=line.find("+");
            std::string layer = line.substr(0, pos);
            TrackingLayer layerSpec = TrackingLayer::createFromString(layer);
            trackingLayerList.push_back(layerSpec);
            line=line.substr(pos+1,std::string::npos);
	    nHitsPerSeed++;
        }
	if(it==layerStringList.cbegin())
	{
	    nHitsPerSeed_ = nHitsPerSeed;
	}
	else if(nHitsPerSeed_!=nHitsPerSeed)
	{
	    throw cms::Exception("FastSimTracking") << "All allowed seed layer definitions must have same elements";
	}
        _seedingTree.insert(trackingLayerList);
    }

    // seed finder selector
    if(conf.exists("seedFinderSelector"))
    {
	seedFinderSelector.reset(new SeedFinderSelector(conf.getParameter<edm::ParameterSet>("seedFinderSelector"),consumesCollector()));
    }

    /// regions
    trackingRegionToken = consumes<edm::OwnVector<TrackingRegion> >(conf.getParameter<edm::InputTag>("trackingRegions"));
    
    // seed creator
    const edm::ParameterSet & seedCreatorPSet = conf.getParameter<edm::ParameterSet>("SeedCreatorPSet");
    std::string seedCreatorName = seedCreatorPSet.getParameter<std::string>("ComponentName");
    seedCreator.reset(SeedCreatorFactory::get()->create( seedCreatorName, seedCreatorPSet));

}


void TrajectorySeedProducer::produce(edm::Event& e, const edm::EventSetup& es) 
{        

    // services
    edm::ESHandle<TrackerTopology> trackerTopology;
    
    es.get<TrackerTopologyRcd>().get(trackerTopology);
    //const TrackerTopology* const tTopo = trackerTopology.product();    
    // input data
    edm::Handle<FastTrackerRecHitCombinationCollection> recHitCombinations;
    e.getByToken(recHitCombinationsToken, recHitCombinations);
    const std::vector<bool> * hitMasks = 0;
    if (!hitMasksToken.isUninitialized())
    {
        edm::Handle<std::vector<bool> > hitMasksHandle;
        e.getByToken(hitMasksToken,hitMasksHandle);
        hitMasks = &(*hitMasksHandle);
    }
    
    // output data
    std::unique_ptr<TrajectorySeedCollection> output(new TrajectorySeedCollection());

    // read the regions;
    edm::Handle<edm::OwnVector<TrackingRegion> > hregions;
    e.getByToken(trackingRegionToken, hregions);
    const auto& regions = *hregions;
    // and make sure there is at least one region
    if(regions.size() == 0)
    {
        e.put(std::move(output));
        return;
    }
    
    // instantiate the seed finder
    SeedFinder seedFinder(_seedingTree,*trackerTopology.product());
    if(seedFinderSelector)
    {
	seedFinderSelector->initEvent(e,es);
	seedFinder.addHitSelector(seedFinderSelector.get(),nHitsPerSeed_);
    }

    /*    unsigned  int layerNo=0;
    uint32_t subdet=0;
    std::string det;
    int z_pos=0;
    */
    // loop over the combinations
    for ( unsigned icomb=0; icomb<recHitCombinations->size(); ++icomb)
    {
        FastTrackerRecHitCombination recHitCombination = (*recHitCombinations)[icomb];

        // create a list of hits cleaned from masked hits
        std::vector<const FastTrackerRecHit * > seedHitCandidates;
        for (const auto & _hit : recHitCombination )
        {
            if(hitMasks && fastTrackingUtilities::hitIsMasked(_hit.get(),*hitMasks))
            {
                continue;
            }
	    //if(_hit.get()->geographicalId().subdetId() == PixelSubdetector::PixelBarrel){
	    /*	    layerNo=tTopo->layer(_hit.get()->geographicalId());
	    subdet=_hit.get()->geographicalId().subdetId();
	    if ( subdet == PixelSubdetector::PixelBarrel )
	      det="PXB";
	    if ( subdet == PixelSubdetector::PixelEndcap ){
	      z_pos=tTopo->side(_hit.get()->geographicalId());
	      det="PXD"; }
	    if ( subdet == StripSubdetector::TIB )
	      det="TIB";
	    if ( subdet == StripSubdetector::TID ){
	      z_pos=tTopo->side(_hit.get()->geographicalId());
	      det="TID"; }
	    if ( subdet == StripSubdetector::TOB )
	      det="TOB";
	    if ( subdet == StripSubdetector::TEC ){
	      z_pos=tTopo->side(_hit.get()->geographicalId());
	      det="TEC"; }

	    	    if(det=="PXD"||det=="PXB")
	      std::cout<<"DetLayer="<<det<<"\tLayerNo="<<layerNo<<"\tSide="<<z_pos<<std::endl;
	    }*/
	    seedHitCandidates.push_back(_hit.get());
        }

        // loop over the regions
        for(const auto& region: regions)
        {
            // set the region used in the selector
	    if(seedFinderSelector)
	    {
		seedFinderSelector->setTrackingRegion(&region);
	    }

            // find hits compatible with the seed requirements
            std::vector<unsigned int> seedHitNumbers = seedFinder.getSeed(seedHitCandidates);

            // create a seed from those hits
            if (seedHitNumbers.size()>1)
            {

                // copy the hits 
                edm::OwnVector<FastTrackerRecHit> seedHits;
                for(unsigned iIndex = 0;iIndex < seedHitNumbers.size();++iIndex)
                {
                    seedHits.push_back(seedHitCandidates[seedHitNumbers[iIndex]]->clone());
                }
		// make them aware of the combination they originate from
                fastTrackingUtilities::setRecHitCombinationIndex(seedHits,icomb);

		// create the seed
                seedCreator->init(region,es,0);
                seedCreator->makeSeed(
                    *output,
                    SeedingHitSet(
                        &seedHits[0],
                        &seedHits[1],
                        seedHits.size() >=3 ? &seedHits[2] : nullptr,
                        seedHits.size() >=4 ? &seedHits[3] : nullptr
			)
		    );
                break; // break the loop over the regions
            }
        }
    }
    e.put(std::move(output));

}

#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(TrajectorySeedProducer);
