cmake_minimum_required(VERSION 3.25)
project(OSCMessenger)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_SHARED_MODULE_PREFIX "")

if(APPLE OR WIN32)
    set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

add_library(OSCMessenger MODULE src/OSCMessenger.cpp)

target_include_directories(OSCMessenger PUBLIC ${SC_SRC_PATH}/include/plugin_interface)
target_include_directories(OSCMessenger PUBLIC ${SC_SRC_PATH}/include/common)
target_include_directories(OSCMessenger PUBLIC ${SC_SRC_PATH}/common)

# external libs
target_include_directories(OSCMessenger PUBLIC ${PROJECT_SOURCE_DIR}/external/oscpp/include)
target_include_directories(OSCMessenger PUBLIC ${PROJECT_SOURCE_DIR}/external/asio/asio/include)

if(NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/install)
endif()

install(TARGETS OSCMessenger LIBRARY DESTINATION ${PROJECT_NAME})
install(FILES src/OSCMessenger.sc DESTINATION ${PROJECT_NAME}/Classes)
