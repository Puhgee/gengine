#!/bin/sh
BUILD_DIR=$1
INSTALL_DIR=$2

# Make sure install directory (and intermediates) exist.
mkdir -p "$INSTALL_DIR"

# Copy the app bundle to the install directory.
cp -r "${BUILD_DIR}/Gabriel Knight 3.app" "$INSTALL_DIR"

# Get rid of copyrighted material in the bundle.
rm "$INSTALL_DIR/Gabriel Knight 3.app/Contents/Resources/Assets/GK3"/*.brn
rm "$INSTALL_DIR/Gabriel Knight 3.app/Contents/Resources/Assets/GK3"/*.bik
rm "$INSTALL_DIR/Gabriel Knight 3.app/Contents/Resources/Assets/GK3"/*.BIK
rm "$INSTALL_DIR/Gabriel Knight 3.app/Contents/Resources/Assets/GK3"/*.avi