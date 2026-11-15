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

# Playing on from a route saves into the route's own copy of the memory card
# (memcard_replay.raw), not saves\memcard_a.raw. This makes that copy the real card, after
# backing the real one up, but only if the game wrote it after $Since and it differs from
# the card the route started with.
function Use-RouteSave([string]$Dir, [datetime]$Since) {
    $card = Join-Path $Dir 'memcard_replay.raw'
    $start = Join-Path $Dir 'memcard_a.raw'
    $wrote = (Test-Path $card) -and (Get-Item $card).LastWriteTime -ge $Since
    if (-not $wrote -or ((Test-Path $start) -and (Get-FileHash $card).Hash -eq (Get-FileHash $start).Hash)) {
        'The game saved nothing new while you played, so your memory card is unchanged.'
        return
    }
    $real = Join-Path $ProjectRoot 'saves\memcard_a.raw'
    if (Test-Path $real) {
        $backup = 'memcard_a.{0}.backup.raw' -f (Get-Date -Format 'yyyyMMdd-HHmmss')
        Copy-Item $real (Join-Path $ProjectRoot "saves\$backup")
        "Backed up your previous memory card to saves\$backup."
    }
    Copy-Item $card $real -Force
    'Your memory card now has the progress you saved. Start the game normally to continue from it.'
}

function Assert-GameBuilt {
    if (-not (Test-Path $GameExe)) { throw "build\sfassault.exe not found: run .\build.ps1 first" }
}
