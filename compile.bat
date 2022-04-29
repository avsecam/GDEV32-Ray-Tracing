@echo off
set include_folder="../../include"
set libraries_folder="../../libraries/"

@echo on
g++ main.cpp ./../../source/glad.c ./../../libraries/** -o out -I %include_folder% -L %libraries_folder%
start "" "./out.exe"