#pragma once

enum class ScenarioType {
  None,
#ifdef ENABLE_NETCDF
  NetCDF,
#endif
  Tohoku,
  Chile,
  ArtificialTsunami,
#ifndef NDEBUG
  Test,
#endif
  Count
};
