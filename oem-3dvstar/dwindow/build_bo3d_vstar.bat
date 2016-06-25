call revision_silent.bat

#build
set dev2003="C:\Program Files (x86)\Microsoft Visual Studio .NET 2003\Common7\IDE\devenv"
set dev2008="C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE\devenv"
cd /d "%~dp0"

#build
del dwindow\ico\ico.ico
copy dwindow\ico\icoVSTAR.ico dwindow\ico\ico.ico
%dev2008% dwindow\dwindow.sln /build "Release_vstar"
%dev2008% my12doomSource\my12doomSource.sln /build "Release Filter"
%dev2008% report_server\report_server.sln /build "Release"
%dev2008% reset\reset.sln /build "Release"
del dwindow\ico\ico.ico
copy dwindow\ico\ico_normal.ico dwindow\ico\ico.ico

#copy
copy/y dwindow\dwindow.ini dwindow_NSIS
copy/y dwindow\alpha.raw dwindow_NSIS
copy/y dwindow\logo.raw dwindow_NSIS
copy/y dwindow\Release_vstar\StereoPlayer.exe dwindow_NSIS
copy/y reset\release\reset.exe dwindow_NSIS
copy/y report_server\release\ErrorReport.exe dwindow_NSIS
del/q dwindow_NSIS\codec\*.*
del/q dwindow_NSIS\skin\*.*
copy/y dwindow_ui\3dvplayer\*.* dwindow_NSIS\skin
copy/y 3rdFilter\*.* dwindow_NSIS\codec
copy/y my12doomSource\bin\Filters_x86\*.ax dwindow_NSIS\codec
rd/s/q dwindow_NSIS\UI
md dwindow_NSIS\UI
xcopy dwindow_UI\*.* dwindow_NSIS\UI /s /e /y
for /r %%f in (dwindow_NSIS\UI\*.lua) do dwindow_NSIS\StereoPlayer.exe compile %%f %%f

pause
NSIS\makensisw.exe dwindow_NSIS\dwindowVSTAR.nsi
cd dwindow_NSIS
set v=%DATE:~2,2%%DATE:~5,2%%DATE:~8,2%
del 3dvplayer%v%.exe
ren dwindow_setup.exe 3dvplayer%v%.exe
cd..
copy/y dwindow\Release_vstar\StereoPlayer.pdb pdbs\3dvplayer%v%.pdb