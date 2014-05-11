#--------------------------------------------------------
# The CMakeLists.txt for:                  pFrontEstimate
# Author(s):                               Henrik Schmidt
#--------------------------------------------------------

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(pFrontEstimateX ${SRC})
   
TARGET_LINK_LIBRARIES(pFrontEstimateX
   ${MOOS_LIBRARIES}
   mbutil
   m
   pthread
   henrik_util
)
