R""(
#version 120

varying vec3 texCoord;
varying vec4 camCoord;
uniform samplerCube cubemap;
const vec3 camlook = vec3( 0.0, 0.0, -1.0 );
uniform bool isDepth;

void main (void) {
  vec4 color = textureCube(cubemap, texCoord);
  if (isDepth) {
    float scale = dot(camCoord.xyz, camlook) / length(camCoord.xyz);
    gl_FragColor = color*scale;
  } else {
    gl_FragColor = color;
  }
}
)""
