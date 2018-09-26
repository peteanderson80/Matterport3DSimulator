#version 120

attribute vec3 vertex;
varying vec3 texCoord;
uniform mat4 PVMM;

void main() {
gl_Position = PVMM * vec4(vertex, 1.0);
texCoord = vertex;
}
