uniform sampler2D u_texture;
uniform vec3 u_torch;
uniform float u_time;

varying vec4 v_pos;
varying vec2 v_tex;
varying vec4 v_color;
varying vec3 v_norm;
varying vec3 v_eye;
varying vec3 v_worldpos;

void main() {
  vec3 p = v_pos.xyz / v_pos.w;
  vec3 e = -normalize(v_pos.xyz);
  float dist = length(p);
  dist *= dist;
  dist += 1.0;
  dist *= 0.1;
  dist = max(1.0, dist);
  
  float torchIntensity = max(0.0, dot(v_norm, e)/dist);
  vec4 light = (v_color + vec4(u_torch * torchIntensity, 1.0));
 
  vec2 tc = vec2(cos(u_time+v_worldpos.z+v_worldpos.y),cos(u_time+v_worldpos.x+v_worldpos.y));
  vec4 t0 = texture2D(u_texture, v_tex+tc*0.0);
  gl_FragColor = t0 * light;
}
