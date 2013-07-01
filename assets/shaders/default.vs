varying vec4 v_pos;
varying vec2 v_tex;
varying vec4 v_color;
varying vec3 v_norm;
varying vec3 v_eye;
varying vec3 v_worldpos;

uniform float u_time;
uniform sampler2D u_texture;
uniform sampler2D u_texture2;

vec3 Distort(vec4 vertex) {
  float t = u_time * 2 * 3.14159;
  return vec3(
    cos(t*1.0+vertex.z+vertex.y)*vertex.x,
    sin(t*0.5+vertex.z+vertex.x)*vertex.y,
    0 //cos(t+vertex.x+vertex.y-vertex.z) 
  );
}

void main() {
  vec4 vertex = gl_Vertex;
  v_worldpos = vertex.xyz;
  
  v_pos = gl_ModelViewMatrix * vertex;
 
  /* turbulence */
  float turbulence = 0.0;
  
// v_pos.xy += (texture2D(u_texture2, v_pos.xy+np).rg - 0.5)*turbulence;
//  v_pos.x *= 1.0+(0.25*cos(u_time*0.125*3.14159*2+v_pos.z+v_pos.y))*turbulence;
//  v_pos.y *= 1.0+(0.25*sin(u_time*0.250*3.14159*2+v_pos.z+v_pos.x))*turbulence;
//  v_pos.z += 0.15*cos(u_time*5+v_pos.z*2);
//  v_pos.xy *= 1.0 + 0.1*cos(v_pos.z+u_time);
  vec3 tc = Distort(v_pos);
  v_pos += vec4(tc, 0.0) * turbulence;

  vec4 pos = gl_ProjectionMatrix * v_pos;
  vec2 np = texture2D(u_texture, pos.xy).rg;

  /* output */
  gl_Position = pos;
  v_color = gl_Color;
  v_tex = (gl_TextureMatrix[0] * gl_MultiTexCoord0).st;
  v_norm = gl_NormalMatrix * gl_Normal;
  v_eye = (gl_ModelViewMatrix * vec4(0,0,1,1)).xyz;
}
