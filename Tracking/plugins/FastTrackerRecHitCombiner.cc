// system include files
#include <memory>
#include <TFile.h>
#include <TROOT.h>
#include <TH2.h>
#include <iostream>
#include <fstream>
// frame work stuff
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/StreamID.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

// specific for this producer
#include "SimDataFormats/TrackingHit/interface/PSimHitContainer.h"
#include "SimDataFormats/Track/interface/SimTrackContainer.h"
#include "DataFormats/TrackerRecHit2D/interface/FastTrackerRecHitCollection.h"
#include "FastSimulation/Tracking/interface/FastTrackerRecHitSplitter.h"
#include "DataFormats/TrackerCommon/interface/TrackerTopology.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/Records/interface/TrackerTopologyRcd.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "Geometry/CommonDetUnit/interface/GeomDet.h"
#include "DataFormats/GeometryVector/interface/GlobalPoint.h"
#include "DataFormats/GeometryVector/interface/LocalPoint.h"
#include "DataFormats/DetId/interface/DetId.h"

class FastTrackerRecHitCombiner : public edm::stream::EDProducer<> {
    public:

    explicit FastTrackerRecHitCombiner(const edm::ParameterSet&);
    ~FastTrackerRecHitCombiner(){;}

    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:

  virtual void beginStream(edm::StreamID) override{;}
  virtual void produce(edm::Event&, const edm::EventSetup&) override;
  virtual void endStream() override{;}
  edm::Service<TFileService> FileService;
  TH2F* hitsZPerp;
  // ----------member data ---------------------------
  edm::EDGetTokenT<edm::PSimHitContainer> simHitsToken; 
  edm::EDGetTokenT<FastTrackerRecHitRefCollection> simHit2RecHitMapToken;
  edm::ESHandle<TrackerGeometry> _trackerGeometry;
  //  iSetup.get<TrackerDigiGeometryRecord>().get(_trackerGeometry);
  edm::ESHandle<TrackerTopology> _trackerTopology;
    unsigned int minNHits;
  //  edm::EDGetTokenT<FastTrackerRecHitCollection>  recHits_;
};


FastTrackerRecHitCombiner::FastTrackerRecHitCombiner(const edm::ParameterSet& iConfig)
    : simHitsToken(consumes<edm::PSimHitContainer>(iConfig.getParameter<edm::InputTag>("simHits")))
    , simHit2RecHitMapToken(consumes<FastTrackerRecHitRefCollection>(iConfig.getParameter<edm::InputTag>("simHit2RecHitMap")))
    , minNHits(iConfig.getParameter<unsigned int>("minNHits"))
      //, recHits_(consumes<FastTrackerRecHitCollection>(iConfig.getParameter<edm::InputTag>("recHits")))
{
  edm::Service<TFileService> fs;
  hitsZPerp = fs->make<TH2F>("simhitsZPerp","",1280,-320,320,520,0,130);
  
    produces<FastTrackerRecHitCombinationCollection>();
}
/*void FastTrackerRecHitCombiner::beginstream()
{
  _hitsZPerp = _fs->make<TH2F>("hitsZPerp","",1280,-320,320,520,0,130);
  }*/
