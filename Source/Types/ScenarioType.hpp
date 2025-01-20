#pragma once

enum class ScenarioType {
  None,
#ifndef __EMSCRIPTEN__
  NetCDF,
#endif
  Tohoku,
  ArtificialTsunami,
  Test,
  Count
};
