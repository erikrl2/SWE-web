#pragma once

enum class ScenarioType {
  None,
#ifdef ENABLE_NETCDF
  NetCDF,
#endif
  Tohoku,
  ArtificialTsunami,
#ifndef NDEBUG
  Test,
#endif
  Count
};
