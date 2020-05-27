$DestinationDirectory = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($args[0]).Replace('\', '\\')
$PkgConfigDirectory = "pkgconfig"

function Prepare {
    Write-Host "Preparing dependency directory..."
    if (-not (Test-Path -LiteralPath $DestinationDirectory)) {
        mkdir $DestinationDirectory >$null 2>&1
    }
    cd $DestinationDirectory
    if (-not (Test-Path -LiteralPath $PkgConfigDirectory)) {
        mkdir $PkgConfigDirectory >$null 2>&1
    }
}

function Clean {
    Remove-Item $DestinationDirectory\* -Recurse -Force
    mkdir $DestinationDirectory\$PkgConfigDirectory >$null 2>&1
}

function MakeBotan2 {
    $Botan2Archive = "botan-2.14.0.tar.gz"
    $Botan2SourcePath = "botan-2.14.0"
    $Botan2BuildPath = $DestinationDirectory + "\\botan2\\"
    $Botan2Url = "https://github.com/randombit/botan/archive/2.14.0.tar.gz"

    Write-Host "Downloading Botan-2..."
    wget $Botan2Url -OutFile $Botan2Archive >$null 2>&1
    Write-Host "Extracting Botan-2..."
    tar xvf $Botan2Archive >$null 2>&1
    del $Botan2Archive
    Write-Host "Builiding Botan-2..."
    cd $DestinationDirectory\$Botan2SourcePath
    python configure.py --cc=gcc --os=mingw --prefix=$Botan2BuildPath >$null 2>&1
    make >$null 2>&1
    Write-Host "Installing Botan-2..."
    make install >$null 2>&1
    cd $DestinationDirectory
    Remove-Item $Botan2SourcePath -Recurse -Force
    copy -Force $Botan2BuildPath\lib\pkgconfig\*.* $PkgConfigDirectory
}

function MakeZlib {
    $ZlibArchive = "zlib-1.2.11.tar.gz"
    $ZlibSourcePath = "zlib-1.2.11"
    $ZlibUrl = "https://www.zlib.net/zlib-1.2.11.tar.gz"
        
    Write-Host "Downloading Zlib..."
    wget $ZlibUrl -OutFile $ZlibArchive >$null 2>&1
    Write-Host "Extracting Zlib..."
    tar xvf $ZlibArchive >$null 2>&1
    del $ZlibArchive
    Write-Host "Building Zlib..."
    cd $ZlibSourcePath
    make -f win32\Makefile.gcc >$null 2>&1
    cd $DestinationDirectory
    echo @"
prefix=$DestinationDirectory\\$ZlibSourcePath\\
exec_prefix=`${prefix}
libdir=`${prefix}
includedir=`${prefix}

Name: Zlib
Description:
Version: 1.2.11

Libs: -L`${libdir} -lzlib1
Cflags: -I`${includedir}
"@ | out-file -encoding ascii $PkgConfigDirectory\zlib.pc
}

function MakeLua {
    $LuaArchive = "lua-5.3.5.tar.gz"
    $LuaSourcePath = "lua-5.3.5"
    $LuaUrl = "https://www.lua.org/ftp/lua-5.3.5.tar.gz"

    Write-Host "Downloading Lua..."
    wget $LuaUrl -OutFile $LuaArchive >$null 2>&1
    Write-Host "Extracting Lua..."
    tar xvf $LuaArchive >$null 2>&1
    del $LuaArchive
    Write-Host "Building Lua..."
    cd $LuaSourcePath
    make PLAT=mingw all >$null 2>&1
    cd $DestinationDirectory
    echo @"
prefix=$DestinationDirectory\\$LuaSourcePath\\src\\
exec_prefix=`${prefix}
libdir=`${prefix}
includedir=`${prefix}

Name: Lua
Description:
Version: 5.3.5

Libs: -L`${libdir} -llua53
Cflags: -I`${includedir}
"@ | out-file -encoding ascii $PkgConfigDirectory\lua.pc
}

function MakeCatch2 {
    $Catch2SourcePath = "catch2"
    $Catch2File = "catch.hpp"
    $Catch2Url = "https://github.com/catchorg/Catch2/releases/download/v2.12.2/catch.hpp"

    Write-Host "Downloading Catch2..."
    mkdir $Catch2SourcePath >$null 2>&1
    wget $Catch2Url -OutFile $Catch2SourcePath\$Catch2File >$null 2>&1
}

function MakeAll {
    Clean
    MakeBotan2
    MakeZlib
    MakeLua
    MakeCatch2
}

$Components = @{
    clean = "Clean"
    botan2 = "MakeBotan2"
    zlib = "MakeZlib"
    lua = "MakeLua"
    catch2 = "MakeCatch2"
    all = "MakeAll"
}

Prepare
foreach ($Component in $args[1..$args.Length]) {
    & $Components[$Component]
}