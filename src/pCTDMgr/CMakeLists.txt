#--------------------------------------------------------
# The CMakeLists.txt for:                       pCTDMgr
# Author(s):                              Nick Rypkema
#--------------------------------------------------------

# FILE(GLOB SRC *.cpp)

SET(SRC
  CTDMgr.cpp
  CTDMgr_Info.cpp
  main.cpp
)



ADD_EXECUTABLE(pCTDMgr ${SRC})

TARGET_LINK_LIBRARIES(pCTDMgr
   ${MOOS_LIBRARIES}
   ufield
   geometry
   mbutil
   m
   pthread)

