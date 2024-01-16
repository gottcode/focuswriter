@ECHO ON>..\focuswriter\resources\windows\dirs.nsh
@ECHO ON>..\focuswriter\resources\windows\files.nsh
@ECHO OFF

SET SRCDIR=..\focuswriter
SET APP=FocusWriter
SET VERSION=1.8.6

ECHO Copying executable
MKDIR %SRCDIR%\%APP%
COPY %APP%.exe %SRCDIR%\%APP%\%APP%.exe >nul

ECHO Copying translations
SET TRANSLATIONS=%SRCDIR%\%APP%\translations
MKDIR %TRANSLATIONS%
COPY *.qm %TRANSLATIONS% >nul

CD %SRCDIR%

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
COPY resources\symbols\symbols1510.dat %APP% >nul

ECHO Copying themes
SET THEMES=%APP%\themes
MKDIR %THEMES%
XCOPY /Q /S /Y resources\themes\* %THEMES% >nul

ECHO Copying Qt
%QTDIR%\bin\windeployqt.exe --verbose 0 --no-opengl-sw --no-system-d3d-compiler %APP%\%APP%.exe
RMDIR /S /Q %APP%\iconengines

ECHO Creating ReadMe
TYPE README >> %APP%\ReadMe.txt
ECHO. >> %APP%\ReadMe.txt
ECHO. >> %APP%\ReadMe.txt
ECHO CREDITS >> %APP%\ReadMe.txt
ECHO ======= >> %APP%\ReadMe.txt
ECHO. >> %APP%\ReadMe.txt
TYPE CREDITS >> %APP%\ReadMe.txt
ECHO. >> %APP%\ReadMe.txt
ECHO. >> %APP%\ReadMe.txt
ECHO NEWS >> %APP%\ReadMe.txt
ECHO ==== >> %APP%\ReadMe.txt
ECHO. >> %APP%\ReadMe.txt
TYPE ChangeLog >> %APP%\ReadMe.txt

ECHO Creating installer
CD %APP%
SETLOCAL EnableDelayedExpansion
SET "parentfolder=%__CD__%"
FOR /R . %%F IN (*) DO (
  SET "var=%%F"
  ECHO Delete "$INSTDIR\!var:%parentfolder%=!" >> ..\resources\windows\files.nsh
)
FOR /R /D %%F IN (*) DO (
  TYPE ..\resources\windows\dirs.nsh > temp.txt
  SET "var=%%F"
  ECHO RMDir "$INSTDIR\!var:%parentfolder%=!" > ..\resources\windows\dirs.nsh
  TYPE temp.txt >> ..\resources\windows\dirs.nsh
)
DEL temp.txt
ENDLOCAL
CD ..
makensis.exe /V0 resources\windows\installer.nsi

ECHO Making portable
MKDIR %APP%\Data
COPY COPYING %APP%\COPYING.txt >nul

ECHO Creating compressed file
CD %APP%
7z a -mx=9 %APP%_%VERSION%.zip * >nul
CD ..
MOVE %APP%\%APP%_%VERSION%.zip . >nul

ECHO Cleaning up
RMDIR /S /Q %APP%
DEL resources\windows\dirs.nsh
DEL resources\windows\files.nsh
