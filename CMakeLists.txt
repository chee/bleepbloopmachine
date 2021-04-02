cmake_minimum_required(VERSION 3.19.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
project(bleepbloopmachine CXX)

add_library(AudioSampleC src/kit/AudioSampleC.h src/kit/AudioSampleC.cpp)
add_library(AudioSampleK src/kit/AudioSampleK.h src/kit/AudioSampleK.cpp)
add_library(AudioSampleS src/kit/AudioSampleS.h src/kit/AudioSampleS.cpp)
add_library(AudioSampleH src/kit/AudioSampleH.h src/kit/AudioSampleH.cpp)
add_library(AudioSampleO src/kit/AudioSampleO.h src/kit/AudioSampleO.cpp)

target_link_arduino_libraries(AudioSampleK AUTO_PUBLIC)
target_link_arduino_libraries(AudioSampleC AUTO_PUBLIC)
target_link_arduino_libraries(AudioSampleS AUTO_PUBLIC)
target_link_arduino_libraries(AudioSampleH AUTO_PUBLIC)
target_link_arduino_libraries(AudioSampleO AUTO_PUBLIC)

add_executable(bleepbloopmachine src/main.h src/main.cc)
target_link_arduino_libraries(bleepbloopmachine AUTO_PUBLIC)

add_library(controls src/controls.h src/controls.cc)
target_link_arduino_libraries(controls AUTO_PUBLIC)

add_library(display src/display.h src/display.cc)
target_link_arduino_libraries(display AUTO_PUBLIC)

add_library(track src/track.h src/track.cc)
target_link_arduino_libraries(track AUTO_PUBLIC)

add_library(block src/block.h src/block.cc)
target_link_arduino_libraries(block AUTO_PUBLIC)

add_library(weirdmath src/weirdmath.h src/weirdmath.cc)
target_link_arduino_libraries(weirdmath AUTO_PUBLIC)

add_library(freq src/freq.h src/freq.cc)
target_link_arduino_libraries(freq AUTO_PUBLIC)

target_link_libraries(track PUBLIC
	AudioSampleK
	AudioSampleC
	AudioSampleS
	AudioSampleH
	AudioSampleO)

target_link_libraries(bleepbloopmachine PUBLIC
	controls
	track
	block
	weirdmath
	freq
)

target_enable_arduino_upload(bleepbloopmachine)
