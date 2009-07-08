#!/bin/bash

APP='FocusWriter'
BUNDLE="$APP.app"
VERSION='1.1.2'

DICTIONARIES="$BUNDLE/Contents/Resources/Dictionaries"
mkdir $DICTIONARIES
cp -f hunspell/en_US/en_US.aff $DICTIONARIES
cp -f hunspell/en_US/en_US.dic $DICTIONARIES
echo 'Copied English dictionary'

macdeployqt $BUNDLE -dmg
mv "$APP.dmg" "${APP}_$VERSION.dmg"
