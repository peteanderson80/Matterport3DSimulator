#!/usr/bin/env python

''' Script to precompute image features using a Caffe ResNet CNN, using 36 discretized views
    at each viewpoint in 30 degree increments, and the provided camera WIDTH, HEIGHT 
    and VFOV parameters. '''

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
import caffe

from timer import Timer


TSV_FIELDNAMES = ['scanId', 'viewpointId', 'image_w','image_h', 'vfov', 'features']
VIEWPOINT_SIZE = 36 # Number of discretized views from one viewpoint
FEATURE_SIZE = 2048
BATCH_SIZE = 4  # Some fraction of viewpoint size - batch size 4 equals 11GB memory
GPU_ID = 0
PROTO = 'models/ResNet-152-deploy.prototxt'
MODEL = 'models/ResNet-152-model.caffemodel'  # You need to download this, see README.md
#MODEL = 'models/resnet152_places365.caffemodel'
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


def build_tsv():
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
    t_render = Timer()
    t_net = Timer()
    with open(OUTFILE, 'wb') as tsvfile:
        writer = csv.DictWriter(tsvfile, delimiter = '\t', fieldnames = TSV_FIELDNAMES)          

        # Loop all the viewpoints in the simulator
        viewpointIds = load_viewpointids()
        for scanId,viewpointId in viewpointIds:
            t_render.tic()
            # Loop all discretized views from this location
            blobs = []
            features = np.empty([VIEWPOINT_SIZE, FEATURE_SIZE], dtype=np.float32)
            for ix in range(VIEWPOINT_SIZE):
                if ix == 0:
                    sim.newEpisode(scanId, viewpointId, 0, math.radians(-30))
                elif ix % 12 == 0:
                    sim.makeAction(0, 1.0, 1.0)
                else:
                    sim.makeAction(0, 1.0, 0)

                state = sim.getState()
                assert state.viewIndex == ix
                
                # Transform and save generated image
                blobs.append(transform_img(state.rgb))

            t_render.toc()
            t_net.tic()
            # Run as many forward passes as necessary
            assert VIEWPOINT_SIZE % BATCH_SIZE == 0
            forward_passes = VIEWPOINT_SIZE / BATCH_SIZE            
            ix = 0
            for f in range(forward_passes):
                for n in range(BATCH_SIZE):
                    # Copy image blob to the net
                    net.blobs['data'].data[n, :, :, :] = blobs[ix]
                    ix += 1
                # Forward pass
                output = net.forward()
                features[f*BATCH_SIZE:(f+1)*BATCH_SIZE, :] = net.blobs['pool5'].data[:,:,0,0]

            writer.writerow({
                'scanId': scanId,
                'viewpointId': viewpointId,
                'image_w': WIDTH,
                'image_h': HEIGHT,
                'vfov' : VFOV,
                'features': base64.b64encode(features)
            })
            count += 1
            t_net.toc()
            if count % 100 == 0:
                print 'Processed %d / %d viewpoints, %.1fs avg render time, %.1fs avg net time, projected %.1f hours' %\
                  (count,len(viewpointIds), t_render.average_time, t_net.average_time, 
                  (t_render.average_time+t_net.average_time)*len(viewpointIds)/3600)


def read_tsv(infile):
    # Verify we can read a tsv
    in_data = []
    with open(infile, "r+b") as tsv_in_file:
        reader = csv.DictReader(tsv_in_file, delimiter='\t', fieldnames = TSV_FIELDNAMES)
        for item in reader:
            item['image_h'] = int(item['image_h'])
            item['image_w'] = int(item['image_w'])   
            item['vfov'] = int(item['vfov'])   
            item['features'] = np.frombuffer(base64.decodestring(item['features']), 
                    dtype=np.float32).reshape((VIEWPOINT_SIZE, FEATURE_SIZE))
            in_data.append(item)
    return in_data


if __name__ == "__main__":

    build_tsv()
    data = read_tsv(OUTFILE)
    print 'Completed %d viewpoints' % len(data)

