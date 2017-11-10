## connectivity
Connectivity graphs indicating the navigable paths between viewpoints in each scan.

Each json file contains an array of annotations, one for each viewpoint in the scan. All annotations share the same basic structure as follows:

```
{
  "image_id": str,
  "pose": [float x 16],
  "included": boolean,
  "visible": [boolean x num_viewpoints],
  "unobstructed": [boolean x num_viewpoints],
  "height": float
}
```
- `image_id`: matterport skybox prefix
- `pose`: 4x4 matrix in row major order that transforms matterport skyboxes to global coordinates (z-up). Pose matrices are based on the assumption that the camera is facing skybox image 3.
- `included`: whether viewpoint is included in the simulator. Some overlapping viewpoints are excluded.
- `visible`: indicates other viewpoints that can be seen from this viewpoint.
- `unobstructed`: indicates transitions to other viewpoints that are considered navigable for an agent.
- `height`: estimated height of the viewpoint above the floor. Not required for the simulator.

Units are in metres.

`scans.txt` contains a list of all the scan ids in the dataset.
