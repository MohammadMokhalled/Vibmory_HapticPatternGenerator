param(
    [Parameter(Mandatory = $true)][string[]]$Paths,
    [Parameter(Mandatory = $true)][string]$Report
)
$ErrorActionPreference = 'Stop'
$reportDirectory = Split-Path -Parent ([IO.Path]::GetFullPath($Report))
New-Item -ItemType Directory -Path $reportDirectory -Force | Out-Null

if (-not $env:VIBMORY_SIGNING_CERTIFICATE_BASE64) {
    @{ signed = $false; reason = 'No signing certificate configured'; files = $Paths } |
        ConvertTo-Json | Set-Content -LiteralPath $Report -Encoding utf8
    Write-Output 'Producing an unsigned test build; no signing certificate is configured.'
    exit 0
}
if (-not $env:RUNNER_TEMP) { throw 'Signing requires an isolated runner temporary directory.' }
$certificatePath = Join-Path $env:RUNNER_TEMP ('vibmory-signing-' + [guid]::NewGuid().ToString() + '.pfx')
$imported = $null
$existingCertificate = $false
try {
    [IO.File]::WriteAllBytes($certificatePath, [Convert]::FromBase64String($env:VIBMORY_SIGNING_CERTIFICATE_BASE64))
    $password = if ($env:VIBMORY_SIGNING_CERTIFICATE_PASSWORD) { ConvertTo-SecureString $env:VIBMORY_SIGNING_CERTIFICATE_PASSWORD -AsPlainText -Force } else { New-Object System.Security.SecureString }
    $preview = Get-PfxData -FilePath $certificatePath -Password $password
    $thumbprint = $preview.EndEntityCertificates[0].Thumbprint
    $existingCertificate = Test-Path -LiteralPath "Cert:\CurrentUser\My\$thumbprint"
    $imported = Import-PfxCertificate -FilePath $certificatePath -Password $password -CertStoreLocation Cert:\CurrentUser\My
    $signTool = Get-Command signtool.exe -ErrorAction SilentlyContinue
    $signToolPath = if ($signTool) { $signTool.Source } else { $null }
    if (-not $signToolPath) {
        $candidate = Get-ChildItem "$env:WindowsSdkDir/bin/*/x64/signtool.exe" | Sort-Object FullName -Descending | Select-Object -First 1
        if ($candidate) { $signToolPath = $candidate.FullName }
    }
    if (-not $signToolPath) { throw 'Windows SDK SignTool was not found.' }
    $timestamp = if ($env:VIBMORY_TIMESTAMP_URL) { $env:VIBMORY_TIMESTAMP_URL } else { 'http://timestamp.digicert.com' }
    foreach ($path in $Paths) {
        & $signToolPath sign /fd SHA256 /sha1 $thumbprint /tr $timestamp /td SHA256 $path
        if ($LASTEXITCODE -ne 0) { throw "Authenticode signing failed: $path" }
        & $signToolPath verify /pa /all $path
        if ($LASTEXITCODE -ne 0) { throw "Authenticode verification failed: $path" }
    }
    @{ signed = $true; thumbprint = $thumbprint; files = $Paths } |
        ConvertTo-Json | Set-Content -LiteralPath $Report -Encoding utf8
}
finally {
    if ($imported -and -not $existingCertificate) {
        Remove-Item -LiteralPath "Cert:\CurrentUser\My\$thumbprint" -ErrorAction SilentlyContinue
    }
    if (Test-Path -LiteralPath $certificatePath) {
        Remove-Item -LiteralPath $certificatePath -Force
    }
}
