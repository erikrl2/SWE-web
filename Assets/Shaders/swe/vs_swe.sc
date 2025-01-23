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

uint mod(uint a, uint b) {
  return a - b * (a / b);
}

vec4 getColor(float z, float scale, vec2 range, vec4 c1, vec4 c2, vec4 c3) {
  if (scale == 0.0) {
    return vec4(0.0);
  }
  if (range.x == range.y) {
    range.x -= 0.1;
    range.y += 0.1;
  }
  range *= scale;
  float t = clamp((z - range.x) / (range.y - range.x), 0.0, 1.0) * 2.0;
  if (t < 1.0) {
    return mix(c1, c2, t);
  } else {
    return mix(c2, c3, t - 1.0);
  }
}

void main() {
  uint  isDry         = uint(a_position);
  vec2  dataRangeWet  = u_dataRanges.xy;
  vec2  dataRangeDry  = u_dataRanges.zw;
  vec2  gridSize      = u_gridData.xy;
  vec2  cellSize      = u_gridData.zw;
  vec2  gridStart     = u_boundaryPos.xz + 0.5 * cellSize;
  vec2  zValueScale   = u_util.xy;
  uint  nx            = uint(gridSize.x);
  uint  id            = uint(gl_VertexID);

  vec2 gridPos  = vec2(mod(id, nx), id / nx);
  vec2 texCoord = (gridPos + 0.5) / gridSize;

  float z = texture2DLod(s_heightMap, texCoord, 0.0).r;

  if (isDry == 0u) {
    z *= zValueScale.x;
    v_color0 = getColor(z, zValueScale.x, dataRangeWet, u_color1, u_color2, u_color3);
  } else {
    z *= zValueScale.y;
    vec4 c1  = vec4(0.15, 0.46, 0.42, 1.0);
    vec4 c2  = vec4(0.62, 0.63, 0.63, 1.0);
    vec4 c3  = vec4(0.67, 0.19, 0.07, 1.0);
    v_color0 = getColor(z, zValueScale.y, dataRangeDry, c1, c2, c3);
  }

  vec3 worldPos = vec3(gridStart + gridPos * cellSize, z);

  gl_Position = mul(u_modelViewProj, vec4(worldPos, 1.0));
}
