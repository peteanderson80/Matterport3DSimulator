import sys
sys.path.append('build')
import MatterSim
import time
import math
import cv2


WIDTH = 640
HEIGHT = 480
VFOV = math.radians(60)
HFOV = VFOV*WIDTH/HEIGHT
TEXT_COLOR = [230, 40, 40]

cv2.namedWindow('displaywin')
sim = MatterSim.Simulator()
sim.setCameraResolution(WIDTH, HEIGHT)
sim.setCameraVFOV(VFOV)
sim.init()
sim.newEpisode('2t7WUuJeko7', '1e6b606b44df4a6086c0f97e826d4d15', 0, 0)


heading = 0
elevation = 0
location = 0
ANGLEDELTA = 5 * math.pi / 180
while True:
    sim.makeAction(location, heading, elevation)
    location = 0
    heading = 0
    elevation = 0
    state = sim.getState()
    locations = state.navigableLocations
    im = state.rgb
    origin = locations[0].point
    for idx, loc in enumerate(locations[1:]):
        # Draw actions on the screen
        fontScale = 3.0/loc.rel_distance
        x = int(WIDTH/2 + loc.rel_heading/HFOV*WIDTH)
        y = int(HEIGHT/2 - loc.rel_elevation/VFOV*HEIGHT)
        cv2.putText(im, str(idx + 1), (x, y), cv2.FONT_HERSHEY_SIMPLEX, 
            fontScale, TEXT_COLOR, thickness=3)
    cv2.imshow('displaywin', im)
    k = cv2.waitKey(1)
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
