# KGrapher

A function and surface grapher built with **Qt 6**. Plot 2D curves and 3D surfaces from equations, with adjustable ranges, colors, and interactive zoom/rotate. Uses only Qt6 (no KDE Frameworks).

## Requirements

- CMake ≥ 3.20
- Qt 6 (≥ 6.0.0)
- C++17 compiler

### Installing dependencies

**Debian/Ubuntu:**

```bash
sudo apt install cmake build-essential qt6-base-dev
```

**Fedora:**

```bash
sudo dnf install cmake gcc-c++ qt6-qtbase-devel
```

**Arch Linux:**

```bash
sudo pacman -S cmake base-devel qt6-base
```

**openSUSE:**

```bash
sudo zypper install cmake gcc-c++ libqt6-qtbase-devel
```

## Build and run

From the project root:

```bash
cmake -B build/ --install-prefix ~/.local
cmake --build build/ --parallel
cmake --install build/
```

Then run:

```bash
~/.local/bin/kgrapher
```

If `~/.local/bin` is in your `PATH`, you can simply run:

```bash
kgrapher
```

If Qt6 is installed in a custom prefix, point CMake to it:

```bash
cmake -B build/ -DCMAKE_PREFIX_PATH=/path/to/qt6
cmake --build build/ --parallel
```

## Project layout

- `main.cpp` – Application entry point and command-line parsing
- `mainwindow.h` / `mainwindow.cpp` – Main window with equation input, 2D/3D graph views
- `kgrapher.desktop` – Desktop entry for the application menu
- `CMakeLists.txt` – CMake build configuration

## License

GPL.