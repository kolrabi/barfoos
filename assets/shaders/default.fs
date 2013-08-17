const int lightCount = 4;

uniform sampler2D u_texture;
uniform vec4 u_torch;
uniform float u_time;

uniform vec4 u_fogColor;
uniform vec4 u_color;
uniform vec4 u_light;
uniform float u_fogLin;

uniform vec4 u_lightColor[4];

uniform mat4 u_matView;

uniform vec4 u_fade;

varying vec3 v_pos;
varying vec2 v_tex;
varying vec4 v_color;
varying vec3 v_norm;

varying vec3 v_light[4];

const float gamma = 1.8;
const float contrast = 1.0;

vec3 getLight(int n) {
  return v_light[n];
/*
  vec3 ld = v_light[n] - v_pos;
  vec3 L = normalize(ld);
  float d = length(ld);

  return u_lightColor[n].rgb * max(0.0, dot(v_norm, L)) / (1.0 +d);
*/
}

vec3 getTotalLight() {
  vec3 light = vec3(0.0);
  light += getLight(0);
  light += getLight(1);
  light += getLight(2);
  light += getLight(3);
/*  light += getLight(4);
  light += getLight(5);
  light += getLight(6);
  light += getLight(7);
*/
  return light;
}

void main() {
  vec4 texSRGB = texture2D(u_texture, v_tex);
  if (texSRGB.a == 0.0) discard;

  vec3 texLin  = pow(texSRGB.rgb, vec3(1.0/gamma));

  vec3 lightLin  = getTotalLight();
  vec3 vColorLin = v_color.rgb;
  vec3 uColorLin = u_color.rgb;
  vec3 uLightLin = u_light.rgb;

  vec3 colorLin = (lightLin + uLightLin) * (vColorLin * uColorLin * texLin);

  vec3 colorSRGB = pow(colorLin, vec3(gamma));
  gl_FragColor = vec4(colorSRGB, texSRGB.a);
/*
  vec4 t0 = texture2D(u_texture, v_tex);
  if (t0.a == 0.0) discard;
 
  t0.rgb = pow(t0.rgb, vec3(1.0/gamma));
 
  vec3 light = (v_color.rgb + getTotalLight()) * contrast;
  
  float fogDepth = length(v_pos)*0.1;
  float fogIntensity = 0.0; //pow(max(0.0, u_fogLin * fogDepth), 0.5);
  
  vec3 color = mix(pow(t0.rgb * light * u_color.rgb, vec3(gamma)), u_fogColor.rgb, min(1.0, fogIntensity)) + u_fade.rgb;
 
  gl_FragColor = vec4(color, t0.a);
*/
}
