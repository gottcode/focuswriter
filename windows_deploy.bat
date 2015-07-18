@ECHO OFF

SET APP=FocusWriter
SET VERSION=1.5.4.1

ECHO Copying executable
MKDIR %APP%
TYPE COPYING | FIND "" /V > %APP%\COPYING.txt
TYPE CREDITS | FIND "" /V > %APP%\CREDITS.txt
TYPE README | FIND "" /V > %APP%\README.txt
COPY release\%APP%.exe %APP% >nul
strip %APP%\%APP%.exe

ECHO Copying translations
SET TRANSLATIONS=%APP%\translations
MKDIR %TRANSLATIONS%
COPY translations\*.qm %TRANSLATIONS% >nul
COPY %QTDIR%\translations\qt_*.qm %TRANSLATIONS% >nul

ECHO Copying icons
SET ICONS=%APP%\icons\hicolor
MKDIR %ICONS%
XCOPY /Q /S /Y resources\images\icons\oxygen\hicolor %ICONS% >nul

ECHO Copying dictionaries
SET DICTIONARIES=%APP%\dictionaries
MKDIR %DICTIONARIES%
XCOPY /Q /S /Y resources\windows\dicts %DICTIONARIES% >nul

ECHO Copying sounds
SET SOUNDS=%APP%\sounds
MKDIR %SOUNDS%
COPY resources\sounds\* %SOUNDS% >nul

ECHO Copying symbols
COPY resources\symbols\symbols630.dat %APP% >nul

ECHO Copying themes
SET THEMES=%APP%\themes
MKDIR %THEMES%
XCOPY /Q /S /Y resources\themes\* %THEMES% >nul

ECHO Making portable
MKDIR %APP%\Data

ECHO Creating compressed file
CD %APP%
7z a -mx=9 %APP%_%VERSION%.zip * >nul
CD ..
MOVE %APP%\%APP%_%VERSION%.zip . >nul

ECHO Cleaning up
RMDIR /S /Q %APP%
