@ECHO OFF

SET APP=FocusWriter
FOR /f %%i IN ('git rev-parse --short HEAD') DO SET VERSION=%%i

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
COPY resources\symbols\symbols510.dat %APP% >nul

ECHO Copying Qt libraries
COPY %QTDIR%\bin\libgcc_s_dw2-1.dll %APP% >nul
COPY %QTDIR%\bin\mingwm10.dll %APP% >nul
COPY %QTDIR%\bin\QtCore4.dll %APP% >nul
COPY %QTDIR%\bin\QtGui4.dll %APP% >nul
COPY %QTDIR%\bin\QtNetwork4.dll %APP% >nul

ECHO Copying Qt plugins
MKDIR %APP%\accessible
XCOPY /Q /S /Y %QTDIR%\plugins\accessible %APP%\accessible >nul
DEL %APP%\accessible\*.a >nul
DEL %APP%\accessible\*d4.dll >nul

MKDIR %APP%\bearer
XCOPY /Q /S /Y %QTDIR%\plugins\bearer %APP%\bearer >nul
DEL %APP%\bearer\*.a >nul
DEL %APP%\bearer\*d4.dll >nul

MKDIR %APP%\codecs
XCOPY /Q /S /Y %QTDIR%\plugins\codecs %APP%\codecs >nul
DEL %APP%\codecs\*.a >nul
DEL %APP%\codecs\*d4.dll >nul

MKDIR %APP%\imageformats
XCOPY /Q /S /Y %QTDIR%\plugins\imageformats %APP%\imageformats >nul
DEL %APP%\imageformats\*.a >nul
DEL %APP%\imageformats\*d4.dll >nul

ECHO Creating compressed file
CD %APP%
7z a -mx=9 %APP%_%VERSION%.zip * >nul
CD ..
MOVE %APP%\%APP%_%VERSION%.zip . >nul

ECHO Cleaning up
RMDIR /S /Q %APP%
