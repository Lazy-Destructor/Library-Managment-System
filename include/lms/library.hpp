#pragma once

#include "duckdb.hpp"

#include <memory>
#include <string>
#include <vector>

namespace lms {

struct Student {
    std::string rollNumber;
    std::string email;
    std::string phone;
    int collegeYear;
    std::string branch;
};

struct Book {
    std::string isbn;
    std::string title;
    std::string author;
    std::string topic;
    std::string edition;
    int copies;
};

struct Loan {
    long long id;
    std::string rollNumber;
    std::string isbn;
    std::string title;
    std::string borrowedAt;
    std::string dueAt;
    std::string returnedAt;
};

struct Recommendation {
    Book book;
    double score;
};

struct TopicForecast {
    std::string topic;
    long long recentLoans;
    double estimatedNextMonth;
};

class Library {
public:
    explicit Library(const std::string& databasePath);

    void addStudent(const Student& student);
    void addBook(const Book& book);
    long long borrowBook(const std::string& rollNumber, const std::string& isbn);
    void returnBook(long long loanId);

    std::vector<Student> students();
    std::vector<Book> books();
    std::vector<Loan> loans();
    std::vector<Recommendation> recommendations(const std::string& rollNumber);
    std::vector<TopicForecast> topicForecast();

private:
    duckdb::DuckDB database_;
    std::unique_ptr<duckdb::Connection> connection_;

    void initializeSchema();
};

} // namespace lms
