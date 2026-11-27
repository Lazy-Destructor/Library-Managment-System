#include "lms/library.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <stdexcept>
#include <utility>

namespace lms {
namespace {

std::string quote(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back('\'');
    for (const char character : value) {
        if (character == '\'') {
            escaped.push_back('\'');
        }
        escaped.push_back(character);
    }
    escaped.push_back('\'');
    return escaped;
}

void execute(duckdb::Connection& connection, const std::string& sql) {
    auto result = connection.Query(sql);
    if (!result || result->HasError()) {
        throw std::runtime_error(result ? result->GetError() : "DuckDB returned no query result");
    }
}

std::vector<std::vector<std::string>> readRows(duckdb::Connection& connection, const std::string& sql) {
    auto result = connection.Query(sql);
    if (!result || result->HasError()) {
        throw std::runtime_error(result ? result->GetError() : "DuckDB returned no query result");
    }

    std::vector<std::vector<std::string>> rows;
    while (auto chunk = result->Fetch()) {
        for (duckdb::idx_t row = 0; row < chunk->size(); ++row) {
            std::vector<std::string> values;
            values.reserve(chunk->ColumnCount());
            for (duckdb::idx_t column = 0; column < chunk->ColumnCount(); ++column) {
                const auto value = chunk->GetValue(column, row);
                values.push_back(value.IsNull() ? std::string{} : value.ToString());
            }
            rows.push_back(std::move(values));
        }
    }
    return rows;
}

std::string required(const std::string& value, const char* field) {
    if (value.empty()) {
        throw std::invalid_argument(std::string(field) + " is required");
    }
    return value;
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

Book bookFrom(const std::vector<std::string>& row) {
    return {row.at(0), row.at(1), row.at(2), row.at(3), row.at(4), std::stoi(row.at(5))};
}

} // namespace

Library::Library(const std::string& databasePath)
    : database_(databasePath.c_str()), connection_(std::make_unique<duckdb::Connection>(database_)) {
    initializeSchema();
}

void Library::initializeSchema() {
    execute(*connection_, R"(
        CREATE TABLE IF NOT EXISTS students (
            roll_number VARCHAR PRIMARY KEY,
            email VARCHAR NOT NULL UNIQUE,
            phone VARCHAR NOT NULL,
            college_year INTEGER NOT NULL CHECK (college_year BETWEEN 1 AND 10),
            branch VARCHAR NOT NULL
        )
    )");
    execute(*connection_, R"(
        CREATE TABLE IF NOT EXISTS books (
            isbn VARCHAR PRIMARY KEY,
            title VARCHAR NOT NULL,
            author VARCHAR NOT NULL,
            topic VARCHAR NOT NULL,
            edition VARCHAR,
            copies INTEGER NOT NULL CHECK (copies > 0)
        )
    )");
    execute(*connection_, R"(
        CREATE TABLE IF NOT EXISTS loans (
            loan_id BIGINT PRIMARY KEY,
            roll_number VARCHAR NOT NULL REFERENCES students(roll_number),
            isbn VARCHAR NOT NULL REFERENCES books(isbn),
            borrowed_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
            due_at TIMESTAMP NOT NULL,
            returned_at TIMESTAMP
        )
    )");
}

void Library::addStudent(const Student& student) {
    required(student.rollNumber, "Roll number");
    required(student.email, "Email");
    required(student.phone, "Phone number");
    required(student.branch, "Branch");
    if (student.collegeYear < 1 || student.collegeYear > 10) {
        throw std::invalid_argument("College year must be between 1 and 10");
    }
    execute(*connection_, "INSERT INTO students VALUES (" + quote(student.rollNumber) + ", " +
        quote(student.email) + ", " + quote(student.phone) + ", " +
        std::to_string(student.collegeYear) + ", " + quote(student.branch) + ")");
}

void Library::addBook(const Book& book) {
    required(book.isbn, "ISBN");
    required(book.title, "Title");
    required(book.author, "Author");
    required(book.topic, "Topic");
    if (book.copies < 1) {
        throw std::invalid_argument("A book must have at least one copy");
    }
    const auto edition = book.edition.empty() ? "NULL" : quote(book.edition);
    execute(*connection_, "INSERT INTO books VALUES (" + quote(book.isbn) + ", " +
        quote(book.title) + ", " + quote(book.author) + ", " + quote(book.topic) + ", " +
        edition + ", " + std::to_string(book.copies) + ")");
}

long long Library::borrowBook(const std::string& rollNumber, const std::string& isbn) {
    required(rollNumber, "Roll number");
    required(isbn, "ISBN");

    execute(*connection_, "BEGIN TRANSACTION");
    try {
        const auto availability = readRows(*connection_,
            "SELECT b.copies - count(l.loan_id) FROM books b "
            "LEFT JOIN loans l ON l.isbn = b.isbn AND l.returned_at IS NULL "
            "WHERE b.isbn = " + quote(isbn) + " GROUP BY b.copies");
        if (availability.empty()) {
            throw std::invalid_argument("Book ISBN was not found");
        }
        if (std::stoll(availability.front().front()) < 1) {
            throw std::invalid_argument("No copies of this book are currently available");
        }

        const auto nextId = readRows(*connection_, "SELECT coalesce(max(loan_id), 0) + 1 FROM loans");
        const auto loanId = std::stoll(nextId.front().front());
        execute(*connection_, "INSERT INTO loans (loan_id, roll_number, isbn, due_at) VALUES (" +
            std::to_string(loanId) + ", " + quote(rollNumber) + ", " + quote(isbn) +
            ", CURRENT_TIMESTAMP + INTERVAL 14 DAY)");
        execute(*connection_, "COMMIT");
        return loanId;
    } catch (...) {
        execute(*connection_, "ROLLBACK");
        throw;
    }
}

void Library::returnBook(long long loanId) {
    const auto activeLoan = readRows(*connection_, "SELECT loan_id FROM loans WHERE loan_id = " +
        std::to_string(loanId) + " AND returned_at IS NULL");
    if (activeLoan.empty()) {
        throw std::invalid_argument("Active loan was not found");
    }
    execute(*connection_, "UPDATE loans SET returned_at = CURRENT_TIMESTAMP WHERE loan_id = " +
        std::to_string(loanId));
}

std::vector<Student> Library::students() {
    const auto rows = readRows(*connection_,
        "SELECT roll_number, email, phone, college_year, branch FROM students ORDER BY roll_number");
    std::vector<Student> result;
    result.reserve(rows.size());
    for (const auto& row : rows) {
        result.push_back({row[0], row[1], row[2], std::stoi(row[3]), row[4]});
    }
    return result;
}

std::vector<Book> Library::books() {
    const auto rows = readRows(*connection_,
        "SELECT isbn, title, author, topic, edition, copies FROM books ORDER BY title");
    std::vector<Book> result;
    result.reserve(rows.size());
    for (const auto& row : rows) {
        result.push_back(bookFrom(row));
    }
    return result;
}

std::vector<Loan> Library::loans() {
    const auto rows = readRows(*connection_,
        "SELECT l.loan_id, l.roll_number, l.isbn, b.title, cast(l.borrowed_at AS VARCHAR), "
        "cast(l.due_at AS VARCHAR), cast(l.returned_at AS VARCHAR) "
        "FROM loans l JOIN books b ON b.isbn = l.isbn ORDER BY l.borrowed_at DESC");
    std::vector<Loan> result;
    result.reserve(rows.size());
    for (const auto& row : rows) {
        result.push_back({std::stoll(row[0]), row[1], row[2], row[3], row[4], row[5], row[6]});
    }
    return result;
}

std::vector<Recommendation> Library::recommendations(const std::string& rollNumber) {
    required(rollNumber, "Roll number");
    const auto history = readRows(*connection_,
        "SELECT b.isbn, b.topic, b.author, count(*) FROM loans l "
        "JOIN books b ON b.isbn = l.isbn WHERE l.roll_number = " + quote(rollNumber) +
        " GROUP BY b.isbn, b.topic, b.author");
    if (history.empty()) {
        throw std::invalid_argument("No borrowing history is available for this student yet");
    }

    std::map<std::string, long long> topicCounts;
    std::map<std::string, long long> authorCounts;
    std::map<std::string, bool> previouslyBorrowed;
    for (const auto& row : history) {
        previouslyBorrowed[row[0]] = true;
        topicCounts[lower(row[1])] += std::stoll(row[3]);
        authorCounts[lower(row[2])] += std::stoll(row[3]);
    }

    const auto catalog = books();
    std::vector<Recommendation> result;
    for (const auto& book : catalog) {
        if (previouslyBorrowed[book.isbn]) {
            continue;
        }
        const auto topicScore = topicCounts[lower(book.topic)];
        const auto authorScore = authorCounts[lower(book.author)];
        if (topicScore + authorScore == 0) {
            continue;
        }
        result.push_back({book, static_cast<double>(topicScore * 2 + authorScore)});
    }
    std::sort(result.begin(), result.end(), [](const Recommendation& left, const Recommendation& right) {
        if (left.score == right.score) {
            return left.book.title < right.book.title;
        }
        return left.score > right.score;
    });
    return result;
}

std::vector<TopicForecast> Library::topicForecast() {
    const auto rows = readRows(*connection_,
        "SELECT b.topic, count(*) FROM loans l JOIN books b ON b.isbn = l.isbn "
        "WHERE l.borrowed_at >= CURRENT_TIMESTAMP - INTERVAL 90 DAY "
        "GROUP BY b.topic ORDER BY count(*) DESC, b.topic");
    std::vector<TopicForecast> result;
    result.reserve(rows.size());
    for (const auto& row : rows) {
        const auto recentCount = std::stoll(row[1]);
        result.push_back({row[0], recentCount, static_cast<double>(recentCount) / 3.0});
    }
    return result;
}

} // namespace lms
