<#
.SYNOPSIS
Build SF Assault Recomp from your own disc: check the tools, check the disc, build.

.DESCRIPTION
The built game can never be distributed (it is made from the disc's code), so everyone
builds it on their own PC from their own disc image. This script:

  1. checks the tools the build needs (Git and the submodules, CMake, Ninja, LLVM-MinGW,
     Python) and, for anything missing, prints the exact step to install it. It never
     downloads or installs anything itself;
  2. finds your disc image in rom\ and checks it is the supported one: Star Fox Assault,
     USA (GF7E01), revision 0, SHA-1 065341b8575adea00fd0a75e389f135d32840423;
  3. configures and builds, and says how to start the game.

Double-click build.bat in the project folder, or from PowerShell:

  .\tools\build.ps1              # check everything, then build
  .\tools\build.ps1 -CheckOnly   # check the tools and the disc, build nothing

Running it again after an update rebuilds only what changed.
#>
param([switch]$CheckOnly)
# 'Continue', not 'Stop': cmake, ninja and the recompiler write progress to stderr.
$ErrorActionPreference = 'Continue'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
$supportedSha1 = '065341b8575adea00fd0a75e389f135d32840423'

function Say([string]$text, [string]$color = 'Gray') { Write-Host $text -ForegroundColor $color }
function Fail([string]$text) { Say ''; Say $text 'Red'; exit 1 }

Say 'SF Assault Recomp: build from your own disc' 'Cyan'
Say ''

# ---- 1. Tools ----
# A window opened before something was installed still has the old PATH, so add the saved
# machine and user PATH to it: installing with winget and running this again just works.
$env:Path = @($env:Path,
    [Environment]::GetEnvironmentVariable('Path', 'Machine'),
    [Environment]::GetEnvironmentVariable('Path', 'User')) -join ';'

$missing = @()
function Find-Tool([string]$name) { (Get-Command $name -ErrorAction SilentlyContinue | Select-Object -First 1).Source }
function Need([string]$what, [string]$winget, [string]$url) {
    $script:missing += [pscustomobject]@{ What = $what; Winget = $winget; Url = $url }
}

if (-not (Find-Tool git)) { Need 'Git' 'winget install --id Git.Git -e' 'https://git-scm.com/download/win' }

$cmake = Find-Tool cmake
if (-not $cmake) { Need 'CMake 3.20 or newer' 'winget install --id Kitware.CMake -e' 'https://cmake.org/download/' }
else {
    $v = (& $cmake --version | Select-Object -First 1) -replace '[^\d.]', ''
    if ([version]($v -replace '^(\d+\.\d+).*', '$1') -lt [version]'3.20') {
        Need "CMake 3.20 or newer (you have $v)" 'winget upgrade --id Kitware.CMake -e' 'https://cmake.org/download/'
    }
}

if (-not (Find-Tool ninja)) { Need 'Ninja' 'winget install --id Ninja-build.Ninja -e' 'https://github.com/ninja-build/ninja/releases' }

# LLVM-MinGW's clang brings its own Windows headers and libraries; plain LLVM would also need
# Visual Studio's Build Tools.
$cc = Find-Tool 'x86_64-w64-mingw32-clang'
$cxx = Find-Tool 'x86_64-w64-mingw32-clang++'
if (-not $cc -or -not $cxx) {
    Need 'LLVM-MinGW (UCRT)' 'winget install --id MartinStorsjo.LLVM-MinGW.UCRT -e' 'https://github.com/mstorsjo/llvm-mingw/releases (the ucrt-x86_64 zip; add its bin folder to PATH)'
}

# `python` can be Windows' shortcut to the Microsoft Store, which runs nothing: ask it to
# run something.
$py = Find-Tool python
$pyOk = $false
if ($py) { $pyOk = (& $py -c 'import sys; print(sys.version_info[0])' 2>$null) -eq '3' }
if (-not $pyOk) { Need 'Python 3' 'winget install --id Python.Python.3.13 -e' 'https://www.python.org/downloads/ (tick "Add python.exe to PATH")' }

# The recompiler and runtime are a submodule (gcn-recomp).
$submodules = $true
if (-not (Test-Path gcn-recomp\CMakeLists.txt)) {
    $submodules = $false
    if (Test-Path .git) {
        Need 'the gcn-recomp submodule' 'git submodule update --init --recursive' 'or clone with: git clone --recursive https://github.com/bossman-CT/sfassault-recomp.git'
    } else {
        Need 'the gcn-recomp submodule (a ZIP download from GitHub leaves it out)' 'git clone --recursive https://github.com/bossman-CT/sfassault-recomp.git' 'then run build.bat in the new folder'
    }
}

if ($missing) {
    Say 'Missing:' 'Yellow'
    foreach ($m in $missing) {
        Say "  - $($m.What)" 'Yellow'
        Say "      $($m.Winget)" 'White'
        Say "      ($($m.Url))"
    }
    Fail 'Install what is missing (the commands above go in PowerShell or Terminal), then run build.bat again.'
}
Say "Tools: OK (cmake, ninja, $(Split-Path $cc -Leaf), python, git)" 'Green'

# ---- 2. The disc ----
$romDir = Join-Path $root 'rom'
$other = @(Get-ChildItem $romDir -File -ErrorAction SilentlyContinue | Where-Object { $_.Extension -in '.rvz', '.gcz', '.wia', '.nkit', '.wbfs' -or $_.Name -like '*.nkit.iso' })
$images = @(Get-ChildItem $romDir -File -ErrorAction SilentlyContinue | Where-Object { $_.Extension -in '.iso', '.ciso' -and $_.Name -notlike '*.nkit.iso' })
if (-not $images) {
    if ($other) {
        Fail ("Found $($other[0].Name) in rom\, but the build needs a plain .iso (or .ciso).`n" +
              "Convert it in Dolphin: right-click the game, Convert File..., format ISO. Put the .iso in rom\.")
    }
    Fail ("No disc image in rom\.`n" +
          "Dump your own Star Fox Assault disc (USA) with CleanRip and put the .iso in`n  $romDir`n" +
          "README.md, 'Getting your game files', says how.")
}

# The disc header: the game's ID (6 letters) and its revision (the byte after the disc number).
# A .ciso keeps the disc's blocks after a 32 KB header of its own.
function Read-Header([IO.FileInfo]$f) {
    $s = [IO.File]::OpenRead($f.FullName)
    try {
        if ($f.Extension -eq '.ciso') { [void]$s.Seek(0x8000, 'Begin') }
        $b = New-Object byte[] 8
        [void]$s.Read($b, 0, 8)
        [pscustomobject]@{ Id = [Text.Encoding]::ASCII.GetString($b, 0, 6); Rev = $b[7] }
    } finally { $s.Close() }
}

$iso = $null
$problems = @()
foreach ($f in $images) {
    $h = Read-Header $f
    switch -regex ($h.Id) {
        '^GF7E01$' {
            if ($h.Rev -ne 0) { $problems += "$($f.Name): Star Fox Assault USA, revision $($h.Rev). Only revision 0 is supported so far."; break }
            if ($f.Extension -eq '.ciso') {
                # Its own SHA-1 differs from the disc's; the build checks main.dol's instead.
                Say "Disc: $($f.Name), Star Fox Assault USA (GF7E01) revision 0 (.ciso: the build checks its main.dol)" 'Green'
                $iso = $f; break
            }
            Say "Checking $($f.Name) (a few seconds)..."
            $sha = (Get-FileHash -Algorithm SHA1 $f.FullName).Hash.ToLower()
            if ($sha -eq $supportedSha1) {
                Say "Disc: $($f.Name), Star Fox Assault USA (GF7E01) revision 0, SHA-1 matches" 'Green'
                $iso = $f; break
            }
            $problems += ("$($f.Name): says Star Fox Assault USA revision 0, but its SHA-1 is $sha, not $supportedSha1. " +
                          "The image was changed (trimmed, scrubbed, NKit or patched) or the dump is bad. Dump the disc again with CleanRip.")
        }
        '^GF7P' { $problems += "$($f.Name): the European version ($($h.Id)). Only the USA version (GF7E01) is supported so far; PAL is planned."; break }
        '^GF7J' { $problems += "$($f.Name): the Japanese version ($($h.Id)). Only the USA version (GF7E01) is supported so far; Japan is planned."; break }
        default { $problems += "$($f.Name): not Star Fox Assault (disc ID '$($h.Id)'). This port needs Star Fox Assault, USA (GF7E01)." }
    }
    if ($iso) { break }
}
if (-not $iso) { Fail ("No supported disc image in rom\:`n  " + ($problems -join "`n  ")) }
if ($problems) { $problems | ForEach-Object { Say "  (skipped $_)" 'DarkYellow' } }

if ($CheckOnly) { Say ''; Say 'Everything is ready to build. Run build.bat (or .\tools\build.ps1) without -CheckOnly.' 'Green'; exit 0 }

# ---- 3. Build ----
Say ''
$start = Get-Date
$cache = Join-Path $root 'build\CMakeCache.txt'
$configured = Test-Path (Join-Path $root 'build\build.ninja')
$cachedIso = if (Test-Path $cache) { (Select-String -Path $cache -Pattern '^GAME_ISO:FILEPATH=(.*)$' | Select-Object -First 1).Matches.Groups[1].Value }
$isoCmake = $iso.FullName -replace '\\', '/'
if (-not $configured -or $cachedIso -ne $isoCmake) {
    Say 'Configuring (the first time, this also unpacks the game code from your disc)...' 'Cyan'
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo "-DCMAKE_C_COMPILER=$cc" "-DCMAKE_CXX_COMPILER=$cxx" "-DGAME_ISO=$isoCmake"
    if ($LASTEXITCODE -ne 0) { Fail 'Configuring failed (the messages above say why). Nothing was changed outside the build folder.' }
}
Say 'Building (the first build recompiles the whole game and takes a while; later ones are quick)...' 'Cyan'
ninja -C build
if ($LASTEXITCODE -ne 0) { Fail 'The build failed. The messages above say why; please report it with them in an issue.' }
$took = (Get-Date) - $start
if (-not (Test-Path build\sfassault.exe)) { Fail 'The build finished but build\sfassault.exe is missing.' }

Say ''
Say ('Built in {0} min {1} s: build\sfassault.exe' -f [int][math]::Floor($took.TotalMinutes), $took.Seconds) 'Green'
Say ''
Say 'To play, start it with the project folder as the working folder, where it keeps saves\ and settings.txt:'
Say "  cd `"$root`"" 'White'
Say '  .\build\sfassault.exe' 'White'
Say 'F1 (or a controller''s Back/View button) opens the menu: graphics, controls and quitting. F11 toggles fullscreen.'
Say 'To add it to Steam, see README.md, "Quick start".'
