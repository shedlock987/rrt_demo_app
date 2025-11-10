# rrt_demo_app

Example/Demo application that builds a Python extension (Boost.Python) around the
`rrt_graph_builder` library and provides a small visualizer in
`graph_visualizer/`.

Below are quick dependency and build instructions for macOS (Homebrew) and
Linux (Ubuntu/Debian). Use the section that matches your platform.

## Dependencies

### macOS (Homebrew)

Install the required tools with Homebrew:

```bash
# install developer tools if needed (Xcode command line tools)
xcode-select --install

# Homebrew packages
brew update
brew install cmake boost python

# Optional: create / use a Python virtualenv
python3 -m venv .venv
source .venv/bin/activate
pip install -U pip wheel
pip install matplotlib numpy
```

Notes:
- Homebrew's `boost` includes Boost.Python components for the active Python.
- If Boost.Python cannot be found, ensure the `python3` from Homebrew is first
	on your PATH when configuring CMake.

### Linux (Ubuntu/Debian)

Install the common build dependencies (package names vary by distro/version):

```bash
# update
sudo apt update

# basic build tools, CMake, Python dev headers and Boost
sudo apt install -y build-essential cmake python3-dev python3-venv \
		libboost-all-dev

# Optional: create virtualenv and install python deps
python3 -m venv .venv
source .venv/bin/activate
pip install -U pip wheel
pip install matplotlib numpy
```

Notes:
- On some distributions you may need specific Boost.Python packages (for
	example `libboost-python3-dev` or `libboost-python1.74-dev`). If CMake
	cannot find Boost.Python, search your distro packages for `boost-python`.

## Prebuilt libraries (optional)

If you have a local checkout or prebuilt artifacts of `rrt_graph_builder`, the
project looks for platform-specific prebuilt libs under:

```
<path-to>/rrt_graph_builder/prebuilt/macOS
<path-to>/rrt_graph_builder/prebuilt/linux
```

Place the compiled libraries there (macOS: `*.dylib`, Linux: `*.so`), or put
the build output under `rrt_graph_builder/build/graphLib` — the top-level
CMake will detect and prefer these local prebuilt artifacts instead of
downloading the upstream repo via CPM.

## Build

From the repository root:

```bash
# clean any previous build
rm -rf build

# configure & build (the provided script does this)
./scripts/build_rrt.sh

# if you want a fresh clean build
./scripts/fresh_build_rrt.sh
```

## Run the visualizer

After a successful build the Python extension will be under `build/`.
Run the visualizer (from the repo root):

```bash
python3 graph_visualizer/graph_visualizer.py
```

If you hit issues during configuration or linking (Boost.Python or Python
library issues), paste the top portion of the CMake configure output and I can
help adjust the CMake settings for your environment.

Happy hacking!

