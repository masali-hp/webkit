@echo off

if "%1"=="" if "%2"=="" (
    :syntax
    echo WebKit Build Script

    echo Basic syntax:
    echo    build ^<Debug^|Release^|MinSizeRel^|RelWithDebInfo^> ^<XP^|CE^> ^[ARM^]
    echo    build all

    echo General Options:
    echo    -usecache       cmake cache is NOT deleted before build
    echo    -skipbuild      command line build is not executed.  .sln and .vcproj files are generated.

    echo Options Affecting WebKit Configuration:
    echo    -disablehp           HP specific extensions ^(like memory manager^) are disabled
    echo    -disablejit          Disable JIT
rem    echo    -jssharedcore   Build JavaScriptCore as a DLL
    echo    -usecf               Use Apple Core Foundation, Core Graphics
    echo    -usegdi              Use GDI for graphics, not cairo ^(only available for CE^)
    echo    -usewinceport        Use the webkit.org wince standard webkit port
    echo    -usewininet          Use wininet ^(instead of CURL^)
    echo    -enablesvg           Enable Scalable Vector Graphics ^(SVG^) support
    echo    -disabledrag         Disable drag and drop support
    echo    -disableperf         Disable performance tracing framework
    echo    -nosmartimagedecode  Disable memory efficient image downsampling
    exit /b 1
)

set OPENSOURCE_PATH=D:\git\webkit\OpenSource
set THIRD_PARTY_DIR=%OPENSOURCE_PATH:\=/%
set WEBKIT_DIR=D:\git\webkit-github

call :devenvsetup
:devenvsetupdone

rem With -usecache cmake cache is NOT deleted before build.
set USE_CACHE="NO"

rem With -skipbuild command line build is not executed.  .sln and .vcproj files are generated.
set SKIP_BUILD="NO"

rem With -usecf we use Core Foundation
set USE_CF=0

rem Unicode option.  WCHAR is the default.
set USE_WCHAR_UNICODE=1
set USE_ICU_UNICODE=0

rem Network option.  CURL is the default.
set USE_CURL=1
set USE_WININET=0

rem With -usegdi we don't use cairo.  With -usecf we don't use cairo
set USE_CAIRO=1
set PORT_FLAVOR=WinCairo

rem With -disablejit we turn JIT off for javascript.
set ENABLE_JIT=ON
set ENABLE_LLINT_C_LOOP=OFF
set ENABLE_LLINT=OFF

rem With -disablehp we turn off HP specific webkit extensions.  (Like memory manager)
set USE_HP=1
set PLATFORM=

rem With -usewinceport we use the webkit.org standard wince port.
set PORT=Win

rem With -enablesvg we build webcore with SVG support
set ENABLE_SVG=OFF

rem With -jssharedcore build JSC as a dll
rem set DJSSHARED_CORE=0
rem set DWTF_USE_EXPORT_MACROS=0

set ENABLE_DRAG=ON
set ENABLE_PERFORMANCE_TRACING=ON

set ENABLE_IMAGE_DECODER_DOWN_SAMPLING=ON

set CPU_SPECIFIC=

for %%x in (%*) do call :parseargument %%x

if %ENABLE_JIT% == OFF (
    set ENABLE_LLINT_C_LOOP=ON
    set ENABLE_LLINT=ON
)

if /I "%1%" == "all" (
    call :dobuild Debug XP %2 %3 %4 %5 %6 %7 %8 %9
    call :dobuild Release XP %2 %3 %4 %5 %6 %7 %8 %9
    call :dobuild Debug CE %2 %3 %4 %5 %6 %7 %8 %9
    call :dobuild Release CE %2 %3 %4 %5 %6 %7 %8 %9
    call :dobuild Debug CE ARM %2 %3 %4 %5 %6 %7 %8 %9
    call :dobuild Release CE ARM %2 %3 %4 %5 %6 %7 %8 %9
) else (
    call :dobuild %1 %2 %3 %4 %5 %6 %7 %8 %9
)
goto :eof

