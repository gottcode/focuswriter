#!/bin/bash

set -euo pipefail

APP='FocusWriter'
BUNDLE="$APP.app"
VERSION='1.9.0'

find_qt_tool() {
	local tool
	for tool in "$@"; do
		if command -v "$tool" >/dev/null 2>&1; then
			command -v "$tool"
			return 0
		elif [ -n "${QTDIR:-}" ] && [ -x "${QTDIR}/bin/${tool}" ]; then
			printf '%s\n' "${QTDIR}/bin/${tool}"
			return 0
		fi
	done

	return 1
}

copy_qt_plugin() {
	local plugin="$1"
	local source="${QT_PLUGINS}/${plugin}"
	local target="${APP}/${BUNDLE}/Contents/PlugIns/${plugin}"

	if [ ! -f "$source" ]; then
		echo "Missing Qt plugin: $source" >&2
		exit 1
	fi

	mkdir -p "$(dirname "$target")"
	cp "$source" "$target"
	DEPLOY_EXECUTABLES+=("-executable=${SCRIPT_DIR}/${target}")
}

# Locate deployment script
BIN_DIR=$(pwd)
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
cd "$SCRIPT_DIR"

QTPATHS_BIN=$(find_qt_tool qtpaths qtpaths6) || {
	echo 'Unable to find qtpaths. Install Qt 6 and ensure its tools are in PATH or set QTDIR.' >&2
	exit 1
}

QT_PREFIX=$("$QTPATHS_BIN" --query QT_INSTALL_PREFIX)
QT_LIBS=$("$QTPATHS_BIN" --query QT_INSTALL_LIBS)
QT_PLUGINS=$("$QTPATHS_BIN" --query QT_INSTALL_PLUGINS)
QT_TRANSLATIONS=$("$QTPATHS_BIN" --query QT_INSTALL_TRANSLATIONS)

MACDEPLOYQT_BIN=$(find_qt_tool macdeployqt macdeployqt6 || true)
if [ -z "$MACDEPLOYQT_BIN" ] && [ -x "${QT_PREFIX}/bin/macdeployqt" ]; then
	MACDEPLOYQT_BIN="${QT_PREFIX}/bin/macdeployqt"
fi
if [ -z "$MACDEPLOYQT_BIN" ]; then
	echo 'Unable to find macdeployqt. Install Qt 6 and ensure its tools are in PATH or set QTDIR.' >&2
	exit 1
fi

# Remove any previous disk folder or DMG
echo -n 'Preparing... '
rm -f "${APP}_$VERSION.dmg"
if [ -e "/Volumes/$APP" ]; then
	hdiutil detach -quiet "/Volumes/$APP"
fi
rm -Rf "$APP"
echo 'Done'

# Create disk folder
echo -n 'Copying application bundle... '
mkdir "$APP"
cp -Rf "${BIN_DIR}/${BUNDLE}" "$APP/"
strip "$APP/$BUNDLE/Contents/MacOS/$APP"
cp COPYING "$APP/License.txt"
echo 'Done'

# Create ReadMe
echo -n 'Creating ReadMe... '
cp README "$APP/Read Me.txt"
echo >> "$APP/Read Me.txt"
echo >> "$APP/Read Me.txt"
echo 'CREDITS' >> "$APP/Read Me.txt"
echo '=======' >> "$APP/Read Me.txt"
echo >> "$APP/Read Me.txt"
cat CREDITS >> "$APP/Read Me.txt"
echo >> "$APP/Read Me.txt"
echo >> "$APP/Read Me.txt"
echo 'NEWS' >> "$APP/Read Me.txt"
echo '====' >> "$APP/Read Me.txt"
echo >> "$APP/Read Me.txt"
cat ChangeLog >> "$APP/Read Me.txt"
echo 'Done'

# Copy Qt translations
echo -n 'Copying Qt translations... '
TRANSLATIONS="$APP/$BUNDLE/Contents/Resources/translations"
mkdir -p "$TRANSLATIONS"
find "$QT_TRANSLATIONS" -maxdepth 1 -type f \( -name 'qt_*.qm' -o -name 'qtbase_*.qm' \) ! -name 'qt_help_*' -exec cp {} "$TRANSLATIONS" \;
echo 'Done'

# Copy frameworks and plugins
echo -n 'Copying frameworks and plugins... '
DEPLOY_EXECUTABLES=()
DEPLOY_ARGS=(
	"-no-plugins"
	"-libpath=${QT_LIBS}"
)
if [ -d "${QT_PREFIX}/Frameworks" ]; then
	DEPLOY_ARGS+=("-libpath=${QT_PREFIX}/Frameworks")
fi

# Deploy only the plugins the app actually needs on macOS.
copy_qt_plugin 'platforms/libqcocoa.dylib'
copy_qt_plugin 'styles/libqmacstyle.dylib'
copy_qt_plugin 'imageformats/libqjpeg.dylib'

"$MACDEPLOYQT_BIN" "${SCRIPT_DIR}/${APP}/${BUNDLE}" "${DEPLOY_ARGS[@]}" "${DEPLOY_EXECUTABLES[@]}"
echo 'Done'

# Copy background
echo -n 'Copying background... '
mkdir "$APP/.background"
cp resources/mac/background.tiff "$APP/.background/background.tiff"
echo 'Done'

# Create disk image
echo -n 'Creating disk image... '
hdiutil create -quiet -srcfolder "$APP" -volname "$APP" -fs HFS+ -format UDRW 'temp.dmg'
echo 'Done'

echo -n 'Configuring disk image... '
hdiutil attach -quiet -readwrite -noverify -noautoopen 'temp.dmg'
echo '
	tell application "Finder"
		tell disk "'$APP'"
			open

			tell container window
				set the bounds to {400, 100, 949, 458}
				set current view to icon view
				set toolbar visible to false
				set statusbar visible to true
				set the bounds to {400, 100, 800, 460}
			end tell

			set viewOptions to the icon view options of container window
			tell viewOptions
				set arrangement to not arranged
				set icon size to 80
				set label position to bottom
				set shows icon preview to true
				set shows item info to false
			end tell
			set background picture of viewOptions to file ".background:background.tiff"

			make new alias file at container window to POSIX file "/Applications" with properties {name:"Applications"}
			set position of item "'$BUNDLE'" of container window to {90, 90}
			set position of item "Applications" of container window to {310, 90}
			set position of item "Read Me.txt" of container window to {140, 215}
			set position of item "License.txt" of container window to {260, 215}
			close
			open

			update without registering applications
			delay 5
		end tell
	end tell
' | osascript
chmod -Rf go-w "/Volumes/$APP" >& /dev/null
sync
hdiutil detach -quiet "/Volumes/$APP"
echo 'Done'

echo -n 'Compressing disk image... '
hdiutil convert -quiet 'temp.dmg' -format UDBZ -o "${APP}_${VERSION}.dmg"
rm -f temp.dmg
echo 'Done'

# Clean up disk folder
echo -n 'Cleaning up... '
rm -Rf "$APP"
echo 'Done'
