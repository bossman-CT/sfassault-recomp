cmake_minimum_required(VERSION 3.20)
project(sfassault C CXX)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# Everything that is not specific to Star Fox Assault lives in gcn-recomp (a submodule):
# the recompiler, the GameCube runtime, the renderer and the frontends. See its README.
add_subdirectory(gcn-recomp)
gcn_enable_languages()

# SHA-1 of GF7E01's main.dol (disc revision 0), which the build checks the user's disc
# against. tools/extract_dol.py prints it, and docs/status.md records the disc's own hash.
set(SFASSAULT_DOL_SHA1 "ae7201e7acd5a29acc807d7668a856392fd37ff9")

if(SFASSAULT_DOL_SHA1 STREQUAL "" OR NOT EXISTS ${CMAKE_SOURCE_DIR}/analysis/symbols.txt)
  message(FATAL_ERROR
    "This project can't build yet: the DOL hash and analysis/symbols.txt come from the "
    "first analysis pass. See docs/status.md, milestone 0.")
endif()

gcn_add_game(
  NAME sfassault
  TITLE "Star Fox Assault"
  LABEL "sfassault-recomp"
  DISC_ID GF7E01
  DOL_SHA1 ${SFASSAULT_DOL_SHA1}
  SYMBOLS ${CMAKE_SOURCE_DIR}/analysis/symbols.txt
  TABLES ${CMAKE_SOURCE_DIR}/recomp
  # The game's native rate; checked against hardware in milestone 0.
  FPS 60)
