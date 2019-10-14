
import MatterSim
import time
import math
import cv2
import numpy as np

WIDTH = 800
HEIGHT = 600
VFOV = math.radians(60)
HFOV = VFOV*WIDTH/HEIGHT
TEXT_COLOR = [230, 40, 40]

cv2.namedWindow('Python RGB')
cv2.namedWindow('Python Depth')

sim = MatterSim.Simulator()
sim.setCameraResolution(WIDTH, HEIGHT)
sim.setCameraVFOV(VFOV)
sim.setDepthEnabled(False) # Turn on depth only after running ./scripts/depth_to_skybox.py (see README.md)
sim.initialize()
#sim.newEpisode(['2t7WUuJeko7'], ['1e6b606b44df4a6086c0f97e826d4d15'], [0], [0])
#sim.newEpisode(['1LXtFkjw3qL'], ['0b22fa63d0f54a529c525afbf2e8bb25'], [0], [0])
sim.newRandomEpisode(['1LXtFkjw3qL'])

heading = 0
elevation = 0
location = 0
ANGLEDELTA = 5 * math.pi / 180

print('\nPython Demo')
print('Use arrow keys to move the camera.')
print('Use number keys (not numpad) to move to nearby viewpoints indicated in the RGB view.')
print('Depth outputs are turned off by default - check driver.py:L20 to enable.\n')

while True:
    sim.makeAction([location], [heading], [elevation])
    location = 0
    heading = 0
    elevation = 0

    state = sim.getState()[0]
    locations = state.navigableLocations
    rgb = np.array(state.rgb, copy=False)
    for idx, loc in enumerate(locations[1:]):
        # Draw actions on the screen
        fontScale = 3.0/loc.rel_distance
        x = int(WIDTH/2 + loc.rel_heading/HFOV*WIDTH)
        y = int(HEIGHT/2 - loc.rel_elevation/VFOV*HEIGHT)
        cv2.putText(rgb, str(idx + 1), (x, y), cv2.FONT_HERSHEY_SIMPLEX, 
            fontScale, TEXT_COLOR, thickness=3)
    cv2.imshow('Python RGB', rgb)

    depth = np.array(state.depth, copy=False)
    cv2.imshow('Python Depth', depth)
    k = cv2.waitKey(1)
    if k == -1:
        continue
    else:
        k = (k & 255)
    if k == ord('q'):
        break
    elif ord('1') <= k <= ord('9'):
        location = k - ord('0')
        if location >= len(locations):
            location = 0
    elif k == 81 or k == ord('a'):
        heading = -ANGLEDELTA
    elif k == 82 or k == ord('w'):
        elevation = ANGLEDELTA
    elif k == 83 or k == ord('d'):
        heading = ANGLEDELTA
    elif k == 84 or k == ord('s'):
        elevation = -ANGLEDELTA
