# runtests.sh

# Check operating system and build test executable
if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    mingw32-make.exe
else
    make
fi

# Run tests
./tests

# Exit with test exit code
exit $?