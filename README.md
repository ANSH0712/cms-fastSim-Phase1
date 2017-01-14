# How to install

```
cmsrel CMSSW_9_0_0_pre1
cd CMSSW_9_0_0_pre1/src
cmsenv
git clone git@github.com:ANSH0712/cms-fastSim-Phase1.git FastSimulation
USER_CXXFLAGS="-g -D=EDM_ML_DEBUG" scram b -j 8 # special flags to switch on debugging code
```

# How to run

```
# create a file with generated events
source FastSimulation/FastSimProducer/test/gen.sh
# pass the generated events to simulation
cmsRun FastSimulation/FastSimProducer/python/conf_cfg.py
# to run validation do instead
cmsRun FastSimulation/FastSimProducer/python/conf_validation_cfg.py
```

# More info on the project

See comments in FastSimulation/Propagation/src/LayerNavigator.cc

# How to push and pull changes

```
git pull 
git push
```
