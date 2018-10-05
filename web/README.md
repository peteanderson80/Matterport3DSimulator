# Web

This directory contains web-based applications for:
- Viewing and saving first-person trajectories
- Amazon Mechanical Turk (AMT) interfaces that were used to collect and evaluate navigation instructions

Code is based on Javascript and the [three.js](https://threejs.org/) wrapper for OpenGL, as well as the [tween.js](https://github.com/tweenjs/tween.js/) library for animation. The [Gulp](https://gulpjs.com/) task runner (based on Node.js) is used for spinning up a web servers and optimizing and minifying javascript for deployment (e.g. to AMT).

To get started, make sure you have [Node.js](https://nodejs.org/en/) >=6.0.0 installed, then install the remaining dependencies using the npm package manager:
```
npm install
```

You will also need to first install the Matterport data as described [here](../README.md). Then, set up symlinks to data (from the app directory) as follows:
```
cd app
ln -s ../../tasks/R2R/data/ R2Rdata
ln -s ../../connectivity connectivity
ln -s ../../data data
```

Also, download the R2R trajectory data by running this script from the top level directory (if you haven't already done this):
```
./tasks/R2R/data/download.sh
```


Now you can start a web server to check out the various visualizations and AMT user interfaces:
```
gulp
```

## Trajectory Visualization

`trajectory.html` is an application for viewing first-person trajectories and downloading them as videos:
- Use `Choose File` to select a trajectory file in the leaderboard submission format. By default, the included file `val_unseen_shortest_agent.json` is selected (containing the shortest paths to goal in the unseen validation set).
- `Play` visualizes the trajectory with the provided index.
- `Download video` visualizes the trajectory then downloads it as a .webm video.
- Camera parameters can be set with the `Width`, `Height` and `V-FOV` fields.
- Change the `Index` field to view different trajectories from the file.


## AMT Interfaces

`collect-hit.html` and `eval-hit.html` are the AMT interfaces used for collecting navigation instructions for the R2R data set, and benchmarking human performance on the R2R test set, respectively. Both interfaces appear as they would to a worker on AMT, except there is not 'Submit' button. Instead, both interfaces have a url parameter `?ix=0` that can be directly edited in your browser address bar to view different HITs. There are also instructions at the top of the UI that can be expanded.

### collect-hit

The UI `collect-hit.html` shows workers a navigation trajectory that must be annotated with a navigation instruction. Workers can only move along the trajectory (either fly-through or by clicking through each step), but cannot move anywhere else. Trajectories are loaded from the file `sample_room_paths.json`. Navigation instructions are collected in the textarea with id `tag1`, which can be integrated with AMT. 

### eval-hit

The UI `eval-hit.html` situates workers in an environment and provides a navigation instruction sourced from `R2R_test.json`. Workers can move anywhere, and must submit when they are as close as possible to the goal location. The actual navigation trajectories are collected in a hidden input with id `traj`, in the form of comma-separated (viewpointID, heading_degrees, elevation_degrees) tuples.

### Integrating with AMT

To actually use these interfaces to collect data they must be integrated with AMT. Please check the AMT docs. At high level, several additional steps are required to achieve this:
- Run `gulp build` to generate optimized and minified javascript (`main.min.js`) in the `dist` directory. 
- Host online the minified javascript files, along with the Matterport skybox images (we suggest downsampling the originals to 50% or smaller to keep the HITs responsive), our connectivity graphs, and any other necessary files for the particular html template (e.g. your own version of `sample_room_paths.json` or `R2R_test.json`) so they are publicly accessible.
- In the html template(s): 
  - Review the HIT instructions and replace references to ACRV with your research group.
  - Replace all local urls with urls linking to your own publicly hosted assets, and
  - Switch to AMT parameters instead of url parameters, i.e., replace `var ix = location.search.split('ix=')[1];` with `var ix = ${ix}` and provide these parameters to AMT (e.g., in an uploaded csv file) when creating a batch of HITs. Note that the `ix` parameter is just an index into `sample_room_paths.json` or `R2R_test.json`.
- Follow the AMT instructions to create a batch of HITs using your modified html template(s), such that the data collected in the `tag1` and/or `traj` fields will be available through AMT.

Disclaimer: We provide this code to assist others collecting AMT annotations on top of Matterport-style data, but this is academic code and not a supported library. We may have forgotten something or left out a step! Feel free to submit pull requests with fixes.
