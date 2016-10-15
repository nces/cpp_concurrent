project(cpp_concurrent)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -stdlib=libc++ -pthread")

find_package(Threads REQUIRED)

add_executable(channels channels.cpp)
target_link_libraries(channels ${CMAKE_THREAD_LIBS_INIT})

add_executable(http_req http_req.cpp)
target_link_libraries(http_req curlpp curl ${CMAKE_THREAD_LIBS_INIT})
