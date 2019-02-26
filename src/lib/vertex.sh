R""(
#version 120

attribute vec3 vertex;
varying vec3 texCoord;
varying vec4 camCoord;
uniform mat4 ProjMat;
uniform mat4 ModelViewMat;

void main() {
  camCoord = ModelViewMat * vec4(vertex, 1.0);
  gl_Position = ProjMat * camCoord;
  texCoord = vertex;
}
)""
