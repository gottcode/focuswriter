#!/bin/bash

APP='FocusWriter'
BUNDLE="$APP.app"
VERSION='1.3.2.1'

# Create disk folder
echo -n 'Copying application bundle... '
rm -f "${APP}_$VERSION.dmg"
rm -Rf "$APP"
mkdir "$APP"
cp -pf COPYING "$APP/COPYING.txt"
cp -pf README "$APP/README.txt"
cp -Rpf "$BUNDLE" "$APP/"
echo 'Done'

# Copy translations
echo -n 'Copying translations... '
TRANSLATIONS="$APP/$BUNDLE/Contents/Resources/translations"
mkdir "$TRANSLATIONS"
cp -Rpf translations/*.qm "$TRANSLATIONS"
echo 'Done'

# Copy Qt translations
echo -n 'Copying Qt translations... '
for translation in $(ls translations | grep qm | cut -d'.' -f1)
do
	LPROJ="$APP/$BUNDLE/Contents/Resources/${translation}.lproj"
	mkdir "$LPROJ"
	sed "s/????/${translation}/" < locversion.plist > "${LPROJ}/locversion.plist"

	QT_TRANSLATION="/Developer/Applications/Qt/translations/qt_${translation}.qm"
	if [ -e "$QT_TRANSLATION" ]; then
		cp -f "$QT_TRANSLATION" "$TRANSLATIONS"
	fi
done
echo 'Done'

# Copy icons
echo -n 'Copying icons... '
ICONS="$APP/$BUNDLE/Contents/Resources/icons"
mkdir "$ICONS"
cp -Rpf icons/oxygen/hicolor "$ICONS"
echo 'Done'

# Copy dictionaries
echo -n 'Copying dictionaries... '
DICTIONARIES="$APP/$BUNDLE/Contents/Resources/Dictionaries"
mkdir "$DICTIONARIES"
cp -pf dict/* "$DICTIONARIES"
echo 'Done'

# Copy sounds
echo -n 'Copying sounds... '
SOUNDS="$APP/$BUNDLE/Contents/Resources/sounds"
mkdir "$SOUNDS"
cp -Rpf sounds/* "$SOUNDS"
echo 'Done'

# Copy frameworks and plugins
echo -n 'Copying frameworks and plugins... '
macdeployqt "$APP/$BUNDLE"
echo 'Done'

# Create disk image
echo -n 'Creating disk image... '
hdiutil create -quiet -ov -srcfolder "$APP" -format UDBZ -volname "$APP" "${APP}_$VERSION.dmg"
hdiutil internet-enable -quiet -yes "${APP}_$VERSION.dmg"
echo 'Done'

# Clean up disk folder
echo -n 'Cleaning up... '
rm -Rf "$APP"
echo 'Done'
