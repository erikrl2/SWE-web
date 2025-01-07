#pragma once

enum class ScenarioType {
  None,
#ifndef __EMSCRIPTEN__
  Tsunami, // Requires NetCDF which isn't available with Emscripten
#endif
  ArtificialTsunami,
  Test,
  Count
};
