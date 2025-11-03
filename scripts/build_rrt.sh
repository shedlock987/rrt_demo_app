parent_path=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; cd .. ; pwd -P)

# Get project directory (repository root)
cd "$parent_path"

echo "Building Demo App"

# Ensure build directory exists one level up (repo root/build)
build_dir="$parent_path/build"
if [ ! -d "$build_dir" ]; then
	echo "Creating build directory at $build_dir"
	mkdir -p "$build_dir" || { echo "Failed to create build directory"; exit 1; }
fi

echo "Running CMake"
# Configure the rrtDemo subproject into build/
cmake -S rrtDemo -B "$build_dir" || { echo "CMake configuration failed"; exit 1; }

echo "Building App"
cmake --build "$build_dir" -- -j$(sysctl -n hw.ncpu) || { echo "Build failed"; exit 1; }

echo "Done"
echo "Complete"

