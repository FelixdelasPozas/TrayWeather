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
The computer location is obtained using the services of [http://ip-api.com/](http://ip-api.com/) and the weather and maps information are obtained from [OpenWeatherMap](http://openweathermap.org/).
To obtain weather information from OpenWeatherMap you need to register in their website and enter the given API Key into the Tray Weather configuration dialog. 

When executed the application sits in the system tray, showing the current weather icon. The tray icon tooltip shows the location, weather description and temperature. The tray menu provides access to the 
weather and configuration dialogs. If there is a network error and the information cannot be retrieved the tray icon will inform of the error and show the error icon. 

## Options
The temperature units (celsius or fahrenheit) and the frequency of update requests are the only configurable options.

# Compilation requirements
## To build the tool:
* cross-platform build system: [CMake](http://www.cmake.org/cmake/resources/software.html).
* compiler: [Mingw64](http://sourceforge.net/projects/mingw-w64/) on Windows.

From version 5.6 onwards the WebKit module has been replaced by a new Chromium-based equivalent that doesn't compile on Mingw. In addition to that, the QtCharts module is only officially available in Qt
from versions 5.6 onwards. So the easiest way to compile the application is to use the 5.5.1 release of Qt and, separately, compile and add to the project the QtCharts module from GitHub. 

## External dependencies
The following libraries are required:
* [Qt 5.5.1 opensource framework](http://www.qt.io/).
* [Qt Charts 2.1.0 submodule](https://github.com/qt/qtcharts).

# Install
The only current option is build from source as binaries are not provided. 

# Screenshots
Tray icon, showing the current weather icon (at night, on clear sky days, it shows the current moon phase).

![trayicon]()

Configuration dialog. The OpenWeatherMap API Key must be entered here.

![configuration]()

Weather dialog, showing the current weather tab. 

![weather]()

Weather forecast tab showing the forecast for the next days. If the user puts the mouse over a point in the temperature line a tooltip will provide the weather conditions for that day and hour. 
The graph can be zoomed by selecting the area to zoom with the mouse, and resetted to the initial state by using the reset button below the graph.

![forecast]()

Weather maps. Interactive map where the user can select to view the current location with the rain precipitations, temperature, wind or cloudiness maps. The maps consumes much more memory than
the rest of the application so the user can be disable them using the button below. 

![maps]()

# Repository information

**Version**: 1.1.0

**Status**: finished.

**cloc statistics**

| Language                     |files          |blank        |comment           |code  |
|:-----------------------------|--------------:|------------:|-----------------:|-----:|
| C++                          |   7           | 258         |   177            | 1048 |
| C/C++ Header                 |   6           | 107         |   288            |  203 |
| HTML                         |   1           |  28         |     0            |  138 |
| CMake                        |   1           |  15         |    10            |   92 |
| **Total**                    | **15**        | **408**     | **475**          | **1481** |
