<#
.SYNOPSIS
Regression check: replay every saved route and compare its frames with a baseline.

.DESCRIPTION
Each route (see route.ps1) is replayed hidden, as fast as the PC allows, and every Nth frame
is saved. Replays are exact, so on an unchanged game the frames match the baseline exactly;
a frame that changes, or a replay that stops before the route's end (a crash), is reported.

  .\tools\regress.ps1                  # all routes
  .\tools\regress.ps1 -Route fortuna   # some routes
  .\tools\regress.ps1 -Update          # accept the current frames as the new baseline
  .\tools\regress.ps1 -Set audio.quality=enhanced   # with a setting changed, against the same baseline

The first run of a route, or any run with -Update, records its baseline. Baselines live in
saves\regress\<route>, the latest frames in build\regress\<route>, and side-by-sides of
changed frames in build\regress\<route>-changes. All of them are game imagery: never commit
them (saves\ and build\ are ignored by git).
#>
param(
    [string[]]$Route,
    [int]$Every = 120,
    [switch]$Update,
    [int]$Scale = 1,
    [int]$TimeoutMin = 30,
    [string[]]$Set      # key=value, passed to the game as --set
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'route-common.ps1')
Assert-GameBuilt
if (-not $Route) { $Route = Get-ChildItem $RoutesDir -Directory -ErrorAction SilentlyContinue | Sort-Object Name | ForEach-Object Name }
if (-not $Route) { throw 'no routes to check: save one first with .\tools\route.ps1 save <name>' }

$results = @()
foreach ($name in $Route) {
    $dir = Join-Path $RoutesDir $name
    if (-not (Test-Path $dir)) { $results += [pscustomobject]@{ Route = $name; Result = 'MISSING'; Detail = 'no such route' }; continue }
    $end = Get-RouteEnd $dir
    $last = $end - ($end % $Every)
    $cur = Join-Path $ProjectRoot "build\regress\$name"
    $base = Join-Path $ProjectRoot "saves\regress\$name"
    if (Test-Path $cur) { Remove-Item -Recurse -Force $cur }
    New-Item -ItemType Directory -Force $cur | Out-Null
    Write-Host "[$name] replaying $(Format-Frames $end), saving every ${Every}th frame..."

    $env:GCN_DUMP_RANGE = "0-$end"
    $env:SDL_AUDIODRIVER = 'dummy'  # silent: the game still mixes its audio, nothing reaches the speakers
    # Relative to the project folder, where the game runs: Start-Process joins its arguments
    # with spaces and doesn't quote them, so a path with a space would split in two.
    # The settings come from a file that isn't there, so the frames don't depend on this PC's
    # settings.txt (an aspect ratio, say).
    $gameArgs = @("--replay=saves\routes\$name", '--hidden', '--fast', '--no-input-log', "--scale=$Scale",
                  "--settings=build\regress\$name\settings.txt",
                  "--dump-dir=build\regress\$name", "--dump-every=$Every") + @($Set | Where-Object { $_ } | ForEach-Object { "--set=$_" })
    $err = Join-Path $cur 'stderr.txt'
    $start = Get-Date
    $p = Start-Process -FilePath $GameExe -ArgumentList $gameArgs -WorkingDirectory $ProjectRoot -PassThru -NoNewWindow `
        -RedirectStandardOutput (Join-Path $cur 'stdout.txt') -RedirectStandardError $err
    $target = Join-Path $cur ('frame_{0:D5}.png' -f $last)
    while (-not $p.HasExited -and -not (Test-Path $target) -and ((Get-Date) - $start).TotalMinutes -lt $TimeoutMin) { Start-Sleep -Seconds 3 }
    Start-Sleep -Seconds 2
    $reached = Test-Path $target
    if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    Remove-Item Env:GCN_DUMP_RANGE, Env:SDL_AUDIODRIVER -ErrorAction SilentlyContinue
    $mins = '{0:N1} min' -f ((Get-Date) - $start).TotalMinutes

    if (-not $reached) {
        $fatal = Get-Content $err -ErrorAction SilentlyContinue | Where-Object { $_ -match 'FATAL|fatal|parted' } | Select-Object -First 2
        $lastFrame = Get-ChildItem $cur -Filter 'frame_*.png' | Sort-Object Name | Select-Object -Last 1
        $results += [pscustomobject]@{ Route = $name; Result = 'STOPPED'; Detail = "before the end (last frame $($lastFrame.BaseName -replace 'frame_', '')) after $mins. $($fatal -join ' | ')" }
        continue
    }
    if ($Update -or -not (Test-Path $base)) {
        if (Test-Path $base) { Remove-Item -Recurse -Force $base }
        New-Item -ItemType Directory -Force $base | Out-Null
        Copy-Item (Join-Path $cur 'frame_*.png') $base
        $results += [pscustomobject]@{ Route = $name; Result = 'BASELINE'; Detail = "$((Get-ChildItem $base -Filter 'frame_*.png').Count) frames saved ($mins)" }
        continue
    }
    $report = & python -I (Join-Path $PSScriptRoot 'frame_diff.py') $base $cur (Join-Path $ProjectRoot "build\regress\$name-changes") 2>&1
    $code = $LASTEXITCODE
    $report | ForEach-Object { Write-Host "  $_" }
    $results += [pscustomobject]@{ Route = $name; Result = $(if ($code -eq 0) { 'SAME' } elseif ($code -eq 1) { 'CHANGED' } else { 'ERROR' }); Detail = "$($report | Select-Object -First 1) ($mins)" }
}

$results | Format-Table -AutoSize -Wrap
if ($results | Where-Object { $_.Result -notin 'SAME', 'BASELINE' }) { exit 1 }
