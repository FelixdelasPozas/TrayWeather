
Tray Weather
============

# Summary
- [Description](#description)
- [Compilation](#compilation-requirements)
- [Install](#install)
- [Screenshots](#screenshots)
- [Translations](#translations)
- [Repository information](#repository-information)

# Description
Tray Weather is a simple application to retrieve and show weather information for a given geographic location in a small dialog and in the Windows OS system tray. When executed the application sits in the system tray, showing the current weather icon and/or temperature. The tray menu provides access to the weather conditions and configuration dialogs. If there is a network error and the information cannot be retrieved the tray icon will inform of the error and show the error icon. 

The computer location can be automatically obtained by using the services of [http://ip-api.com/](http://ip-api.com/) or entered manually by specifying latitude and longitude coordinates. The weather information are obtained from a weather data provider.

## Weather data providers

You can select one the following data providers in the condifuration dialog:

* [OpenWeatherMap](http://openweathermap.org/)
* [Open-Meteo](https://open-meteo.com/)

Not all them provide the same data:

| Name               | Weather forecast   | Pollution forecast | UV index forecast  | Weather alerts     | Geolocation        | Maps               | Requires API key   |
|:-------------------|-------------------:|-------------------:|-------------------:|-------------------:|-------------------:|-------------------:|-------------------:|
| OpenWeatherMap 2.5 | :heavy_check_mark: | :heavy_check_mark: | :x:                | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: |
| OpenWeatherMap 3.0 | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: |
| Open-Meteo         | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | :x:                | :heavy_check_mark: | :x:                | :x:                |

> [!WARNING]
> Currently the OpenWeatherMap 3.0 is untested. Please report any bug or inconsistencies by creating an [issue](https://github.com/FelixdelasPozas/TrayWeather/issues) to help.

To obtain weather information from providers that require a registration, like OpenWeatherMap, you need to register in their website and enter the given API Key into the Tray Weather configuration dialog (currently only the free plan is supported). 

If you want a provider to be added you can ask for it in the [Discussions page](https://github.com/FelixdelasPozas/TrayWeather/discussions). The only requirements are to be free and to provide a REST API.

> [!NOTE] 
> Please note that after you register in OpenWeatherMap and are given an API key, that key **may not be valid right away**. If you enter the API key in TrayWeather and get an "host requires authentication" error, you must wait a little longer until it becomes valid.

## Options
The temperature units (Celsius or Fahrenheit) and the frequency of update requests are some of the configurable options. Other visual configuration options can be seen in the configuration dialog screenshot.

# Compilation requirements
## To build the tool:
* cross-platform build system: [CMake](http://www.cmake.org/cmake/resources/software.html).
* compiler: [Mingw64](http://sourceforge.net/projects/mingw-w64/) on Windows.

From version 5.6 onwards the WebKit module has been replaced by a new Chromium-based equivalent that doesn't compile on Mingw. In addition to that, the QtCharts module is only officially available in Qt from versions 5.6 onwards. So the easiest way to compile the application is to use the 5.5.1 release of Qt and, separately, compile and add to the project the QtCharts module from GitHub. 

## External dependencies
The following libraries are required:
* [Qt 5.5.1 opensource framework](http://www.qt.io/) compiled with SSL support.
* [Qt Charts 2.1.0 submodule](https://github.com/qt/qtcharts).

# Install

Download the latest ![release](https://github.com/FelixdelasPozas/TrayWeather/releases/) installer or zip file (portable mode).

Supported versions are from Windows 7 onwards. Neither the application or the installer are digitally signed so the system will ask for approval before running it the first time.

> **_NOTE:_**  The application will use a ini file in the same folder as the executable if the file exists and can be written. So
its possible to use the application in "portable mode" even if it has been installed using the installer by just copying the installation 
folder to another location and creating an empty *TrayWeather.ini* file in it. It is recommended to execute the application before 
creating the empty file because then the setting will be loaded from the windows registry and saved to the ini file. 

# Screenshots

> [!NOTE]
> Some screenshots are from older versions of Tray Weather. The tabs the appear in the main dialog depend on the data that the selected weather provider can supply.

Tray icon showing the current weather icon. At night and on clear sky days it shows the current moon phase as the weather icon.

![icon](https://cloud.githubusercontent.com/assets/12167134/20938095/f03e2474-bbe9-11e6-83b9-e2bc8c716bf4.jpg)
![icon_menu](https://user-images.githubusercontent.com/12167134/145942029-01678f72-11ae-4b55-b8f4-dbbc70183341.png)

Tray icon can also show the temperature alone or composed with the weather icon. It can also be configured to show two tray icons, one with the temperature and other with the weather icon. 
The temperature text color can be set by the user or can change dynamically between a range of colors according to the current value. Temperature icon color, size and composition can be modified in the configuration dialog. 

![icon_temp](https://user-images.githubusercontent.com/12167134/85929400-b6e05280-b8b4-11ea-9574-bf27537f38e3.png)

Several different icon themes are available for the application weather icons. The color of the mono-color themes can be selected by the user.

![icon_themes](https://user-images.githubusercontent.com/12167134/147830694-fed5769d-836f-4d75-9bc9-5510b7288f8e.png) 

Configuration dialog. The providers can be selected here and, if the selected provider requires configuration, the API key can be entered and tested. The geographic location coordinates can
be entered manually or detected by the geo-location. If the selected provider has Geo-Location you can search for a location coordinates by entering its name. A 'roaming' mode can be enabled, where the geographical coordinates are requested before any weather data request, so the weather information is up to date even if the computer location is moving. Roaming mode can only be enabled if the geolocation services are being used.

The rest of the available options are self-explanatory. 

Configuration dialog in English with the application light theme.

![config](https://github.com/user-attachments/assets/95a57ddc-e504-4afb-b214-025f16aa0f91)

In the configuration dialog the location can be specified manually by coordinates, automatically using IP GeoLocation or can be selected by using the 'Find' dialog.

Location coordinates search dialog. Searches by location name using the provider Geolocation.

![location](https://github.com/FelixdelasPozas/TrayWeather/assets/12167134/fc0f2f45-5d07-46e7-927d-696a0f78cd7d)

Weather dialog, showing the current weather tab. 

![weather](https://user-images.githubusercontent.com/12167134/127046991-e2eb1e5c-73d7-4ece-b9c4-dfff8dd1648e.png)

Weather forecast for the next days. If the user puts the mouse over a point in the temperature line or a bar a tooltip will provide the weather conditions for that day and hour. Background is colored to day/night according to sunrise/sunset values for the day.
The graph can be zoomed by selecting the area to zoom with the mouse and reset to the initial state by using the reset button below the graph. Data series can be hidden and shown again by clicking on its legend text below the graph.

![forecast_graph](https://github.com/FelixdelasPozas/TrayWeather/assets/12167134/4fa05da5-238e-4a28-9bcb-cc021a05cc4b)

Pollution forecast can be obtained in the Pollution tab, showing the projections for the next days. The chart can be zoomed in the X axis and
reset by using the reset button below. The pollution chart also has a tooltip with detailed information for each point of the lines and
the background is colored according to air quality value. As with the weather forecast it can be zoomed in and graph series can be hidden and shown again in the same way. 

![pollution](https://user-images.githubusercontent.com/12167134/109207327-4bcf7e80-77a9-11eb-89a0-dd704e8969ad.png)

Ultraviolet radiation index forecast can be obtained from de UV tab, showing the forecast for the next days. As the other charts it can be zoomed and reset. The tooltip of each points gives the UV index and the recommendation for it. 

![uv](https://github.com/user-attachments/assets/b98e565d-2270-4f94-a4e9-cac6d218c5c5)

Weather maps are interactive. While initially the map is centered in the detected location it can be moved and zoomed in and out. The maps consumes much more memory than the rest of the application so the user can be disable them using the button below. 

![maps0](https://user-images.githubusercontent.com/12167134/109207325-4bcf7e80-77a9-11eb-8744-8b928c5d2c3e.png)
![maps1](https://cloud.githubusercontent.com/assets/12167134/20938099/f07daa22-bbe9-11e6-9efb-07466ef36748.jpg)
![maps2](https://cloud.githubusercontent.com/assets/12167134/20938097/f0623792-bbe9-11e6-8ebf-0ae4b5b679a9.jpg)
![maps3](https://cloud.githubusercontent.com/assets/12167134/20938100/f0851d34-bbe9-11e6-80c9-d7d952632cc4.jpg)

Available maps:
* OpenStreetMap.
* Google Maps (roadmaps).
* Google Hybrid (Satellite images & roadmaps).

# Translations

Tray Weather is available in:
* English
* Spanish
* Russian
* German
* French
* Chinese (Simplified)
* Portuguese (Brazilian)
* Ukrainian
* Slovenian
* Korean
* Polish
* Turkish
* Italian

If 'TrayWeather' hasn't a translation for your language you can collaborate and translate the application using the 
[Qt Linguistic Tools](https://doc.qt.io/qt-5/qtlinguist-index.html) (available [here](https://github.com/lelegard/qtlinguist-installers))
or manually editing the ['empty' translation source file](https://raw.githubusercontent.com/FelixdelasPozas/TrayWeather/master/languages/empty.ts)
and making a pull request. Currently it's 504 texts.

To do it manually just edit the 'empty translation' file in the 'languages' directory (empty.ts) and replace the untranslated messages, for example:

```
    <message>
        <location filename="../AboutDialog.ui" line="429"/>
        <source>Weather data provided by</source>
        <translation type="unfinished"></translation>
    </message>
```
    
To the translation in your language. For example in Spanish it is:

```
    <message>
        <location filename="../AboutDialog.ui" line="429"/>
        <source>Weather data provided by</source>
        <translation>Datos meteorológicos proporcionados por</translation>
    </message>
```

# Repository information

**Version**: 1.33.3

**Status**: finished.

**cloc statistics**

| Language                     |files          |blank        |comment           |code  |
|:-----------------------------|--------------:|------------:|-----------------:|-----:|
| C++                          |  15           | 1434        |   593            | 7071 |
| C/C++ Header                 |  15           |  463        |  1327            | 1348 |
| HTML                         |   2           |   66        |     0            |  304 |
| CMake                        |   1           |   19        |     9            |  132 |
| **Total**                    | **33**        | **1982**    | **1929**         | **8855** |
