parent_path=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; cd .. ; pwd -P)

#Get Project Directory
echo Cleaning Build Files
cd "$parent_path"

# remove macOS Finder artifacts
rm -rf .DS_Store
rm -rf rrtDemo/.DS_Store

rm -rf build

rm -rf CMakeFiles
rm -rf cmake_install.cmake
rm -rf Makefile
rm -rf CMakeCache.txt

cd rrtDemo
rm -rf CMakeFiles
rm -rf cmake_install.cmake
rm -rf Makefile
rm -rf rrtDemo
rm -rf rrtDemo.so
rm -rf librrtDemo.a
cd ..


rm -rf _deps
