# Dedupe++

A C++17 utility for finding duplicate files in directories. Available as both a command-line tool and a Qt-based GUI application.

## Features

- Recursive directory scanning
- Duplicate file detection using SHA-256 hashing
- Progress reporting and cancellation support
- Modern C++17 implementation
- Clean architecture separating core functionality from UI
- Both CLI and GUI interfaces

## Building

### Prerequisites

- C++17 compatible compiler
- CMake 3.15 or higher
- Qt6 (Core, Widgets, Concurrent modules)
- OpenSSL development libraries
- Filesystem library (usually included with C++17)

### Build Instructions

```bash
mkdir build
cd build
cmake ..
cmake --build .
```
This will build both the CLI (`dedupe_cli`) and GUI (`dedupe_gui`) applications.

## Quick command reference
Use Visual Studio Powershell from the Tools menu to set up tool paths for VS2022. These commands roughly in the order they were used.

Original git create
```powershell
git remote add origin https://github.com/richardjameslewis/dedupeplusplus.git
```

Base cmake command
```powershell
 cmake .. -DCMAKE_TOOLCHAIN_FILE="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\vcpkg\scripts\buildsystems\vcpkg.cmake" -DQt6_DIR="C:\Qt\6.8.3\msvc2022_64\lib\cmake\Qt6"
```
Update packages using vcpkg
```powershell
  vcpkg install
```
Install qt dlls and dependencies for exe
```powershell
 <<Qt Directory>>\windeployqt.exe "C:\Projects\dedupe++\build\Release\dedupe_gui.exe"
```
Build Debug only followed by build tests - choose one
```powershell
  cd build
  cmake --build . --config Debug
  cmake --build . --target dedupe_tests
```
Run tests in gtest
```powershell
ctest -C Debug --output-on-failure
```
Run programs. Can be debugged and run from VS2022 too.
```powershell
.\Debug\dedupe_gui.exe
.\Debug\dedupe_cli.exe 'C:\TODO\Peel Sessions\'
```

### Bugs & TODO
Identical directories need work. isIdentical isn't right.
               
Run the worker on a thread, fix UI updates. - Fixed with QCoreApplication::processEvents() for now.

Performance - can the algorithms be more efficient?

Performance - are we efficient in our C++ implementation?

Run hashing in parallel.

Remove or improve command-line version.

Elegance???

### Qt LGPL Compliance

This project uses Qt under the LGPL v3 license. To comply with LGPL requirements:

1. The application uses dynamic linking for Qt libraries
2. Users must be able to replace Qt DLLs with their own versions
3. Instructions for replacing Qt DLLs:
   - Qt DLLs are located in your Qt installation directory
   - Required DLLs: Qt6Core.dll, Qt6Gui.dll, Qt6Widgets.dll, Qt6Concurrent.dll
   - Copy these DLLs to the same directory as dedupe_gui.exe
   - Users can replace these DLLs with their own versions

For more information about Qt LGPL compliance, see:
- [Qt LGPL FAQ](https://www.qt.io/licensing/open-source-lgpl-obligations)
- [Qt LGPL License](https://www.gnu.org/licenses/lgpl-3.0.html)

## Usage

### GUI Application
```bash
dedupe_gui
```

Features:
- Directory selection via browse button
- Recursive scanning option
- Progress bar and status updates
- Cancelable scanning
- Detailed results display

### Command Line Application
```bash
dedupe_cli [options] <directory>
```

Options:
- `--help`: Show help message
- `--no-recursive`: Do not scan directories recursively (default: recursive)

## Project Structure

```
dedupe++/
├── core/           # Core functionality (file system, hashing)
├── interface/      # Interface layer for UI integration
├── ui/            # Qt-based GUI application
└── CMakeLists.txt # Build configuration
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

Note: The Qt components used in this project are licensed under the LGPL v3 license.
