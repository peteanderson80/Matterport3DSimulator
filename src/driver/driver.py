import sys
sys.path.append('lib')
import MatterSim
import time
import math

import cv2
cv2.namedWindow('displaywin')
sim = MatterSim.Simulator()
sim.setDatasetPath('../data')
sim.setNavGraphPath('connectivity')
sim.setScanId('2t7WUuJeko7')
sim.setScreenResolution(640, 480)
sim.init()
sim.newEpisode()

heading = 0
elevation = 0
location = 0
while True:
    sim.makeAction(location, heading, elevation)
    location = 0
    locations = sim.getState().navigableLocations
    im = sim.getState().rgb
    origin = locations[0].location
    adjustedheading = heading + math.pi / 2
    for idx, loc in enumerate(locations[1:]):
        angle = math.atan2(loc.location[1] - origin[1], loc.location[0] - origin[0])
        anglediff = angle - adjustedheading
        while anglediff > math.pi:
            anglediff -= 2 * math.pi
        while anglediff < -math.pi:
            anglediff += 2 * math.pi
        if abs(anglediff) < math.pi / 4:
            dist = math.sqrt((loc.location[0] - origin[0]) ** 2 + (loc.location[1] - origin[1]) ** 2)
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
        heading -= math.pi / 180
    elif k == 82 or k == ord('w'):
        elevation += math.pi / 180
    elif k == 83 or k == ord('d'):
        heading += math.pi / 180
    elif k == 84 or k == ord('s'):
        elevation -= math.pi / 180
