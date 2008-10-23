MKDIR FocusWriter
COPY release\FocusWriter.exe FocusWriter
COPY %QTDIR%\bin\mingwm10.dll FocusWriter
COPY %QTDIR%\bin\QtCore4.dll FocusWriter
COPY %QTDIR%\bin\QtGui4.dll FocusWriter
MKDIR FocusWriter\imageformats
COPY %QTDIR%\plugins\imageformats\qgif4.dll FocusWriter\imageformats
COPY %QTDIR%\plugins\imageformats\qico4.dll FocusWriter\imageformats
COPY %QTDIR%\plugins\imageformats\qjpeg4.dll FocusWriter\imageformats
COPY %QTDIR%\plugins\imageformats\qmng4.dll FocusWriter\imageformats
COPY %QTDIR%\plugins\imageformats\qsvg4.dll FocusWriter\imageformats
COPY %QTDIR%\plugins\imageformats\qtiff4.dll FocusWriter\imageformats
