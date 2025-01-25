$input  a_position // works as "isDry" bool
$output v_color0

#include "common.sh"

uniform vec4 u_gridData;
uniform vec4 u_boundaryPos;
uniform vec4 u_dataRanges;
uniform vec4 u_util;
uniform vec4 u_color1;
uniform vec4 u_color2;
uniform vec4 u_color3;

SAMPLER2D(s_heightMap, 0);

#define id uint(gl_VertexID)
#define nx uint(u_gridData.x)

#define gridSize   u_gridData.xy
#define cellSize   u_gridData.zw
#define gridOrigin u_boundaryPos.xz

#define dataRangeWet u_dataRanges.xy
#define dataRangeDry u_dataRanges.zw
#define wetScale     u_util.x
#define dryScale     u_util.y

#define isDry (a_position == 1.0)

#define darkGreen vec4(0.15, 0.46, 0.42, 1.0)
#define lightGrey vec4(0.62, 0.63, 0.63, 1.0)
#define darkRed   vec4(0.67, 0.19, 0.07, 1.0)

uint mod(uint a, uint b) {
  return a - b * (a / b);
}

vec4 getColor(float z, vec2 range, vec4 c1, vec4 c2, vec4 c3) {
  float t = clamp((z - range.x) / (range.y - range.x), 0.0, 1.0) * 2.0;
  if (t < 1.0) {
    return mix(c1, c2, t);
  } else {
    return mix(c2, c3, t - 1.0);
  }
}

void main() {
  vec2 gridPos = vec2(mod(id, nx), id / nx) + 0.5;

  float z = texture2DLod(s_heightMap, gridPos / gridSize, 0.0).r;

  if (!isDry) {
    v_color0 = getColor(z, dataRangeWet, u_color1, u_color2, u_color3);
    z *= wetScale;
  } else {
    v_color0 = getColor(z, dataRangeDry, darkGreen, lightGrey, darkRed);
    z *= dryScale;
  }

  gl_Position = mul(u_modelViewProj, vec4(gridOrigin + gridPos * cellSize, z, 1.0));
}
