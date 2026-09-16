cmake_minimum_required(VERSION 3.21)

project(git-shepherd
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(git-shepherd
    WIN32
    main.cpp
    src/AppSettings.cpp
    src/AppSettings.h
    src/GitRunner.cpp
    src/GitRunner.h
    src/RepoScanner.cpp
    src/RepoScanner.h
    src/RepoModel.cpp
    src/RepoModel.h
    src/RepoFilterProxy.cpp
    src/RepoFilterProxy.h
)

set_source_files_properties(qml/GitShepherd/Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(git-shepherd
    URI GitShepherd
    QML_FILES
        qml/GitShepherd/Main.qml
        qml/GitShepherd/Theme.qml
        qml/GitShepherd/AppButton.qml
        qml/GitShepherd/StatusBadge.qml
        qml/GitShepherd/RepoCard.qml
        qml/GitShepherd/BranchMenu.qml
        qml/GitShepherd/ScanDialog.qml
        qml/GitShepherd/SettingsMenu.qml
)

target_include_directories(git-shepherd PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(git-shepherd PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
)
