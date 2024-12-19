$input a_position
$output v_color0

#include "common.sh"

uniform vec4 u_util;
uniform vec4 u_color;

void main() {
  float colorAdd   = u_util[0];
  float colorDiv   = u_util[1];
  float heightExag = u_util[2];

  gl_Position = mul(u_modelViewProj, vec4(a_position.xy, a_position.z * heightExag, 1.0));

  float h = colorAdd + a_position.z / colorDiv;
  v_color0 = vec4(h * u_color.xyz, 1.0);
}
