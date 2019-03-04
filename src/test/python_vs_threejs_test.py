import sys
sys.path.append('build')

from MatterSim import Simulator
import math
import cv2
import json
import numpy as np

from os import listdir
from os.path import isfile, join

# Download some images using web/app/trajectory.html, then recreate identical images with this script


sim = Simulator()
sim.setCameraResolution(1140, 650)
sim.setCameraVFOV(math.radians(80))
sim.setElevationLimits(math.radians(-40),math.radians(50))
sim.initialize()


download_path = '.'
threejs_files = [f for f in listdir(download_path) if 'threejs' in f and isfile(join(download_path, f))]

for f in threejs_files:
    print f
    s = f.split("_")
    scanId = s[1]
    viewpointId = s[2]
    heading = float(s[3])
    elevation = float(s[4].replace('.png',''))
    sim.newEpisode([scanId], [viewpointId], [heading], [elevation])
    state = sim.getState()
    pyim = np.array(state[0].rgb, copy=False)
    cv2.imwrite(f.replace('threejs','python-1'), pyim);
    jsim = cv2.imread(f)
    im = cv2.addWeighted(jsim, 0.5, pyim, 0.5, 0) 

    cv2.imshow('ThreeJS', jsim)
    cv2.imshow('Python', pyim)
    cv2.imshow('Blend', im)
    cv2.waitKey(0)


