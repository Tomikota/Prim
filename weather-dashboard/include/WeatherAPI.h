#pragma once
#include <QString>
#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>

struct WeatherData {
    QString city;
    QString country;
    double temperature;
    double feelsLike;
    double tempMin;
    double tempMax;
    int humidity;
    int pressure;
    double windSpeed;
    int cloudiness;
    QString description;
    QString icon;
    QString mainWeather;
    double visibility;
    int uvIndex;
};

struct ForecastData {
    QString date;
    double tempDay;
    double tempNight;
    double tempMin;
    double tempMax;
    int humidity;
    int pressure;
    double windSpeed;
    QString description;
    QString icon;
    double precipitationProb;
};

class WeatherAPI : public QObject {
    Q_OBJECT

public:
    explicit WeatherAPI(QObject* parent = nullptr);
    ~WeatherAPI();

    void fetchCurrentWeather(const QString& city);
    void fetchForecast(const QString& city, int days = 5);
    void setAPIKey(const QString& key);

signals:
    void weatherDataReady(const WeatherData& data);
    void forecastDataReady(const QList<ForecastData>& forecasts);
    void errorOccurred(const QString& error);
    void loadingStarted();
    void loadingFinished();

private slots:
    void onCurrentWeatherReply();
    void onForecastReply();
    void onNetworkError();

private:
    QNetworkAccessManager* m_networkManager;
    QString m_apiKey;
    const QString API_BASE_URL = "https://api.openweathermap.org/data/2.5";

    WeatherData parseCurrentWeatherJson(const QJsonDocument& doc);
    QList<ForecastData> parseForecastJson(const QJsonDocument& doc);
    QString getWeatherIconPath(const QString& iconCode);
    QString getWeatherDescription(const QString& mainWeather);
};