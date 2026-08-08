#!/bin/sh

PROJECT_NAME=$(find . -name "*.xcodeproj" | sed 's/\.xcodeproj//g' | sed 's/^\.\///g' | tr '[:upper:]' '[:lower:]')
VERSION=$(grep "#define VERSION " src/include/config.h | cut -d " " -f 3 | tr -d '"')

AVAILABLE_PLATFORMS="mac win lin"

echo "Building $PROJECT_NAME.xpl version $VERSION. Is this correct? (y/n) [default: y]:"
read CONFIRM

if [ -z "$CONFIRM" ]; then
    CONFIRM="y"
fi

if [ "$CONFIRM" != "y" ]; then
    echo "Please update the version number in config.h and try again."
    exit 1
fi

echo "Which platforms would you like to build? ($AVAILABLE_PLATFORMS) [default: all]:"
read PLATFORMS

if [ -z "$PLATFORMS" ]; then
    PLATFORMS=$AVAILABLE_PLATFORMS
fi

for platform in $PLATFORMS; do
    if ! echo $AVAILABLE_PLATFORMS | grep -q $platform; then
        echo "Invalid platform: $platform. Exiting."
        exit 1
    fi
done

echo "Building for platforms: \033[1m$PLATFORMS\033[0m\n"

if [ ! -d "SDK" ]; then
    echo "SDK/ folder not found. Please ensure the SDK is present in the project root."
    exit 1
fi

SDK_VERSION=$(grep "#define kXPLM_Version" SDK/CHeaders/XPLM/XPLMDefs.h | awk '{print $3}' | tr -d '()')
JOBS=$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)

echo "Building with SDK version $SDK_VERSION\n"

echo "Clean build directory? (y/n) [default: n]:"
read CLEAN_BUILD

if [ -z "$CLEAN_BUILD" ]; then
    CLEAN_BUILD="n"
fi

echo "Upload to Google Drive after build? (y/n) [default: y]:"
read UPLOAD_TO_DRIVE

if [ -z "$UPLOAD_TO_DRIVE" ]; then
    UPLOAD_TO_DRIVE="y"
fi

if [ "$CLEAN_BUILD" = "y" ]; then
    echo "Cleaning build directories..."
    if [ -d "build" ]; then
        rm -rf build
    fi
fi

for platform in $PLATFORMS; do
    echo "Building $platform..."
    if [ $platform = "lin" ]; then
        docker build -t xplane-build:jammy-gcc13 -f ./docker/Dockerfile.linux . && \
        docker run --user $(id -u):$(id -g) --rm -v $(pwd):/src -w /src xplane-build:jammy-gcc13 bash -c "\
        cmake -DCMAKE_CXX_FLAGS='-march=x86-64' -DCMAKE_TOOLCHAIN_FILE=toolchain-$platform.cmake -DSDK_VERSION=$SDK_VERSION -Bbuild/$platform -H. && \
        make -C build/$platform -j\$(nproc) && \
        if objdump -T build/$platform/${platform}_x64/${PROJECT_NAME}.xpl | grep -q '__isoc23_'; then \
            echo 'ERROR: this build links post-glibc-2.36 symbols:'; \
            objdump -T build/$platform/${platform}_x64/${PROJECT_NAME}.xpl | grep -o '__isoc23_[a-z]*' | sort -u; \
            echo 'The Linux image base is too new; the plugin will not load on older distros. See docker/Dockerfile.linux.'; \
            exit 1; \
        fi"
    else
        cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-$platform.cmake -DSDK_VERSION=$SDK_VERSION -Bbuild/$platform -H.
        make -C build/$platform -j$JOBS
    fi

    if [ $? -eq 0 ]; then
        echo "\n\n"
        echo "\033[1;32m$platform build succeeded.\033[0m\nProduct: build/$platform/${platform}_x64/${PROJECT_NAME}.xpl"
        file build/$platform/${platform}_x64/${PROJECT_NAME}.xpl
        sleep 3
    else
        echo "\033[1;31m$platform build failed.\033[0m"
        exit 1
    fi
done

echo "Building has finished."

echo "Creating distribution bundle..."
if [ -d "build/dist" ]; then
    rm -rf build/dist
fi

for platform in $AVAILABLE_PLATFORMS; do
    mkdir -p build/dist/${platform}_x64
    if [ -d "build/$platform/${platform}_x64" ]; then
        cp build/$platform/${platform}_x64/${PROJECT_NAME}.xpl build/dist/${platform}_x64/${PROJECT_NAME}.xpl
    fi
done

cp -r fonts build/dist

# Only add Skunkcrafts for XP12
if [ $SDK_VERSION -ge 400 ]; then
    echo "module|https://ramonster.nl/winctrl-plugin\nname|WINCTRL\nversion|$VERSION\nlocked|false\ndisabled|false\nzone|custom" > build/dist/skunkcrafts_updater.cfg
fi

cd build
mv dist $PROJECT_NAME

if [ $SDK_VERSION -lt 400 ]; then
    XPLANE_VERSION=XP11
else
    XPLANE_VERSION=XP12
fi

VERSION=$VERSION-$XPLANE_VERSION

rm -f $PROJECT_NAME-$VERSION.zip
zip -rq $PROJECT_NAME-$VERSION.zip $PROJECT_NAME -x "*/.DS_Store" -x "*/__MACOSX/*"

mv $PROJECT_NAME dist
cd ..

echo "Bundle created. Distribution: build/dist/$PROJECT_NAME-$VERSION.zip"

# Upload to Google Drive if requested and gdrive is available
if [ "$UPLOAD_TO_DRIVE" = "y" ] && command -v gdrive &> /dev/null; then
    echo "Uploading to Google Drive..."
    FOLDER="1NtjQGUKH9Y8hrfOscwMPC99bVMnh7C0x"

    # Delete old file with same name if it exists
    OLD_FILE_ID=$(gdrive files list --parent $FOLDER | grep "$PROJECT_NAME-$VERSION.zip" | awk '{print $1}')
    if [ ! -z "$OLD_FILE_ID" ]; then
        echo "Removing old version..."
        gdrive files delete $OLD_FILE_ID
    fi

    FILE_ID=$(gdrive files upload --parent $FOLDER --print-only-id "build/$PROJECT_NAME-$VERSION.zip")
    if [ ! -z "$FILE_ID" ]; then
        echo "\033[1;36mFile was uploaded to Google Drive:\033[0m"
        echo "\033[1;36mhttps://drive.google.com/file/d/$FILE_ID/view\033[0m"
        echo "\n\033[1;36mFolder link:\033[0m"
        echo "\033[1;36mhttps://drive.google.com/drive/folders/$FOLDER\033[0m"
    fi
fi

./update_readme.sh
