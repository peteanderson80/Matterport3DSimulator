#!/usr/bin/env python3

''' Script for filling missing values in undistorted depth images. '''

import os
import math
import cv2
import numpy as np
from multiprocessing import Pool
from depth_to_skybox import camera_parameters

import sys
sys.path.append('build')
from MatterSim import cbf


base_dir = 'data/v1/scans'
color_template = '%s/%s/undistorted_color_images/%s_i%s.jpg'
depth_template = '%s/%s/undistorted_depth_images/%s_d%s.png'
filled_depth_template = '%s/%s/undistorted_depth_images/%s_d%s_filled.png'

def fill_joint_bilateral_filter(scan):

  # Load camera parameters
  intrinsics,_ = camera_parameters(scan)
  pano_ids = list(set([item.split('_')[0] for item in intrinsics.keys()]))
  print('Processing scan %s with %d panoramas' % (scan, len(pano_ids)))

  for pano in pano_ids:

    # Load undistorted depth and rgb images
    for c in range(3):
      for i in range(6):
        name = '%d_%d' % (c,i)
        rgb = cv2.imread(color_template % (base_dir,scan,pano,name))
        intensity = cv2.cvtColor(rgb, cv2.COLOR_BGR2GRAY)

        # Load 16bit depth image
        depth = cv2.imread(depth_template % (base_dir,scan,pano,name), cv2.IMREAD_ANYDEPTH)

        # Convert the depth image to uint8.
        maxDepth = np.max(depth)+1
        depth = (depth.astype(np.float64)/maxDepth)
        depth[depth > 1] = 1
        depth = (depth*255).astype(np.uint8)

        #cv2.imshow('input', cv2.applyColorMap(depth, cv2.COLORMAP_JET))

        # Convert to col major order
        depth = np.asfortranarray(depth)
        intensity = np.asfortranarray(intensity)
        mask = (depth == 0)
        result = np.zeros_like(depth)

        # Fill holes
        cbf(depth, intensity, mask, result)

        #cv2.imshow('result', cv2.applyColorMap(result, cv2.COLORMAP_JET))
        #cv2.waitKey(0)

        result = (result.astype(np.float64)/255*maxDepth).astype(np.uint16)
        assert cv2.imwrite(filled_depth_template % (base_dir,scan,pano,name), result)


if __name__ == '__main__':

  with open('connectivity/scans.txt') as f:
    scans = [scan.strip() for scan in f.readlines()]
    p = Pool(10)
    p.map(fill_joint_bilateral_filter, scans)
