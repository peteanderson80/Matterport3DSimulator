

var step = 0;
var playing = false;
var downloading = false;
var scan;
var curr_image_id;
var capturer;
var frameRate = 60.0;
var pauseStart = 1000;
var pauseEnd = 3000;

// declare a bunch of variable we will need later
var camera, camera_pose, scene, controls, renderer, connections, id_to_ix, world_frame, cylinder_frame, cubemap_frame;
var fp_scene, fp_camera, fp_renderer, dollhouse, mesh_names;
var mouse = new THREE.Vector2();
var id, path, gt, traj;

var ix = 0;
var SIZE_X = 1140;
var SIZE_Y = 650;
var VFOV = 80;
var ASPECT = SIZE_X/SIZE_Y;

var FP_SIZE_X = 960;
var FP_SIZE_Y = 720;
var FP_VFOV = 70;
var FP_ASPECT = FP_SIZE_X/FP_SIZE_Y;

var $ix=document.getElementById('ix');
var $width=document.getElementById('width');
var $height=document.getElementById('height');
var $vfov=document.getElementById('vfov');
var $instr_id=document.getElementById('instr_id');
var $play=document.getElementById('play');
var $download=document.getElementById('download');
var $canvas=document.getElementById('skybox');

// set the initial input-text values to the width/height vars
$ix.value=ix;
$width.value=SIZE_X;
$height.value=SIZE_Y;
$vfov.value=VFOV;

// listen for keyup events on width & height input-text elements
// Get the current values from input-text & set the width/height vars
// call draw to redraw the rect with the current width/height values
$ix.addEventListener("keyup", function(){
  playing=false;
  ix=this.value;
  draw();
}, false);

function reset() {
  playing = false;
  downloading = false;
  step = 0;
  $play.disabled = false;
  $download.disabled = false;
}

function left() {
  reset();
  ix = ix - 1;
  if (ix < 0) { ix = traj.length-1;}
  $ix.value=ix;
  draw();
}

function right() {
  reset();
  ix = ix + 1;
  if (ix >= traj.length) { ix = 0;}
  $ix.value=ix;
  draw();
}

$width.addEventListener("keyup", function(){
  if (this.value > 100 && this.value <=1280){
    SIZE_X=this.value;
    ASPECT = SIZE_X/SIZE_Y;
    draw();
  }
}, false);

$height.addEventListener("keyup", function(){
  if (this.value > 100 && this.value <=720){
    SIZE_Y=this.value;
    ASPECT = SIZE_X/SIZE_Y;
    draw();
  }
}, false);

$vfov.addEventListener("keyup", function(){
    VFOV=this.value;
    draw();
}, false);

document.getElementById('show-instructions').addEventListener("change", function(){
  move_to(curr_image_id);
});

document.getElementById('trajFile').addEventListener("change", function(evt){
  reset();
  ix = 0;
  $ix.value=ix;
  var file = evt.target.files[0];
  $('#fileName').text(file.name);
  initialize();
}, false);

function draw(){
  id = traj[ix]['instr_id'];
  $instr_id.value=id;
  path = traj[ix]['trajectory'];
  var found = false;
  for (var i = 0; i < gt.length; i++) {
    if (gt[i]['path_id'] == id.split('_')[0]) {
      scan = gt[i]['scan'];
      curr_image_id = gt[i]['path'][0];
      instr = gt[i]['instructions'][parseInt(id.split('_')[1])];
      $('#instruction').text(instr);
      found = true;
      break;
    }
  }
  if (found){
    skybox_init();
    load_connections(scan, curr_image_id);
  } else {
    console.error('instruction id ' + id + ' not in something');
  }
}

function initialize(){
  matt.loadJson($('#fileName').text()).then(function(data){
    traj = data;
    d3.queue()
    .defer(d3.json, "/R2Rdata/R2R_val_seen.json")
    .defer(d3.json, "/R2Rdata/R2R_val_unseen.json")
    .defer(d3.json, "/R2Rdata/R2R_test.json")
    .await(function(error, d1, d2, d3) {
      if (error) {
        console.error(error);
      }
      else {
        gt = d1.concat(d2).concat(d3);
        draw();
      }
    });
  });
}
var matt = new Matterport3D("");
initialize();

function download() {
  if (!playing && !downloading){
    downloading = true;
    mediaStream = $canvas.captureStream(frameRate);
    var options = {
      mimeType : "video/webm;codecs=H264"
    }
    capturer = new MediaRecorder(mediaStream, options);
    capturer.ondataavailable = function(event){
      // save download
      var url = URL.createObjectURL(event.data);
      var a = document.createElement('a');
      document.body.appendChild(a);
      a.style = 'display: none';
      a.href = url;
      a.download = $instr_id.value + '.webm';
      a.click();
      window.URL.revokeObjectURL(url);
      a.remove();
    };
    capturer.start();
    setTimeout(function(){ play(); }, pauseStart);
  }
}

