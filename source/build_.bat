@echo off
REM Go to the directory where the C++ files are located
cd /d %~dp0

REM Clean any old .obj or .exe files if necessary
del *.obj
del Limited8086Sim.exe

REM Step 1: Compile each .cpp file into a .obj file
for %%f in (*.cpp) do (
    clang++ -std=c++17 -O2 -c "%%f" -o "%%~nf.obj"
    if %ERRORLEVEL% EQU 0 (
        echo Successfully compiled %%f to %%~nf.obj
    ) else (
        echo Error compiling %%f
        exit /b 1
    )
)

REM Step 2: Link all the .obj files into one executable
clang++ -o Limited8086Sim.exe *.obj

REM Check if the linking was successful
if %ERRORLEVEL% EQU 0 (
    echo Successfully linked all object files to Limited8086Sim.exe
) else (
    echo Error during linking
)

pause
