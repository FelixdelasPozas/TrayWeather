Tray Weather
============

# Summary
- [Description](#description)
- [Compilation](#compilation-requirements)
- [Install](#install)
- [Screenshots](#screenshots)
- [Repository information](#repository-information)

# Description
Tray Weather is a simple application to retrieve and show weather information for a given geographic location in a small dialog and in the Windows OS system tray. When executed the application sits
in the system tray, showing the current weather icon. The tray icon tooltip shows the location, weather description and temperature. The tray menu provides access to the weather and configuration
dialogs. If there is a network error and the information cannot be retrieved the tray icon will inform of the error and show the error icon. 

The computer location is obtained using the services of [http://ip-api.com/](http://ip-api.com/) and the weather and maps information are obtained from [OpenWeatherMap](http://openweathermap.org/).
To obtain weather information from OpenWeatherMap you need to register in their website and enter the given API Key into the Tray Weather configuration dialog. 

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

Download the latest ![release](https://github.com/FelixdelasPozas/TrayWeather/releases/tag/1.1.4) installer.

# Screenshots
Tray icon showing the current weather icon. At night and on clear sky days it shows the current moon phase as the weather icon.

![icon](https://cloud.githubusercontent.com/assets/12167134/20938095/f03e2474-bbe9-11e6-83b9-e2bc8c716bf4.jpg)
![icon_menu](https://cloud.githubusercontent.com/assets/12167134/20938103/f0b22126-bbe9-11e6-8639-30161344d1d3.jpg)

Configuration dialog. It shows the detected location properties and the options to change the frequency of updates and temperature units. The OpenWeatherMap API Key must be entered here.

![config](https://user-images.githubusercontent.com/12167134/34379925-45f2066c-eafe-11e7-83a6-58710b7df802.jpg)

Weather dialog, showing the current weather tab. 

![weather](https://cloud.githubusercontent.com/assets/12167134/20938101/f091d718-bbe9-11e6-8bb5-40be8d80f444.jpg)

Weather forecast for the next days. If the user puts the mouse over a point in the temperature line a tooltip will provide the weather conditions for that day and hour. 
The graph can be zoomed by selecting the area to zoom with the mouse and resetted to the initial state by using the reset button below the graph.

![forecast_graph](https://user-images.githubusercontent.com/12167134/34379926-460a5cbc-eafe-11e7-99f9-b0ee23005b73.jpg)
![forecast_tooltip](https://user-images.githubusercontent.com/12167134/34379927-4622b118-eafe-11e7-8933-c7fae870fb1d.jpg)

Weather maps are interactive. While initially the map is centered in the detected location it can be moved and zoomed in and out. The maps consumes much more memory than
the rest of the application so the user can be disable them using the button below. 

![maps1](https://cloud.githubusercontent.com/assets/12167134/20938099/f07daa22-bbe9-11e6-9efb-07466ef36748.jpg)
![maps2](https://cloud.githubusercontent.com/assets/12167134/20938097/f0623792-bbe9-11e6-8ebf-0ae4b5b679a9.jpg)
![maps3](https://cloud.githubusercontent.com/assets/12167134/20938100/f0851d34-bbe9-11e6-80c9-d7d952632cc4.jpg)

# Repository information

**Version**: 1.1.7

**Status**: finished.

**cloc statistics**

| Language                     |files          |blank        |comment           |code  |
|:-----------------------------|--------------:|------------:|-----------------:|-----:|
| C++                          |   7           | 306         |   185            | 1281 |
| C/C++ Header                 |   6           | 113         |   310            |  215 |
| HTML                         |   1           |  28         |     0            |  138 |
| CMake                        |   1           |  15         |    10            |   93 |
| **Total**                    | **15**        | **462**     | **505**          | **1727** |
