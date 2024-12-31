cmake_minimum_required(VERSION 3.31.3)

project(how_to_use_onetbb LANGUAGES CXX)

set(
  examples
  parallel_for
  parallel_reduce
  parallel_reduce_rvalue
  parallel_pipeline
)
find_package(TBB)
foreach(example ${examples})
  add_executable(${example} ${example}.cpp)
  target_link_libraries(${example} PRIVATE TBB::tbb)
  target_compile_options(${example} PRIVATE -std=c++20)
endforeach()
