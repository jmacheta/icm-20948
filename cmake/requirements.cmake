include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)

if (ICM20948_TESTS)
  cpmaddpackage("gh:google/googletest#main")
endif ()
