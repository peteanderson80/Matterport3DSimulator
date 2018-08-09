
// Matterport3D utils for three.js

function Matterport3D(data_dir) {
  this.data_dir = (typeof data_dir !== 'undefined') ?  data_dir : "v1/scans/";
};

// Load a textured scene mesh
Matterport3D.prototype.load_mesh = function(scan_id, mesh_id, callback) { 
  var base_url = this.data_dir+scan+"/matterport_mesh/"+mesh+"/"
  var obj_url = base_url + mesh + ".obj";
  var mat_url = base_url + mesh + ".mtl"; 
  var mtlLoader = new THREE.MTLLoader();
  mtlLoader.setTexturePath(this.data_dir+scan+"/matterport_mesh/"+mesh+"/");
  mtlLoader.load(mat_url, function( materials ) {
    materials.preload();
    var objLoader = new THREE.OBJLoader();
    objLoader.setMaterials( materials );
    objLoader.load(obj_url, function ( object ) {
      callback(object);
    });
  });
};

// Load cube texture and return a promise
Matterport3D.prototype.loadCubeTexture = function(urls) {
  return new Promise(function(resolve, reject) {
    const onLoad = function (texture) { return resolve(texture); }
    const onError = function (event) { return reject(event); }
    var loader = new THREE.CubeTextureLoader();
    loader.setCrossOrigin('anonymous');
    loader.load(urls, onLoad, null, onError);
  });
};

// Load json file and return a promise
Matterport3D.prototype.loadJson = function(url) {
  return new Promise(function(resolve, reject) {
    d3.json(url, function(error, data) {
      if (error) reject(error);
      else resolve(data);
    });
  });
};

// Load small cylinders representing viewpoints (projected down to floor level)
Matterport3D.prototype.load_viewpoints = function(data, {h=0.5, opacity=0.7} = {}) {
  var group = new THREE.Group();
  for (var i = 0; i < data.length; i++) {
    var pose = data[i]['pose'];
    for(var k=0; k<pose.length;k++) pose[k] = parseFloat(pose[k]);
    var height = parseFloat(data[i]['height']);
    pose[11] -= height; // drop to surface level
    var m = new THREE.Matrix4();
    m.fromArray(pose);
    m.transpose(); // switch row major to column major to suit three.js
    var geometry = new THREE.CylinderBufferGeometry(0.15, 0.15, h, 128);
    var material = new THREE.MeshLambertMaterial({color: 0x0000ff});
    material.transparent = true;
    material.opacity = opacity;
    var cylinder = new THREE.Mesh(geometry, material);
    cylinder.applyMatrix(m);
    cylinder.height = height;
    cylinder.name = data[i]['image_id'];
    group.add(cylinder);
    cylinder.included = true;
    if (data[i].hasOwnProperty('included') ) {
      if (data[i]['included'] == false){
        cylinder.included = false;
      }
    }
  }
  return group;
};

