#pragma once

enum class ScenarioType {
  None,
#ifdef ENABLE_NETCDF
  Tsunami, // Requires NetCDF which isn't available with Emscripten
#endif
  ArtificialTsunami,
  Test,
  Count
};
