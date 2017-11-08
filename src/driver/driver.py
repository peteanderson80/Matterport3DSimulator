import sys
sys.path.append('build')
import MatterSim
import time
import math

import cv2
cv2.namedWindow('displaywin')
sim = MatterSim.Simulator()
sim.setDatasetPath('../data')
sim.setNavGraphPath('connectivity')
sim.setCameraResolution(640, 480)
#sim.setDiscretizedViewingAngles(True)
sim.init()
#sim.newEpisode('2t7WUuJeko7', 'ee3c278732514701bacdfdacf81f4915', 0, 0)
#sim.newEpisode('2t7WUuJeko7', '8d51f41d9ce04dfdaae944da9c6d3847', 0, 0)
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
        angle = math.pi/2 - math.atan2(loc.point[1] - origin[1], loc.point[0] - origin[0])
        anglediff = angle - state.heading
        while anglediff > math.pi:
            anglediff -= 2 * math.pi
        while anglediff < -math.pi:
            anglediff += 2 * math.pi
        if abs(anglediff) < math.pi / 4:
            dist = math.sqrt((loc.point[0] - origin[0]) ** 2 + (loc.point[1] - origin[1]) ** 2)
            colour = [230, 40, 40]
            cv2.putText(im, str(idx + 1), (320 + int(320 * anglediff * 4 / math.pi), 480 - int(dist * 100)), cv2.FONT_HERSHEY_SIMPLEX, 2, colour, thickness=3)
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
