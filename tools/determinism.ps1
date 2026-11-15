<#
Do two instances fed the same inputs stay identical? (issue #36, docs/dev/netplay.md)

Replays a route in two instances of the game at once, hidden, silent and unpaced, each with
its own arguments, and compares their guest-state hash logs (GCN_STATE_HASH in gcn-recomp's
docs/diagnostics.md) with gcn-recomp's tools/state_hash_diff.py.

    .\tools\determinism.ps1 -Route fortuna                            # the same settings twice
    .\tools\determinism.ps1 -Route fortuna -A '--scale=1' -B '--scale=4'
    .\tools\determinism.ps1 -Route fortuna -A '--aspect=4:3' -B '--aspect=16:9' -Every 30

Each instance replays its own copy of the route (build\determinism\<tag>\route), so neither
writes the other's scratch memory card. The logs are build\determinism\<tag>\a.txt and b.txt.
#>
param(
    [Parameter(Mandatory)] [string] $Route,
    [string] $A = '',
    [string] $B = '',
    [int] $Every = 60,
    [int] $UpTo = 0,          # stop at this frame instead of the route's end
    [string] $Tag = '',
    [int] $TimeoutMin = 40
)
$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot)

$src = "saves\routes\$Route"
if (-not (Test-Path "$src\inputs.txt")) { throw "no route $Route (saves\routes\$Route\inputs.txt)" }
if (-not $UpTo) {
    $endLine = Get-Content "$src\inputs.txt" -Tail 5 | Where-Object { $_ -match '^# end (\d+) (\d+)' } | Select-Object -Last 1
    if (-not $endLine) { throw "the route has no '# end' line; pass -UpTo" }
    $UpTo = [int]($endLine -replace '^# end \d+ (\d+).*', '$1')
}
$last = $UpTo - ($UpTo % $Every)
if (-not $Tag) { $Tag = "$Route-" + (Get-Date -Format 'yyyyMMdd-HHmmss') }
$out = "build\determinism\$Tag"
New-Item -ItemType Directory -Force $out | Out-Null

$exe = (Resolve-Path build\sfassault.exe).Path
$env:SDL_AUDIODRIVER = 'dummy'
$procs = @()
foreach ($side in 'a', 'b') {
    $dir = "$out\$side-route"
    if (Test-Path $dir) { Remove-Item -Recurse -Force $dir }
    New-Item -ItemType Directory $dir | Out-Null
    Copy-Item "$src\inputs.txt" $dir
    if (Test-Path "$src\memcard_a.raw") { Copy-Item "$src\memcard_a.raw" $dir }
    $extra = if ($side -eq 'a') { $A } else { $B }
    $log = (Join-Path (Get-Location) "$out\$side.txt")
    if (Test-Path $log) { Remove-Item $log }
    $env:GCN_STATE_HASH = "${Every}:$log"
    # Settings from a file that isn't there, so this PC's settings.txt plays no part.
    $args_ = @("--replay=$dir", '--hidden', '--fast', '--no-input-log', "--settings=$out\$side-settings.txt") +
             ($extra -split ' ' | Where-Object { $_ })
    $procs += Start-Process -FilePath $exe -ArgumentList $args_ -PassThru -WindowStyle Hidden `
        -RedirectStandardError "$out\$side-stderr.txt" -RedirectStandardOutput "$out\$side-stdout.txt"
    Write-Host "[$side] $($args_ -join ' ')"
}
Remove-Item Env:GCN_STATE_HASH, Env:SDL_AUDIODRIVER -ErrorAction SilentlyContinue

function Reached($side) {
    $f = "$out\$side.txt"
    if (-not (Test-Path $f)) { return $false }
    $tail = Get-Content $f -Tail 1
    return $tail -and -not $tail.StartsWith('#') -and [int]($tail.Split(' ')[0]) -ge $last
}
$start = Get-Date
Write-Host "replaying to frame $last in both, a hash every $Every frames..."
while (((Get-Date) - $start).TotalMinutes -lt $TimeoutMin) {
    $done = 0
    foreach ($i in 0, 1) {
        $side = 'a', 'b' | Select-Object -Index $i
        if ((Reached $side) -or $procs[$i].HasExited) { $done++ }
    }
    if ($done -eq 2) { break }
    Start-Sleep -Seconds 3
}
foreach ($p in $procs) { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force } }
$mins = [math]::Round(((Get-Date) - $start).TotalMinutes, 1)
foreach ($side in 'a', 'b') {
    $fatal = Select-String -Path "$out\$side-stderr.txt" -Pattern 'fatal|panic|parted from' -ErrorAction SilentlyContinue
    if ($fatal) { Write-Host "[$side] $($fatal.Line -join ' | ')" }
    if (-not (Reached $side)) { Write-Host "[$side] stopped before frame $last" }
}
Write-Host "after $mins min:"
python gcn-recomp\tools\state_hash_diff.py "$out\a.txt" "$out\b.txt"
exit $LASTEXITCODE
