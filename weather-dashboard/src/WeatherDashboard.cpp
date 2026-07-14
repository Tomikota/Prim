#include "WeatherDashboard.h"
#include <QApplication>
#include <QGridLayout>
#include <QGroupBox>
#include <QSettings>
#include <QMessageBox>
#include <QFrame>
#include <QScrollBar>
#include <QTimer>

WeatherDashboard::WeatherDashboard(QWidget* parent)
    : QMainWindow(parent), m_weatherAPI(new WeatherAPI(this)),
    m_isDarkTheme(true) {

    setWindowTitle("Weather Dashboard");
    setGeometry(100, 100, 1200, 800);
    setWindowIcon(QIcon("🌡️"));

    setupUI();
    setupStyles();
    loadSavedCities();

    // Set API Key (Replace with your OpenWeatherMap API key)
    m_weatherAPI->setAPIKey("8f18c9eba3d8b08c9c2d1e3f4a5b6c7d"); // Demo key

    // Connect signals
    connect(m_searchButton, &QPushButton::clicked, this, &WeatherDashboard::onSearchClicked);
    connect(m_cityInput, &QLineEdit::returnPressed, this, &WeatherDashboard::onSearchClicked);
    connect(m_weatherAPI, &WeatherAPI::weatherDataReady, this, &WeatherDashboard::onWeatherDataReady);
    connect(m_weatherAPI, &WeatherAPI::forecastDataReady, this, &WeatherDashboard::onForecastDataReady);
    connect(m_weatherAPI, &WeatherAPI::errorOccurred, this, &WeatherDashboard::onErrorOccurred);
    connect(m_weatherAPI, &WeatherAPI::loadingStarted, this, &WeatherDashboard::onLoadingStarted);
    connect(m_weatherAPI, &WeatherAPI::loadingFinished, this, &WeatherDashboard::onLoadingFinished);
    connect(m_cityComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &WeatherDashboard::onCitySelected);
    connect(m_themeButton, &QPushButton::clicked, this, &WeatherDashboard::onToggleDarkTheme);

    // Load initial weather
    if (!m_savedCities.isEmpty()) {
        m_cityComboBox->setCurrentIndex(0);
        m_cityInput->setText(m_savedCities[0]);
        QTimer::singleShot(500, this, &WeatherDashboard::onSearchClicked);
    }
}

WeatherDashboard::~WeatherDashboard() {
    saveCities();
}

