rem $Id$

@echo off
setlocal

rem execute svn info, extract "Revision: 1234" line, and take SVN rev from there
for /F " usebackq tokens=1,2 delims=: " %%i in (`svn info`) do set key=%%i&set value=%%j&call :read-svn-rev
rem @echo SVN revision "%SVNREV%"

rem extract "#define FR_VERSION_SVN 1234" line, and take header SVN rev from there
for /F " usebackq tokens=2,3 " %%i in (src\frsvnrev.h) do set name=%%i&set value=%%j&call :read-header-rev
rem @echo Header svn revision "%HEADERREV%"

rem check for valid SVN rev
if "%SVNREV%" EQU "" goto :no-svn-ref
rem don't write file if SVN ref is equal
if "%HEADERREV%" EQU "%SVNREV%" goto :EOF

@echo Writing svn revision %SVNREV% to frsvnrev.h
@echo #define FR_VERSION_SVN %SVNREV% > src\frsvnrev.h
goto :EOF

:no-svn-ref
rem don't write file if SVN ref is already unset
if "%HEADERREV%" EQU "" goto :EOF
@echo Deleting svn revision from frsvnrev.h
@echo #undef FR_VERSION_SVN > src\frsvnrev.h
goto :EOF

endlocal
goto :EOF

:read-svn-rev
if "%key%" EQU "Revision" set SVNREV=%value%&
goto :EOF

:read-header-rev
if "%name%" EQU "FR_VERSION_SVN" set HEADERREV=%value%&
goto :EOF
