param(
    [Parameter(Mandatory = $false)]
    [string]$Version = "v0.1.0"
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$distRoot = Join-Path $repoRoot "dist"
$packageName = "WorldSync-$Version-windows-x64"
$packageRoot = Join-Path $distRoot $packageName
$archivePath = Join-Path $distRoot "$packageName.zip"
$dllPath = Join-Path $repoRoot "build\Release\WorldSync.dll"
$includePath = Join-Path $repoRoot "include\worldsync.inc"

if (-not (Test-Path $dllPath)) {
    throw "Missing build artifact: $dllPath. Run: cmake --build build --config Release --target WorldSync"
}

if (-not (Test-Path $includePath)) {
    throw "Missing Pawn include: $includePath"
}

if (Test-Path $packageRoot) {
    Remove-Item -Recurse -Force $packageRoot
}

New-Item -ItemType Directory -Force (Join-Path $packageRoot "components") | Out-Null
New-Item -ItemType Directory -Force (Join-Path $packageRoot "pawno\include") | Out-Null
New-Item -ItemType Directory -Force (Join-Path $packageRoot "docs") | Out-Null
New-Item -ItemType Directory -Force (Join-Path $packageRoot "examples") | Out-Null

Copy-Item $dllPath (Join-Path $packageRoot "components\WorldSync.dll")
Copy-Item $includePath (Join-Path $packageRoot "pawno\include\worldsync.inc")
Copy-Item (Join-Path $repoRoot "README.md") (Join-Path $packageRoot "README.md")
Copy-Item (Join-Path $repoRoot "CHANGELOG.md") (Join-Path $packageRoot "CHANGELOG.md") -ErrorAction SilentlyContinue
Copy-Item (Join-Path $repoRoot "LICENSE") (Join-Path $packageRoot "LICENSE") -ErrorAction SilentlyContinue
Copy-Item (Join-Path $repoRoot "CONTRIBUTING.md") (Join-Path $packageRoot "CONTRIBUTING.md") -ErrorAction SilentlyContinue
Copy-Item (Join-Path $repoRoot "ROADMAP.md") (Join-Path $packageRoot "ROADMAP.md") -ErrorAction SilentlyContinue
Copy-Item (Join-Path $repoRoot "docs\*") (Join-Path $packageRoot "docs") -Recurse
Copy-Item (Join-Path $repoRoot "examples\*") (Join-Path $packageRoot "examples") -Recurse

if (Test-Path $archivePath) {
    Remove-Item -Force $archivePath
}

Compress-Archive -Path (Join-Path $packageRoot "*") -DestinationPath $archivePath -Force

Write-Host "Created $archivePath"
