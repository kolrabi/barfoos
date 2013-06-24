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
    cos(t+vertex.z)*vertex.x,
    sin(t+vertex.z)*vertex.y,
    0 //cos(t+vertex.x+vertex.y-vertex.z) 
  );
}

void main() {
  vec4 vertex = gl_Vertex;
  v_worldpos = vertex.xyz;
  
  v_pos = gl_ModelViewMatrix * vertex;
 
  /* turbulence */
  float turbulence = 0.0;
  v_pos.x *= 1.0+(0.25*cos(u_time*2+v_pos.z))*turbulence;
  v_pos.y *= 1.0+(0.25*sin(u_time*2.5+v_pos.z))*turbulence;
//  v_pos.z += 0.15*cos(u_time*5+v_pos.z*2);
//  v_pos.xy *= 1.0 + 0.1*cos(v_pos.z+u_time);
//  vec3 tc = Distort(v_pos);
//  v_pos = v_pos + vec4(tc*0.2, 0.0);

  vec4 pos = gl_ProjectionMatrix * v_pos;

  /* output */
  gl_Position = pos;
  v_color = gl_Color;
  v_tex = gl_MultiTexCoord0.st;
  v_norm = gl_NormalMatrix * gl_Normal;
  v_eye = (gl_ModelViewMatrix * vec4(0,0,1,1)).xyz;
}
