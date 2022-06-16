::根据shade的输入路径，将VSMain和PSMain编译到其所在的目录下
@echo off
set hlslFilePath=%1
set hlslFileFolderPath=%~dp1
set hlslFileName=%~n1
set vsFilePath=%hlslFileFolderPath%%hlslFileName%_vs.cso
set psFilePath=%hlslFileFolderPath%%hlslFileName%_ps.cso
::fxc工具的路径自行修改
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" %hlslFilePath% /Od /Zi /T vs_5_0 /E "VSMain" /Fo %vsFilePath%
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" %hlslFilePath% /Od /Zi /T ps_5_0 /E "PSMain" /Fo %psFilePath%
pause