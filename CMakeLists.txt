cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

process_src_dir(${CMAKE_CURRENT_LIST_DIR}/basic ${PROJECT_NAME})
process_src_dir(${CMAKE_CURRENT_LIST_DIR}/gnss ${PROJECT_NAME})
process_src_dir(${CMAKE_CURRENT_LIST_DIR}/message ${PROJECT_NAME})
process_src_dir(${CMAKE_CURRENT_LIST_DIR}/tree_accessor ${PROJECT_NAME})
