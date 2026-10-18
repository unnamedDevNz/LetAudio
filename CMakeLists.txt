cmake_minimum_required(VERSION 3.28)
project(LetAudio LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

include(FetchContent)
include(GenerateExportHeader)

find_package(OpenAL CONFIG REQUIRED)
find_package(SndFile CONFIG REQUIRED)

add_library(LetAudio SHARED
    src/LetAudio.cpp
)

generate_export_header(LetAudio EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/LetAudio_export.h)
target_link_libraries(LetAudio PRIVATE
    OpenAL::OpenAL
    SndFile::sndfile
)
target_include_directories(LetAudio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR}/include)