cmake_minimum_required(VERSION 3.8)
project(el6695_ethercat_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

find_path(ETHERCAT_INCLUDE_DIR
        NAMES ecrt.h
        PATHS /usr/local/etherlab/include
)

find_library(ETHERCAT_LIBRARY
        NAMES ethercat
        PATHS /usr/local/etherlab/lib
)

if(NOT ETHERCAT_INCLUDE_DIR OR NOT ETHERCAT_LIBRARY)
  message(FATAL_ERROR "EtherCAT (ecrt.h) not found. Please install IgH EtherCAT Master.")
endif()

add_executable(ethercat_bridge
        src/el6695_slave.cpp
        src/bridge_node.cpp
        src/main.cpp
)

target_include_directories(ethercat_bridge PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
        ${ETHERCAT_INCLUDE_DIR}
)

target_compile_features(ethercat_bridge PUBLIC c_std_99 cxx_std_17)

target_link_libraries(ethercat_bridge
        ${ETHERCAT_LIBRARY}
)

ament_target_dependencies(ethercat_bridge
        rclcpp
        std_msgs
)

install(TARGETS ethercat_bridge
        DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

#test counter publisher executable
add_executable(counter_publisher
        src/counter_publisher.cpp
)

target_include_directories(counter_publisher PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

target_compile_features(counter_publisher PUBLIC c_std_99 cxx_std_17)

ament_target_dependencies(counter_publisher
        rclcpp
        std_msgs
)

install(TARGETS counter_publisher
        DESTINATION lib/${PROJECT_NAME}
)
# executable bool toggler
add_executable(bool_toggler
        src/bool_toggler.cpp
)

target_include_directories(bool_toggler PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

target_compile_features(bool_toggler PUBLIC c_std_99 cxx_std_17)

ament_target_dependencies(bool_toggler
        rclcpp
        std_msgs
)

install(TARGETS bool_toggler
        DESTINATION lib/${PROJECT_NAME}
)
ament_package()