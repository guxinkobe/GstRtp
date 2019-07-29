PROJECT(RtpStream)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

set(CMAKE_VERBOSE_MAKEFILE on)

if (NATIVE_ATLAS7)
    set(CMAKE_TOOLCHAIN_FILE /home/guxin/Project/MediaStream/toolchain-A7.cmake)
    include(/home/guxin/Project/MediaStream/toolchain-A7.cmake)
endif (NATIVE_ATLAS7)

if (NATIVE_IMX6)
    set(CMAKE_TOOLCHAIN_FILE /home/guxin/Project/MediaStream/toolchain-imx6.cmake)
    include(/home/guxin/Project/MediaStream/toolchain-imx6.cmake)
endif (NATIVE_IMX6)

if (NATIVE_IMX8)
    set(CMAKE_TOOLCHAIN_FILE /home/guxin/Project/MediaStream/toolchain-imx8.cmake)
    include(/home/guxin/Project/MediaStream/toolchain-imx8.cmake)
endif (NATIVE_IMX8)

if (UBUNTU)
    set(CMAKE_TOOLCHAIN_FILE /home/guxin/Project/toolchain-ubuntu.cmake)
    include(/home/guxin/Project/toolchain-ubuntu.cmake)
endif (UBUNTU)

INCLUDE_DIRECTORIES(${SYSROOT_DIR}/usr/include/gstreamer-1.0
                    ${SYSROOT_DIR}/usr/lib/gstreamer-1.0/include
                    ${SYSROOT_DIR}/usr/include/glib-2.0
                    ${SYSROOT_DIR}/usr/lib/glib-2.0/include
                    ${SYSROOT_DIR}/usr/include/libxml2
                    ${SYSROOT_DIR}/usr/src/kernel/include/uapi
                    ${SYSROOT_DIR}/usr/src/kernel/include)

LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib)

ADD_EXECUTABLE(RtpClientTest main.cpp RtpVideo.h JenDebug.h)
TARGET_LINK_LIBRARIES(RtpClientTest RtpStream gstreamer-1.0 pthread gobject-2.0 glib-2.0 gthread-2.0 gmodule-2.0 xml2 z)




