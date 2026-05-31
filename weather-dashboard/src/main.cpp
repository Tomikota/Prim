#include <QApplication>
#include "WeatherDashboard.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("Weather Dashboard");
    app.setApplicationVersion("1.0.0");
    app.setStyle("Fusion");

    WeatherDashboard dashboard;
    dashboard.show();

    return app.exec();
}