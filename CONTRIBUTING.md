# Contributing to Coi

Thank you for your interest in contributing to Coi! We welcome contributions from everyone.

## Getting Started

1.  **Fork the repository** on GitHub.
2.  **Clone your fork** locally:
    ```bash
    git clone https://github.com/your-username/coi.git
    cd coi
    ```
3.  **Install dependencies**:
    *   Ensure you have a C++ compiler (Clang) and Ninja.
    *   Ensure you have Python 3 installed.
4.  **Build the project**:
    ```bash
    ./build.sh
    ```

## How to Contribute

### Reporting Bugs

If you find a bug, please create a new issue. Include:
*   A minimal code snippet that reproduces the error.
*   The expected behavior vs the actual behavior.
*   Your OS and Coi version.

### Suggesting Features (RFCs)

For major language changes, please open a standard issue first to discuss the idea. If the change is substantial, we may ask for a Request for Comments (RFC) document describing the syntax and semantics in detail.

### Pull Requests

1.  **Create a new branch** on your fork (avoid submitting PRs from `main` so you can keep it synced).
2.  Write tests for your changes in the `tests/` directory.
3.  Ensure all tests pass:
    ```bash
    ./tests/run.sh
    ```
4.  Submit a Pull Request and describe your changes.

## Code Style

*   Please **orient yourself to the surrounding code style**.
*   Ensure your code is formatted similarly to the existing codebase before submitting.

## Community

Join our Discord server (linked in README) to discuss development and get help.
