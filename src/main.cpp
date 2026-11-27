#include "lms/main_window.hpp"

#include <QApplication>
#include <QDir>
#include <QStandardPaths>

#include <exception>
#include <iostream>
#include <string>

namespace {

std::string toUtf8String(const QString& value) {
    const auto bytes = value.toUtf8();
    return {bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    QCoreApplication::setOrganizationName("Lazy-Destructor");
    QCoreApplication::setApplicationName("Library Management System");

    auto dataDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!QDir().mkpath(dataDirectory)) {
        std::cerr << "Unable to create the application data directory: "
                  << toUtf8String(dataDirectory) << '\n';
        return 1;
    }

    try {
        lms::Library library(toUtf8String(dataDirectory + "/library.duckdb"));
        lms::MainWindow window(library);
        window.show();
        return application.exec();
    } catch (const std::exception& error) {
        std::cerr << "Unable to start the library database: " << error.what() << '\n';
        return 1;
    }
}
