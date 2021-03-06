# Project definition
cmake_minimum_required(VERSION 3.1)
project(open_gl_test)

# Source files
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(SOURCES "${SRC_DIR}/main.cpp" src/shader.h src/camera.h
        src/drawer.h src/drawer2.h src/drawer3.h src/render_video_opengl2.h)
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libraries")

# Executable definition and properties
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "${SRC_DIR}")
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)

# GLFW
set(GLFW_DIR "${LIB_DIR}/glfw")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
add_subdirectory("${GLFW_DIR}")
#target_link_libraries(${PROJECT_NAME} "glfw" "${GLFW_LIBRARIES}")
target_include_directories(${PROJECT_NAME} PRIVATE "${GLFW_DIR}/include")
target_compile_definitions(${PROJECT_NAME} PRIVATE "GLFW_INCLUDE_NONE")

# glad
#set(GLAD_DIR "${LIB_DIR}/glad")
#add_library("glad" "${GLAD_DIR}/src/glad.c")
#target_include_directories("glad" PRIVATE "${GLAD_DIR}/include")
#target_include_directories(${PROJECT_NAME} PRIVATE "${GLAD_DIR}/include")
#target_link_libraries(${PROJECT_NAME} "glad" "${CMAKE_DL_LIBS}")

add_library(STB_IMAGE "src/stb_image.cpp" src/drawer2.h)
#target_link_libraries(${PROJECT_NAME} STB_IMAGE)

# OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIR})
link_libraries(${OpenCV_LIBS})
#target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS})

# openvslam
find_package(openvslam REQUIRED)
include_directories(${openvslam_INCLUDE_DIR})
link_libraries(${openvslam_LIBS})

include_directories(/usr/include/eigen3)
include_directories(/usr/local/include/openvslam/3rd/json/include)

find_package(DBoW2 REQUIRED)
set(BoW_INCLUDE_DIR ${DBoW2_INCLUDE_DIRS})
set(BoW_LIBRARY ${DBoW2_LIBS})
target_compile_definitions(${PROJECT_NAME} PUBLIC USE_DBOW2)

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})
link_libraries(${GLEW_LIBRARIES})

find_package(glm REQUIRED)
include_directories(${glm_INCLUDE_DIRS})
link_libraries(${glm_LIBRARIES})


target_link_libraries(${PROJECT_NAME}
        PUBLIC
        "glfw" "${GLFW_LIBRARIES}"
#        "glad" "${CMAKE_DL_LIBS}"
        STB_IMAGE
        Threads::Threads
        Eigen3::Eigen
        yaml-cpp
        opencv_core
        opencv_features2d
        opencv_calib3d
        ${OpenCV_LIBS}
        ${openvslam_LIBS}
        openvslam
        ${BoW_LIBRARY}
#        GLEW::GLEW
        ${GLEW_LIBRARIES}
        ${OPENGL_LIBRARIES}
        ${glm_LIBRARIES}
        #        g2o::core
        #        g2o::stuff
        #        g2o::types_sba
        #        g2o::types_sim3
        #        g2o::solver_dense
        #        g2o::solver_eigen
        #        g2o::solver_csparse
        #        g2o::csparse_extension
        #        ${CXSPARSE_LIBRARIES}
        #        ${SUITESPARSE_LIBRARIES}
#        PRIVATE
#        ${LAPACK_LIBRARIES}
        )
