# Configure (first run) and build on Windows. Extra arguments are passed to ninja.
#
# Uses clang rather than MSVC: the recompiled game code and gcn-recomp's runtime rely on
# GCC-style flags and builtins, and clang handles the very large generated files better.
#
# 'Continue', not 'Stop': cmake, ninja and the recompiler write progress to stderr, which
# 'Stop' would turn into a fatal error. Failures are detected via $LASTEXITCODE instead.
$ErrorActionPreference = 'Continue'
Set-Location $PSScriptRoot

# A terminal opened before CMake, Ninja or LLVM were installed still has the old PATH, so
# add the saved machine and user PATH to it.
$env:Path = @($env:Path,
    [Environment]::GetEnvironmentVariable('Path', 'Machine'),
    [Environment]::GetEnvironmentVariable('Path', 'User')) -join ';'

# The LLVM installer (and winget) doesn't always put clang on PATH; look where it installs.
$llvm = Join-Path $env:ProgramFiles 'LLVM\bin'
if ((Test-Path "$llvm\clang.exe") -and -not (Get-Command clang -ErrorAction SilentlyContinue)) {
    $env:Path = "$llvm;$env:Path"
}

function Need($name) {
    $c = Get-Command $name -ErrorAction SilentlyContinue
    if (-not $c) { throw "$name not found on PATH. See README.md for requirements." }
    return $c.Source
}

$cc = Need 'clang'
$cxx = (Get-Command 'clang++' -ErrorAction SilentlyContinue).Source
if (-not $cxx) { $cxx = $cc }
Need 'cmake' | Out-Null
Need 'ninja' | Out-Null
Need 'python' | Out-Null

# The shared recompiler and runtime are a submodule.
if (-not (Test-Path gcn-recomp/CMakeLists.txt)) {
    git submodule update --init
    if ($LASTEXITCODE -ne 0) { throw "git submodule update failed" }
}

if (-not (Test-Path build/build.ninja)) {
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo `
        "-DCMAKE_C_COMPILER=$cc" "-DCMAKE_CXX_COMPILER=$cxx" @args
    if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }
    ninja -C build
} else {
    ninja -C build @args
}
if ($LASTEXITCODE -ne 0) { throw "build failed" }
