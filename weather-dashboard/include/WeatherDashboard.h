#pragma once
#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QProgressBar>
#include <QComboBox>
#include "WeatherAPI.h"

class WeatherDashboard : public QMainWindow {
    Q_OBJECT

public:
    explicit WeatherDashboard(QWidget* parent = nullptr);
    ~WeatherDashboard();

private slots:
    void onSearchClicked();
    void onWeatherDataReady(const WeatherData& data);
    void onForecastDataReady(const QList<ForecastData>& forecasts);
    void onErrorOccurred(const QString& error);
    void onLoadingStarted();
    void onLoadingFinished();
    void onCitySelected(int index);
    void onToggleDarkTheme();

private:
    void setupUI();
    void setupStyles();
    void loadSavedCities();
    void saveCities();
    void displayCurrentWeather(const WeatherData& data);
    void displayForecast(const QList<ForecastData>& forecasts);
    void createCurrentWeatherWidget(const WeatherData& data);
    void createForecastWidget(const QList<ForecastData>& forecasts);

    WeatherAPI* m_weatherAPI;
    QStringList m_savedCities;

    // UI Components
    QLineEdit* m_cityInput;
    QPushButton* m_searchButton;
    QComboBox* m_cityComboBox;
    QLabel* m_loadingLabel;
    QProgressBar* m_loadingBar;

    // Current Weather
    QLabel* m_temperatureLabel;
    QLabel* m_descriptionLabel;
    QLabel* m_feelsLikeLabel;
    QLabel* m_minMaxLabel;
    QLabel* m_humidityLabel;
    QLabel* m_windSpeedLabel;
    QLabel* m_pressureLabel;
    QLabel* m_visibilityLabel;
    QLabel* m_iconLabel;

    // Forecast
    QScrollArea* m_forecastScrollArea;
    QWidget* m_forecastContainer;
    QHBoxLayout* m_forecastLayout;

    // Theme
    bool m_isDarkTheme;
    QPushButton* m_themeButton;
};