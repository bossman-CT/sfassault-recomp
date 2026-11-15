<#
.SYNOPSIS
Pull the frames around each marker out of a recording, for review.

.DESCRIPTION
While you play, F9 or a controller's right stick click (R3) marks the moment in the run's
input log (saves\inputs\<stamp>) and shows "Marked". This replays that recording hidden,
silent and as fast as the PC allows, and saves:

  build\review\<stamp>\marker_NN\  frames from -Before s before each marker to -After s after, every -Stride s
  build\review\<stamp>\overview\   one frame every -Every s across the whole run
  build\review\<stamp>\index.txt   the markers with their times, and what is where

Replays are exact, so these are the frames the player saw, drawn with this PC's settings
at -Scale. Times are the game's own clock since boot, which stands still while the menu is
open, so they can run behind the wall clock.

  .\tools\review.ps1 saves\inputs\20261115-100834
  .\tools\review.ps1 20261115-100834 -Every 60 -Stride 0.1
  .\tools\review.ps1 20261115-100834 -IndexOnly     # just list the markers

The frames are game imagery: they stay under build\, which git ignores. Never attach them
to issues or pull requests.
#>
param(
    [Parameter(Mandatory, Position = 0)][string]$Recording,
    [double]$Every = 30,      # seconds between overview frames; 0 for none
    [double]$Before = 5,
    [double]$After = 2,
    [double]$Stride = 0.25,   # seconds between frames around a marker
    [int]$Scale = 1,
    [int]$TimeoutMin = 600,
    [switch]$IndexOnly
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'route-common.ps1')
$TB = 40500000.0  # time-base ticks a second
$inv = [cultureinfo]::InvariantCulture

function Format-Time([double]$s) {
    $t = [timespan]::FromSeconds([math]::Max(0.0, $s))  # 0, an int, would pick Max(int, int) and round $s
    '{0}:{1:D2}:{2:D2}.{3}' -f [int][math]::Floor($t.TotalHours), $t.Minutes, $t.Seconds, [int][math]::Floor($t.Milliseconds / 100)
}

