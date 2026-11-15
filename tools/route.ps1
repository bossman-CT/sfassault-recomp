<#
.SYNOPSIS
Named routes: recorded runs you can replay instead of playing through again.

.DESCRIPTION
Every run of the game records its controller input to saves\inputs\<date-time>, and a replay
of it is exact. Save a run you want to come back to as a route, and `play` fast-forwards
through it and hands you the controller where it ends.

  .\tools\route.ps1 list
  .\tools\route.ps1 save <name>                       # the newest recording
  .\tools\route.ps1 save <name> -From saves\inputs\20261103-201727
  .\tools\route.ps1 play <name> [-Fullscreen] [-Scale 4] [-KeepSave]
  .\tools\route.ps1 info <name>

Playing on from a route is recorded too, as a new run: save that as a new route to make a
longer one. Progress saved in game while playing on from a route goes to the route's own
copy of the memory card, not to saves\memcard_a.raw. Add -KeepSave to make it your real
memory card when you quit (the old one is backed up first).

Routes stay under saves\routes on your machine; saves\ is ignored by git.
#>
param(
    [Parameter(Position = 0, Mandatory)][ValidateSet('list', 'save', 'play', 'info')][string]$Command,
    [Parameter(Position = 1)][string]$Name,
    [string]$From,
    [switch]$Fullscreen,
    [int]$Scale = 0,
    [switch]$KeepSave,
    [switch]$Force
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'route-common.ps1')

function Need-Name { if (-not $Name) { throw "give the route a name: .\tools\route.ps1 $Command <name>" } }
function Route-Dir { Join-Path $RoutesDir $Name }

switch ($Command) {
    'list' {
        if (-not (Test-Path $RoutesDir) -or -not (Get-ChildItem $RoutesDir -Directory)) {
            'No routes yet. Play, then: .\tools\route.ps1 save <name>'
            return
        }
        Get-ChildItem $RoutesDir -Directory | Sort-Object Name | ForEach-Object {
            $src = (Get-Content (Join-Path $_.FullName 'route.txt') -ErrorAction SilentlyContinue | Select-Object -First 1) -replace '^from ', ''
            [pscustomobject]@{ Route = $_.Name; Length = Format-Frames (Get-RouteEnd $_.FullName); From = $src }
        } | Format-Table -AutoSize
    }
    'save' {
        Need-Name
        if ($Name -notmatch '^[A-Za-z0-9._-]+$') { throw "use letters, digits, '.', '_' and '-' in a route name" }
        if (-not $From) {
            $newest = Get-ChildItem (Join-Path $ProjectRoot 'saves\inputs') -Directory -ErrorAction SilentlyContinue |
                Where-Object { Test-Path (Join-Path $_.FullName 'inputs.txt') } | Sort-Object Name | Select-Object -Last 1
            if (-not $newest) { throw 'no recordings in saves\inputs yet: play the game first' }
            $From = $newest.FullName
        } elseif (-not [IO.Path]::IsPathRooted($From)) {
            $From = Join-Path $ProjectRoot $From
        }
        $dest = Route-Dir
        if ((Test-Path $dest) -and -not $Force) { throw "route '$Name' exists; add -Force to replace it" }
        if (Test-Path $dest) { Remove-Item -Recurse -Force $dest }
        New-Item -ItemType Directory -Force $dest | Out-Null
        foreach ($f in 'inputs.txt', 'memcard_a.raw') {
            if (Test-Path (Join-Path $From $f)) { Copy-Item (Join-Path $From $f) $dest }
        }
        @("from $(Split-Path $From -Leaf)", "saved $(Get-Date -Format s)") | Set-Content (Join-Path $dest 'route.txt')
        "Saved route '$Name' ($(Format-Frames (Get-RouteEnd $dest)) of play) from $(Split-Path $From -Leaf)."
    }
    'info' {
        Need-Name
        $dir = Route-Dir
        if (-not (Test-Path $dir)) { throw "no route '$Name' (.\tools\route.ps1 list)" }
        Get-Content (Join-Path $dir 'route.txt') -ErrorAction SilentlyContinue
        "length $(Format-Frames (Get-RouteEnd $dir)) ($(Get-RouteEnd $dir) frames)"
    }
    'play' {
        Need-Name
        Assert-GameBuilt
        $dir = Route-Dir
        if (-not (Test-Path $dir)) { throw "no route '$Name' (.\tools\route.ps1 list)" }
        "Fast-forwarding $(Format-Frames (Get-RouteEnd $dir)) of '$Name'. When it ends the game slows to normal speed and the controller is yours."
        $gameArgs = @("--replay=saves\routes\$Name", '--fast-forward')  # relative: the game runs in the project folder
        if ($Fullscreen) { $gameArgs += '--fullscreen' }
        if ($Scale -gt 0) { $gameArgs += "--scale=$Scale" }
        $since = Get-Date
        Push-Location $ProjectRoot
        try { & $GameExe @gameArgs } finally { Pop-Location }
        if ($KeepSave) { Use-RouteSave $dir $since }
    }
}
