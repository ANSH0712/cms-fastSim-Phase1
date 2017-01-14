import FWCore.ParameterSet.Config as cms

# import the full tracking equivalent of this file
import RecoTracker.IterativeTracking.HighPtTripletStep_cff as _standard
from FastSimulation.Tracking.SeedingMigration import _hitSetProducerToFactoryPSet

# fast tracking mask producer
import FastSimulation.Tracking.FastTrackerRecHitMaskProducer_cfi
highPtTripletStepMasks = FastSimulation.Tracking.FastTrackerRecHitMaskProducer_cfi.maskProducerFromClusterRemover(_standard.highPtTripletStepClusters)

# tracking regions
highPtTripletStepTrackingRegions = _standard.highPtTripletStepTrackingRegions.clone()

# trajectory seeds
import FastSimulation.Tracking.TrajectorySeedProducer_cfi
highPtTripletStepSeeds = FastSimulation.Tracking.TrajectorySeedProducer_cfi.trajectorySeedProducer.clone(
    layerList = _standard.highPtTripletStepSeedLayers.layerList.value(),
    trackingRegions = "highPtTripletStepTrackingRegions",
    hitMasks = cms.InputTag("highPtTripletStepMasks"),
)
highPtTripletStepSeeds.seedFinderSelector.pixelTripletGeneratorFactory = _hitSetProducerToFactoryPSet(_standard.highPtTripletStepHitTriplets)
#highPtTripletStepSeeds.pixelTripletGeneratorFactory.SeedComparitorPSet=cms.PSet(  ComponentName = cms.string( "none" ) )

# track candidates
import FastSimulation.Tracking.TrackCandidateProducer_cfi
highPtTripletStepTrackCandidates = FastSimulation.Tracking.TrackCandidateProducer_cfi.trackCandidateProducer.clone(
    src = cms.InputTag("highPtTripletStepSeeds"),
    MinNumberOfCrossedLayers = 3,
    hitMasks = cms.InputTag("highPtTripletStepMasks"),
)

# tracks
highPtTripletStepTracks = _standard.highPtTripletStepTracks.clone(TTRHBuilder = 'WithoutRefit')

# final selection
highPtTripletStep = _standard.highPtTripletStep.clone()
highPtTripletStep.vertices = "firstStepPrimaryVerticesBeforeMixing"

# Final swquence 
HighPtTripletStep = cms.Sequence(highPtTripletStepMasks
                                +highPtTripletStepTrackingRegions
                                +highPtTripletStepSeeds
                                +highPtTripletStepTrackCandidates
                                +highPtTripletStepTracks  
                                +highPtTripletStep   
                                )
