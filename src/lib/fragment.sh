#version 120

varying vec3 texCoord;
uniform samplerCube cubemap;

void main (void) {
gl_FragColor = textureCube(cubemap, vec3(-texCoord.x, texCoord.y, texCoord.z));
}
