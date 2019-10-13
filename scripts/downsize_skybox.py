#!/usr/bin/env python3

''' Script for downsizing skybox images. '''

import os
import math
import cv2
import numpy as np
from multiprocessing import Pool
from depth_to_skybox import camera_parameters


NUM_WORKER_PROCESSES = 20
DOWNSIZED_WIDTH = 512
DOWNSIZED_HEIGHT = 512

# Constants
SKYBOX_WIDTH = 1024
SKYBOX_HEIGHT = 1024
base_dir = 'data/v1/scans'
skybox_template = '%s/%s/matterport_skybox_images/%s_skybox%d_sami.jpg'
skybox_small_template = '%s/%s/matterport_skybox_images/%s_skybox%d_small.jpg'
skybox_merge_template = '%s/%s/matterport_skybox_images/%s_skybox_small.jpg'



def downsizeWithMerge(scan):
  # Load pano ids
  intrinsics,_ = camera_parameters(scan)
  pano_ids = list(set([item.split('_')[0] for item in intrinsics.keys()]))
  print('Processing scan %s with %d panoramas' % (scan, len(pano_ids)))

  for pano in pano_ids:

    ims = []
    for skybox_ix in range(6):

      # Load and downsize skybox image
      skybox = cv2.imread(skybox_template % (base_dir,scan,pano,skybox_ix))
      ims.append(cv2.resize(skybox,(DOWNSIZED_WIDTH,DOWNSIZED_HEIGHT),interpolation=cv2.INTER_AREA))

    # Save output
    newimg = np.concatenate(ims, axis=1)
    assert cv2.imwrite(skybox_merge_template % (base_dir,scan,pano), newimg)


def downsize(scan):

  # Load pano ids
  intrinsics,_ = camera_parameters(scan)
  pano_ids = list(set([item.split('_')[0] for item in intrinsics.keys()]))
  print('Processing scan %s with %d panoramas' % (scan, len(pano_ids)))

  for pano in pano_ids:

    for skybox_ix in range(6):

      # Load and downsize skybox image
      skybox = cv2.imread(skybox_template % (base_dir,scan,pano,skybox_ix))
      newimg = cv2.resize(skybox,(DOWNSIZED_WIDTH,DOWNSIZED_HEIGHT),interpolation=cv2.INTER_AREA)

      # Save output
      assert cv2.imwrite(skybox_small_template % (base_dir,scan,pano,skybox_ix), newimg)


if __name__ == '__main__':

  with open('connectivity/scans.txt') as f:
    scans = [scan.strip() for scan in f.readlines()]
    p = Pool(NUM_WORKER_PROCESSES)
    p.map(downsizeWithMerge, scans)
