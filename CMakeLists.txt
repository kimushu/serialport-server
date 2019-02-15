cmake_minimum_required(VERSION 3.1)
project(serialport-server)
add_executable(serialport-server main.cpp options.cpp)

if (CMAKE_HOST_WIN32)

#----------------------------------------------------------------
# for Windows

target_sources(serialport-server PUBLIC osport_win32.cpp)
target_link_libraries(serialport-server ws2_32)

elseif (CMAKE_HOST_UNIX)

#----------------------------------------------------------------
# for Linux/macOS
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
target_sources(serialport-server PUBLIC osport_unix.cpp)

if (CMAKE_HOST_APPLE)

#----------------------------------------------------------------
# for macOS
target_sources(serialport-server PUBLIC osport_macos.cpp)

else (CMAKE_HOST_APPLE)

#----------------------------------------------------------------
# for Linux
target_sources(serialport-server PUBLIC osport_linux.cpp)

endif (CMAKE_HOST_APPLE)
endif (CMAKE_HOST_WIN32)
