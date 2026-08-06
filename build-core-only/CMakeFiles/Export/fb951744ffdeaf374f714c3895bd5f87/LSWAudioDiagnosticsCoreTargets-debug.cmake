#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "LSW::AudioDiagnostics" for configuration "Debug"
set_property(TARGET LSW::AudioDiagnostics APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(LSW::AudioDiagnostics PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/lsw_audio_diagnostics.lib"
  )

list(APPEND _cmake_import_check_targets LSW::AudioDiagnostics )
list(APPEND _cmake_import_check_files_for_LSW::AudioDiagnostics "${_IMPORT_PREFIX}/lib/lsw_audio_diagnostics.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
