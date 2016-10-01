#!/bin/bash

APP='FocusWriter'
BUNDLE="$APP.app"
VERSION=$(git describe)

# Remove any previous disk folder or DMG
echo -n 'Preparing... '
rm -f "${APP}_$VERSION.dmg"
if [ -e "/Volumes/${APP}" ]; then
	hdiutil detach -quiet "/Volumes/${APP}"
fi
rm -Rf "$APP"
echo 'Done'

# Create disk folder
echo -n 'Copying application bundle... '
mkdir "$APP"
cp -Rpf "$BUNDLE" "$APP/"
strip "$APP/$BUNDLE/Contents/MacOS/$APP"
cp COPYING "$APP/License.txt"
cp CREDITS "$APP/Credits.txt"
cp README "$APP/Read Me.txt"
echo 'Done'

# Copy translations
echo -n 'Copying translations... '
TRANSLATIONS="$APP/$BUNDLE/Contents/Resources/translations"
mkdir "$TRANSLATIONS"
cp -Rpf translations/*.qm "$TRANSLATIONS"
echo 'Done'

# Copy Qt translations
echo -n 'Copying Qt translations... '
for translation in $(ls translations | grep qm | cut -d'.' -f1 | cut -d'_' -f2- | uniq)
do
	LPROJ="$APP/$BUNDLE/Contents/Resources/${translation}.lproj"
	mkdir "$LPROJ"
	sed "s/????/${translation}/" < 'resources/mac/locversion.plist' > "${LPROJ}/locversion.plist"

	QT_TRANSLATION="${QTDIR}/translations/qtbase_${translation}.qm"
	if [ -e "$QT_TRANSLATION" ]; then
		cp -f "$QT_TRANSLATION" "$TRANSLATIONS"
	fi

	QT_TRANSLATION="${QTDIR}/translations/qtbase_${translation:0:2}.qm"
	if [ -e "$QT_TRANSLATION" ]; then
		cp -f "$QT_TRANSLATION" "$TRANSLATIONS"
	fi
done
echo 'Done'

# Copy frameworks and plugins
echo -n 'Copying frameworks and plugins... '
macdeployqt "$APP/$BUNDLE"
# Remove QML copied in by macdeployqt with >= 4.7.2
rm -Rf "$APP/$BUNDLE/Contents/Frameworks/QtDeclarative.framework"
rm -Rf "$APP/$BUNDLE/Contents/Frameworks/QtScript.framework"
rm -Rf "$APP/$BUNDLE/Contents/Frameworks/QtSql.framework"
rm -Rf "$APP/$BUNDLE/Contents/Frameworks/QtSvg.framework"
rm -Rf "$APP/$BUNDLE/Contents/Frameworks/QtXmlPatterns.framework"
rm -Rf "$APP/$BUNDLE/Contents/PlugIns/qmltooling"
echo 'Done'

# Copy background
echo -n 'Copying background... '
mkdir "${APP}/.background"
cp 'resources/mac/background.png' "${APP}/.background/background.png"
echo 'Done'

# Create disk image
echo -n 'Creating disk image... '
hdiutil create -quiet -srcfolder "${APP}" -volname "${APP}" -fs HFS+ -format UDRW 'temp.dmg'
echo 'Done'

echo -n 'Configuring disk image... '
hdiutil attach -quiet -readwrite -noverify -noautoopen 'temp.dmg'
echo '
	tell application "Finder"
		tell disk "'${APP}'"
			open

			tell container window
				set the bounds to {400, 100, 949, 458}
				set current view to icon view
				set toolbar visible to false
				set statusbar visible to true
				set the bounds to {400, 100, 800, 420}
			end tell

			set viewOptions to the icon view options of container window
			tell viewOptions
				set arrangement to not arranged
				set icon size to 80
				set label position to bottom
				set shows icon preview to true
				set shows item info to false
			end tell
			set background picture of viewOptions to file ".background:background.png"

			make new alias file at container window to POSIX file "/Applications" with properties {name:"Applications"}
			set position of item "'${BUNDLE}'" of container window to {90, 90}
			set position of item "Applications" of container window to {310, 90}
			set position of item "Credits.txt" of container window to {100, 215}
			set position of item "License.txt" of container window to {200, 215}
			set position of item "Read Me.txt" of container window to {300, 215}
			close
			open

			update without registering applications
			delay 5
		end tell
	end tell
' | osascript
chmod -Rf go-w "/Volumes/${APP}" >& /dev/null
sync
hdiutil detach -quiet "/Volumes/${APP}"
echo 'Done'

echo -n 'Compressing disk image... '
hdiutil convert -quiet 'temp.dmg' -format UDBZ -o "${APP}_${VERSION}.dmg"
hdiutil internet-enable -quiet -yes "${APP}_${VERSION}.dmg"
rm -f temp.dmg
echo 'Done'

# Clean up disk folder
echo -n 'Cleaning up... '
rm -Rf "$APP"
echo 'Done'
