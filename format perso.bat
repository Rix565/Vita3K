@echo off
cd vita3k
for /f %%f in ('dir *.cpp *.h /b/s') do "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-format" -i %%f
cd ..\tools
for /f %%f in ('dir *.cpp *.h /b/s') do "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-format" -i %%f
cd ..