void FastTrackerRecHitCombiner::produce(edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  // input
  edm::Handle<edm::PSimHitContainer> simHits;
  iEvent.getByToken(simHitsToken,simHits);
  //edm::Handle<FastTrackerRecHitCollection> recHits;
  //iEvent.getByToken(recHits_,recHits);
  iSetup.get<TrackerDigiGeometryRecord>().get(_trackerGeometry);  
  // services                                                                                                                            
  /*edm::ESHandle<TrackerTopology> trackerTopology;
  iSetup.get<TrackerTopologyRcd>().get(trackerTopology);
  const TrackerTopology* const tTopo = trackerTopology.product();
  */
  edm::Handle<FastTrackerRecHitRefCollection> simHit2RecHitMap;
  iEvent.getByToken(simHit2RecHitMapToken,simHit2RecHitMap);
  
  // output
  std::unique_ptr<FastTrackerRecHitCombinationCollection> output(new FastTrackerRecHitCombinationCollection);
  /*unsigned  int simhitlayer=0;
  uint32_t simhitsubdet=0;
  std::string simhitdet;
  int simhitz_pos=0;
  unsigned  int rechitlayer=0;
  uint32_t rechitsubdet=0;
  std::string rechitdet;
  int rechitz_pos=0;
  */
  /*  if(!(simHits->size()==recHits->size()))
    throw cms::Exception("FastSimulation/Tracking","#simHits is not equal to #rechits");
  */ 
  FastTrackerRecHitCombination currentCombination;
  for(unsigned int simHitCounter = 0;simHitCounter < simHits->size();simHitCounter++){
    
    // get simHit and recHit
    const PSimHit & simHit = (*simHits)[simHitCounter];
    const FastTrackerRecHitRef & recHit = (*simHit2RecHitMap)[simHitCounter];
    //    const FastTrackerRecHit & rhit = static_cast<const FastTrackerRecHit &>(simHitCounter);

    // r vs z histogram
    //    std::cout<<"Making hist"<<std::endl;
    const LocalPoint& localPoint = simHit.localPosition();
    //std::cout<<"Found localpos"<<std::endl;
    const DetId& theDetId = simHit.detUnitId();
    //std::cout<<"Found det ID"<<std::endl;
    const GeomDet* theGeomDet = _trackerGeometry->idToDet(theDetId);
    //std::cout<<"Found pointer to Geom det from ID"<<std::endl;
    const GlobalPoint& globalPoint = theGeomDet->toGlobal(localPoint);
    //std::cout<<"Found global pos"<<std::endl;
    hitsZPerp->Fill(globalPoint.z(),std::sqrt(globalPoint.x()*globalPoint.x()+globalPoint.y()*globalPoint.y()));

    /*    simhitlayer=tTopo->layer(simHit.detUnitId());
    simhitsubdet=DetId(simHit.detUnitId()).subdetId();
    if ( simhitsubdet == PixelSubdetector::PixelBarrel )
      simhitdet="PXB";
    if ( simhitsubdet == PixelSubdetector::PixelEndcap ){
      simhitz_pos=tTopo->side(simHit.detUnitId());
      simhitdet="PXD"; }
    if ( simhitsubdet == StripSubdetector::TIB )
      simhitdet="TIB";
    if ( simhitsubdet == StripSubdetector::TID ){
      simhitz_pos=tTopo->side(simHit.detUnitId());
      simhitdet="TID"; }
    if ( simhitsubdet == StripSubdetector::TOB )
      simhitdet="TOB";
    if ( simhitsubdet == StripSubdetector::TEC ){
      simhitz_pos=tTopo->side(simHit.detUnitId());
      simhitdet="TEC"; }
    
    if(simhitdet=="PXD"||simhitdet=="PXB")
      std::cout<<"SimHit position:\nDetLayer="<<simhitdet<<"\tLayerNo="<<simhitlayer<<"\tSide="<<simhitz_pos<<std::endl;
    */
    /* rechitlayer=tTopo->layer(rhit.id(simHitCounter));
    rechitsubdet=DetId(rhit.id(simHitCounter)).subdetId();
    if ( rechitsubdet == PixelSubdetector::PixelBarrel )
      rechitdet="PXB";
    if ( rechitsubdet == PixelSubdetector::PixelEndcap ){
      rechitz_pos=tTopo->side(rhit.id(simHitCounter));
      rechitdet="PXD"; }
    if ( rechitsubdet == StripSubdetector::TIB )
      rechitdet="TIB";
    if ( rechitsubdet == StripSubdetector::TID ){
      rechitz_pos=tTopo->side(rhit.id(simHitCounter));
      rechitdet="TID"; }
    if ( rechitsubdet == StripSubdetector::TOB )
      rechitdet="TOB";
    if ( rechitsubdet == StripSubdetector::TEC ){
      rechitz_pos=tTopo->side(rhit.id(simHitCounter));
      rechitdet="TEC"; }
    
    if(rechitdet=="PXD"|| rechitdet=="PXB")
      std::cout<<"RecHit position:\nDetLayer="<<rechitdet<<"\tLayerNo="<<rechitlayer<<"\tSide="<<rechitz_pos<<std::endl;
   */ 
    //const FastTrackerRecHitRef & recHit = (*simHit2RecHitMap)[simHitCounter];
    
    // add recHit to latest combination
    if(!recHit.isNull())
      currentCombination.push_back(recHit);
    
    // if simTrackId is about to change, add combination
    if(simHits->size()-simHitCounter == 1 || simHit.trackId() != (*simHits)[simHitCounter+1].trackId() ){
      // combination must have sufficient hits
      if(currentCombination.size() >= minNHits){
	currentCombination.shrink_to_fit();
	output->push_back(currentCombination);
      }
      currentCombination.clear();
    }
  }
  
  /*  for (unsigned int hitIt = 0 ;  hitIt != recHits->size(); ++hitIt) {
    //    if(!(*hitIt)->isValid())
    //continue;
    const FastTrackerRecHit & rhit = static_cast<const FastTrackerRecHit &>(*(*hitIt));
    rechitlayer=tTopo->layer(rhit.id());
    rechitsubdet=DetId(rhit.id()).subdetId();
    if ( rechitsubdet == PixelSubdetector::PixelBarrel )
      rechitdet="PXB";
    if ( rechitsubdet == PixelSubdetector::PixelEndcap ){
      rechitz_pos=tTopo->side(rhit.id());
      rechitdet="PXD"; }
    if ( rechitsubdet == StripSubdetector::TIB )
      rechitdet="TIB";
    if ( rechitsubdet == StripSubdetector::TID ){
      rechitz_pos=tTopo->side(rhit.id());
      rechitdet="TID"; }
    if ( rechitsubdet == StripSubdetector::TOB )
      rechitdet="TOB";
    if ( rechitsubdet == StripSubdetector::TEC ){
      rechitz_pos=tTopo->side(rhit.id());
      rechitdet="TEC"; }

    if(rechitdet=="PXD"|| rechitdet=="PXB")
      std::cout<<"RecHit position:\nDetLayer="<<rechitdet<<"\tLayerNo="<<rechitlayer<<"\tSide="<<rechitz_pos<<std::endl;
  }
  */   
  // put output in event
  iEvent.put(std::move(output));
  
}
  

       
// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void FastTrackerRecHitCombiner::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    //The following says we do not know what parameters are allowed so do no validation
    // Please change this to state exactly what you do use, even if it is no parameters
    edm::ParameterSetDescription desc;
    desc.setUnknown();
    descriptions.addDefault(desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(FastTrackerRecHitCombiner);
