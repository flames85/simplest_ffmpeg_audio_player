cmake_minimum_required(VERSION 3.7)
project(simplest_ffmpeg_audio_player)

set(CMAKE_CXX_STANDARD 11)

########################################
if($ENV{CLION_IDE})
    message(STATUS "Using clion ide:$ENV{CLION_IDE}")
    SET(FFMPEG_PATH "/opt/ffmpeg")

    SET(SDL_LIBRARY_DIR "/usr/lib/x86_64-linux-gnu/")
    SET(SDL_INCLUDE_DIR "/usr/include/")

endif()
########################################

# ffmpeg
if(DEFINED FFMPEG_PATH)
    set(FFMPEG_INCLUDE_DIR ${FFMPEG_PATH}/include)
    set(FFMPEG_LIBRARY_DIR ${FFMPEG_PATH}/lib)

    INCLUDE_DIRECTORIES(${FFMPEG_INCLUDE_DIR})
    LINK_DIRECTORIES(${FFMPEG_LIBRARY_DIR})
    set(FFMPEG_LIBRARIES
            avcodec
            avformat
            swresample
            avutil
            )

    message(STATUS "FFMPEG_INCLUDE_DIR: ${FFMPEG_INCLUDE_DIR}")
    message(STATUS "FFMPEG_LIBRARY_DIR: ${FFMPEG_LIBRARY_DIR}")
    message(STATUS "FFMPEG_LIBRARIES: ${FFMPEG_LIBRARIES}")
else()
    message(SEND_ERROR "Please set FFMPEG_PATH")
endif()


# SDL
if(DEFINED SDL_PATH)
    set(SDL_INCLUDE_DIR ${SDL_PATH}/include)
    set(SDL_LIBRARY_DIR ${SDL_PATH}/lib)

    INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
    LINK_DIRECTORIES(${SDL_LIBRARY_DIR})
    set(SDL_LIBRARIES SDL)

    message(STATUS "SDL_INCLUDE_DIR: ${SDL_INCLUDE_DIR}")
    message(STATUS "SDL_LIBRARY_DIR: ${SDL_LIBRARY_DIR}")
    message(STATUS "SDL_LIBRARIES: ${SDL_LIBRARIES}")
else()
    if(DEFINED SDL_INCLUDE_DIR AND SDL_LIBRARY_DIR)
        INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
        LINK_DIRECTORIES(${SDL_LIBRARY_DIR})
        set(SDL_LIBRARIES SDL)
        message(STATUS "SDL_INCLUDE_DIR: ${SDL_INCLUDE_DIR}")
        message(STATUS "SDL_LIBRARY_DIR: ${SDL_LIBRARY_DIR}")
        message(STATUS "SDL_LIBRARIES: ${SDL_LIBRARIES}")
    else()
        message(SEND_ERROR "Please set SDL_PATH or SDL_LIB_PATH SDL_HEAD_PATH")
    endif()
endif()


set(SOURCE_FILES main.cpp)

add_executable(simplest_ffmpeg_audio_player ${SOURCE_FILES})

target_link_libraries(simplest_ffmpeg_audio_player ${FFMPEG_LIBRARIES} ${SDL_LIBRARIES})
