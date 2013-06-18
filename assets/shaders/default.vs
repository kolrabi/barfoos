varying vec4 v_pos;
varying vec2 v_tex;
varying vec4 v_color;
varying vec3 v_norm;
varying vec3 v_eye;

void main() {
  v_pos = gl_ModelViewMatrix * gl_Vertex;
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
  v_color = gl_Color;
  v_tex = gl_MultiTexCoord0.st;
  v_norm = gl_NormalMatrix * gl_Normal;
  v_eye = (gl_ModelViewMatrix * vec4(0,0,1,1)).xyz;
}
