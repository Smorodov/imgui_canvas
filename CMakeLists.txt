# ----------------------------------------------------------------------------
# 
# ----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.1...3.20)
project(imgui_canvas)
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

add_definitions(-DUNICODE -D_UNICODE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_DEBUG_POSTFIX _d)
add_definitions(-D_USE_MATH_DEFINES -D_CRT_SECURE_NO_WARNINGS -DWIN32 -D__STDC__)

add_subdirectory(glfw)
 
 add_definitions(-DUNICODE -D_UNICODE)
# set(CMAKE_DEBUG_POSTFIX _d)
 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT POLICY CMP0070 AND POLICY CMP0053)
  # Only set CMP0053 to OLD with CMake<3.10, otherwise there is a warning.
  cmake_policy(SET CMP0053 OLD)
endif()

if(POLICY CMP0071)
  cmake_policy(SET CMP0071 NEW)
endif()

#find_package(CGAL REQUIRED OPTIONAL_COMPONENTS Qt5)


#if(CGAL_Qt5_FOUND)
#  add_definitions(-DCGAL_USE_BASIC_VIEWER -DQT_NO_KEYWORDS)
#endif()

set(BLEND2D_DIR "${CMAKE_SOURCE_DIR}/blend2d" CACHE PATH "Location of 'blend2d'")

# -------
# Blend2D
# -------

set(BLEND2D_STATIC TRUE)
include("${BLEND2D_DIR}/CMakeLists.txt")


#----------------------------------------------------------------------------  
#  
# ----------------------------------------------------------------------------
SET(SOURCE
main/main.cpp
main/MainWindow.cpp
main/LogWindow.cpp
main/BLImageGL.cpp
main/ImGui2dCanvas.cpp
main/ImGui3dCanvas.cpp
glew-1.9.0/src/glew.c
common/shader.cpp

)

SET(IMGUI_SOURCE
			   #imgui/examples/libs/gl3w/GL/gl3w.c
			   imgui/backends/imgui_impl_glfw.cpp
			   imgui/backends/imgui_impl_opengl3.cpp
			   imgui/imgui.cpp
			   imgui/imgui_draw.cpp
			   imgui/imgui_tables.cpp
			   imgui/imgui_widgets.cpp			   
)
#----------------------------------------------------------------------------  
#  
# ----------------------------------------------------------------------------
SET(HEADERS
main/MainWindow.h
main/main.h
main/ImGui2dCanvas.h
main/ImGui3dCanvas.h
main/LogWindow.h
)
#----------------------------------------------------------------------------  
#  
# ----------------------------------------------------------------------------
file(GLOB ADD_HEADERS "./imgui/*.h*")
SET(HEADERS ${HEADERS} ${ADD_HEADERS})

SOURCE_GROUP("Source Files" FILES ${SOURCE})
SOURCE_GROUP("Header Files" FILES ${HEADERS})
# ----------------------------------------------------------------------------  
# 
# ----------------------------------------------------------------------------
  ADD_EXECUTABLE( ${PROJECT_NAME}
  				  ${SOURCE}
  				  ${IMGUI_SOURCE}
  				  ${HEADERS}
  				  )

#----------------------------------------------------------------------------  
#  
# ----------------------------------------------------------------------------
set(Boost_USE_STATIC_LIBS   ON)
set(Boost_USE_STATIC_RUNTIME   ON)
find_package(Boost 1.70 REQUIRED COMPONENTS system thread filesystem regex)
target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${Boost_LIBRARY_DIRS})
get_directory_property(_my_link_dirs LINK_DIRECTORIES)
message(STATUS "_my_link_dirs = ${_my_link_dirs}") 
# ----------------------------------------------------------------------------   
# 
# ----------------------------------------------------------------------------
INCLUDE_DIRECTORIES("./")
INCLUDE_DIRECTORIES("./common")
INCLUDE_DIRECTORIES("glm-0.9.4.0")
INCLUDE_DIRECTORIES("glew-1.9.0/include")

INCLUDE_DIRECTORIES("include")
INCLUDE_DIRECTORIES("imgui")
INCLUDE_DIRECTORIES("imgui/examples")
INCLUDE_DIRECTORIES("imgui/backends")
INCLUDE_DIRECTORIES("imgui/examples/libs/gl3w")
INCLUDE_DIRECTORIES("imgui/examples/libs/glfw/include")


# target_include_directories(${PROJECT_NAME} PRIVATE icu/source/common)
# ----------------------------------------------------------------------------
# 
# ----------------------------------------------------------------------------
TARGET_LINK_LIBRARIES(${PROJECT_NAME}
                      #CGAL::CGAL
                      Blend2D::Blend2D
					  optimized ${LIBRARY_OUTPUT_PATH}/Release/glfw3.lib
                      debug ${LIBRARY_OUTPUT_PATH}/Debug/glfw3_d.lib
					  opengl32.lib

					  )
set_target_properties( ${PROJECT_NAME} PROPERTIES  VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
# ----------------------------------------------------------------------------
# 
# ----------------------------------------------------------------------------
 file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/fonts DESTINATION ${CMAKE_BINARY_DIR}/Release)
 file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/fonts DESTINATION ${CMAKE_BINARY_DIR}/Debug)
 file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/StandardShadingRTT.vertexshader DESTINATION ${CMAKE_BINARY_DIR}/Release)
 file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/StandardShadingRTT.fragmentshader DESTINATION ${CMAKE_BINARY_DIR}/Release)
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/StandardShadingRTT.vertexshader DESTINATION ${CMAKE_BINARY_DIR}/Debug)
 file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/StandardShadingRTT.fragmentshader DESTINATION ${CMAKE_BINARY_DIR}/Debug)


