#CMakeLists.txt for Trill Transformation Tool

cmake_minimum_required(VERSION 3.10)
project(TrillTransformation VERSION 1.0)

#Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Source files
set(SOURCES
    TrillTransformation.cpp
    main.cpp
)

#Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

#Platform-specific settings
if(WIN32)
    # Windows-specific settings
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_WINDOWS)
    target_link_libraries(${PROJECT_NAME} PRIVATE comctl32)
elseif(UNIX AND NOT APPLE)
    # Linux-specific settings
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_LINUX)

    # Find X11
    find_package(X11 REQUIRED)
    if(X11_FOUND)
        target_include_directories(${PROJECT_NAME} PRIVATE ${X11_INCLUDE_DIR})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES})
    endif()
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

#Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#Install target
install(TARGETS ${PROJECT_NAME} DESTINATION bin)