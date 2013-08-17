#version 120
const int lightCount = 4;

uniform float u_time;
uniform sampler2D u_texture;
uniform sampler2D u_texture2;

uniform mat4 u_matProjection;
uniform mat4 u_matModelView;
uniform mat4 u_matView;
uniform mat4 u_matTexture;
uniform mat4 u_matNormal;

uniform vec3 u_lightPos[lightCount];
uniform vec4 u_lightColor[lightCount];

varying vec3 v_pos;
varying vec2 v_tex;
varying vec4 v_color;
varying vec3 v_norm;
varying vec3 v_light[lightCount];

vec3 Distort(vec4 vertex) {
  float t = u_time * 2.0 * 3.14159;
  return vec3(
    cos(t*1.0+vertex.z+vertex.y)*vertex.x,
    sin(t*0.5+vertex.z+vertex.x)*vertex.y,
    0 //cos(t+vertex.x+vertex.y-vertex.z) 
  );
}

void main() {
  /* turbulence */
  //float turbulence = 0.0;
  //v_pos       += vec4(Distort(v_pos), 0.0)*turbulence;

  /* output */
  gl_Position = u_matProjection * (u_matModelView * gl_Vertex);
  v_pos       = vec3(u_matModelView * gl_Vertex);
  v_color     = gl_Color;
  v_tex       = (u_matTexture * gl_MultiTexCoord0).st;
  v_norm      = normalize(mat3(u_matNormal) * gl_Normal);
 
  for (int i=0; i<lightCount; i++) {
    vec3 v_l = vec3(u_matView * vec4(u_lightPos[i], 1.0));
    vec3 ld = v_l - v_pos;
    vec3 L = normalize(ld);
    float d = length(ld);
    v_light[i] = u_lightColor[i].rgb * max(0.0, dot(v_norm, L)) / (1.0 +d);
  }  
}
