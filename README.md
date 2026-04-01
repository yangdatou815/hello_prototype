# Hello World Prototype

This project is a simplified prototype based on the structure of `/var/fpwork/m7yang/netconf_9299`,
demonstrating a basic C++ application with separate unit tests (UT) and system component tests (SCT).

## Project Structure

- `src/`: Main C++ implementation (`Client.cpp`, `Server.cpp`, `Session.cpp`, `Crypto.cpp`, `Database.cpp`, `main.cpp`).
- `include/hello_prototype/`: Public headers (`Client.hpp`, `Server.hpp`, `Session.hpp`, `Crypto.hpp`, `Database.hpp`, `Peer.hpp`).
- `test/ut/`: C++ unit tests using GoogleTest (`test_greeter.cpp`).
- `test/sct/`: Python system component tests using pytest/tox (`test_hello_app.py`).
- `.github/review/SKILL.md`: Review checklist skill (提交前评审门禁).
- `.github/coverage/SKILL.md`: Coverage gate skill (UT>=80%, SCT=100%).
- `.github/gitpush/SKILL.md`: End-to-end submission workflow skill.
- `CMakeLists.txt`: Top-level build entry.

## How to Build and Run

### 1. Prerequisites

- C++ compiler (g++)
- CMake (>= 3.10)
- Python 3.7+
- `tox` (`python3 -m pip install --user tox`)
- `gcovr` (`python3 -m pip install --user gcovr`) for UT coverage report

### 2. Build the C++ Application and Unit Tests

```bash
mkdir -p build
cd build
cmake ..
make -j
```

This will create two executables:
- `build/src/hello_app`: The main application.
- `build/test/ut/ut_greeter`: The unit test runner.

### 3. Run the Unit Tests (UT)

After building, run the unit tests:

```bash
ctest --output-on-failure
```

### 4. Run the System Component Tests (SCT)

The SCT tests are managed by `tox`. They will run the compiled `hello_app` and check its output.

From the project root directory (`hello_prototype`):

```bash
cd test/sct
python3 -m tox
```

You can also run a specific test case by passing pytest arguments after `--`:

```bash
cd test/sct
python3 -m tox -- test_hello_app.py::test_hello_exception
```

### 5. Run Coverage Gate (Required before push)

Current gate rules:
- UT line coverage (`src/*`) must be `>= 80%`
- SCT pass coverage must be `= 100%`

Run coverage workflow:

```bash
cmake -S . -B build-coverage -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS='--coverage -O0' -DCMAKE_CXX_FLAGS='--coverage -O0'
cmake --build build-coverage -j
ctest --test-dir build-coverage --output-on-failure
python3 -m gcovr -r . --object-directory build-coverage --filter 'src/' --exclude 'test/' --exclude-directories 'build-coverage/CMakeFiles|build-coverage/_deps' --gcov-ignore-errors source_not_found --gcov-ignore-errors no_working_dir_found --txt
cd test/sct && python3 -m tox
```

Coverage evidence files are maintained under:
- `.github/coverage/REPORT.md`
- `.github/coverage/artifacts/*`

## Submission Workflow

For "提交代码", follow `.github/gitpush/SKILL.md`:
1. Run review gate (`.github/review/SKILL.md`)
2. Run coverage gate (`.github/coverage/SKILL.md`)
3. Build + UT/SCT checks
4. `git add` + `git commit` + `git push`

## Design Philosophy

- **Separation of Concerns**: Communication (`Client`/`Server`/`Session`), state (`Peer`), crypto (`Crypto`) and persistence (`Database`) are separated.
- **Test Layers**:
  - **Unit Tests (UT)**: Test crypto/state/database/handshake and key error paths in C++/GoogleTest.
  - **System Component Tests (SCT)**: Test the compiled application as a black box. They are run using Python and `pytest`, simulating how an external component would interact with the application. This layer is great for integration and end-to-end checks.
- **Automation with `tox`**: `tox` automates the setup and execution of the SCT environment, ensuring consistency, similar to how the `netconf_9299` project works.
- **Quality Gates**: Review + coverage gates are required before push.
- **Extensible Build System**: CMake is used to manage the build process, making it easy to add new source files and test cases.
