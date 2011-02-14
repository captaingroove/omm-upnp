#SET(CMAKE_LIBRARY_PATH
#${CMAKE_SYSTEM_LIBRARY_PATH}
#)

FIND_PATH(LINUXDVB_INCLUDE_DIR
linux/dvb/frontend.h
PATHS ${CMAKE_INCLUDE_PATH}
)

FIND_PATH(POCO_INCLUDE_DIR
Poco/Poco.h
PATHS ${CMAKE_INCLUDE_PATH}
)

FIND_LIBRARY(LIB_POCO_FOUNDATION
NAME PocoFoundation
PATHS ${CMAKE_LIBRARY_PATH}
)

if(LINUXDVB_INCLUDE_DIR AND POCO_INCLUDE_DIR AND LIB_POCO_FOUNDATION)
message(STATUS "linux DVB headers found in " ${LINUXDVB_INCLUDE_DIR})
message(STATUS "Poco headers found in " ${POCO_INCLUDE_DIR})

INCLUDE_DIRECTORIES(
${LINUXDVB_INCLUDE_DIR}
${ommlib_SOURCE_DIR}/Omm
)

ADD_DEFINITIONS(
)

SET( OMM_DVB_LIB_SRCS
Dvb.cpp
)

INCLUDE_DIRECTORIES(
)

ADD_LIBRARY( ommdvb
${OMM_DVB_LIB_SRCS}
)

set(OMM_DVB
ommdvb
CACHE INTERNAL libommutil
)


TARGET_LINK_LIBRARIES( ommdvb
${LIB_POCO_FOUNDATION}
# ${LIB_POCO_UTIL}
)

INSTALL( TARGETS ommdvb
LIBRARY DESTINATION lib
ARCHIVE DESTINATION lib
)

else(LINUXDVB_INCLUDE_DIR AND POCO_INCLUDE_DIR AND LIB_POCO_FOUNDATION)
if(LINUXDVB_INCLUDE_DIR)
message(STATUS "Poco not found")
else(LINUXDVB_INCLUDE_DIR)
message(STATUS "linux DVB headers not found")
endif(LINUXDVB_INCLUDE_DIR)
endif(LINUXDVB_INCLUDE_DIR AND POCO_INCLUDE_DIR AND LIB_POCO_FOUNDATION)