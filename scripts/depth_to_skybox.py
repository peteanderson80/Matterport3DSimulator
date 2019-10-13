#!/usr/bin/env python3

''' Script for generating depth skyboxes based on undistorted depth images,
    in order to support depth output in the simulator. The current version
    assumes that undistorted depth images are aligned to matterport skyboxes,
    and uses simple blending. Images are downsized 50%. '''

import os
import math
import cv2
import numpy as np
from multiprocessing import Pool
from numpy.linalg import inv,norm
from io import StringIO


# Parameters
DOWNSIZED_WIDTH = 512
DOWNSIZED_HEIGHT = 512
NUM_WORKER_PROCESSES = 20
FILL_HOLES = True
VISUALIZE_OUTPUT = False

if FILL_HOLES:
  from MatterSim import cbf

# Constants
# Note: Matterport camera is really y=up, x=right, -z=look.
SKYBOX_WIDTH = 1024
SKYBOX_HEIGHT = 1024
base_dir = 'data/v1/scans'
skybox_template = '%s/%s/matterport_skybox_images/%s_skybox%d_sami.jpg'
color_template = '%s/%s/undistorted_color_images/%s_i%s.jpg'
depth_template = '%s/%s/undistorted_depth_images/%s_d%s.png'
camera_template = '%s/%s/undistorted_camera_parameters/%s.conf'
skybox_depth_template = '%s/%s/matterport_skybox_images/%s_skybox_depth_small.png'


# camera transform for skybox images 0-5 relative to image 1
skybox_transforms = [
  np.array([[1,0,0],[0,0,-1],[0,1,0]], dtype=np.double), #up (down)
  np.eye(3, dtype=np.double),
  np.array([[0,0,-1],[0,1,0],[1,0,0]], dtype=np.double), # right
  np.array([[-1,0,0],[0,1,0],[0,0,-1]], dtype=np.double), # 180
  np.array([[0,0,1],[0,1,0],[-1,0,0]], dtype=np.double), # left
  np.array([[1,0,0],[0,0,1],[0,-1,0]], dtype=np.double) # down (up)
]


def camera_parameters(scan):
  ''' Returns two dicts containing undistorted camera intrinsics (3x3) and extrinsics (4x4),
      respectively, for a given scan. Viewpoint IDs are used as dict keys. '''
  intrinsics = {}
  extrinsics = {}
  with open(camera_template % (base_dir,scan,scan)) as f:
    pos = -1
    for line in f.readlines():
      if 'intrinsics_matrix' in line:
        intr = line.split()
        C = np.zeros((3, 3), np.double)
        C[0,0] = intr[1] # fx
        C[1,1] = intr[5] # fy
        C[0,2] = intr[3] # cx
        C[1,2] = intr[6] # cy
        C[2,2] = 1.0
        pos = 0
      elif pos >= 0 and pos < 6:
        q = line.find('.jpg')
        camera = line[q-37:q]
        if pos == 0:
          intrinsics[camera[:-2]] = C
        T = np.loadtxt(StringIO(line.split('jpg ')[1])).reshape((4,4))
        # T is camera-to-world transform, invert for world-to-camera
        extrinsics[camera] = (T,inv(T))
        pos += 1
  return intrinsics,extrinsics


def z_to_euclid(K_inv, depth):
  ''' Takes inverse intrinsics matrix and a depth image. Returns a new depth image with
      depth converted from z-distance into euclidean distance from the camera centre. '''

  assert len(depth.shape) == 2
  h = depth.shape[0]
  w = depth.shape[1]

  y,x = np.indices((h,w))
  homo_pixels = np.vstack((x.flatten(),y.flatten(),np.ones((x.size))))
  rays = K_inv.dot(homo_pixels)
  cos_theta = np.array([0,0,1]).dot(rays) / norm(rays,axis=0)

  output = depth / cos_theta.reshape(h,w)
  return output


def instrinsic_matrix(width, height):
  ''' Construct an ideal camera intrinsic matrix. '''
  K = np.zeros((3, 3), np.double)
  K[0,0] = width/2 #fx
  K[1,1] = height/2 #fy
  K[0,2] = width/2 #cx
  K[1,2] = height/2 #cy
  K[2,2] = 1.0
  return K



def fill_joint_bilateral_filter(rgb, depth):
  ''' Fill holes in a 16bit depth image given corresponding rgb image '''

  intensity = cv2.cvtColor(rgb, cv2.COLOR_BGR2GRAY)

  # Convert the depth image to uint8.
  maxDepth = np.max(depth)+1
  depth = (depth.astype(np.float64)/maxDepth)
  depth[depth > 1] = 1
  depth = (depth*255).astype(np.uint8)

  # Convert to col major order
  depth = np.asfortranarray(depth)
  intensity = np.asfortranarray(intensity)
  mask = (depth == 0)
  result = np.zeros_like(depth)

  # Fill holes
  cbf(depth, intensity, mask, result)
  result = (result.astype(np.float64)/255*maxDepth).astype(np.uint16)
  return result