:dobuild

@if /I "%1%" == "Debug" (goto variantdone)
@if /I "%1%" == "Release" (goto variantdone)
@if /I "%1%" == "MinSizeRel" (goto variantdone)
@if /I "%1%" == "RelWithDebInfo" (goto variantdone)
goto syntax

:variantdone
set VARIANT=%1%

if /I "%2%" == "XP" (
    set OS_ARCH=XP-x86
    set SDK=Win32
    set GENERATOR="Visual Studio 9 2008"
)

if /I "%2%" == "CE" (
if /I "%3%" == "ARM" (
    set OS_ARCH=CE-ARM
    set SDK=Khan Yeti SDK ^(ARMV4I^)
    set CPU_SPECIFIC=-DWTF_CPU_ARM=1
    set GENERATOR="Visual Studio 9 2008 Khan Yeti SDK (ARMV4I)"
) else (
    set OS_ARCH=CE-x86
    set SDK=Jedi Thin Client ^(x86^)
    set GENERATOR="Visual Studio 9 2008 Jedi Thin Client (x86)"
)
)

if %USE_HP% EQU 1 (set BUILD_DIR=hp-%PORT%) else (set BUILD_DIR=webkit-%PORT%)
if %USE_CAIRO% EQU 1 (set BUILD_DIR=%BUILD_DIR%-cairo)
if %USE_CF% EQU 1 (set BUILD_DIR=%BUILD_DIR%-cf)
if %USE_ICU_UNICODE% EQU 1 (set BUILD_DIR=%BUILD_DIR%-icu)
if %ENABLE_JIT% == OFF (set BUILD_DIR=%BUILD_DIR%-nojit)
if %ENABLE_SVG% == ON (set BUILD_DIR=%BUILD_DIR%-svg)
if %USE_WININET% EQU 1 (set BUILD_DIR=%BUILD_DIR%-wininet)
if %ENABLE_DRAG% == OFF (set BUILD_DIR=%BUILD_DIR%-nodrag)
if %ENABLE_PERFORMANCE_TRACING% == OFF (set BUILD_DIR=%BUILD_DIR%-noperf)
if %ENABLE_IMAGE_DECODER_DOWN_SAMPLING% == OFF (set BUILD_DIR=%BUILD_DIR%-nodownsample)

if %USE_HP% EQU 1 (
    set PLATFORM=-DPLATFORM=HP
)

set REL_PATH=..\Source\WebCore
if NOT exist %REL_PATH% (
    echo Error: this script must be executed from the webkit "Build" directory.
    exit /b 1
)

if NOT exist %BUILD_DIR% mkdir %BUILD_DIR%
if NOT exist %BUILD_DIR%\%OS_ARCH% mkdir %BUILD_DIR%\%OS_ARCH%

pushd %BUILD_DIR%\%OS_ARCH%

if %USE_CACHE% == "NO" (
    rem   In case any default options have changed, we need to delete the CMakeCache.txt file
    rem   to use the new defaults.
    if exist CMakeCache.txt del CMakeCache.txt
)

@echo on

