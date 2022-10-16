#
# TrayWeather CMake configuration.
#
project(TrayWeather)

cmake_minimum_required (VERSION 3.0.0)

# Version Number
set (TRAYWEATHER_VERSION_MAJOR 1)
set (TRAYWEATHER_VERSION_MINOR 26)
set (TRAYWEATHER_VERSION_PATCH 1)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Current year, used in configured files. 
string(TIMESTAMP CURRENT_YEAR "%Y")

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Find the Qt5 library
find_package(Qt5 COMPONENTS Widgets Network WebkitWidgets Charts LinguistTools)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-deprecated -mwindows -m64")

if (CMAKE_BUILD_TYPE MATCHES Debug)
  set(CORE_EXTERNAL_LIBS ${CORE_EXTERNAL_LIBS} ${QT_QTTEST_LIBRARY})
endif (CMAKE_BUILD_TYPE MATCHES Debug)

set(CHARTS_INCLUDE_DIR "D:/Desarrollo/Bibliotecas/qtcharts-dev/src")
set(CHARTS_INCLUDE_DIRS
  "${CHARTS_INCLUDE_DIR}" 
  "${CHARTS_INCLUDE_DIR}/charts"
  "${CHARTS_INCLUDE_DIR}/charts/animations"
  "${CHARTS_INCLUDE_DIR}/charts/areachart"
  "${CHARTS_INCLUDE_DIR}/charts/axis"
  "${CHARTS_INCLUDE_DIR}/axis/barcategoryaxis"
  "${CHARTS_INCLUDE_DIR}/charts/axis/categoryaxis"
  "${CHARTS_INCLUDE_DIR}/charts/axis/datetimeaxis"
  "${CHARTS_INCLUDE_DIR}/charts/axis/logvalueaxis"
  "${CHARTS_INCLUDE_DIR}/charts/axis/valueaxis"
  "${CHARTS_INCLUDE_DIR}/charts/barchart"
  "${CHARTS_INCLUDE_DIR}/charts/barchart/vertical"
  "${CHARTS_INCLUDE_DIR}/charts/barchart/vertical/bar"
  "${CHARTS_INCLUDE_DIR}/charts/barchart/vertical/percent"
  "${CHARTS_INCLUDE_DIR}/charts/barchart/vertical/stacked"
  "${CHARTS_INCLUDE_DIR}/charts/barchart/horizontal"
  "${CHARTS_INCLUDE_DIR}/charts/barchart/horizontal/bar"
  "${CHARTS_INCLUDE_DIR}/charts/barchart/horizontal/percent"
  "${CHARTS_INCLUDE_DIR}/charts/barchart/horizontal/stacked"
  "${CHARTS_INCLUDE_DIR}/charts/boxplotchart"
  "${CHARTS_INCLUDE_DIR}/charts/domain"
  "${CHARTS_INCLUDE_DIR}/charts/candlestickchart"
  "${CHARTS_INCLUDE_DIR}/charts/layout"
  "${CHARTS_INCLUDE_DIR}/charts/legend"
  "${CHARTS_INCLUDE_DIR}/charts/linechart"
  "${CHARTS_INCLUDE_DIR}/charts/piechart"
  "${CHARTS_INCLUDE_DIR}/charts/scatterchart"
  "${CHARTS_INCLUDE_DIR}/charts/splinechart"
  "${CHARTS_INCLUDE_DIR}/charts/themes"
  "${CHARTS_INCLUDE_DIR}/charts/xychart"
)

if(DEFINED MINGW)
  configure_file("${PROJECT_SOURCE_DIR}/resources.rc.in" "${PROJECT_BINARY_DIR}/resources.rc")
  configure_file("${PROJECT_SOURCE_DIR}/installer/script.iss.in" "${PROJECT_BINARY_DIR}/script.iss")
  set(CORE_SOURCES ${CORE_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/resources.rc)
  set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -o <OBJECT> -i <SOURCE>")
  enable_language(RC)
endif(DEFINED MINGW)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}          # Generated .h files
  ${CMAKE_CURRENT_BINARY_DIR}  # For wrap/ui files
  ${Qt5Widgets_INCLUDE_DIRS}
  ${Qt5Multimedia_INCLUDE_DIRS}
  ${Qt5WebKitWidgets_INCLUDE_DIRS}
  ${CHARTS_INCLUDE_DIRS}
  )

set(TRANSLATIONS_TS_FILES
  languages/es_ES.ts
  languages/ru_RU.ts
  languages/de_DE.ts
  languages/fr_FR.ts
  languages/zh_CN.ts
  languages/pt_BR.ts
  languages/uk_UA.ts
  languages/sl_SI.ts
  languages/ko_KR.ts  
  languages/pl_PL.ts
  languages/tr_TR.ts
)

qt5_add_translation(TRANSLATIONS_QM_FILES ${TRANSLATIONS_TS_FILES})
add_custom_target(translations DEPENDS ${TRANSLATIONS_QM_FILES})
configure_file(languages/translations.qrc ${CMAKE_BINARY_DIR} COPYONLY)

# Add Qt Resource files
qt5_add_resources(RESOURCES
  rsc/resources.qrc
  rsc/darktheme/style.qrc
  ${CMAKE_BINARY_DIR}/translations.qrc
)

qt5_wrap_ui(CORE_UI
  # .ui for Qt
  ConfigurationDialog.ui
  AboutDialog.ui
  WeatherDialog.ui
  WeatherWidget.ui
  PollutionWidget.ui
  UVWidget.ui
  AlertDialog.ui
  ErrorWidget.ui
)
	
set (CORE_SOURCES
  # project files
  ${CORE_SOURCES}
  ${RESOURCES}
  ${CORE_UI}
  ${TRANSLATIONS_QM_FILES}
  Main.cpp
  Utils.cpp
  AboutDialog.cpp
  ConfigurationDialog.cpp
  TrayWeather.cpp
  WeatherDialog.cpp
  WeatherWidget.cpp
  PollutionWidget.cpp
  UVWidget.cpp
  AlertDialog.cpp
)

set(CORE_EXTERNAL_LIBS
  Qt5::Charts
  Qt5::Widgets
  Qt5::Network
  Qt5::WebKitWidgets
  advapi32
)
 
add_executable(TrayWeather ${CORE_SOURCES})
target_link_libraries (TrayWeather ${CORE_EXTERNAL_LIBS})
qt5_use_modules(TrayWeather Widgets Network WebKitWidgets Charts)