function download_image() {
  if (!playing && !downloading){
    var canvas = document.getElementById("skybox");
    image = canvas.toDataURL();//.replace("image/png", "image/octet-stream");
    // save download
    var a = document.createElement('a');
    document.body.appendChild(a);
    a.style = 'display: none';
    a.href = image;
    a.download = 'threejs_' + get_pose_string() + '.png';
    a.click();
    a.remove();
  }
}

function play() {
  if (!playing){
    $play.disabled = true;
    $download.disabled = true;
    if (step != 0 || curr_image_id != path[0][0]){
      // First move back to start
      var image_id = path[0][0];
      matt.loadCubeTexture(cube_urls(scan, image_id)).then(function(texture){
        camera.rotation.x = 0;
        camera.rotation.y = 0;
        camera.rotation.z = 0;
        scene.background = texture;
        render();
        move_to(image_id, true);
        step = 0;
        playing = true;
        step_forward();
      });
    } else {
      step = 0;
      playing = true;
      step_forward();
    }
  }
}

function step_forward(){
  step += 1;
  if (step >= path.length) {
    step -= 1;
    playing = false;
    $play.disabled = false;
    $download.disabled = false;
    if (downloading){
      downloading = false;
      setTimeout(function(){ capturer.stop(); }, pauseEnd);
    }
  } else {
    take_action(path[step]);
  }
};

// ## Initialize everything
function skybox_init(scan, image) {
  // test if webgl is supported
  if (! Detector.webgl) Detector.addGetWebGLMessage();

  // create the camera (kinect 2)
  camera = new THREE.PerspectiveCamera(VFOV, ASPECT, 0.01, 1000);
  camera_pose = new THREE.Group();
  camera_pose.add(camera);
  
  // create the Matterport world frame
  world_frame = new THREE.Group();
  
  // create the cubemap frame
  cubemap_frame = new THREE.Group();
  cubemap_frame.rotation.x = -Math.PI; // Adjust cubemap for z up
  cubemap_frame.add(world_frame);
  
  // create the Scene
  scene = new THREE.Scene();
  world_frame.add(camera_pose);
  scene.add(cubemap_frame);

  var light = new THREE.DirectionalLight( 0xFFFFFF, 1 );
  light.position.set(0, 0, 100);
  world_frame.add(light);
  world_frame.add(new THREE.AmbientLight( 0xAAAAAA )); // soft light

  // init the WebGL renderer - preserveDrawingBuffer is needed for toDataURL()
  renderer = new THREE.WebGLRenderer({canvas: $canvas, antialias: true, preserveDrawingBuffer: true } );
  renderer.setSize(SIZE_X, SIZE_Y);

  controls = new THREE.PTZCameraControls(camera, renderer.domElement);
  controls.minZoom = 1;
  controls.maxZoom = 3.0;
  controls.minTilt = -0.6*Math.PI/2;
  controls.maxTilt = 0.6*Math.PI/2;
  controls.enableDamping = true;
  controls.panSpeed = -0.25;
  controls.tiltSpeed = -0.25;
  controls.zoomSpeed = 1.5;
  controls.dampingFactor = 0.5;
  controls.addEventListener( 'change', render );
}

function load_connections(scan, image_id) {
  var pose_url = "/connectivity/"+scan+"_connectivity.json";
  d3.json(pose_url, function(error, data) {
    if (error) return console.warn(error);
    connections = data;
    // Create a cylinder frame for showing arrows of directions
    cylinder_frame = matt.load_viewpoints(data, {opacity:0.4});
    // Keep a structure of connection graph
    id_to_ix = {};
    for (var i = 0; i < data.length; i++) {
      var im = data[i]['image_id'];
      id_to_ix[im] = i;
    }
    world_frame.add(cylinder_frame);
    var image_id = path[0][0];
    matt.loadCubeTexture(cube_urls(scan, image_id)).then(function(texture){
      scene.background = texture;
      move_to(image_id, true);
      $play.disabled = false;
      $download.disabled = false;
    });
  });
}

function cube_urls(scan, image_id) {
  var urlPrefix  = "data/v1/scans/" + scan + "/matterport_skybox_images/" + image_id;
  return [ urlPrefix + "_skybox2_sami.jpg", urlPrefix + "_skybox4_sami.jpg",
      urlPrefix + "_skybox0_sami.jpg", urlPrefix + "_skybox5_sami.jpg",
      urlPrefix + "_skybox1_sami.jpg", urlPrefix + "_skybox3_sami.jpg" ];
}

