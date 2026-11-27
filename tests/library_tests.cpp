#include "lms/library.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename Function>
void requireThrows(Function function, const std::string& message) {
    try {
        function();
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error(message);
}

} // namespace

int main() {
    const auto path = std::filesystem::temp_directory_path() / "lms-service-test.duckdb";
    std::filesystem::remove(path);
    try {
        {
            lms::Library library(path.string());
            library.addStudent({"S-001", "student@example.edu", "+1 555 0100", 2, "Computer Science"});
            library.addBook({"978-0000000001", "Data Systems", "A. Writer's Guide", "Databases", "2nd", 1});
            library.addBook({"978-0000000002", "Query Design", "A. Writer", "Databases", "", 2});

            const auto loanId = library.borrowBook("S-001", "978-0000000001");
            require(library.loans().size() == 1, "Borrowing should create a history record");
            requireThrows([&] { library.borrowBook("S-001", "978-0000000001"); },
                "A fully checked-out book must not be borrowable");

            const auto recommendations = library.recommendations("S-001");
            require(recommendations.size() == 1, "Previously borrowed books should not be recommended again");
            require(recommendations.front().book.isbn == "978-0000000002",
                "The borrowed topic and author should influence recommendation ranking");

            library.returnBook(loanId);
            require(!library.loans().front().returnedAt.empty(), "Returning should retain a timestamp in history");
            require(library.borrowBook("S-001", "978-0000000001") > loanId,
                "A returned copy should be available for borrowing again");
            const auto forecast = library.topicForecast();
            require(forecast.size() == 1 && forecast.front().recentLoans == 2,
                "The topic forecast should count recent borrow events");
            require(library.students().size() == 1, "Student registration should be persisted");
            require(library.books().size() == 2, "Books should be persisted");
        }
        std::filesystem::remove(path);
    } catch (const std::exception& error) {
        std::filesystem::remove(path);
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
    std::cout << "All library service tests passed.\n";
    return 0;
}
