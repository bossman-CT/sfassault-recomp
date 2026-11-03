cmake_minimum_required(VERSION 3.20)
project(sfassault C CXX)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# Everything that is not specific to Star Fox Assault lives in gcn-recomp (a submodule):
# the recompiler, the GameCube runtime, the renderer and the frontends. See its README.
add_subdirectory(gcn-recomp)
gcn_enable_languages()

# The game keeps much of its code in relocatable modules on the disc, which it links at
# run time with OSLink. Each is recompiled at a canonical address (gcn-recomp docs/rels.md),
# using its symbols in analysis/rels/<module>.txt.
#
# m7, m8 and m009 all have module ID 11, which gcn-recomp's linker rejects. Until it can
# take several modules per ID (issue #22), only m8 is built: mb02 and mb10a import from it.
set(SFASSAULT_RELS
  md0 md1 md2 m0 m1 m10 m2 m3 m4 m5 m6 m8 m9
  mb01a mb01b mb02 mb05 mb10a mbg0 mbg1 mbg2)
list(TRANSFORM SFASSAULT_RELS APPEND .rel OUTPUT_VARIABLE SFASSAULT_REL_FILES)

gcn_add_game(
  NAME sfassault
  TITLE "Star Fox Assault"
  LABEL "sfassault-recomp"
  DISC_ID GF7E01
  # main.dol of GF7E01 revision 0 (disc SHA-1 065341b8575adea00fd0a75e389f135d32840423)
  DOL_SHA1 ae7201e7acd5a29acc807d7668a856392fd37ff9
  SYMBOLS ${CMAKE_SOURCE_DIR}/analysis/symbols.txt
  TABLES ${CMAKE_SOURCE_DIR}/recomp
  RELS ${SFASSAULT_REL_FILES}
  REL_SYMBOLS ${CMAKE_SOURCE_DIR}/analysis/rels
  # The game's native rate; to be confirmed per mode (issue #7).
  FPS 60)
