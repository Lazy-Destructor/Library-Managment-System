# Library Management System

A cross-platform desktop library system built with C++17, Qt 6 Widgets, and DuckDB.

## Features

- Register students by roll number, email, phone number, college year, and branch.
- Maintain a book catalog with ISBN, title, author, topic, optional edition, and copy count.
- Record borrowing and returns, due dates, and each student's full loan history.
- Recommend books using each student's borrowing history (topic and author preferences).
- Estimate next-month topic demand from the previous 90 days of borrowing activity. This is a transparent trend estimate, not a trained machine-learning model.

The database is stored locally in the platform's application-data directory. DuckDB creates and migrates the required tables when the app starts.

## Build

Requirements: CMake 3.21+, a C++17 compiler, and Qt 6 Widgets. CMake downloads the pinned, precompiled DuckDB C++ library for Windows or Linux; it does not build DuckDB from source.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Run `build/library-management-system` (on Windows, `build/Release/library-management-system.exe` for multi-configuration generators).

## Data model

- `students`: roll number (primary key), email (unique), phone, college year, branch.
- `books`: ISBN (primary key), title, author, topic, optional edition, and number of copies.
- `loans`: student and book references, borrow and due timestamps, and nullable return timestamp. A return updates the existing loan row so history is retained.

The GitHub Actions workflow builds and runs the service tests on Ubuntu and Windows.
