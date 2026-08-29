param(
    [Parameter(Mandatory = $true)]
    [string] $RepositoryRoot,

    [Parameter(Mandatory = $true)]
    [string] $ComponentDll,

    [Parameter(Mandatory = $true)]
    [string] $OutputDirectory,

    [Parameter(Mandatory = $true)]
    [string] $Version
)

$ErrorActionPreference = 'Stop'

function Assert-PathWithin {
    param([string] $Candidate, [string] $Parent, [string] $Label)
    $candidateFull = [System.IO.Path]::GetFullPath($Candidate)
    $parentFull = [System.IO.Path]::GetFullPath($Parent).TrimEnd('\') + '\'
    if (-not $candidateFull.StartsWith($parentFull, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "$Label must stay within $Parent; actual path: $candidateFull"
    }
    return $candidateFull
}

function Get-PeMachine {
    param([string] $Path)
    $stream = [System.IO.File]::OpenRead($Path)
    try {
        $reader = [System.IO.BinaryReader]::new($stream)
        if ($reader.ReadUInt16() -ne 0x5A4D) { throw "Not a PE file: $Path" }
        $stream.Position = 0x3C
        $peOffset = $reader.ReadUInt32()
        $stream.Position = $peOffset
        if ($reader.ReadUInt32() -ne 0x00004550) { throw "Invalid PE signature: $Path" }
        return $reader.ReadUInt16()
    }
    finally {
        $stream.Dispose()
    }
}

function Get-Sha256 {
    param([string] $Path)
    $stream = [System.IO.File]::OpenRead($Path)
    $algorithm = [System.Security.Cryptography.SHA256]::Create()
    try {
        return ([System.BitConverter]::ToString($algorithm.ComputeHash($stream))).Replace('-', '')
    }
    finally {
        $algorithm.Dispose()
        $stream.Dispose()
    }
}

if ($Version -notmatch '^\d+\.\d+\.\d+-[0-9A-Za-z]+(?:\.[0-9A-Za-z]+)*$') {
    throw "A SemVer prerelease version is required; actual value: $Version"
}

$root = [System.IO.Path]::GetFullPath($RepositoryRoot)
$dll = Assert-PathWithin -Candidate $ComponentDll -Parent (Join-Path $root 'build') -Label 'Component DLL'
if (-not (Test-Path -LiteralPath $dll -PathType Leaf)) { throw "Component DLL is missing: $dll" }
if ([System.IO.Path]::GetFileName($dll) -cne 'foo_pod_bridge.dll') { throw "Unexpected component filename: $dll" }
if ($dll -notmatch '[\\/]Release[\\/]foo_pod_bridge\.dll$') { throw "Only the Release component may be packaged: $dll" }
if ((Get-PeMachine -Path $dll) -ne 0x8664) { throw "Component DLL is not AMD64: $dll" }

$versionInfo = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($dll)
if ($versionInfo.ProductVersion -ne $Version) {
    throw "DLL ProductVersion '$($versionInfo.ProductVersion)' does not match package version '$Version'."
}

$outputRootExpected = Join-Path $root 'dist'
$outputRoot = [System.IO.Path]::GetFullPath($OutputDirectory)
if ($outputRoot.TrimEnd('\') -ine $outputRootExpected.TrimEnd('\')) {
    throw "Output directory must be the repository dist directory: $outputRootExpected"
}
if (-not (Test-Path -LiteralPath $outputRoot)) {
    New-Item -ItemType Directory -Path $outputRoot | Out-Null
}

$packagePath = Join-Path $outputRoot "FooPodBridge-$Version.fb2k-component"
if (Test-Path -LiteralPath $packagePath) {
    throw "Candidate packages are immutable and may not be overwritten: $packagePath"
}

$tempRoot = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
$stage = Join-Path $tempRoot ("foopodbridge-package-" + [System.Guid]::NewGuid().ToString('N'))
$stage = Assert-PathWithin -Candidate $stage -Parent $tempRoot -Label 'Package staging directory'

try {
    New-Item -ItemType Directory -Path $stage | Out-Null
    Copy-Item -LiteralPath $dll -Destination (Join-Path $stage 'foo_pod_bridge.dll')
    Copy-Item -LiteralPath (Join-Path $root 'LICENSES\LGPL-3.0-or-later.txt') -Destination (Join-Path $stage 'LICENSE.txt')
    Copy-Item -LiteralPath (Join-Path $root 'THIRD_PARTY_NOTICES.md') -Destination (Join-Path $stage 'THIRD_PARTY_NOTICES.txt')

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::CreateFromDirectory(
        $stage,
        $packagePath,
        [System.IO.Compression.CompressionLevel]::Optimal,
        $false)

    $archive = [System.IO.Compression.ZipFile]::OpenRead($packagePath)
    try {
        $entries = @($archive.Entries | ForEach-Object { $_.FullName })
        $expectedEntries = @('foo_pod_bridge.dll', 'LICENSE.txt', 'THIRD_PARTY_NOTICES.txt')
        if ($entries.Count -ne $expectedEntries.Count -or
            @($entries | Where-Object { $_ -notin $expectedEntries }).Count -ne 0) {
            throw "Package contains unexpected entries: $($entries -join ', ')"
        }
    }
    finally {
        $archive.Dispose()
    }

    $dllHash = Get-Sha256 -Path $dll
    $packageHash = Get-Sha256 -Path $packagePath
    Write-Output "Package: $packagePath"
    Write-Output "Package SHA-256: $packageHash"
    Write-Output "DLL SHA-256: $dllHash"
}
catch {
    if (Test-Path -LiteralPath $packagePath) {
        Remove-Item -LiteralPath $packagePath -Force
    }
    throw
}
finally {
    if (Test-Path -LiteralPath $stage) {
        $validatedStage = Assert-PathWithin -Candidate $stage -Parent $tempRoot -Label 'Cleanup staging directory'
        Remove-Item -LiteralPath $validatedStage -Recurse -Force
    }
}
