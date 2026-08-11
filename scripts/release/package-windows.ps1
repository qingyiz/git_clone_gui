param(
    [Parameter(Mandatory = $true)]
    [string]$InstallBin,

    [Parameter(Mandatory = $true)]
    [string]$OutputZip
)

$ErrorActionPreference = 'Stop'

$installBinPath = (Resolve-Path -LiteralPath $InstallBin).Path
$executable = Join-Path $installBinPath 'GitCloneGui.exe'

$requiredFiles = @(
    $executable,
    (Join-Path $installBinPath 'Qt6Core.dll'),
    (Join-Path $installBinPath 'Qt6Gui.dll'),
    (Join-Path $installBinPath 'Qt6Widgets.dll'),
    (Join-Path $installBinPath 'platforms\qwindows.dll')
)

foreach ($requiredFile in $requiredFiles) {
    if (-not (Test-Path -LiteralPath $requiredFile -PathType Leaf)) {
        throw "Windows 安装树缺少：$requiredFile"
    }
}

$runtimeNames = @('msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')
$missingRuntime = $runtimeNames | Where-Object {
    -not (Test-Path -LiteralPath (Join-Path $installBinPath $_) -PathType Leaf)
}

if ($missingRuntime.Count -gt 0) {
    $redistPattern = "${env:ProgramFiles}\Microsoft Visual Studio\2022\*\VC\Redist\MSVC\*\x64\Microsoft.VC143.CRT"
    $redistDirectory = Get-ChildItem -Path $redistPattern -Directory -ErrorAction SilentlyContinue |
        Sort-Object FullName |
        Select-Object -Last 1

    if (-not $redistDirectory) {
        throw "缺少 MSVC runtime 且无法定位 Microsoft.VC143.CRT：$($missingRuntime -join ', ')"
    }

    Copy-Item -Path (Join-Path $redistDirectory.FullName '*.dll') -Destination $installBinPath -Force
}

foreach ($runtimeName in $runtimeNames) {
    $runtimePath = Join-Path $installBinPath $runtimeName
    if (-not (Test-Path -LiteralPath $runtimePath -PathType Leaf)) {
        throw "Windows 便携包缺少 MSVC runtime：$runtimeName"
    }
}

$outputDirectory = Split-Path -Parent $OutputZip
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$stageRoot = Join-Path $env:RUNNER_TEMP 'git-clone-gui-windows-package'
$stageApp = Join-Path $stageRoot 'GitCloneGui'
Remove-Item -LiteralPath $stageRoot -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $stageApp -Force | Out-Null
Copy-Item -Path (Join-Path $installBinPath '*') -Destination $stageApp -Recurse -Force
Remove-Item -LiteralPath $OutputZip -Force -ErrorAction SilentlyContinue
Compress-Archive -Path $stageApp -DestinationPath $OutputZip -CompressionLevel Optimal

Write-Host "Windows 发布包：$OutputZip"
