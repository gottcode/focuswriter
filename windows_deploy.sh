#!/bin/bash

APP='FocusWriter'
VERSION=$(git describe)

echo -n 'Preparing... '
QTDIR=$(mingw32-qmake-qt5 -query QT_INSTALL_PREFIX)
rm -f ${APP}_${VERSION}.zip
rm -rf $APP
echo 'Done'

echo -n 'Copying executable... '
mkdir $APP
cp release/$APP.exe $APP
echo 'Done'

echo -n 'Copying translations... '
mkdir $APP/translations
cp translations/*.qm $APP/translations
cp $QTDIR/share/qt5/translations/qt_*.qm $APP/translations
cp $QTDIR/share/qt5/translations/qtbase_*.qm $APP/translations
cp $QTDIR/share/qt5/translations/qtmultimedia_*.qm $APP/translations
echo 'Done'

echo -n 'Copying icons... '
mkdir $APP/icons
cp -r resources/images/icons/oxygen/hicolor/ $APP/icons
echo 'Done'

echo -n 'Copying dictionaries... '
mkdir $APP/dictionaries
cp -r resources/windows/dicts/* $APP/dictionaries
echo 'Done'

echo -n 'Copying sounds... '
cp -r resources/sounds/ $APP
echo 'Done'

echo -n 'Copying symbols... '
cp resources/symbols/symbols900.dat $APP
echo 'Done'

echo -n 'Copying themes... '
cp -r resources/themes/ $APP
echo 'Done'

echo -n 'Copying Qt libraries... '
cp $QTDIR/bin/iconv.dll $APP
cp $QTDIR/bin/libgcc_s_sjlj-1.dll $APP
cp $QTDIR/bin/libGLESv2.dll $APP
cp $QTDIR/bin/libglib-2.0-0.dll $APP
cp $QTDIR/bin/libharfbuzz-0.dll $APP
cp $QTDIR/bin/libintl-8.dll $APP
cp $QTDIR/bin/libpcre-1.dll $APP
cp $QTDIR/bin/libpcre16-0.dll $APP
cp $QTDIR/bin/libpng16-16.dll $APP
cp $QTDIR/bin/libjpeg-62.dll $APP
cp $QTDIR/bin/libstdc++-6.dll $APP
cp $QTDIR/bin/libtiff-5.dll $APP
cp $QTDIR/bin/libwebp-6.dll $APP
cp $QTDIR/bin/libwinpthread-1.dll $APP
cp $QTDIR/bin/zlib1.dll $APP
cp $QTDIR/bin/Qt5Core.dll $APP
cp $QTDIR/bin/Qt5Gui.dll $APP
cp $QTDIR/bin/Qt5Multimedia.dll $APP
cp $QTDIR/bin/Qt5Network.dll $APP
cp $QTDIR/bin/Qt5PrintSupport.dll $APP
cp $QTDIR/bin/Qt5Svg.dll $APP
cp $QTDIR/bin/Qt5Widgets.dll $APP
cp $QTDIR/bin/Qt5WinExtras.dll $APP
echo 'Done'

echo -n 'Copying Qt plugins... '
cp -r $QTDIR/lib/qt5/plugins/audio/ $APP
cp -r $QTDIR/lib/qt5/plugins/bearer/ $APP
cp -r $QTDIR/lib/qt5/plugins/platforms/ $APP
cp -r $QTDIR/lib/qt5/plugins/imageformats/ $APP
cp -r $QTDIR/lib/qt5/plugins/mediaservice/ $APP
cp -r $QTDIR/lib/qt5/plugins/printsupport/ $APP
echo 'Done'

echo -n 'Create ReadMe... '
cp README $APP/ReadMe.txt
echo -e '\n\nCredits\n=======\n\n' >> $APP/ReadMe.txt
cat CREDITS >> $APP/ReadMe.txt
echo -e '\n\nNews\n====\n\n' >> $APP/ReadMe.txt
cat NEWS >> $APP/ReadMe.txt
unix2dos -q -o $APP/ReadMe.txt
unix2dos -q -n COPYING $APP/Copying.txt
echo 'Done'

echo -n 'Stripping files... '
mingw-strip --strip-unneeded -p $APP/$APP.exe $APP/*.dll $APP/*/*.dll
echo 'Done'

echo -n 'Making portable... '
mkdir $APP/Data
echo 'Done'

echo -n 'Compressing... '
cd $APP
7za a -mx=9 ../${APP}_${VERSION}.zip *
cd ..
echo 'Done'

echo -n 'Cleaning up... '
rm -rf $APP
echo 'Done'
