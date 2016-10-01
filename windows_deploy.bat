@ECHO OFF

SET APP=FocusWriter
FOR /f %%i IN ('git describe') DO SET VERSION=%%i

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
COPY %QTDIR%\translations\qtbase_*.qm %TRANSLATIONS% >nul

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
COPY resources\symbols\symbols900.dat %APP% >nul

ECHO Copying themes
SET THEMES=%APP%\themes
MKDIR %THEMES%
XCOPY /Q /S /Y resources\themes\* %THEMES% >nul

ECHO Copying Qt libraries
COPY %QTDIR%\bin\libgcc_s_dw2-1.dll %APP% >nul
COPY "%QTDIR%\bin\libstdc++-6.dll" %APP% >nul
COPY %QTDIR%\bin\libwinpthread-1.dll %APP% >nul
COPY %QTDIR%\bin\Qt5Core.dll %APP% >nul
COPY %QTDIR%\bin\Qt5Gui.dll %APP% >nul
COPY %QTDIR%\bin\Qt5Multimedia.dll %APP% >nul
COPY %QTDIR%\bin\Qt5Network.dll %APP% >nul
COPY %QTDIR%\bin\Qt5PrintSupport.dll %APP% >nul
COPY %QTDIR%\bin\Qt5Svg.dll %APP% >nul
COPY %QTDIR%\bin\Qt5Widgets.dll %APP% >nul
COPY %QTDIR%\bin\Qt5WinExtras.dll %APP% >nul

ECHO Copying Qt plugins
MKDIR %APP%\audio
COPY %QTDIR%\plugins\audio\qtaudio_windows.dll %APP%\audio >nul

MKDIR %APP%\bearer
XCOPY /Q /S /Y %QTDIR%\plugins\bearer %APP%\bearer >nul
DEL %APP%\bearer\*d.dll >nul

MKDIR %APP%\platforms
COPY %QTDIR%\plugins\platforms\qwindows.dll %APP%\platforms >nul

MKDIR %APP%\imageformats
XCOPY /Q /S /Y %QTDIR%\plugins\imageformats %APP%\imageformats >nul
DEL %APP%\imageformats\*d.dll >nul

MKDIR %APP%\mediaservice
XCOPY /Q /S /Y %QTDIR%\plugins\mediaservice %APP%\mediaservice >nul
DEL %APP%\mediaservice\*d.dll >nul

MKDIR %APP%\printsupport
COPY %QTDIR%\plugins\printsupport\windowsprintersupport.dll %APP%\printsupport >nul

ECHO Making portable
MKDIR %APP%\Data

ECHO Creating compressed file
CD %APP%
7z a -mx=9 %APP%_%VERSION%.zip * >nul
CD ..
MOVE %APP%\%APP%_%VERSION%.zip . >nul

ECHO Cleaning up
RMDIR /S /Q %APP%
