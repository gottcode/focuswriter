@ECHO OFF

SET APP=FocusWriter
SET VERSION=1.2.2

ECHO Copying executable
MKDIR %APP%
TYPE COPYING | FIND "" /V > %APP%\COPYING.txt
TYPE README | FIND "" /V > %APP%\README.txt
COPY release\%APP%.exe %APP% >nul
strip %APP%\%APP%.exe

ECHO Copying icons
SET ICONS=%APP%\icons\hicolor
MKDIR %ICONS%
XCOPY /Q /S /Y icons\oxygen\hicolor %ICONS% >nul

ECHO Copying English dictionary
SET DICTIONARIES=%APP%\Dictionaries
MKDIR %DICTIONARIES%
COPY hunspell\en_US\en_US.aff %DICTIONARIES% >nul
COPY hunspell\en_US\en_US.dic %DICTIONARIES% >nul

ECHO Copying Qt libraries
COPY %QTDIR%\bin\libgcc_s_dw2-1.dll %APP% >nul
COPY %QTDIR%\bin\mingwm10.dll %APP% >nul
COPY %QTDIR%\bin\QtCore4.dll %APP% >nul
COPY %QTDIR%\bin\QtGui4.dll %APP% >nul

ECHO Copying Qt image plugins
MKDIR %APP%\imageformats
COPY %QTDIR%\plugins\imageformats\qgif4.dll %APP%\imageformats >nul
COPY %QTDIR%\plugins\imageformats\qico4.dll %APP%\imageformats >nul
COPY %QTDIR%\plugins\imageformats\qjpeg4.dll %APP%\imageformats >nul
COPY %QTDIR%\plugins\imageformats\qmng4.dll %APP%\imageformats >nul
COPY %QTDIR%\plugins\imageformats\qsvg4.dll %APP%\imageformats >nul
COPY %QTDIR%\plugins\imageformats\qtiff4.dll %APP%\imageformats >nul

ECHO Creating compressed file
CD %APP%
7z a %APP%_%VERSION%.zip * >nul
CD ..
MOVE %APP%\%APP%_%VERSION%.zip . >nul

ECHO Cleaning up
RMDIR /S /Q %APP%
