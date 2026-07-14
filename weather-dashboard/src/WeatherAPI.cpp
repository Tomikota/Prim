#include "WeatherAPI.h"
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QDebug>
#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QDateTime>

WeatherAPI::WeatherAPI(QObject* parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)),
    m_apiKey("")
{
}

WeatherAPI::~WeatherAPI() {
}

void WeatherAPI::setAPIKey(const QString& key) {
    m_apiKey = key;
}

void WeatherAPI::fetchCurrentWeather(const QString& city) {
    if (m_apiKey.isEmpty()) {
        emit errorOccurred("API Key not set");
        return;
    }

    emit loadingStarted();

    QUrl url(API_BASE_URL + "/weather");
    QUrlQuery query;
    query.addQueryItem("q", city);
    query.addQueryItem("appid", m_apiKey);
    query.addQueryItem("units", "metric");
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, &WeatherAPI::onCurrentWeatherReply);
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &WeatherAPI::onNetworkError);
}

void WeatherAPI::fetchForecast(const QString& city, int days) {
    if (m_apiKey.isEmpty()) {
        emit errorOccurred("API Key not set");
        return;
    }

    emit loadingStarted();

    QUrl url(API_BASE_URL + "/forecast");
    QUrlQuery query;
    query.addQueryItem("q", city);
    query.addQueryItem("appid", m_apiKey);
    query.addQueryItem("units", "metric");
    query.addQueryItem("cnt", QString::number(days * 8)); // 8 forecasts per day
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, &WeatherAPI::onForecastReply);
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &WeatherAPI::onNetworkError);
}

void WeatherAPI::onCurrentWeatherReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    reply->deleteLater();
    emit loadingFinished();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred("Network error: " + reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject()) {
        emit errorOccurred("Invalid JSON response");
        return;
    }

    WeatherData weather = parseCurrentWeatherJson(doc);
    emit weatherDataReady(weather);
}

void WeatherAPI::onForecastReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    reply->deleteLater();
    emit loadingFinished();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred("Network error: " + reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject()) {
        emit errorOccurred("Invalid JSON response");
        return;
    }

    QList<ForecastData> forecasts = parseForecastJson(doc);
    emit forecastDataReady(forecasts);
}

void WeatherAPI::onNetworkError() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    emit errorOccurred("Network error: " + reply->errorString());
    emit loadingFinished();
}

WeatherData WeatherAPI::parseCurrentWeatherJson(const QJsonDocument& doc) {
    WeatherData data;
    QJsonObject obj = doc.object();

    // Basic info
    data.city = obj["name"].toString();
    data.country = obj["sys"].toObject()["country"].toString();

    // Main weather data
    QJsonObject main = obj["main"].toObject();
    data.temperature = main["temp"].toDouble();
    data.feelsLike = main["feels_like"].toDouble();
    data.tempMin = main["temp_min"].toDouble();
    data.tempMax = main["temp_max"].toDouble();
    data.humidity = main["humidity"].toInt();
    data.pressure = main["pressure"].toInt();

    // Wind data
    QJsonObject wind = obj["wind"].toObject();
    data.windSpeed = wind["speed"].toDouble();

    // Weather description
    QJsonArray weather = obj["weather"].toArray();
    if (!weather.isEmpty()) {
        QJsonObject w = weather[0].toObject();
        data.mainWeather = w["main"].toString();
        data.description = w["description"].toString();
        data.icon = w["icon"].toString();
    }

    // Other data
    data.cloudiness = obj["clouds"].toObject()["all"].toInt();
    data.visibility = obj["visibility"].toDouble() / 1000.0; // Convert to km

    return data;
}

QList<ForecastData> WeatherAPI::parseForecastJson(const QJsonDocument& doc) {
    QList<ForecastData> forecasts;
    QJsonObject obj = doc.object();
    QJsonArray list = obj["list"].toArray();

    QMap<QString, ForecastData> dailyForecasts;

    for (const QJsonValue& value : list) {
        QJsonObject item = value.toObject();
        QDateTime dt = QDateTime::fromSecsSinceEpoch(item["dt"].toInt());
        QString dateStr = dt.date().toString("yyyy-MM-dd");

        ForecastData forecast;
        forecast.date = dateStr;

        QJsonObject main = item["main"].toObject();
        forecast.tempDay = main["temp"].toDouble();
        forecast.tempMin = main["temp_min"].toDouble();
        forecast.tempMax = main["temp_max"].toDouble();
        forecast.humidity = main["humidity"].toInt();
        forecast.pressure = main["pressure"].toInt();

        QJsonObject wind = item["wind"].toObject();
        forecast.windSpeed = wind["speed"].toDouble();

        QJsonArray weather = item["weather"].toArray();
        if (!weather.isEmpty()) {
            QJsonObject w = weather[0].toObject();
            forecast.description = w["description"].toString();
            forecast.icon = w["icon"].toString();
        }

        forecast.precipitationProb = item["pop"].toDouble() * 100;

        if (!dailyForecasts.contains(dateStr)) {
            dailyForecasts[dateStr] = forecast;
        }
    }

    return dailyForecasts.values();
}

QString WeatherAPI::getWeatherIconPath(const QString& iconCode) {
    // Map OpenWeatherMap icons to local resources
    QMap<QString, QString> iconMap;
    iconMap["01d"] = "☀️";
    iconMap["01n"] = "🌙";
    iconMap["02d"] = "⛅";
    iconMap["02n"] = "☁️";
    iconMap["03d"] = "☁️";
    iconMap["03n"] = "☁️";
    iconMap["04d"] = "☁️";
    iconMap["04n"] = "☁️";
    iconMap["09d"] = "🌧️";
    iconMap["09n"] = "🌧️";
    iconMap["10d"] = "🌦️";
    iconMap["10n"] = "🌧️";
    iconMap["11d"] = "⛈️";
    iconMap["11n"] = "⛈️";
    iconMap["13d"] = "❄️";
    iconMap["13n"] = "❄️";
    iconMap["50d"] = "🌫️";
    iconMap["50n"] = "🌫️";

    return iconMap.value(iconCode, "🌡️");
}

QString WeatherAPI::getWeatherDescription(const QString& mainWeather) {
    // Provide human-readable descriptions
    QMap<QString, QString> descriptions;
    descriptions["Thunderstorm"] = "Грозовой дождь";
    descriptions["Drizzle"] = "Морось";
    descriptions["Rain"] = "Дождь";
    descriptions["Snow"] = "Снег";
    descriptions["Mist"] = "Туман";
    descriptions["Smoke"] = "Дым";
    descriptions["Haze"] = "Мгла";
    descriptions["Dust"] = "Пыль";
    descriptions["Fog"] = "Туман";
    descriptions["Sand"] = "Песок";
    descriptions["Ash"] = "Вулканический пепел";
    descriptions["Squall"] = "Шквал";
    descriptions["Tornado"] = "Торнадо";
    descriptions["Clear"] = "Ясно";
    descriptions["Clouds"] = "Облачно";

    return descriptions.value(mainWeather, mainWeather);
}