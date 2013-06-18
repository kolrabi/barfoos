uniform sampler2D u_texture;
uniform vec3 u_torch;

varying vec4 v_pos;
varying vec2 v_tex;
varying vec4 v_color;
varying vec3 v_norm;

void main() {
  vec3 p = v_pos.xyz / v_pos.w;
  float dist = length(p) + 1.0;
  
  float torchIntensity = 3.0/(dist*dist);
 
  gl_FragColor = texture2D(u_texture, v_tex) * (v_color + vec4(u_torch * torchIntensity, 1.0));
}
