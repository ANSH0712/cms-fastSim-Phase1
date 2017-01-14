import FWCore.ParameterSet.Config as cms

# import the full tracking equivalent of this file
import RecoTracker.IterativeTracking.DetachedQuadStep_cff as _standard
from FastSimulation.Tracking.SeedingMigration import _hitSetProducerToFactoryPSet

# fast tracking mask producer
import FastSimulation.Tracking.FastTrackerRecHitMaskProducer_cfi
detachedQuadStepMasks = FastSimulation.Tracking.FastTrackerRecHitMaskProducer_cfi.maskProducerFromClusterRemover(_standard.detachedQuadStepClusters)

# tracking regions
detachedQuadStepTrackingRegions = _standard.detachedQuadStepTrackingRegions.clone()

# trajectory seeds
import FastSimulation.Tracking.TrajectorySeedProducer_cfi
detachedQuadStepSeeds = FastSimulation.Tracking.TrajectorySeedProducer_cfi.trajectorySeedProducer.clone(
    layerList = _standard.detachedQuadStepSeedLayers.layerList.value(),
    trackingRegions = "detachedQuadStepTrackingRegions",
    hitMasks = cms.InputTag("detachedQuadStepMasks")
)
detachedQuadStepSeeds.seedFinderSelector.pixelTripletGeneratorFactory = _hitSetProducerToFactoryPSet(_standard.detachedQuadStepHitTriplets)

# track candidates
import FastSimulation.Tracking.TrackCandidateProducer_cfi
detachedQuadStepTrackCandidates = FastSimulation.Tracking.TrackCandidateProducer_cfi.trackCandidateProducer.clone(
    src = cms.InputTag("detachedQuadStepSeeds"),
    MinNumberOfCrossedLayers = 3,
    hitMasks = cms.InputTag("detachedQuadStepMasks")
    )

# tracks 
detachedQuadStepTracks = _standard.detachedQuadStepTracks.clone(TTRHBuilder = 'WithoutRefit')

detachedQuadStepClassifier1 = _standard.detachedQuadStepClassifier1.clone()
detachedQuadStepClassifier1.vertices = "firstStepPrimaryVerticesBeforeMixing"
detachedQuadStepClassifier2 = _standard.detachedQuadStepClassifier2.clone()
detachedQuadStepClassifier2.vertices = "firstStepPrimaryVerticesBeforeMixing"

detachedQuadStep = _standard.detachedQuadStep.clone()

# Final sequence 
DetachedQuadStep = cms.Sequence(detachedQuadStepMasks
                                   +detachedQuadStepTrackingRegions
                                   +detachedQuadStepSeeds
                                   +detachedQuadStepTrackCandidates
                                   +detachedQuadStepTracks
                                   +detachedQuadStepClassifier1*detachedQuadStepClassifier2
                                   +detachedQuadStep
                                   )
