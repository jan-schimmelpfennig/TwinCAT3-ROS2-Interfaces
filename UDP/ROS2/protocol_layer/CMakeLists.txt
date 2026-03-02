cmake_minimum_required(VERSION 3.8)
project(protocol_layer)

# Set compiler options
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()
set (CMAKE_CXX_STANDARD 20)

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(udp_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rosidl_typesupport_c REQUIRED)


# Create library for serialization utilities
add_library(protocol_utils SHARED
        src/message_serializer.cpp
        src/crc_calculator.cpp
)

# Specify include directories for the library
target_include_directories(protocol_utils PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

# Link dependencies to library
ament_target_dependencies(protocol_utils
        rclcpp
        std_msgs
)

# Set C++ standard
target_compile_features(protocol_utils PUBLIC cxx_std_17)

# UDP Protocol Node executable
add_executable(udp_protocol_node src/udp_protocol_node.cpp)

# Link the library to the executable
target_link_libraries(udp_protocol_node protocol_utils)

# Specify include directories for the executable
target_include_directories(udp_protocol_node PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

# Link dependencies to executable
ament_target_dependencies(udp_protocol_node
        rclcpp
        udp_msgs
)

# Set C++ standard
target_compile_features(udp_protocol_node PUBLIC cxx_std_17)

# Install library
install(TARGETS protocol_utils
        EXPORT export_${PROJECT_NAME}
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)

# Install executable
install(TARGETS udp_protocol_node
        DESTINATION lib/${PROJECT_NAME}
)

# Install header files
install(DIRECTORY include/
        DESTINATION include/${PROJECT_NAME}
)

# Install launch files (if they exist)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/launch)
  install(DIRECTORY launch/
          DESTINATION share/${PROJECT_NAME}/launch
  )
endif()

# Export dependencies
ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(rclcpp std_msgs)
ament_export_include_directories(include/${PROJECT_NAME})

# Testing
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()