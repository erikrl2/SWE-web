$output v_color0

#include "common.sh"

uniform vec4 u_gridData;
uniform vec4 u_boundaryPos;
uniform vec4 u_util;
uniform vec4 u_color;

SAMPLER2D(u_heightMap, 0);

uint mod(uint a, uint b) {
  return a - b * (a / b);
}

void main() {
  vec2  dataRange  = u_util.xy;
  vec2  gridSize   = u_gridData.xy;
  vec2  cellSize   = u_gridData.zw;
  vec2  gridStart  = u_boundaryPos.xz + 0.5 * cellSize;
  float valueScale = u_util.z;
  uint  nx         = uint(gridSize.x);
  uint  id         = uint(gl_VertexID);

  vec2 gridPos  = vec2(mod(id, nx), id / nx);
  vec2 texCoord = (gridPos + 0.5) / gridSize;

  float height = texture2DLod(u_heightMap, texCoord, 0.0).r * valueScale;

  vec3 worldPos = vec3(gridStart + gridPos * cellSize, height);

  gl_Position = mul(u_modelViewProj, vec4(worldPos, 1.0));

  if (valueScale != 0.0) {
    if (dataRange.x == dataRange.y) {
      dataRange.x -= 0.1;
      dataRange.y += 0.1;
    }
    dataRange *= valueScale;
    float colFactor = (worldPos.z - dataRange.x) / (dataRange.y - dataRange.x);
    v_color0 = vec4(colFactor * u_color.xyz, 1.0);
  }
}
