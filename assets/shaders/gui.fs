uniform sampler2D u_texture;

varying vec2 v_tex;
varying vec4 v_color;

void main() {
  vec4 t0 = texture2D(u_texture, v_tex);
  if (t0.a == 0.0) discard;
  vec4 color = v_color;
  color.rgb *= color.a;
  gl_FragColor = color;
}
