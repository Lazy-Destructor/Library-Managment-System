#include "lms/main_window.hpp"

#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <exception>
#include <string>

namespace lms {
namespace {

QTableWidget* createTable(int columns, const QStringList& headers) {
    auto* table = new QTableWidget(0, columns);
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    return table;
}

void setRow(QTableWidget* table, int row, const QStringList& values) {
    table->insertRow(row);
    for (int column = 0; column < values.size(); ++column) {
        table->setItem(row, column, new QTableWidgetItem(values[column]));
    }
}

QLineEdit* addLine(QFormLayout* form, const QString& label) {
    auto* input = new QLineEdit;
    form->addRow(label, input);
    return input;
}

std::string toUtf8String(const QString& value) {
    const auto bytes = value.toUtf8();
    return {bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

QString fromUtf8String(const std::string& value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

} // namespace

MainWindow::MainWindow(Library& library)
    : library_(library),
      studentsTable_(nullptr),
      booksTable_(nullptr),
      loansTable_(nullptr),
      recommendationsTable_(nullptr),
      forecastTable_(nullptr),
      rollNumberInput_(nullptr),
      emailInput_(nullptr),
      phoneInput_(nullptr),
      branchInput_(nullptr),
      yearInput_(nullptr),
      isbnInput_(nullptr),
      titleInput_(nullptr),
      authorInput_(nullptr),
      topicInput_(nullptr),
      editionInput_(nullptr),
      copiesInput_(nullptr),
      loanRollInput_(nullptr),
      loanIsbnInput_(nullptr),
      recommendationRollInput_(nullptr) {
    setWindowTitle("Library Management System");
    resize(1100, 720);
    auto* tabs = new QTabWidget(this);
    setCentralWidget(tabs);

    auto* studentsPage = new QWidget;
    auto* studentsLayout = new QVBoxLayout(studentsPage);
    auto* studentForm = new QFormLayout;
    rollNumberInput_ = addLine(studentForm, "Roll number");
    emailInput_ = addLine(studentForm, "Email");
    phoneInput_ = addLine(studentForm, "Phone number");
    yearInput_ = new QSpinBox;
    yearInput_->setRange(1, 10);
    studentForm->addRow("College year", yearInput_);
    branchInput_ = addLine(studentForm, "Branch");
    studentsLayout->addLayout(studentForm);
    auto* addStudentButton = new QPushButton("Register student");
    studentsLayout->addWidget(addStudentButton);
    studentsTable_ = createTable(5, {"Roll number", "Email", "Phone", "Year", "Branch"});
    studentsLayout->addWidget(studentsTable_);
    connect(addStudentButton, &QPushButton::clicked, this, [this] {
        try {
            library_.addStudent({toUtf8String(rollNumberInput_->text()), toUtf8String(emailInput_->text()),
                toUtf8String(phoneInput_->text()), yearInput_->value(), toUtf8String(branchInput_->text())});
            rollNumberInput_->clear();
            emailInput_->clear();
            phoneInput_->clear();
            branchInput_->clear();
            refresh();
        } catch (const std::exception& error) {
            showError(error);
        }
    });
    tabs->addTab(studentsPage, "Students");

    auto* booksPage = new QWidget;
    auto* booksLayout = new QVBoxLayout(booksPage);
    auto* bookForm = new QFormLayout;
    isbnInput_ = addLine(bookForm, "ISBN");
    titleInput_ = addLine(bookForm, "Title");
    authorInput_ = addLine(bookForm, "Author");
    topicInput_ = addLine(bookForm, "Topic");
    editionInput_ = addLine(bookForm, "Edition (optional)");
    copiesInput_ = new QSpinBox;
    copiesInput_->setRange(1, 100000);
    copiesInput_->setValue(1);
    bookForm->addRow("Copies", copiesInput_);
    booksLayout->addLayout(bookForm);
    auto* addBookButton = new QPushButton("Add book");
    booksLayout->addWidget(addBookButton);
    booksTable_ = createTable(6, {"ISBN", "Title", "Author", "Topic", "Edition", "Copies"});
    booksLayout->addWidget(booksTable_);
    connect(addBookButton, &QPushButton::clicked, this, [this] {
        try {
            library_.addBook({toUtf8String(isbnInput_->text()), toUtf8String(titleInput_->text()),
                toUtf8String(authorInput_->text()), toUtf8String(topicInput_->text()),
                toUtf8String(editionInput_->text()), copiesInput_->value()});
            isbnInput_->clear();
            titleInput_->clear();
            authorInput_->clear();
            topicInput_->clear();
            editionInput_->clear();
            refresh();
        } catch (const std::exception& error) {
            showError(error);
        }
    });
    tabs->addTab(booksPage, "Books");

    auto* loansPage = new QWidget;
    auto* loansLayout = new QVBoxLayout(loansPage);
    auto* loanForm = new QFormLayout;
    loanRollInput_ = addLine(loanForm, "Student roll number");
    loanIsbnInput_ = addLine(loanForm, "Book ISBN");
    loansLayout->addLayout(loanForm);
    auto* loanButtons = new QWidget;
    auto* loanButtonsLayout = new QHBoxLayout(loanButtons);
    auto* borrowButton = new QPushButton("Borrow book");
    auto* returnButton = new QPushButton("Return selected loan");
    loanButtonsLayout->addWidget(borrowButton);
    loanButtonsLayout->addWidget(returnButton);
    loansLayout->addWidget(loanButtons);
    loansTable_ = createTable(7, {"Loan ID", "Roll number", "ISBN", "Title", "Borrowed", "Due", "Returned"});
    loansLayout->addWidget(loansTable_);
    connect(borrowButton, &QPushButton::clicked, this, [this] {
        try {
            library_.borrowBook(toUtf8String(loanRollInput_->text()), toUtf8String(loanIsbnInput_->text()));
            refresh();
        } catch (const std::exception& error) {
            showError(error);
        }
    });
    connect(returnButton, &QPushButton::clicked, this, [this] {
        const auto row = loansTable_->currentRow();
        if (row < 0) {
            QMessageBox::information(this, "Select a loan", "Select an active loan in the table first.");
            return;
        }
        try {
            library_.returnBook(loansTable_->item(row, 0)->text().toLongLong());
            refresh();
        } catch (const std::exception& error) {
            showError(error);
        }
    });
    tabs->addTab(loansPage, "Borrowing history");

    auto* insightsPage = new QWidget;
    auto* insightsLayout = new QVBoxLayout(insightsPage);
    auto* recommendationForm = new QFormLayout;
    recommendationRollInput_ = addLine(recommendationForm, "Student roll number");
    insightsLayout->addLayout(recommendationForm);
    auto* recommendButton = new QPushButton("Recommend books");
    insightsLayout->addWidget(recommendButton);
    recommendationsTable_ = createTable(4, {"ISBN", "Title", "Author", "Preference score"});
    insightsLayout->addWidget(recommendationsTable_);
    insightsLayout->addWidget(new QLabel("Estimated next-month borrowing demand by topic (last 90 days):"));
    forecastTable_ = createTable(3, {"Topic", "Loans in last 90 days", "Estimated next month"});
    insightsLayout->addWidget(forecastTable_);
    connect(recommendButton, &QPushButton::clicked, this, [this] {
        try {
            refreshRecommendations();
        } catch (const std::exception& error) {
            showError(error);
        }
    });
    tabs->addTab(insightsPage, "Recommendations & trends");

    refresh();
}

void MainWindow::showError(const std::exception& error) {
    QMessageBox::warning(this, "Library operation failed", QString::fromUtf8(error.what()));
}

void MainWindow::refresh() {
    studentsTable_->setRowCount(0);
    for (const auto& student : library_.students()) {
        setRow(studentsTable_, studentsTable_->rowCount(),
            {fromUtf8String(student.rollNumber), fromUtf8String(student.email),
             fromUtf8String(student.phone), QString::number(student.collegeYear),
             fromUtf8String(student.branch)});
    }

    booksTable_->setRowCount(0);
    for (const auto& book : library_.books()) {
        setRow(booksTable_, booksTable_->rowCount(),
            {fromUtf8String(book.isbn), fromUtf8String(book.title),
             fromUtf8String(book.author), fromUtf8String(book.topic),
             fromUtf8String(book.edition), QString::number(book.copies)});
    }

    loansTable_->setRowCount(0);
    for (const auto& loan : library_.loans()) {
        setRow(loansTable_, loansTable_->rowCount(),
            {QString::number(loan.id), fromUtf8String(loan.rollNumber),
             fromUtf8String(loan.isbn), fromUtf8String(loan.title),
             fromUtf8String(loan.borrowedAt), fromUtf8String(loan.dueAt),
             fromUtf8String(loan.returnedAt)});
    }

    forecastTable_->setRowCount(0);
    for (const auto& forecast : library_.topicForecast()) {
        setRow(forecastTable_, forecastTable_->rowCount(),
            {fromUtf8String(forecast.topic), QString::number(forecast.recentLoans),
             QString::number(forecast.estimatedNextMonth, 'f', 1)});
    }
}

void MainWindow::refreshRecommendations() {
    recommendationsTable_->setRowCount(0);
    const auto recommendations = library_.recommendations(toUtf8String(recommendationRollInput_->text()));
    for (const auto& recommendation : recommendations) {
        setRow(recommendationsTable_, recommendationsTable_->rowCount(),
            {fromUtf8String(recommendation.book.isbn),
             fromUtf8String(recommendation.book.title),
             fromUtf8String(recommendation.book.author),
             QString::number(recommendation.score, 'f', 1)});
    }
}

} // namespace lms
