uniform sampler2D u_texture;
uniform vec4 u_torch;
uniform float u_time;

uniform vec4 u_fogColor;
uniform vec4 u_color;
uniform float u_fogLin;

uniform vec4 u_lightColor[32];
uniform int  u_lightPos_length;

uniform mat4 u_matView;

uniform vec4 u_fade;

varying vec3 v_pos;
varying vec2 v_tex;
varying vec4 v_color;
varying vec3 v_norm;

varying vec3 v_light[8];

const float gamma = 2.2;
const float contrast = 1.25;

vec3 getLight(int n) {
  vec3 ld = v_light[n] - v_pos;
  vec3 L = normalize(ld);
  float d = length(ld);

  return u_lightColor[n].rgb * max(0.0, dot(v_norm, L)) / (1.0 + d * 0.5);
}

vec3 getTotalLight() {
  vec3 light = vec3(0.0);
  for (int i=0; i<u_lightPos_length; i++) {
    light += getLight(i);
  }
  return light;
}

void main() {
  vec4 t0 = texture2D(u_texture, v_tex);
  if (t0.a == 0.0) discard;
  
  vec3 light = pow( v_color.rgb + getTotalLight(), vec3(gamma) ) * u_color.rgb;
  
  float fogDepth = length(v_pos)*0.1;
  float fogIntensity = pow(max(0.0, u_fogLin * fogDepth), 0.5);
  
  vec3 color = mix(t0.rgb * light, u_fogColor.rgb, min(1.0, fogIntensity)) + u_fade.rgb;
 
  gl_FragColor = vec4(color, t0.a);
}
