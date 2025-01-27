#pragma once

enum class ScenarioType {
  None,
#ifdef ENABLE_NETCDF
  NetCDF,
#endif
  Tohoku,
  TohokuZoomed,
  Chile,
  ArtificialTsunami,
#ifndef NDEBUG
  Test,
#endif
  Count
};
