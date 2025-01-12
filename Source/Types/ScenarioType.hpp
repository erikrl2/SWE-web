#pragma once

enum class ScenarioType {
  None,
#ifndef NDEBUG
  Test,
#endif
#ifdef ENABLE_NETCDF
  Tsunami,
#endif
  ArtificialTsunami,
  Count
};
