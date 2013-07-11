uniform sampler2D u_texture;
uniform vec4 u_torch;
uniform float u_time;

uniform vec4 u_fogColor;
uniform vec4 u_color;
uniform float u_fogLin;

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
  float torchIntensity = abs(dot(v_norm, eyeDir)) - dist*0.1;
  vec3 torch = u_torch.rgb * torchIntensity;
  vec3 light = pow( v_color.rgb, vec3(2.2) ) * u_color.rgb + torch;
  
  float fogDepth = dist;
  //float fogIntensity = 1.0 - 1.0 / (1.0 + fogDepth*u_fogLin*10);
  float fogIntensity = pow(max(0.0, u_fogLin * fogDepth), 0.5);
  
  vec3 color = mix(t0.rgb * light, u_fogColor.rgb, min(1.0, fogIntensity));
 
  gl_FragColor = vec4(color, t0.a);
}
