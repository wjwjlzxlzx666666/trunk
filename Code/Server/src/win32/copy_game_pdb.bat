@echo off
set inputPath=%1
set outputPath=%2

call :CopyBinFile gameserver.pdb gameserver\bin\gameserver.pdb

echo ============
echo 操作成功
echo ============
pause
exit 0

pause

: CopyBinFile
echo "copy %inputPath%%1"
echo " ==> %outputPath%%2"
copy /Y %inputPath%%1 /B %outputPath%%2 /B
if not %errorlevel% == 0 (call:ErrorExit)
exit /b

: ErrorExit
echo ============
echo 操作失败
echo ============
pause
exit -1
goto :eof