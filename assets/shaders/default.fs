uniform sampler2D u_texture;
uniform vec4 u_torch;
uniform float u_time;

uniform vec4 u_fogColor;
uniform float u_fogLin;
uniform float u_fogExp2;

varying vec4 v_pos;
varying vec2 v_tex;
varying vec4 v_color;
varying vec3 v_norm;

void main() {
  vec4 t0 = texture2D(u_texture, v_tex);
  if (t0.a == 0.0) discard;
  
  vec3 pos = v_pos.xyz / v_pos.w;
  vec3 eyeDir = -normalize(v_pos.xyz);
  
  float dist = length(pos);
  float torchIntensity = max(0.0, dot(v_norm, eyeDir) - dist*0.1);
  vec4 light = v_color + u_torch * torchIntensity;
  
  float fogDepth = dist;
  float fogIntensity = pow(max(0.0, u_fogLin * fogDepth), 2);
  
  vec4 color = mix(t0 * light, u_fogColor, min(1.0, fogIntensity));
 
  gl_FragColor = color;
}
