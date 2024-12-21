$input a_position
$output v_color0

#include "common.sh"

uniform vec4 u_util;
uniform vec4 u_color;

void main() {
  float hMin       = u_util[0];
  float hMax       = u_util[1];
  float valueScale = u_util[2];

  gl_Position = mul(u_modelViewProj, vec4(a_position.xy, a_position.z * valueScale, 1.0));

  float factor = 0.0;
  if (hMax != hMin) {
    factor = (a_position.z - hMin) / (hMax - hMin);
  }
  v_color0 = vec4(factor * u_color.xyz, 1.0);
}