cmake -G %GENERATOR% ^
 -D3RDPARTY_DIR=D:\git\webkit\OpenSource ^
 -DPORT=%PORT% ^
 -DPORT_FLAVOR=%PORT_FLAVOR% ^
 %PLATFORM% ^
 %CPU_SPECIFIC% ^
 -DENABLE_ACCESSIBILITY=OFF ^
 -DENABLE_CONTEXT_MENUS=OFF ^
 -DENABLE_DRAG_SUPPORT=%ENABLE_DRAG% ^
 -DENABLE_FTPDIR=OFF ^
 -DENABLE_GEOLOCATION=OFF ^
 -DENABLE_HIGH_DPI_CANVAS=OFF ^
 -DENABLE_IMAGE_DECODER_DOWN_SAMPLING=ON ^
 -DENABLE_JIT=%ENABLE_JIT% ^
 -DENABLE_LLINT=%ENABLE_LLINT% ^
 -DENABLE_LLINT_C_LOOP=%ENABLE_LLINT_C_LOOP% ^
 -DENABLE_LEGACY_WEB_AUDIO=OFF ^
 -DENABLE_MATHML=%ENABLE_SVG% ^
 -DENABLE_NETSCAPE_PLUGIN_API=OFF ^
 -DENABLE_PERFORMANCE_TRACING=%ENABLE_PERFORMANCE_TRACING% ^
 -DENABLE_SVG=%ENABLE_SVG% ^
 -DENABLE_SVG_FONTS=%ENABLE_SVG% ^
 -DENABLE_TOUCH_EVENTS=ON ^
 -DENABLE_XHR_TIMEOUT=ON ^
 -DUSE_SYSTEM_MALLOC=ON ^
 -DWTF_USE_CF=%USE_CF% ^
 -DWTF_USE_CURL=%USE_CURL% ^
 -DWTF_USE_ICU_UNICODE=%USE_ICU_UNICODE% ^
 -DWTF_USE_WCHAR_UNICODE=%USE_WCHAR_UNICODE% ^
 -DWTF_USE_WININET=%USE_WININET% ^
 -DENABLE_IMAGE_DECODER_DOWN_SAMPLING=%ENABLE_IMAGE_DECODER_DOWN_SAMPLING% ^
 %WEBKIT_DIR%

@echo off
rem  -DSHARED_CORE=%DSHARED_CORE% ^

rem  Feature doesn't compile today:
rem  -DENABLE_WEB_TIMING=ON ^
rem  -DENABLE_PERFORMANCE_TIMELINE=ON ^
rem  -DENABLE_USER_TIMING=ON ^

@echo off

if %ERRORLEVEL% NEQ 0 goto :buildfail

if %SKIP_BUILD% == "NO" (
    echo WebKit Output Directory: %CD%\%VARIANT%
    echo Building Configuration: "%VARIANT%|%SDK%"
    devenv /build "%VARIANT%|%SDK%" WebKit.sln
    if %ERRORLEVEL% NEQ 0 goto :buildfail
)

popd
goto :eof

:buildfail
    echo Build Failure.
    exit /b %ERRORLEVEL%

:parseargument
@if /I "%1" == "-usecf" (
    set USE_CF=1
    set USE_WCHAR_UNICODE=0
    set USE_ICU_UNICODE=1
    set THIRD_PARTY_DIR=
)
@if /I "%1" == "-disablehp" (
    set USE_HP=0
)
@if /I "%1" == "-disablejit" (
    set ENABLE_JIT=OFF
)
@if /I "%1" == "-skipbuild" (
    set SKIP_BUILD="YES"
)
rem @if /I "%1" == "-staticbuild" (
rem     set DJSSHARED_CORE=0
rem     set DWTF_USE_EXPORT_MACROS=0
rem )
@if /I "%1" == "-usecache" (
    set USE_CACHE="YES"
)
@if /I "%1" == "-usegdi" (
    set USE_CAIRO=0
    set PORT_FLAVOR=WinCE-GDI
)
@if /I "%1" == "-usewinceport" (
    set PORT=WinCE
)
@if /I "%1" == "-usewininet" (
    set USE_WININET=1
    set USE_CURL=0
)
@if /I "%1" == "-enablesvg" (
    set ENABLE_SVG=ON
)
@if /I "%1" == "-disabledrag" (
    set ENABLE_DRAG=OFF
)
@if /I "%1" == "-disableperf" (
    set ENABLE_PERFORMANCE_TRACING=OFF
)
@if /I "%1" == "-nosmartimagedecode" (
    set ENABLE_IMAGE_DECODER_DOWN_SAMPLING=OFF
)

goto :eof

:devenvsetup
if "%DevEnvDir%" == "" (
    if exist "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"
        goto devenvsetupdone
    )
    if exist "C:\Program Files\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"
        goto devenvsetupdone
    )
    echo "Unable to find Visual C++ installation path"
    goto :eof
)
goto :eof