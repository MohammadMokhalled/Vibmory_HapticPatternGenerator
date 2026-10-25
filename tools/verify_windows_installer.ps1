param(
    [Parameter(Mandatory = $true)][string]$Installer,
    [Parameter(Mandatory = $true)][string]$Stage,
    [Parameter(Mandatory = $true)][string]$Report
)
$ErrorActionPreference = 'Stop'
if (-not $env:RUNNER_TEMP) { throw 'Installer verification requires an isolated CI runner.' }
$installerPath = (Resolve-Path -LiteralPath $Installer).Path
$stagePath = (Resolve-Path -LiteralPath $Stage).Path
$registryPath = 'HKCU:\Software\Vibmory\Installer'
$uninstallRegistryPath = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\Vibmory'
$shortcutDirectory = Join-Path ([Environment]::GetFolderPath('Programs')) 'Vibmory'
if ((Test-Path -LiteralPath $registryPath) -or (Test-Path -LiteralPath $shortcutDirectory)) {
    throw 'Installer verification requires a clean runner user profile.'
}
$installDirectory = Join-Path ([IO.Path]::GetFullPath($env:RUNNER_TEMP)) ('Vibmory installer QA ä-' + [guid]::NewGuid().ToString())
if (Test-Path -LiteralPath $installDirectory) { throw 'Installer QA directory already exists.' }

function Invoke-TestProcess([string]$Executable, [string]$Arguments, [int]$ExpectedExit = 0) {
    $process = Start-Process -FilePath $Executable -ArgumentList $Arguments -WindowStyle Hidden -PassThru
    if (-not $process.WaitForExit(60000)) { $process.Kill(); throw "Timed out: $Executable" }
    if ($process.ExitCode -ne $ExpectedExit) { throw "Unexpected exit code $($process.ExitCode), expected ${ExpectedExit}: $Executable" }
}

function Compare-InstalledFiles {
    foreach ($file in Get-ChildItem -LiteralPath $stagePath -Recurse -File) {
        $relative = $file.FullName.Substring($stagePath.Length + 1)
        $installed = Join-Path $installDirectory $relative
        if (-not (Test-Path -LiteralPath $installed)) { throw "Installed file is missing: $relative" }
        if ((Get-FileHash -LiteralPath $installed -Algorithm SHA256).Hash -ne (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash) {
            throw "Installed file differs from verified stage: $relative"
        }
    }
}

Invoke-TestProcess $installerPath "/S /D=$installDirectory"
Compare-InstalledFiles
if ((Get-ItemPropertyValue -LiteralPath $registryPath -Name InstallDir) -ne $installDirectory) { throw 'Per-user installation registration is incorrect.' }
if (-not (Test-Path -LiteralPath (Join-Path $shortcutDirectory 'Vibmory.lnk'))) { throw 'Start Menu shortcut is missing.' }
$env:QT_QPA_PLATFORM = 'windows'
$env:QT_PLUGIN_PATH = ''
$env:QT_QPA_PLATFORM_PLUGIN_PATH = ''
$env:PATH = "$env:WINDIR\System32;$env:WINDIR"
$installedExecutable = Join-Path $installDirectory 'Vibmory.exe'
Invoke-TestProcess $installedExecutable '--smoke-test'
$project = Join-Path $installDirectory 'my-pattern.vibmory'
[IO.File]::WriteAllText($project, 'User-owned project must survive upgrade and uninstall.')
$locked = [IO.File]::Open($installedExecutable, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
try { Invoke-TestProcess $installerPath "/S /D=$installDirectory" 5 }
finally { $locked.Dispose() }
Compare-InstalledFiles
Invoke-TestProcess $installerPath "/S /D=$installDirectory"
Compare-InstalledFiles
if ([IO.File]::ReadAllText($project) -ne 'User-owned project must survive upgrade and uninstall.') { throw 'Upgrade modified a user project.' }
Invoke-TestProcess $installedExecutable '--smoke-test'
$uninstaller = Join-Path $installDirectory 'Uninstall.exe'
$locked = [IO.File]::Open($installedExecutable, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
try { Invoke-TestProcess $uninstaller "/S _?=$installDirectory" 5 }
finally { $locked.Dispose() }
Compare-InstalledFiles
Invoke-TestProcess $uninstaller '/S'
$deadline = [DateTime]::UtcNow.AddSeconds(30)
while ((Test-Path -LiteralPath $uninstallRegistryPath) -or (Test-Path -LiteralPath $uninstaller)) {
    if ([DateTime]::UtcNow -gt $deadline) { throw 'Uninstaller did not finish cleanup.' }
    Start-Sleep -Milliseconds 200
}
if (Test-Path -LiteralPath $installedExecutable) { throw 'Uninstaller left the application executable.' }
if (Test-Path -LiteralPath $registryPath) { throw 'Uninstaller left its installation registration.' }
if (Test-Path -LiteralPath (Join-Path $shortcutDirectory 'Vibmory.lnk')) { throw 'Uninstaller left its Start Menu shortcut.' }
if ([IO.File]::ReadAllText($project) -ne 'User-owned project must survive upgrade and uninstall.') { throw 'Uninstaller modified a user project.' }
@{ installed = $true; upgraded = $true; launchedWithoutQt = $true; busyInstallRejected = $true; busyUninstallRejected = $true; userProjectPreserved = $true; uninstalled = $true } |
    ConvertTo-Json | Set-Content -LiteralPath $Report -Encoding utf8
# Only delete this test's sentinel and now-empty directory. Unexpected leftover
# files cause failure and remain available for diagnostics.
Remove-Item -LiteralPath $project
Remove-Item -LiteralPath $installDirectory
Write-Output 'Verified per-user install, upgrade, busy-file guards, runtime launch, and uninstall preserving user data.'
