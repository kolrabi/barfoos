#version 120

uniform mat4 u_matProjection;
uniform mat4 u_matModelView;
uniform vec4 u_color;
uniform mat4 u_matTexture;

varying vec2 v_tex;
varying vec4 v_color;

void main() {
  /* output */
  gl_Position = u_matModelView * u_matProjection * gl_Vertex;
  v_color     = u_color * gl_Color;
  v_tex       = vec2(u_matTexture * gl_MultiTexCoord0);
}
