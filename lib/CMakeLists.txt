# Copyright: (C) 2016 Social Robotics Lab - Yale University
# Authors: Álvaro Castro González, Olivier Mangin, Alessandro Roncone
# CopyPolicy: Released under the terms of the GNU GPL v3.0.

cmake_minimum_required(VERSION 2.8.3)
project(baxterControl)

# find_package(Qt4 REQUIRED COMPONENTS QtCore QtGui)
# find_package(OpenCV REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${baxter_collaboration_INCLUDE_DIRS})

## Declare a C++ library
# add_library(robot_utils     include/robot_interface/utils.h
#                             src/robot_interface/utils.cpp)

# add_library(baxter_interface include/baxter_interface/ros_thread.h
#                             include/baxter_interface/robot_interface.h
#                             include/baxter_interface/arm_ctrl.h
#                             include/baxter_interface/utils.h
#                             src/baxter_interface/ros_thread.cpp
#                             src/baxter_interface/baxter_trac_ik.cpp
#                             src/baxter_interface/utils.cpp
#                             src/baxter_interface/robot_interface.cpp
#                             src/baxter_interface/arm_ctrl.cpp)

add_library(baxter_interface include/baxter_interface/arm_ctrl.h
                            src/baxter_interface/arm_ctrl.cpp)

## Add cmake target dependencies of the library
## as an example, code may need to be generated before libraries
## either from message generation or dynamic reconfigure
# add_dependencies(robot_utils            ${catkin_EXPORTED_TARGETS})
add_dependencies(baxter_interface        ${catkin_EXPORTED_TARGETS})

## Specify libraries to link a library or executable target against
target_link_libraries(baxter_interface   ${catkin_LIBRARIES})

## Mark libraries for installation
install (TARGETS baxter_interface
         ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
         LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
)

# ## Mark cpp header files for installation
install(DIRECTORY   ${PROJECT_SOURCE_DIR}/include/baxter_interface
        DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
        FILES_MATCHING PATTERN "*.h"
)
