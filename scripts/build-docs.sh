# Build documentation.
# The target directory should contain a Doxyfile.
# Call this script with the directory containing the Doxyfile
# ./scripts/build-docs libs/client-sdk

cd "$1"
doxygen ./Doxyfile
