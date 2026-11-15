# Shared by route.ps1 and regress.ps1. Routes are recorded runs kept under saves/routes/<name>;
# saves/ is ignored by git, since a route holds your inputs and a copy of your memory card.

$ProjectRoot = Split-Path $PSScriptRoot -Parent
$RoutesDir = Join-Path $ProjectRoot 'saves\routes'
$GameExe = Join-Path $ProjectRoot 'build\sfassault.exe'

# How far a recording goes, in frames: its end marker, or for a run killed from outside, the
# frame of its last controller change. Both are decimal in the log.
function Get-RouteEnd([string]$Dir) {
    $log = Join-Path $Dir 'inputs.txt'
    if (-not (Test-Path $log)) { throw "no inputs.txt in $Dir" }
    $lines = Get-Content $log
    $end = $lines | Where-Object { $_ -match '^# end (\d+) (\d+)' } | Select-Object -Last 1
    if ($end -and $end -match '^# end (\d+) (\d+)') { return [int]$Matches[2] }
    $pad = $lines | Where-Object { $_ -match '^\d+ (\d+) ' } | Select-Object -Last 1
    if ($pad -and $pad -match '^\d+ (\d+) ') { return [int]$Matches[1] }
    return 0
}

function Format-Frames([int]$Frames) {
    $s = [int]($Frames / 60)
    '{0}:{1:D2}' -f [int][math]::Floor($s / 60), ($s % 60)
}

function Assert-GameBuilt {
    if (-not (Test-Path $GameExe)) { throw "build\sfassault.exe not found: run .\build.ps1 first" }
}