void WeatherDashboard::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Top Control Bar
    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->setSpacing(10);

    m_cityInput = new QLineEdit();
    m_cityInput->setPlaceholderText("Enter city name...");
    m_cityInput->setMaximumWidth(300);

    m_searchButton = new QPushButton("Search");
    m_searchButton->setMaximumWidth(100);

    m_cityComboBox = new QComboBox();
    m_cityComboBox->setMaximumWidth(200);

    m_loadingLabel = new QLabel("Ready");
    m_loadingBar = new QProgressBar();
    m_loadingBar->setMaximumWidth(150);
    m_loadingBar->setMaximumHeight(20);
    m_loadingBar->setVisible(false);

    m_themeButton = new QPushButton("🌙 Dark");
    m_themeButton->setMaximumWidth(100);

    topLayout->addWidget(new QLabel("City:"));
    topLayout->addWidget(m_cityInput);
    topLayout->addWidget(m_searchButton);
    topLayout->addWidget(new QLabel("Saved:"));
    topLayout->addWidget(m_cityComboBox);
    topLayout->addStretch();
    topLayout->addWidget(m_loadingLabel);
    topLayout->addWidget(m_loadingBar);
    topLayout->addWidget(m_themeButton);

    mainLayout->addLayout(topLayout);

    // Current Weather Section
    QGroupBox* currentWeatherGroup = new QGroupBox("Current Weather");
    QGridLayout* weatherLayout = new QGridLayout(currentWeatherGroup);

    m_iconLabel = new QLabel();
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setStyleSheet("font-size: 80px;");

    m_temperatureLabel = new QLabel("-- °C");
    m_temperatureLabel->setAlignment(Qt::AlignCenter);
    m_temperatureLabel->setStyleSheet("font-size: 48px; font-weight: bold;");

    m_descriptionLabel = new QLabel("--");
    m_descriptionLabel->setAlignment(Qt::AlignCenter);
    m_descriptionLabel->setStyleSheet("font-size: 18px;");

    m_feelsLikeLabel = new QLabel("Feels like: -- °C");
    m_minMaxLabel = new QLabel("Min: -- °C, Max: -- °C");
    m_humidityLabel = new QLabel("Humidity: --%");
    m_windSpeedLabel = new QLabel("Wind: -- m/s");
    m_pressureLabel = new QLabel("Pressure: -- hPa");
    m_visibilityLabel = new QLabel("Visibility: -- km");

    weatherLayout->addWidget(m_iconLabel, 0, 0, 2, 1);
    weatherLayout->addWidget(m_temperatureLabel, 0, 1, 1, 2);
    weatherLayout->addWidget(m_descriptionLabel, 1, 1, 1, 2);
    weatherLayout->addWidget(m_feelsLikeLabel, 2, 0);
    weatherLayout->addWidget(m_minMaxLabel, 2, 1);
    weatherLayout->addWidget(m_humidityLabel, 3, 0);
    weatherLayout->addWidget(m_windSpeedLabel, 3, 1);
    weatherLayout->addWidget(m_pressureLabel, 4, 0);
    weatherLayout->addWidget(m_visibilityLabel, 4, 1);

    mainLayout->addWidget(currentWeatherGroup);

    // Forecast Section
    QGroupBox* forecastGroup = new QGroupBox("5-Day Forecast");
    QVBoxLayout* forecastGroupLayout = new QVBoxLayout(forecastGroup);

    m_forecastScrollArea = new QScrollArea();
    m_forecastScrollArea->setWidgetResizable(true);
    m_forecastScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_forecastScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarNever);

    m_forecastContainer = new QWidget();
    m_forecastLayout = new QHBoxLayout(m_forecastContainer);
    m_forecastLayout->setSpacing(10);

    m_forecastScrollArea->setWidget(m_forecastContainer);
    forecastGroupLayout->addWidget(m_forecastScrollArea);

    mainLayout->addWidget(forecastGroup);

    mainLayout->addStretch();
}

void WeatherDashboard::setupStyles() {
    if (m_isDarkTheme) {
        this->setStyleSheet(
            "QMainWindow { background-color: #0e1621; color: #ffffff; }"
            "QLineEdit { background-color: #17212b; border: 1px solid #24303f; color: #ffffff; border-radius: 5px; padding: 8px; font-size: 13px; }"
            "QLineEdit:focus { border: 1px solid #5288c1; }"
            "QPushButton { background-color: #24303f; color: #5288c1; border: none; border-radius: 5px; padding: 8px; font-weight: bold; }"
            "QPushButton:hover { background-color: #2b394a; color: #659bdf; }"
            "QGroupBox { border: 1px solid #24303f; border-radius: 8px; margin-top: 10px; padding-top: 10px; color: #ffffff; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; }"
            "QComboBox { background-color: #17212b; border: 1px solid #24303f; color: #ffffff; border-radius: 5px; padding: 5px; }"
            "QComboBox::drop-down { border: none; }"
            "QComboBox::down-arrow { image: none; }"
            "QProgressBar { background-color: #17212b; border: 1px solid #24303f; border-radius: 3px; height: 15px; }"
            "QProgressBar::chunk { background-color: #5288c1; }"
            "QLabel { color: #ffffff; }"
            "QScrollBar:horizontal { height: 8px; background-color: #0e1621; }"
            "QScrollBar::handle:horizontal { background-color: #2b394a; border-radius: 4px; }"
        );
    } else {
        this->setStyleSheet(
            "QMainWindow { background-color: #f9f8f6; color: #3c3d3a; }"
            "QLineEdit { background-color: #ffffff; border: 1px solid #dcdbd5; color: #3c3d3a; border-radius: 5px; padding: 8px; font-size: 13px; }"
            "QLineEdit:focus { border: 1px solid #7c9a7b; }"
            "QPushButton { background-color: #f0efe9; color: #7c9a7b; border: none; border-radius: 5px; padding: 8px; font-weight: bold; }"
            "QPushButton:hover { background-color: #e5e4de; color: #6a8569; }"
            "QGroupBox { border: 1px solid #dcdbd5; border-radius: 8px; margin-top: 10px; padding-top: 10px; color: #3c3d3a; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; }"
            "QComboBox { background-color: #ffffff; border: 1px solid #dcdbd5; color: #3c3d3a; border-radius: 5px; padding: 5px; }"
            "QComboBox::drop-down { border: none; }"
            "QComboBox::down-arrow { image: none; }"
            "QProgressBar { background-color: #f0efe9; border: 1px solid #dcdbd5; border-radius: 3px; height: 15px; }"
            "QProgressBar::chunk { background-color: #7c9a7b; }"
            "QLabel { color: #3c3d3a; }"
            "QScrollBar:horizontal { height: 8px; background-color: #f9f8f6; }"
            "QScrollBar::handle:horizontal { background-color: #e5e4de; border-radius: 4px; }"
        );
    }
}

