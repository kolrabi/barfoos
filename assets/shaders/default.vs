varying vec4 v_pos;
varying vec2 v_tex;
varying vec4 v_color;
varying vec3 v_norm;
varying vec3 v_eye;
varying vec3 v_worldpos;

uniform float u_time;

vec3 Distort(vec4 vertex) {
  float t = u_time;
  return vec3(
    cos(t+vertex.y),
    cos(t+vertex.x),
    0 //cos(t+vertex.x+vertex.y-vertex.z) 
  );
}

void main() {
  vec4 vertex = gl_Vertex;
  v_worldpos = vertex.xyz;
  
  v_pos = gl_ModelViewMatrix * vertex;
 
  /* turbulence */
  vec3 tc = Distort(gl_ModelViewMatrix * vertex);
  vertex = vertex + vec4(tc*0.2, 0.0);

  vec4 pos = gl_ModelViewProjectionMatrix * vertex;

  /* output */
  gl_Position = pos;
  v_color = gl_Color;
  v_tex = gl_MultiTexCoord0.st;
  v_norm = gl_NormalMatrix * gl_Normal;
  v_eye = (gl_ModelViewMatrix * vec4(0,0,1,1)).xyz;
}