def depth_to_skybox(scan, visualize=VISUALIZE_OUTPUT, fill_holes=FILL_HOLES):

  # Load camera parameters
  intrinsics,extrinsics = camera_parameters(scan)
  # Skybox camera intrinsics
  K_skybox = instrinsic_matrix(SKYBOX_WIDTH, SKYBOX_HEIGHT)

  pano_ids = list(set([item.split('_')[0] for item in intrinsics.keys()]))
  print('Processing scan %s with %d panoramas' % (scan, len(pano_ids)))

  if visualize:
    cv2.namedWindow('RGB')
    cv2.namedWindow('Depth')
    cv2.namedWindow('Skybox')

  for pano in pano_ids:

    # Load undistorted depth and rgb images
    depth = {}
    rgb = {}
    for c in range(3):
      K_inv = inv(intrinsics['%s_i%d' % (pano,c)])
      for i in range(6):
        name = '%d_%d' % (c,i)
        if visualize:
          rgb[name] = cv2.imread(color_template % (base_dir,scan,pano,name))
        # Load 16bit grayscale image
        d_im = cv2.imread(depth_template % (base_dir,scan,pano,name), cv2.IMREAD_ANYDEPTH)
        depth[name] = z_to_euclid(K_inv, d_im)

    ims = []
    for skybox_ix in range(6):

      # Load skybox image
      skybox = cv2.imread(skybox_template % (base_dir,scan,pano,skybox_ix))

      # Skybox index 1 is the same orientation as camera image 1_5
      skybox_ctw,_ = extrinsics[pano + '_i1_5']
      skybox_ctw = skybox_ctw[:3,:3].dot(skybox_transforms[skybox_ix])
      skybox_wtc = inv(skybox_ctw)

      base_depth = np.zeros((SKYBOX_HEIGHT,SKYBOX_WIDTH), np.uint16)
      if visualize:
        base_rgb = np.zeros((SKYBOX_HEIGHT,SKYBOX_WIDTH,3), np.uint8)

      for camera in range(3):
        for angle in range(6):

          # Camera parameters
          im_name = '%d_%d' % (camera,angle)
          K_im = intrinsics[pano + '_i' + im_name[0]]
          T_ctw,T_wtc = extrinsics[pano + '_i' + im_name]
          R_ctw = T_ctw[:3,:3]

          # Check if this image can be skipped (facing away)
          z = np.array([0,0,1])
          if R_ctw.dot(z).dot(skybox_ctw.dot(z)) < 0:
            continue

          # Compute homography
          H = K_skybox.dot(skybox_wtc.dot(R_ctw.dot(inv(K_im))))

          # Warp and blend the depth image
          flip = cv2.flip(depth[im_name], 1) # flip around y-axis
          warp = cv2.warpPerspective(flip, H, (SKYBOX_HEIGHT,SKYBOX_WIDTH), flags=cv2.INTER_NEAREST)
          mask = cv2.warpPerspective(np.ones_like(flip), H, (SKYBOX_HEIGHT,SKYBOX_WIDTH), flags=cv2.INTER_LINEAR)
          mask[warp == 0] = 0 # Set mask to zero where we don't have any depth values
          mask = cv2.erode(mask,np.ones((3,3),np.uint8),iterations = 1)
          locs = np.where(mask == 1)
          base_depth[locs[0], locs[1]] = warp[locs[0], locs[1]]

          if visualize:
            # Warp and blend the rgb image
            flip = cv2.flip(rgb[im_name], 1) # flip around y-axis
            warp = cv2.warpPerspective(flip, H, (SKYBOX_HEIGHT,SKYBOX_WIDTH), flags=cv2.INTER_LINEAR)
            mask = cv2.warpPerspective(np.ones_like(flip), H, (SKYBOX_HEIGHT,SKYBOX_WIDTH), flags=cv2.INTER_LINEAR)
            mask = cv2.erode(mask,np.ones((3,3),np.uint8),iterations = 1)
            locs = np.where(mask == 1)
            base_rgb[locs[0], locs[1]] = warp[locs[0], locs[1]]

      depth_small = cv2.resize(cv2.flip(base_depth, 1),(DOWNSIZED_WIDTH,DOWNSIZED_HEIGHT),interpolation=cv2.INTER_NEAREST) # flip around y-axis, downsize
      if fill_holes:
        depth_filled = fill_joint_bilateral_filter(skybox, depth_small) # Fill holes
        ims.append(depth_filled)
      else:
        ims.append(depth_small)

      if visualize and False:
        cv2.imshow('Skybox', skybox)
        cv2.imshow('Depth', cv2.applyColorMap((depth_small/256).astype(np.uint8), cv2.COLORMAP_JET))
        rgb_output = cv2.flip(base_rgb, 1) # flip around y-axis
        cv2.imshow('RGB', rgb_output)
        cv2.waitKey(0)

    newimg = np.concatenate(ims, axis=1)

    if visualize:
      maxDepth = np.max(newimg)+1
      newimg = (newimg.astype(np.float64)/maxDepth)
      newimg = (newimg*255).astype(np.uint8)
      cv2.imshow('Depth pano', cv2.applyColorMap(newimg, cv2.COLORMAP_JET))
      cv2.waitKey(0)
    else:
      # Save output
      outfile = skybox_depth_template % (base_dir,scan,pano)
      assert cv2.imwrite(outfile, newimg), ('Could not write to %s' % outfile)

  if visualize:
    cv2.destroyAllWindows()
  print('Completed scan %s' % (scan))



if __name__ == '__main__':

  with open('connectivity/scans.txt') as f:
    scans = [scan.strip() for scan in f.readlines()]
    p = Pool(NUM_WORKER_PROCESSES)
    p.map(depth_to_skybox, scans)
