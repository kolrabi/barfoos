varying vec4 v_pos;
varying vec2 v_tex;
varying vec4 v_color;
varying vec3 v_norm;
varying vec3 v_eye;
varying vec3 v_worldpos;

uniform float u_time;

void main() {
  vec4 vertex = gl_Vertex;

  vec3 tc;
  float t = u_time * 0;
  tc = vec3(
    cos(t+vertex.z*0.5+vertex.y*0.3),
    cos(t+vertex.x*0.3+vertex.z*0.4),
    cos(t+vertex.x*0.4+vertex.y*0.5) 
  );
  tc *= vec3(
    cos(t*0.6+vertex.x*0.4+vertex.y*0.4),
    cos(t*0.5+vertex.y*0.3+vertex.z*0.5),
    cos(t*0.7+vertex.z*0.5+vertex.x*0.3) 
  );

  vertex = vertex + vec4(tc*0.05, 0.0);

  v_worldpos = vertex.xyz;
  v_pos = gl_ModelViewMatrix * vertex;

//  vertex.y -= (v_pos.z*v_pos.z)*0.01;

  vec4 pos = gl_ModelViewProjectionMatrix * vertex;
  gl_Position = pos;

  v_color = gl_Color;
  v_tex = gl_MultiTexCoord0.st;
  v_norm = gl_NormalMatrix * gl_Normal;
  v_eye = (gl_ModelViewMatrix * vec4(0,0,1,1)).xyz;
}
