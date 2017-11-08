#!/usr/bin/env python


import numpy as np
import cv2
import json
import math
import base64
import csv
import sys

csv.field_size_limit(sys.maxsize)


# Caffe and MatterSim need to be on the Python path
sys.path.insert(0, 'build')
import MatterSim

#caffe_root = '../'  # your caffe build
#sys.path.insert(0, caffe_root + 'python')
#import caffe


TSV_FIELDNAMES = ['scanId', 'viewpointId', 'image_w','image_h', 'vfov', 'features']
BATCH_SIZE = 36             # All the discretized views from one viewpoint
GPU_ID = 0
PROTO = 'models/ResNet-152-deploy.prototxt'
MODEL = ''                  # You need to download this, see README.md
OUTFILE = 'img_features/ResNet-152-imagenet.tsv'
GRAPHS = 'connectivity/'

# Simulator image parameters
WIDTH=640
HEIGHT=480
VFOV=60


def load_viewpointids():
    viewpointIds = []
    with open(GRAPHS+'scans.txt') as f:
        scans = [scan.strip() for scan in f.readlines()]
        for scan in scans:
            with open(GRAPHS+scan+'_connectivity.json')  as j:
                data = json.load(j)
                for item in data:
                    if item['included']:
                        viewpointIds.append((scan, item['image_id']))
    print 'Loaded %d viewpoints' % len(viewpointIds)
    return viewpointIds


def transform_img(im):
    ''' Prep opencv 3 channel image for the network '''
    im_orig = im.astype(np.float32, copy=True)
    im_orig -= np.array([[[103.1, 115.9, 123.2]]]) # BGR pixel mean
    blob = np.zeros((1, im.shape[0], im.shape[1], 3), dtype=np.float32)
    blob[0, :, :, :] = im_orig
    blob = blob.transpose((0, 3, 1, 2))
    return blob


if __name__ == "__main__":

    # Set up the simulator
    sim = MatterSim.Simulator()
    sim.setCameraResolution(WIDTH, HEIGHT)
    sim.setCameraVFOV(math.radians(VFOV))
    sim.setDiscretizedViewingAngles(True)
    sim.init()

    # Set up Caffe resnet
    caffe.set_device(GPU_ID)
    caffe.set_mode_gpu()
    net = caffe.Net(PROTO, MODEL, caffe.TEST)
    net.blobs['data'].reshape(BATCH_SIZE, 3, HEIGHT, WIDTH)

    count = 0
    with open(OUTFILE, 'wb') as tsvfile:
        writer = csv.DictWriter(tsvfile, delimiter = '\t', fieldnames = TSV_FIELDNAMES)          

        # Loop all the viewpoints in the simulator
        viewpointIds = load_viewpointids()
        for scanId,viewpointId in viewpointIds:
            # Loop all discretized views from this location
            for ix in range(BATCH_SIZE):
                if ix == 0:
                    sim.newEpisode(scanId, viewpointId, 0, math.radians(-30))
                elif ix % 12 == 0:
                    sim.makeAction(0, 1.0, 1.0)
                else:
                    sim.makeAction(0, 1.0, 0)

                state = sim.getState()
                assert state.viewIndex == ix

                # Copy generated image to the net
                blob = transform_img(state.rgb)
                net.blobs['data'].data[ix, :, :, :] = blob

            # Forward pass
            output = net.forward()
            pool5 = net.blobs['pool5'].data
            writer.writerow({
                'scanId': scanId,
                'viewpointId': viewpointId,
                'image_w': WIDTH,
                'image_h': HEIGHT,
                'vfov' : VFOV,
                'features': base64.b64encode(pool5)
            })
            count += 1
            if count % 100 == 0:
                print 'Processed %d / %d viewpoints' % (count,len(viewpointIds))