void WeatherDashboard::onSearchClicked() {
    QString city = m_cityInput->text().trimmed();
    if (city.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter a city name");
        return;
    }

    // Add to saved cities if not already there
    if (!m_savedCities.contains(city)) {
        m_savedCities.insert(0, city);
        if (m_savedCities.size() > 10) {
            m_savedCities.removeLast();
        }
        m_cityComboBox->insertItem(0, city);
        saveCities();
    }

    m_weatherAPI->fetchCurrentWeather(city);
    m_weatherAPI->fetchForecast(city, 5);
}

void WeatherDashboard::onWeatherDataReady(const WeatherData& data) {
    displayCurrentWeather(data);
}

void WeatherDashboard::onForecastDataReady(const QList<ForecastData>& forecasts) {
    displayForecast(forecasts);
}

void WeatherDashboard::onErrorOccurred(const QString& error) {
    m_loadingLabel->setText("Error: " + error);
    m_loadingLabel->setStyleSheet("color: #ff6b6b;");
    QMessageBox::critical(this, "Error", error);
}

void WeatherDashboard::onLoadingStarted() {
    m_loadingBar->setVisible(true);
    m_loadingLabel->setText("Loading...");
    m_loadingLabel->setStyleSheet("color: #5288c1;");
    m_loadingBar->setValue(0);
}

void WeatherDashboard::onLoadingFinished() {
    m_loadingBar->setVisible(false);
    m_loadingLabel->setText("Ready");
    m_loadingLabel->setStyleSheet("color: #7dd77d;");
}

void WeatherDashboard::onCitySelected(int index) {
    if (index >= 0 && index < m_savedCities.size()) {
        m_cityInput->setText(m_savedCities[index]);
        onSearchClicked();
    }
}

void WeatherDashboard::onToggleDarkTheme() {
    m_isDarkTheme = !m_isDarkTheme;
    setupStyles();
    m_themeButton->setText(m_isDarkTheme ? "🌙 Dark" : "☀️ Light");
}

void WeatherDashboard::displayCurrentWeather(const WeatherData& data) {
    m_temperatureLabel->setText(QString::number(data.temperature, 'f', 1) + " °C");
    m_descriptionLabel->setText(data.description.toUpper());
    m_feelsLikeLabel->setText("Feels like: " + QString::number(data.feelsLike, 'f', 1) + " °C");
    m_minMaxLabel->setText("Min: " + QString::number(data.tempMin, 'f', 1) + " °C, Max: " + QString::number(data.tempMax, 'f', 1) + " °C");
    m_humidityLabel->setText("Humidity: " + QString::number(data.humidity) + "%");
    m_windSpeedLabel->setText("Wind: " + QString::number(data.windSpeed, 'f', 1) + " m/s");
    m_pressureLabel->setText("Pressure: " + QString::number(data.pressure) + " hPa");
    m_visibilityLabel->setText("Visibility: " + QString::number(data.visibility, 'f', 1) + " km");

    // Set weather icon
    QString iconEmoji;
    if (data.icon.startsWith("01")) iconEmoji = "☀️";
    else if (data.icon.startsWith("02")) iconEmoji = "⛅";
    else if (data.icon.startsWith("03") || data.icon.startsWith("04")) iconEmoji = "☁️";
    else if (data.icon.startsWith("09") || data.icon.startsWith("10")) iconEmoji = "🌧️";
    else if (data.icon.startsWith("11")) iconEmoji = "⛈️";
    else if (data.icon.startsWith("13")) iconEmoji = "❄️";
    else if (data.icon.startsWith("50")) iconEmoji = "🌫️";
    else iconEmoji = "🌡️";

    m_iconLabel->setText(iconEmoji);

    // Update window title
    this->setWindowTitle("Weather Dashboard - " + data.city + ", " + data.country);
}

