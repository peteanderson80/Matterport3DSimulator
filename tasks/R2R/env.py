#!/usr/bin/env python

import sys
sys.path.append('build')
import MatterSim
import csv
import numpy as np
import math
import base64

csv.field_size_limit(sys.maxsize)


class EnvBatch():
    """ A simple wrapper for a batch of MatterSim environments, 
        using discretized viewpoints and pretrained features """

    def __init__(self, feature_store='img_features/ResNet-152-imagenet.tsv', batch_size=100):
        print 'Loading image features'
        tsv_fieldnames = ['scanId', 'viewpointId', 'image_w','image_h', 'vfov', 'features']
        self.features = {}
        with open(feature_store, "r+b") as tsv_in_file:
            reader = csv.DictReader(tsv_in_file, delimiter='\t', fieldnames = tsv_fieldnames)
            for item in reader:
                self.image_h = int(item['image_h'])
                self.image_w = int(item['image_w'])   
                self.vfov = int(item['vfov'])   
                long_id = self._make_id(item['scanId'], item['viewpointId'])
                self.features[long_id] = np.frombuffer(base64.decodestring(item['features']), 
                        dtype=np.float32).reshape((36, 2048))
        print 'Setting up simulators'
        self.sims = []
        for i in range(batch_size):
            sim = MatterSim.Simulator()
            sim.setRenderingEnabled(False)
            sim.setDiscretizedViewingAngles(True)
            sim.setCameraResolution(self.image_w, self.image_h)
            sim.setCameraVFOV(math.radians(self.vfov))
            sim.init()
            self.sims.append(sim)

    def _make_id(self, scanId, viewpointId):
        return scanId + '_' + viewpointId   

    def newEpisodes(self, scanIds, viewpointIds, headings):
        for i, (scanId, viewpointId, heading) in enumerate(zip(scanIds, viewpointIds, headings)):
            self.sims[i].newEpisode(scanId, viewpointId, heading, 0)
  
    def getStates(self):
        ''' Get list of states augmented with precomputed image features. rgb field will be empty. '''
        feature_states = []
        for sim in self.sims:
            state = sim.getState()
            long_id = self._make_id(state.scanId, state.location.viewpointId)
            feature = self.features[long_id][state.viewIndex,:]
            feature_states.append((feature, state))
        return feature_states

    def makeActions(self, indices, headings, elevations):
        ''' Take an action using the full state dependent action interface. '''
        for i, (index, heading, elevation) in enumerate(zip(indices, headings, elevations)):
            self.sims[i].makeAction(index, heading, elevation)

    def makeSimpleActions(self, simple_indices):
        ''' Take an action using a simple interface: 0-forward, 1-turn left, 2-turn right, 3-look up, 4-look down. 
            All viewpoint changes are 30 degrees. Forward, look up and look down may not succeed - check state.
            WARNING - Very likely this simple interface restricts some edges in the graph. Parts of the
            environment may not longer be navigable. '''
        for i, index in enumerate(simple_indices):
            if index == 0:
                self.sims[i].makeAction(1, 0, 0)
            elif index == 1:
                self.sims[i].makeAction(0,-1, 0)
            elif index == 2:
                self.sims[i].makeAction(0, 1, 0)
            elif index == 3:
                self.sims[i].makeAction(0, 0, 1)
            elif index == 4:
                self.sims[i].makeAction(0, 0,-1)
            else:
                sys.exit("Invalid simple action");     

    
if __name__ == "__main__":

    # Demo of EnvBatch
    sim = EnvBatch(batch_size=2)
    sim.newEpisodes(["2t7WUuJeko7", "17DRP5sb8fy"],   
                    ["cc34e9176bfe47ebb23c58c165203134", "5b9b2794954e4694a45fc424a8643081"], 
                    [0.2, 0.3])
    states = sim.getStates()
    feature,state = states[0]
    print feature
    sim.makeSimpleActions([0,1])
    feature,state = states[1]
    print feature      

        


