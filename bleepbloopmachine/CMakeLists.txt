cmake_minimum_required(VERSION 3.0.0)
project(bleepbloopmachine CXX)
add_executable(bleepbloopmachine main.cc)
target_link_arduino_libraries(bleepbloopmachine AUTO_PUBLIC)
