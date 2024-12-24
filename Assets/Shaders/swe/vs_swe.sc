$output v_color0

#include "common.sh"

uniform vec4 u_gridData;
uniform vec4 u_boundaryPos;
uniform vec4 u_util;
uniform vec4 u_color;

uniform sampler2D u_heightMap;

int mod(int a, int b) {
    return a - b * (a / b);
}

void main() {
  vec2  dataRange  = u_util.xy;
  vec2  gridSize   = u_gridData.xy;
  vec2  cellSize   = u_gridData.zw;
  vec2  gridStart  = u_boundaryPos.xz + 0.5 * cellSize;
  float valueScale = u_util.z;
  int   nx         = int(gridSize.x);

  vec2 gridPos  = vec2(mod(gl_VertexID, nx), gl_VertexID / nx);
  vec3 worldPos = vec3(gridStart + gridPos * cellSize, texture(u_heightMap, gridPos / gridSize).r);

  gl_Position = mul(u_modelViewProj, vec4(worldPos.xy, worldPos.z * valueScale, 1.0));

  float colFactor = 0.0;
  if (dataRange.x != dataRange.y) {
    colFactor = (worldPos.z - dataRange.x) / (dataRange.y - dataRange.x);
  }
  v_color0 = vec4(colFactor * u_color.xyz, 1.0);
}
