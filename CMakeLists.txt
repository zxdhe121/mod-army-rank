AC_ADD_SCRIPT("${CMAKE_CURRENT_LIST_DIR}/src/ArmyRank.cpp")
AC_ADD_SCRIPT_LOADER("ArmyRank" "${CMAKE_CURRENT_LIST_DIR}/src/ArmyRank_loader.h")

AC_ADD_CONFIG_FILE("${CMAKE_CURRENT_LIST_DIR}/conf/armyrank.conf.dist")