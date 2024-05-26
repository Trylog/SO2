cmake_minimum_required(VERSION 3.26)
project(SO2)

set(CMAKE_CXX_STANDARD 23)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(GLUT REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

add_executable(SO2 main.cpp)
target_link_libraries(SO2 PRIVATE glfw)
target_link_libraries(SO2 PRIVATE glad::glad)
target_link_libraries(SO2 PRIVATE GLUT::GLUT)
target_link_libraries(SO2
        PRIVATE
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)




