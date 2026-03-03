# Hello World Prototype

This project is a simplified prototype based on the structure of `/var/fpwork/m7yang/netconf_9299`,
demonstrating a basic C++ application with separate unit tests (UT) and system component tests (SCT).

## Project Structure

- `src/`: Contains the main application source code (`main.cpp`, `Greeter.cpp`, `Greeter.hpp`).
- `test/ut/`: Contains the C++ unit tests using GoogleTest (`test_greeter.cpp`).
- `test/sct/`: Contains the Python-based system component tests using pytest and tox.
- `CMakeLists.txt`: Main CMake file to build the project.

## How to Build and Run

### 1. Prerequisites

- C++ compiler (g++)
- CMake (>= 3.10)
- GoogleTest library (`libgtest-dev`)
- Python 3.7+
- `tox` (`pip install tox`)

### 2. Build the C++ Application and Unit Tests

```bash
mkdir build
cd build
cmake ..
make
```

This will create two executables:
- `build/src/hello_app`: The main application.
- `build/test/ut/ut_greeter`: The unit test runner.

### 3. Run the Unit Tests (UT)

After building, run the unit tests:

```bash
./test/ut/ut_greeter
```
You should see output indicating that the test passed.

### 4. Run the System Component Tests (SCT)

The SCT tests are managed by `tox`. They will run the compiled `hello_app` and check its output.

From the project root directory (`hello_prototype`):

```bash
cd test/sct
tox
```

You can also run a specific test case by passing pytest arguments after `--`:

```bash
cd test/sct
tox -- test_hello_app.py::test_hello_exception
```

Tox will create a virtual environment, run the `prepare_env.sh` script, and then execute the `pytest` tests defined in `test_hello_app.py`.

## Design Philosophy

- **Separation of Concerns**: The application logic (`Greeter` class) is separate from the main entry point (`main.cpp`).
- **Test Layers**:
  - **Unit Tests (UT)**: Test the `Greeter` class in isolation using C++ and GoogleTest. They are fast and check correctness at the class level.
  - **System Component Tests (SCT)**: Test the compiled application as a black box. They are run using Python and `pytest`, simulating how an external component would interact with the application. This layer is great for integration and end-to-end checks.
- **Automation with `tox`**: `tox` automates the setup and execution of the SCT environment, ensuring consistency, similar to how the `netconf_9299` project works.
- **Extensible Build System**: CMake is used to manage the build process, making it easy to add new source files and test cases.
