# Serenkai

Serenkai is a cozy life adventure about quiet days, chance encounters, and unexpected discoveries.

Explore, meet people, listen to their stories, and discover the little things that make a place feel like home.

## Status

In development.

## Building from Source

Serenkai uses CMake and vcpkg to manage its build configuration and dependencies. A copy of vcpkg is included in the repository.

### Prerequisites

#### Linux

Make sure the following packages are installed:

* A C++23-compatible compiler
* CMake 3.21 or later
* Ninja
* Git
* Autoconf
* Autoconf Archive
* Automake
* Libtool

On Arch Linux:

```bash
sudo pacman -S base-devel cmake ninja git autoconf autoconf-archive automake libtool
```

#### Windows

Make sure the following tools are installed:

* A C++23-compatible compiler, such as MSVC or Clang
* CMake 3.21 or later
* Ninja
* Git

### Clone the Repository

Clone the repository:

```bash
git clone https://github.com/SerenkaiGames/serenkai.git
cd serenkai
```

### Bootstrap vcpkg

Initialize the bundled vcpkg installation. This only needs to be done once.

#### Linux / macOS

```bash
./vcpkg/bootstrap-vcpkg.sh -disableMetrics
```

#### Windows

```powershell
.\vcpkg\bootstrap-vcpkg.bat -disableMetrics
```

### Build

Configure the project using the provided CMake preset:

```bash
cmake --preset debug
```

For a release build:

```bash
cmake --preset release
```

Then build the project:

```bash
cmake --build --preset debug
```

Or:

```bash
cmake --build --preset release
```

After a successful build, the executable will be located at:

```text
build/debug/bin/serenkai
build/release/bin/serenkai
```

### Run

Run the game from the project root:

```bash
cmake --build --preset debug --target run
```

For a release build:

```bash
cmake --build --preset release --target run
```

Alternatively, run the executable directly:

```bash
./build/debug/bin/serenkai
```

On Windows, run the corresponding executable from the project root.

The game should be run from the project root so that it can locate its assets correctly.
