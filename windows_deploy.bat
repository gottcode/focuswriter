@ECHO OFF
ECHO Copying executable
MKDIR FocusWriter
COPY release\FocusWriter.exe FocusWriter
STRIP FocusWriter\FocusWriter.exe
ECHO Copying libraries
COPY %QTDIR%\bin\libgcc_s_dw2-1.dll FocusWriter
COPY %QTDIR%\bin\mingwm10.dll FocusWriter
COPY %QTDIR%\bin\QtCore4.dll FocusWriter
COPY %QTDIR%\bin\QtGui4.dll FocusWriter
ECHO Copying image plugins
MKDIR FocusWriter\imageformats
COPY %QTDIR%\plugins\imageformats\qgif4.dll FocusWriter\imageformats
COPY %QTDIR%\plugins\imageformats\qico4.dll FocusWriter\imageformats
COPY %QTDIR%\plugins\imageformats\qjpeg4.dll FocusWriter\imageformats
COPY %QTDIR%\plugins\imageformats\qmng4.dll FocusWriter\imageformats
COPY %QTDIR%\plugins\imageformats\qsvg4.dll FocusWriter\imageformats
COPY %QTDIR%\plugins\imageformats\qtiff4.dll FocusWriter\imageformats
ECHO Copying English dictionary
MKDIR FocusWriter\Dictionaries
COPY hunspell\en_US\en_US.aff FocusWriter\Dictionaries
COPY hunspell\en_US\en_US.dic FocusWriter\Dictionaries