function move_to(image_id, isInitial=false) {
  // Adjust cylinder visibility
  var cylinders = cylinder_frame.children;
  for (var i = 0; i < cylinders.length; ++i){
    if ($('#show-instructions').is(":checked")){
      cylinders[i].visible = connections[id_to_ix[image_id]]['unobstructed'][i];
    } else {
      cylinders[i].visible = false;
    }
  }
  // Correct world frame for individual skybox camera rotation
  var inv = new THREE.Matrix4();
  var cam_pose = cylinder_frame.getObjectByName(image_id);
  inv.getInverse(cam_pose.matrix);
  var ignore = new THREE.Vector3();
  inv.decompose(ignore, world_frame.quaternion, world_frame.scale);
  world_frame.updateMatrix();
  if (isInitial){
    set_camera_pose(cam_pose.matrix, cam_pose.height);
  } else {
    set_camera_position(cam_pose.matrix, cam_pose.height);
  }
  render();
  curr_image_id = image_id;
  // Animation
  if (playing) {
    step_forward();
  }
}

function set_camera_pose(matrix4d, height){
  matrix4d.decompose(camera_pose.position, camera_pose.quaternion, camera_pose.scale);
  camera_pose.position.z += height;
  camera_pose.rotateX(Math.PI); // convert matterport camera to webgl camera
}

function set_camera_position(matrix4d, height) {
  var ignore_q = new THREE.Quaternion();
  var ignore_s = new THREE.Vector3();
  matrix4d.decompose(camera_pose.position, ignore_q, ignore_s);
  camera_pose.position.z += height;
}

function get_camera_pose(){
  camera.updateMatrix();
  camera_pose.updateMatrix();
  var m = camera.matrix.clone();
  m.premultiply(camera_pose.matrix);
  return m;
}

function get_pose_string(){
  var m = get_camera_pose();

  // calculate heading
  var rot = new THREE.Matrix3();
  rot.setFromMatrix4(m);
  var cam_look = new THREE.Vector3(0,0,1); // based on matterport camera
  cam_look.applyMatrix3(rot);
  heading = -Math.PI/2.0 -Math.atan2(cam_look.y, cam_look.x);
  if (heading < 0) {
    heading += 2.0*Math.PI;
  }

  // calculate elevation
  elevation = -Math.atan2(cam_look.z, Math.sqrt(Math.pow(cam_look.x,2) + Math.pow(cam_look.y,2)))
  
  return scan+"_"+curr_image_id+"_"+heading+"_"+elevation;
}

function take_action(dest) {
  image_id = dest[0]
  heading = dest[1]
  elevation = dest[2]
  if (image_id !== curr_image_id) {
    var texture_promise = matt.loadCubeTexture(cube_urls(scan, image_id)); // start fetching textures
    var target = cylinder_frame.getObjectByName(image_id);

    // Camera up vector
    var camera_up = new THREE.Vector3(0,1,0);
    var camera_look = new THREE.Vector3(0,0,-1);
    var camera_m = get_camera_pose();// Animation
    var zero = new THREE.Vector3(0,0,0);
    camera_m.setPosition(zero);
    camera_up.applyMatrix4(camera_m);
    camera_up.normalize();
    camera_look.applyMatrix4(camera_m);
    camera_look.normalize();

    // look direction
    var look = target.position.clone();
    look.sub(camera_pose.position);
    look.projectOnPlane(camera_up);
    look.normalize();
    // Simplified - assumes z is zero
    var rotate = Math.atan2(look.y,look.x) - Math.atan2(camera_look.y,camera_look.x);
    if (rotate < -Math.PI) rotate += 2*Math.PI;
    if (rotate > Math.PI) rotate -= 2*Math.PI;

    var target_y = camera.rotation.y + rotate;
    var rotate_tween = new TWEEN.Tween({
      x: camera.rotation.x,
      y: camera.rotation.y,
      z: camera.rotation.z})
    .to( {
      x: 0,
      y: target_y,
      z: 0 }, 2000*Math.abs(rotate) )
    .easing( TWEEN.Easing.Cubic.InOut)
    .onUpdate(function() {
      camera.rotation.x = this.x;
      camera.rotation.y = this.y;
      camera.rotation.z = this.z;
      render();
    });
    var new_vfov = VFOV*0.95;
    var zoom_tween = new TWEEN.Tween({
      vfov: VFOV})
    .to( {vfov: new_vfov }, 1000 )
    .easing(TWEEN.Easing.Cubic.InOut)
    .onStart(function() {
      // Color change effect
      target.material.emissive.setHex( 0xff0000 );
      setTimeout(function(){ target.material.emissive.setHex( target.currentHex ); }, 200);
    })
    .onUpdate(function() {
      camera.fov = this.vfov;
      camera.updateProjectionMatrix();
      render();
    })
    .onComplete(function(){
      cancelAnimationFrame(id);
      texture_promise.then(function(texture) {
        scene.background = texture; 
        camera.fov = VFOV;
        camera.updateProjectionMatrix();
        move_to(image_id);
      });
    });
    rotate_tween.chain(zoom_tween);
    animate();
    rotate_tween.start();
  } else {
    // Just move the camera
    // Animation
    if (playing) {
      step_forward();
    }
  }
}

// Display the Scene
function render() {
  renderer.render(scene, camera);
}

// tweening
function animate() {
  id = requestAnimationFrame( animate );
  TWEEN.update();
}


