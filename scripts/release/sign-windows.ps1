param(
    [Parameter(Mandatory = $true)]
    [string]$Executable
)

$ErrorActionPreference = 'Stop'

$certificate = $env:WINDOWS_CERTIFICATE
$password = $env:WINDOWS_CERTIFICATE_PASSWORD

if ([string]::IsNullOrWhiteSpace($certificate) -or [string]::IsNullOrWhiteSpace($password)) {
    if ($env:GITHUB_STEP_SUMMARY) {
        Add-Content -Path $env:GITHUB_STEP_SUMMARY -Value '- Windows：未配置完整 PFX Secrets，本次生成 unsigned 测试包。'
    }
    exit 0
}

if (-not (Test-Path -LiteralPath $Executable -PathType Leaf)) {
    throw "待签名的 Windows 程序不存在：$Executable"
}

$signToolPattern = "${env:ProgramFiles(x86)}\Windows Kits\10\bin\*\x64\signtool.exe"
$signTool = Get-ChildItem -Path $signToolPattern -ErrorAction SilentlyContinue |
    Sort-Object FullName |
    Select-Object -Last 1

if (-not $signTool) {
    throw '找不到 Windows SDK signtool.exe。'
}

$pfxPath = Join-Path $env:RUNNER_TEMP 'git-clone-gui-authenticode.pfx'
try {
    [IO.File]::WriteAllBytes($pfxPath, [Convert]::FromBase64String($certificate))
    $signArguments = @(
        'sign',
        '/fd', 'SHA256',
        '/td', 'SHA256',
        '/tr', 'http://timestamp.digicert.com',
        '/f', $pfxPath,
        '/p', $password,
        $Executable
    )
    & $signTool.FullName @signArguments
    if ($LASTEXITCODE -ne 0) {
        throw "signtool sign 失败，退出码：$LASTEXITCODE"
    }

    & $signTool.FullName verify /pa /v $Executable
    if ($LASTEXITCODE -ne 0) {
        throw "signtool verify 失败，退出码：$LASTEXITCODE"
    }

    if ($env:GITHUB_STEP_SUMMARY) {
        Add-Content -Path $env:GITHUB_STEP_SUMMARY -Value '- Windows：Authenticode SHA-256 签名与时间戳验证通过。'
    }
}
finally {
    Remove-Item -LiteralPath $pfxPath -Force -ErrorAction SilentlyContinue
}
