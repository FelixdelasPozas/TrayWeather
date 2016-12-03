Tray Weather
============

# Summary
- [Description](#description)
- [Compilation](#compilation-requirements)
- [Install](#install)
- [Screenshots](#screenshots)
- [Repository information](#repository-information)

# Description
Tray Weather is a simple application to retrieve and show weather and forecast information for a given geographic location in a dialog and in the Windows OS system tray.
The computer location is obtained using the services of [http://ip-api.com/](http://ip-api.com/) and the weather information is obtained from [OpenWeatherMap](http://openweathermap.org/).
To obtain weather information from OpenWeatherMap you need to register in their website and enter the given API Key into the Tray Weather configuration dialog. 

## Options
The temperature units (celsius or fahrenheit) and the frequency of update requests are the only configuration options.

# Compilation requirements
## To build the tool:
* cross-platform build system: [CMake](http://www.cmake.org/cmake/resources/software.html).
* compiler: [Mingw64](http://sourceforge.net/projects/mingw-w64/) on Windows.

## External dependencies
The following libraries are required:
* [Qt 5 opensource framework](http://www.qt.io/).
* [Qt 5.5.1 QWebKit sumodule](https://download.qt.io/official_releases/qt/5.5/5.5.1/submodules/).

# Install
The only current option is build from source as binaries are not provided. 

# Screenshots
Configuration dialog.

![configuration]()

Current weather. 

![weather]()

Weather forecast for the next days. If the user puts the mouse over a point in the temperature line a tooltip will provide the weather conditions for that day and hour. The graph can be zoomed.

![forecast]()

Tray icon

![trayicon]()

# Repository information

**Version**: 1.0.0

**Status**: finished.

**cloc statistics**

| Language                     |files          |blank        |comment           |code  |
|:-----------------------------|--------------:|------------:|-----------------:|-----:|
| C++                          |   7           | 237         |   169            |  919 |
| C/C++ Header                 |   6           | 102         |   275            |  193 |
| CMake                        |   1           |  14         |    10            |   56 |
| **Total**                    | **14**        | **353**     | **454**          | **1168** |
