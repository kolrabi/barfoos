const int lightCount = 4;

uniform sampler2D u_texture;
uniform vec4 u_torch;
uniform float u_time;

uniform vec4 u_fogColor;
uniform vec4 u_color;
uniform vec4 u_light;
uniform float u_fogLin;

uniform mat4 u_matView;

uniform vec4 u_fade;

varying vec3 v_pos;
varying vec2 v_tex;
varying vec4 v_color;
varying vec3 v_norm;

varying vec3 v_light;

const float gamma = 2.2;
const float contrast = 1.0;

void main() {
  vec4 texSRGB = texture2D(u_texture, v_tex);
  if (texSRGB.a == 0.0) discard;

  vec3 texLin  = pow(texSRGB.rgb, vec3(1.0/gamma));

  vec3 lightLin  = v_light;
  vec3 vColorLin = pow(v_color.rgb, vec3(1.0/gamma));
  vec3 uColorLin = pow(u_color.rgb, vec3(1.0/gamma));
  vec3 uLightLin = pow(u_light.rgb, vec3(1.0/gamma));

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
