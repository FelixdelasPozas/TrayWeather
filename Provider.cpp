/*
 File: Provider.cpp
 Created on: 01/11/2024
 Author: Felix de las Pozas Alvarez

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Project
#include <Provider.h>
#include <Providers/OpenMeteo.h>
#include <Providers/OWM25.h>
#include <Providers/OWM30.h>
#include <Providers/WeatherAPI.h>

// Qt
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDir>
#include <QSettings>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStringList>

// C++
#include <chrono>
#include <cmath>

/** Providers list, CAN'T be const because is used as extern elsewhere.
 */
QList<ProviderData> WEATHER_PROVIDERS = { { OWM_25_PROVIDER,     ":/TrayWeather/application.svg" }, 
                                          { OWM_30_PROVIDER,     ":/TrayWeather/application.svg" }, 
                                          { OPENMETEO_PROVIDER,  ":/TrayWeather/application.svg" },
                                          { WEATHERAPI_PROVIDER, ":/TrayWeather/application.svg"} };

//----------------------------------------------------------------------------
std::unique_ptr<WeatherProvider> WeatherProviderFactory::createProvider(const QString &name, Configuration &config)
{
  if(name.compare(OWM_25_PROVIDER) == 0)
    return std::make_unique<OWM25Provider>(config);

  if(name.compare(OWM_30_PROVIDER) == 0)
    return std::make_unique<OWM30Provider>(config);
  
  if(name.compare(OPENMETEO_PROVIDER) == 0)    
    return std::make_unique<OpenMeteoProvider>(config);

  if(name.compare(WEATHERAPI_PROVIDER) == 0)    
    return std::make_unique<WeatherAPIProvider>(config);

  return nullptr;
}

//----------------------------------------------------------------------------
int WeatherProviderFactory::indexOf(const QString &name)
{
  for (int i = 0; i < WEATHER_PROVIDERS.size(); ++i)
  {
    if (WEATHER_PROVIDERS.at(i).id.compare(name, Qt::CaseInsensitive) == 0)
      return i;
  }
  return -1;
}
