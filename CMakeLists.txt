cmake_minimum_required(VERSION 2.6)
PROJECT(print-service C CXX)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION_MAJOR 1)
SET(VERSION "${VERSION_MAJOR}.2.7")

message(cmake install prefix : ${CMAKE_INSTALL_PREFIX})

FILE(GLOB SRCS src/*.c)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

INCLUDE(FindPkgConfig)
#pkg_check_modules(utils_pkgs REQUIRED ecore eina avahi cups dlog vconf capi-appfw-application)
pkg_check_modules(utils_pkgs REQUIRED ecore eina dlog vconf capi-appfw-application)

FOREACH(flag ${utils_pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag} -fdata-sections -ffunction-sections -Wl,--gc-sections")
ENDFOREACH(flag)

LINK_DIRECTORIES(/usr/lib)

SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -g -fno-omit-frame-pointer -finstrument-functions")
#SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -DEXPORT_API=\"\" ")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -I/usr/include/cups/")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CFLAGS}")
MESSAGE("FLAGS: ${CMAKE_C_FLAGS}")

ADD_DEFINITIONS("-DPREFIX=\"${PREFIX}\"")
ADD_DEFINITIONS("-DFACTORYFS=\"$ENV{FACTORYFS}\"")
ADD_DEFINITIONS("-DDATAFS=\"$ENV{DATAFS}\"")
ADD_DEFINITIONS("-Wall") #enable all warnings

ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS})  
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES SOVERSION ${VERSION_MAJOR})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES VERSION ${VERSION})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS} -lcups)

CONFIGURE_FILE(${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc @ONLY)

MESSAGE("destination: ${DESTINATION}")
INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib )
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/pt_api.h DESTINATION "include/print-service")

FILE(GLOB samsung_cms "${CMAKE_CURRENT_SOURCE_DIR}/data/samsung/cms/*.cts" )
INSTALL(FILES ${samsung_cms} DESTINATION /usr/share/cups/ppd/samsung/cms/)
MESSAGE("destination: ${DESTINATION}")

FILE(GLOB hp_ppds "${CMAKE_CURRENT_SOURCE_DIR}/data/hp/*.drv.gz" )
INSTALL(FILES ${hp_ppds} DESTINATION /usr/share/cups/ppd/hp/)
FILE(GLOB samsung_ppds "${CMAKE_CURRENT_SOURCE_DIR}/data/samsung/*.drv.gz" )
INSTALL(FILES ${samsung_ppds} DESTINATION /usr/share/cups/ppd/samsung/)
FILE(GLOB epson_ppds "${CMAKE_CURRENT_SOURCE_DIR}/data/epson/*.drv.gz" )
INSTALL(FILES ${epson_ppds} DESTINATION /usr/share/cups/ppd/epson/)

ADD_SUBDIRECTORY(libptdb)
ADD_SUBDIRECTORY(TC)
