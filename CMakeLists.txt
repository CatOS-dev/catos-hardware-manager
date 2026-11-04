cmake_minimum_required(VERSION 3.26)
project(CatOSHardwareManager VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Svg Concurrent LinguistTools)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(UDEV REQUIRED IMPORTED_TARGET libudev)

add_library(catos-hardware-core STATIC
    src/model/HardwareTypes.h
    src/model/ProfileSelection.h
    src/model/ProfileSelection.cpp
    src/services/DeviceDiscovery.h
    src/services/DeviceDiscovery.cpp
    src/services/ProfileCatalog.h
    src/services/ProfileCatalog.cpp
    src/services/HardwareService.h
    src/services/HardwareService.cpp
)
target_include_directories(catos-hardware-core PUBLIC src)
target_link_libraries(catos-hardware-core PUBLIC
    Qt6::Core yaml-cpp::yaml-cpp PkgConfig::UDEV
)

qt_add_executable(catos-hardware-manager
    src/app/Main.cpp
    src/app/HardwareManagerWindow.h
    src/app/HardwareManagerWindow.cpp
    src/pages/DevicesPage.h
    src/pages/DevicesPage.cpp
    src/pages/InstalledProfilesPage.h
    src/pages/InstalledProfilesPage.cpp
    src/pages/ProfileCatalogPage.h
    src/pages/ProfileCatalogPage.cpp
    src/widgets/HardwareUi.h
    src/widgets/HardwareUi.cpp
    src/widgets/DeviceRow.h
    src/widgets/DeviceRow.cpp
    src/widgets/ProfileRow.h
    src/widgets/ProfileRow.cpp
    src/dialogs/ApplyDialog.h
    src/dialogs/ApplyDialog.cpp
)
target_link_libraries(catos-hardware-manager PRIVATE
    catos-hardware-core Qt6::Widgets Qt6::Svg Qt6::Concurrent
)

qt_add_lrelease(catos-hardware-manager
    TS_FILES translations/catos-hardware-manager_zh_CN.ts
    QM_FILES_OUTPUT_VARIABLE QM_FILES
)

qt_add_executable(catos-hardware-apply src/privileged/HardwareApplyExecutor.cpp)
target_link_libraries(catos-hardware-apply PRIVATE catos-hardware-core)

install(TARGETS catos-hardware-manager RUNTIME DESTINATION bin)
install(TARGETS catos-hardware-apply RUNTIME DESTINATION lib/catos)
install(FILES data/catos-hardware-manager.desktop DESTINATION share/applications)
install(FILES ${QM_FILES} DESTINATION share/catos-hardware-manager/translations)
install(DIRECTORY data/profiles/ DESTINATION share/catos-hardware-manager/profiles
    FILES_MATCHING PATTERN "*.yaml"
)
