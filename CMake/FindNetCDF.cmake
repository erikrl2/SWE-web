# - Find NetCDF
# Find the native NetCDF includes and library
#
#  NETCDF_INCLUDES    - where to find netcdf.h, etc
#  NETCDF_LIBRARIES   - Link these libraries when using NetCDF
#  NETCDF_FOUND       - True if NetCDF found including required interfaces (see below)

if(NETCDF_INCLUDES AND NETCDF_LIBRARIES)
  set(NETCDF_FIND_QUIETLY TRUE)
endif()

find_path(NETCDF_INCLUDES netcdf.h HINTS NETCDF_DIR ENV NETCDF_DIR)

find_library(NETCDF_LIBRARIES_C NAMES netcdf)

set(NetCDF_libs "${NETCDF_LIBRARIES_C}")

get_filename_component(NetCDF_lib_dirs "${NETCDF_LIBRARIES_C}" PATH)

find_path(NETCDF_INCLUDES_CXX NAMES netcdf HINTS "${NETCDF_INCLUDES}" NO_DEFAULT_PATH)
find_library(NETCDF_LIBRARIES_CXX NAMES netcdf_c++4 HINTS "${NetCDF_lib_dirs}" NO_DEFAULT_PATH)

if(NETCDF_INCLUDES_CXX AND NETCDF_LIBRARIES_CXX)
  list(INSERT NetCDF_libs 0 ${NETCDF_LIBRARIES_CXX})
else()
  message(STATUS "Failed to find NetCDF interface for CXX")
endif()

set(NETCDF_LIBRARIES "${NetCDF_libs}" CACHE STRING "All NetCDF libraries required for interface level")

# handle the QUIETLY and REQUIRED arguments and set NETCDF_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NetCDF DEFAULT_MSG NETCDF_LIBRARIES NETCDF_INCLUDES)
