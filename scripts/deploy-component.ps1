param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('dev')]
    [string] $Instance,

    [Parameter(Mandatory = $true)]
    [string] $ComponentDll,

    [switch] $ValidateOnly
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$buildRoot = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'build')).TrimEnd('\') + '\'
$dll = [System.IO.Path]::GetFullPath($ComponentDll)

if (-not $dll.StartsWith($buildRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Component DLL must be under the FooPodBridge build directory: $dll"
}
if ([System.IO.Path]::GetFileName($dll) -cne 'foo_pod_bridge.dll') {
    throw "Unexpected component filename: $dll"
}
if ($dll -notmatch '[\\/]Release[\\/]foo_pod_bridge\.dll$') {
    throw "Only a Release component may be deployed: $dll"
}
if (-not (Test-Path -LiteralPath $dll -PathType Leaf)) {
    throw "Component DLL does not exist: $dll"
}

$workspaceRoot = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot '..\..'))
$instanceRoot = [System.IO.Path]::GetFullPath((Join-Path $workspaceRoot 'FooCrate\.local\foobar-dev'))
$allowedRoot = [System.IO.Path]::GetFullPath('D:\dev\foo\FooCrate\.local\foobar-dev')
if ($instanceRoot.TrimEnd('\') -ine $allowedRoot.TrimEnd('\')) {
    throw "Resolved instance is not the approved foobar-dev path: $instanceRoot"
}
if (-not (Test-Path -LiteralPath $instanceRoot -PathType Container)) {
    throw "Approved foobar-dev instance is missing: $instanceRoot"
}

$runningDev = Get-Process -Name foobar2000 -ErrorAction SilentlyContinue | Where-Object {
    try { $_.Path -and [System.IO.Path]::GetFullPath($_.Path).StartsWith($instanceRoot, [System.StringComparison]::OrdinalIgnoreCase) }
    catch { $false }
}
if ($runningDev) {
    throw 'foobar-dev is running. Close it before deploying FooPodBridge.'
}

$targetDirectory = Join-Path $instanceRoot 'profile\user-components-x64\foo_pod_bridge'
$targetDll = Join-Path $targetDirectory 'foo_pod_bridge.dll'
Write-Output "Validated deployment target: $targetDll"

if ($ValidateOnly) { return }

if (-not (Test-Path -LiteralPath $targetDirectory)) {
    New-Item -ItemType Directory -Path $targetDirectory | Out-Null
}
Copy-Item -LiteralPath $dll -Destination $targetDll -Force
Write-Output "Deployed FooPodBridge to: $targetDll"