# The recording: a path, or a stamp under saves\inputs. The game is started in the project
# folder with a relative path, since Start-Process doesn't quote arguments with spaces.
$dir = if (Test-Path $Recording) { (Resolve-Path $Recording).Path } else { Join-Path $ProjectRoot "saves\inputs\$Recording" }
$log = Join-Path $dir 'inputs.txt'
if (-not (Test-Path $log)) { throw "no inputs.txt in $dir" }
$dir = (Resolve-Path $dir).Path.TrimEnd('\')
if (-not $dir.StartsWith("$ProjectRoot\", [StringComparison]::OrdinalIgnoreCase)) {
    throw "the recording must be inside the project folder (copy it to saves\inputs first): $dir"
}
$rel = $dir.Substring($ProjectRoot.Length + 1)
if ($rel -match ' ') { throw "the recording's path can't have spaces in it: $rel" }
$stamp = Split-Path $dir -Leaf

# The markers (`# mark <n> <poll> <frame> <present> <tick>`) and where the run ended.
$version = 0
$marks = @()
$endTick = 0.0
$lastPad = $null
foreach ($line in [IO.File]::ReadLines($log)) {
    if ($line.StartsWith('#')) {
        if ($line -match '^# gcn-recomp input log v(\d+)') { $version = [int]$Matches[1] }
        elseif ($line.StartsWith('# mark ')) {
            $f = $line.Split(' ')
            $marks += [pscustomobject]@{ N = [int]$f[2]; Frame = [uint32]$f[5]; Time = [double]$f[6] / $TB }
        } elseif ($line.StartsWith('# end ')) {
            $f = $line.Split(' ')
            if ($f.Count -ge 5) { $endTick = [double]$f[4] }
        }
    } elseif ($line.Length -and [char]::IsDigit($line[0])) { $lastPad = $line }
}
if ($version -lt 6) { throw "this log (v$version) has no virtual times; replay it once with logging on for a v6 copy" }
if (-not $endTick -and $lastPad) { $endTick = [double]$lastPad.Split(' ')[11] }
$endS = $endTick / $TB

$out = Join-Path $ProjectRoot "build\review\$stamp"
$outRel = "build\review\$stamp"
$indexLines = @("Review of $rel`: $(Format-Time $endS) of play by the game's clock, $($marks.Count) marker(s)", '')
if ($marks) {
    $indexLines += '  #  time          frame   frames'
    foreach ($m in $marks) {
        $indexLines += '{0,3}  {1,-12} {2,7}   marker_{0:D2}\' -f $m.N, (Format-Time $m.Time), $m.Frame
    }
} else {
    $indexLines += 'No markers: press F9 or R3 while playing to add one.'
}
if ($IndexOnly) { $indexLines | ForEach-Object { Write-Host $_ }; exit 0 }
Assert-GameBuilt

# Every moment to save, in seconds, and what each is for.
$plan = @()
foreach ($m in $marks) {
    $steps = [int][math]::Floor(($Before + $After) / $Stride + 1e-6)
    for ($i = 0; $i -le $steps; $i++) {
        $o = -$Before + $i * $Stride
        $t = $m.Time + $o
        if ($t -lt 0) { continue }
        $plan += [pscustomobject]@{ Key = $t.ToString('F3', $inv); Dir = 'marker_{0:D2}' -f $m.N; Name = '{0:D2}_{1}s.png' -f $i, $o.ToString('+0.00;-0.00', $inv) }
    }
}
if ($Every -gt 0) {
    for ($t = 0.0; $t -le $endS; $t += $Every) {
        $ts = [timespan]::FromSeconds($t)
        $plan += [pscustomobject]@{ Key = $t.ToString('F3', $inv); Dir = 'overview'; Name = '{0}h{1:D2}m{2:D2}s.png' -f [int][math]::Floor($ts.TotalHours), $ts.Minutes, $ts.Seconds }
    }
}
$keys = $plan | ForEach-Object Key | Sort-Object { [double]::Parse($_, $inv) } -Unique
if (-not $keys) { throw 'nothing to save: no markers and -Every 0' }

if (Test-Path $out) { Remove-Item -Recurse -Force $out }
$raw = Join-Path $out 'raw'
New-Item -ItemType Directory -Force $raw | Out-Null
[IO.File]::WriteAllLines((Join-Path $out 'times.txt'), [string[]]$keys)
# This PC's settings, so the frames look as they did; a copy, so nothing writes back to them.
$settings = Join-Path $ProjectRoot 'settings.txt'
if (Test-Path $settings) { Copy-Item $settings (Join-Path $out 'settings.txt') }

Write-Host "Replaying $rel ($(Format-Time $endS)) for $($keys.Count) frames: $($marks.Count) marker(s), an overview frame every $Every s..."
$env:GCN_DUMP_TIMES = "$outRel\times.txt"
$env:SDL_AUDIODRIVER = 'dummy'
$gameArgs = @("--replay=$rel", '--hidden', '--fast', '--no-input-log', "--scale=$Scale",
              "--settings=$outRel\settings.txt", "--dump-dir=$outRel\raw")
$err = Join-Path $out 'stderr.txt'
$start = Get-Date
$p = Start-Process -FilePath $GameExe -ArgumentList $gameArgs -WorkingDirectory $ProjectRoot -PassThru -NoNewWindow `
    -RedirectStandardOutput (Join-Path $out 'stdout.txt') -RedirectStandardError $err
Remove-Item Env:GCN_DUMP_TIMES, Env:SDL_AUDIODRIVER -ErrorAction SilentlyContinue

# `[dump] <time> s: frame_<n>.png` for each moment, as the replay reaches it.
function Read-Dumps {
    $map = @{}
    try {
        $fs = [IO.File]::Open($err, 'Open', 'Read', 'ReadWrite')
        $text = (New-Object IO.StreamReader($fs)).ReadToEnd()
        $fs.Close()
    } catch { return $map }
    foreach ($m in [regex]::Matches($text, '(?m)^\[dump\] (\S+) s: (frame_\S+\.png)')) { $map[$m.Groups[1].Value] = $m.Groups[2].Value }
    $map
}
$lastSay = Get-Date
$dumps = @{}
while (-not $p.HasExited -and ((Get-Date) - $start).TotalMinutes -lt $TimeoutMin) {
    Start-Sleep -Seconds 5
    $dumps = Read-Dumps
    if ($dumps.Count -ge $keys.Count) { break }
    if (((Get-Date) - $lastSay).TotalSeconds -ge 60) {
        Write-Host ("  {0}/{1} frames after {2:N1} min" -f $dumps.Count, $keys.Count, ((Get-Date) - $start).TotalMinutes)
        $lastSay = Get-Date
    }
}
Start-Sleep -Seconds 2  # the last PNG is written after its line
if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force; $p.WaitForExit() }
$dumps = Read-Dumps
$mins = '{0:N1} min' -f ((Get-Date) - $start).TotalMinutes

# Sort the frames into their folders, by what each moment was for.
$missing = 0
foreach ($e in $plan) {
    $file = $dumps[$e.Key]
    $src = if ($file) { Join-Path $raw $file }
    if (-not $src -or -not (Test-Path $src)) { $missing++; continue }
    $to = Join-Path $out $e.Dir
    New-Item -ItemType Directory -Force $to | Out-Null
    Copy-Item $src (Join-Path $to $e.Name)
}
Remove-Item -Recurse -Force $raw

$indexLines += ''
if ($Every -gt 0) { $indexLines += "overview\: a frame every $Every s, named by the game's clock" }
if ($marks) { $indexLines += "marker_NN\: from $Before s before the marker to $After s after, every $Stride s, named by the offset (+0.00s is the marker)" }
$fatal = Get-Content $err -ErrorAction SilentlyContinue | Where-Object { $_ -match 'FATAL|fatal|parted' } | Select-Object -First 2
if ($missing) { $indexLines += "$missing frame(s) never came: the replay stopped first. $($fatal -join ' | ')" }
$indexLines += "Replayed in $mins at --scale=$Scale."
[IO.File]::WriteAllLines((Join-Path $out 'index.txt'), [string[]]$indexLines)
$indexLines | ForEach-Object { Write-Host $_ }
Write-Host "Saved to $outRel"
if ($missing) { exit 1 }
