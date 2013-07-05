uniform float u_time;
uniform sampler2D u_texture;
uniform sampler2D u_texture2;

uniform mat4 u_matProjection;
uniform mat4 u_matModelView;
uniform mat4 u_matTexture;

uniform vec4 u_color;

varying vec4 v_pos;
varying vec2 v_tex;
varying vec4 v_color;
varying vec3 v_norm;
varying vec3 v_eye;
varying vec3 v_worldpos;

vec3 Distort(vec4 vertex) {
  float t = u_time * 2.0 * 3.14159;
  return vec3(
    cos(t*1.0+vertex.z+vertex.y)*vertex.x,
    sin(t*0.5+vertex.z+vertex.x)*vertex.y,
    0 //cos(t+vertex.x+vertex.y-vertex.z) 
  );
}

void main() {
  vec4 vertex = gl_Vertex;
  v_worldpos = vertex.xyz;
  
  v_pos = u_matModelView * vertex;
 
  /* turbulence */
  float turbulence = 0.0;
  
  v_pos += vec4(Distort(v_pos), 0.0)*turbulence;

  /* output */
  gl_Position = u_matProjection * v_pos;
  v_color = gl_Color;
  v_tex = (u_matTexture * gl_MultiTexCoord0).st;
  v_norm = gl_NormalMatrix * gl_Normal;
  v_eye = (u_matModelView * vec4(0,0,1,1)).xyz;
}