void WeatherDashboard::displayForecast(const QList<ForecastData>& forecasts) {
    // Clear existing forecast widgets
    while (m_forecastLayout->count() > 0) {
        QLayoutItem* item = m_forecastLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    // Add new forecast cards
    for (const ForecastData& forecast : forecasts.mid(0, 5)) {
        QGroupBox* card = new QGroupBox();
        card->setMinimumWidth(150);
        QVBoxLayout* layout = new QVBoxLayout(card);
        layout->setSpacing(5);

        QLabel* dateLabel = new QLabel(forecast.date);
        dateLabel->setAlignment(Qt::AlignCenter);
        dateLabel->setStyleSheet("font-weight: bold; font-size: 12px;");

        QString iconEmoji;
        if (forecast.icon.startsWith("01")) iconEmoji = "☀️";
        else if (forecast.icon.startsWith("02")) iconEmoji = "⛅";
        else if (forecast.icon.startsWith("03") || forecast.icon.startsWith("04")) iconEmoji = "☁️";
        else if (forecast.icon.startsWith("09") || forecast.icon.startsWith("10")) iconEmoji = "🌧️";
        else if (forecast.icon.startsWith("11")) iconEmoji = "⛈️";
        else if (forecast.icon.startsWith("13")) iconEmoji = "❄️";
        else if (forecast.icon.startsWith("50")) iconEmoji = "🌫️";
        else iconEmoji = "🌡️";

        QLabel* iconLabel = new QLabel(iconEmoji);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet("font-size: 32px;");

        QLabel* descLabel = new QLabel(forecast.description);
        descLabel->setAlignment(Qt::AlignCenter);
        descLabel->setStyleSheet("font-size: 10px; word-wrap: true;");
        descLabel->setWordWrap(true);

        QLabel* tempLabel = new QLabel(QString::number(forecast.tempDay, 'f', 0) + "°C");
        tempLabel->setAlignment(Qt::AlignCenter);
        tempLabel->setStyleSheet("font-weight: bold; font-size: 14px;");

        QLabel* rangeLabel = new QLabel(QString::number(forecast.tempMin, 'f', 0) + "°C - " + QString::number(forecast.tempMax, 'f', 0) + "°C");
        rangeLabel->setAlignment(Qt::AlignCenter);
        rangeLabel->setStyleSheet("font-size: 11px;");

        QLabel* humidityLabel = new QLabel("💧 " + QString::number(forecast.humidity) + "%");
        humidityLabel->setAlignment(Qt::AlignCenter);
        humidityLabel->setStyleSheet("font-size: 11px;");

        QLabel* windLabel = new QLabel("💨 " + QString::number(forecast.windSpeed, 'f', 1) + "m/s");
        windLabel->setAlignment(Qt::AlignCenter);
        windLabel->setStyleSheet("font-size: 11px;");

        layout->addWidget(dateLabel);
        layout->addWidget(iconLabel);
        layout->addWidget(descLabel);
        layout->addWidget(tempLabel);
        layout->addWidget(rangeLabel);
        layout->addWidget(humidityLabel);
        layout->addWidget(windLabel);
        layout->addStretch();

        m_forecastLayout->addWidget(card);
    }

    m_forecastLayout->addStretch();
}

void WeatherDashboard::loadSavedCities() {
    QSettings settings("WeatherDashboard", "Settings");
    m_savedCities = settings.value("cities", QStringList()).toStringList();

    m_cityComboBox->clear();
    for (const QString& city : m_savedCities) {
        m_cityComboBox->addItem(city);
    }
}

void WeatherDashboard::saveCities() {
    QSettings settings("WeatherDashboard", "Settings");
    settings.setValue("cities", m_savedCities);
}
