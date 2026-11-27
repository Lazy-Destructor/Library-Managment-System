#pragma once

#include "lms/library.hpp"

#include <QMainWindow>

class QLineEdit;
class QSpinBox;
class QTableWidget;

namespace lms {

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(Library& library);

private:
    Library& library_;
    QTableWidget* studentsTable_;
    QTableWidget* booksTable_;
    QTableWidget* loansTable_;
    QTableWidget* recommendationsTable_;
    QTableWidget* forecastTable_;
    QLineEdit* rollNumberInput_;
    QLineEdit* emailInput_;
    QLineEdit* phoneInput_;
    QLineEdit* branchInput_;
    QSpinBox* yearInput_;
    QLineEdit* isbnInput_;
    QLineEdit* titleInput_;
    QLineEdit* authorInput_;
    QLineEdit* topicInput_;
    QLineEdit* editionInput_;
    QSpinBox* copiesInput_;
    QLineEdit* loanRollInput_;
    QLineEdit* loanIsbnInput_;
    QLineEdit* recommendationRollInput_;

    void refresh();
    void showError(const std::exception& error);
    void refreshRecommendations();
};

} // namespace lms
