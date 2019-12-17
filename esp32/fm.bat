@rem Shorthand to flash then monitor
@echo off

rem Bypass "Terminate Batch Job" prompt.
rem https://superuser.com/a/498798
if "%~1"=="-FIXED_CTRL_C" (
   REM Remove the -FIXED_CTRL_C parameter
   SHIFT
) ELSE (
   REM Run the batch with <NUL and -FIXED_CTRL_C
   CALL <NUL %0 -FIXED_CTRL_C %*
   GOTO :EOF
)

idf.py flash

IF ERRORLEVEL 1 GOTO error

echo [92mFlashed successfully.[0m
idf.py monitor
exit /b

:error
rem echo with colour, source: https://stackoverflow.com/a/38617204
echo [91mFlash error![0m
exit /b