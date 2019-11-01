# benchbuild.sh

# Change to source directory
cd src

# Check operating system and build executable
if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    mingw32-make.exe debug && ./delocto bench
else
    make debug && ./delocto bench
fi
