param(
    [Parameter(Mandatory = $true)]
    [string] $RepositoryRoot
)

$ErrorActionPreference = 'Stop'
$strictUtf8 = [System.Text.UTF8Encoding]::new($false, $true)
$root = [System.IO.Path]::GetFullPath($RepositoryRoot)

if (-not (Test-Path -LiteralPath $root -PathType Container)) {
    throw "Repository root does not exist: $root"
}

$coreRoots = @(
    (Join-Path $root 'src\core'),
    (Join-Path $root 'include\foopodbridge\core')
)
$coreForbidden = '(?i)(foobar2000|columns[_ -]?ui|foo[_ -]?crate|service_v1\.h)'
$projectForbidden = '(?i)(iTunesCrypt|Apple Mobile Device|iPhoneCalc|hash72|\bCBK\b)'
$contractForbidden = 'std::(vector|string|map|set|shared_ptr|unique_ptr|function)'
$failures = [System.Collections.Generic.List[string]]::new()

foreach ($coreRoot in $coreRoots) {
    foreach ($file in Get-ChildItem -LiteralPath $coreRoot -Recurse -File) {
        $text = $strictUtf8.GetString([System.IO.File]::ReadAllBytes($file.FullName))
        if ($text -match $coreForbidden) {
            $failures.Add("Core dependency boundary violation: $($file.FullName)")
        }
    }
}

foreach ($file in Get-ChildItem -LiteralPath (Join-Path $root 'src') -Recurse -File) {
    $text = $strictUtf8.GetString([System.IO.File]::ReadAllBytes($file.FullName))
    if ($text -match $projectForbidden) {
        $failures.Add("Forbidden implementation dependency: $($file.FullName)")
    }
}

$contractPath = Join-Path $root 'include\foopodbridge\service_v1.h'
$contractText = $strictUtf8.GetString([System.IO.File]::ReadAllBytes($contractPath))
if ($contractText -match $contractForbidden) {
    $failures.Add("Public contract exposes an STL ownership/container type: $contractPath")
}

if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Output 'Source dependency boundaries passed.'
