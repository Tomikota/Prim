cmake_minimum_required(VERSION 3.16)
project(Prim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS
    Core
    Gui
    Widgets
    Network
    Sql
    Multimedia
    REQUIRED
)

set(PROJECT_SOURCES
    src/main.cpp
    src/Prim.h
    src/Prim.cpp
    src/LoginDialog.h
    src/LoginDialog.cpp
    src/DatabaseManager.h
    src/DatabaseManager.cpp
    src/NetworkManager.h
    src/NetworkManager.cpp
    src/MessageModel.h
    src/MessageModel.cpp
    src/MediaManager.h
    src/MediaManager.cpp
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Multimedia
)

# Installation
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
