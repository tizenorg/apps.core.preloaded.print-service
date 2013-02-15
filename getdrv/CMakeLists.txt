CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(getdrv C)
SET(PACKAGE_NAME print-service)

SET(PREFIX /usr)
SET(BINDIR "${PREFIX}/bin")

SET(SRCS
	main.c
   	stack.c
	main.h
)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

SET(CMAKE_C_FLAGS "-pipe -g2 -std=c99 -Wfatal-errors -Wextra -Wall -Werror -Wunused-macros -Wunreachable-code -Wformat=2 -fdata-sections -ffunction-sections -fno-show-column -I.")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_C_FLAGS_RELEASE "-O2")

IF("${ARCH}" STREQUAL "arm")
	ADD_DEFINITIONS("-DTARGET")
	MESSAGE("add -DTARGET")
ENDIF("${ARCH}" STREQUAL "arm")

ADD_DEFINITIONS("-DPREFIX=\"${PREFIX}\"")

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${getdrv_LDFLAGS})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})

