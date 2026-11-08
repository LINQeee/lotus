param(
    [string]$Prefix = "$env:LOCALAPPDATA\Lotus",
    [string]$Source = '',
    [string]$MsysRoot = "$env:SystemDrive\msys64",
    [switch]$NoDeps,
    [switch]$NoPath
)
$ErrorActionPreference = 'Stop'
$Prefix = [IO.Path]::GetFullPath($Prefix)
if (-not [Environment]::Is64BitOperatingSystem) { throw 'Lotus requires 64-bit Windows' }
$bash = Join-Path $MsysRoot 'usr\bin\bash.exe'
if (-not (Test-Path $bash)) {
    if ($NoDeps) { throw "MSYS2 not found at $MsysRoot" }
    & winget install --id MSYS2.MSYS2 --exact --location $MsysRoot --accept-package-agreements --accept-source-agreements
    if ($LASTEXITCODE -ne 0) { throw 'MSYS2 installation failed' }
}
$env:MSYSTEM = 'CLANG64'
$env:CHERE_INVOKING = '1'
$env:LOTUS_HOME = [IO.Path]::GetFullPath($Prefix)
$env:LOTUS_SOURCE = if ($Source) { [IO.Path]::GetFullPath($Source) } else { '' }
$env:LOTUS_NO_DEPS = if ($NoDeps) { '1' } else { '0' }
$invokeBash = {
    param([string]$Command)
    $commandFile = Join-Path ([IO.Path]::GetTempPath()) ([IO.Path]::GetRandomFileName() + '.sh')
    try {
        [IO.File]::WriteAllText($commandFile, $Command.Replace("`r", ''), [Text.UTF8Encoding]::new($false))
        & $bash --login ($commandFile -replace '\\', '/')
        if ($LASTEXITCODE -ne 0) { throw 'Lotus setup failed; see the output above' }
    } finally {
        Remove-Item $commandFile -ErrorAction SilentlyContinue
    }
}
if (-not $NoDeps) {
    # MSYS2 may terminate its first shell when updating the runtime
    & $bash --login -c 'pacman -Syu --noconfirm'
    & $invokeBash 'pacman -Syu --noconfirm'
}
$script = Join-Path ([IO.Path]::GetTempPath()) ([IO.Path]::GetRandomFileName() + '.sh')
try {
    if ($Source) {
        Copy-Item (Join-Path $Source 'scripts\install.sh') $script
    } else {
        $ref = if ($env:LOTUS_REF) { $env:LOTUS_REF } else { 'main' }
        Invoke-WebRequest "https://raw.githubusercontent.com/LINQeee/lotus/$ref/scripts/install.sh" -OutFile $script
    }
    $env:LOTUS_INSTALL_SCRIPT = $script
    & $invokeBash @'
set -e
export LOTUS_HOME=$(cygpath -u "$LOTUS_HOME")
if [ -n "$LOTUS_SOURCE" ]; then export LOTUS_SOURCE=$(cygpath -u "$LOTUS_SOURCE"); fi
args=(--no-path)
if [ "$LOTUS_NO_DEPS" = 1 ]; then args+=(--no-deps); fi
bash "$(cygpath -u "$LOTUS_INSTALL_SCRIPT")" "${args[@]}"
'@
} finally {
    Remove-Item $script -ErrorAction SilentlyContinue
}
$bin = Join-Path $Prefix 'cmd'
New-Item -ItemType Directory -Force $bin | Out-Null
$toolchain = Join-Path $MsysRoot 'clang64\bin'
@"
@echo off
setlocal
set "PATH=$toolchain;%PATH%"
"%~dp0..\bin\lotus.exe" %*
exit /b %ERRORLEVEL%
"@ | Set-Content (Join-Path $bin 'lotus.cmd') -Encoding ASCII
if (-not $NoPath) {
    $userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
    if ($bin -notin ($userPath -split ';')) {
        [Environment]::SetEnvironmentVariable('Path', "$bin;$userPath", 'User')
    }
    $env:Path = "$bin;$env:Path"
}
Write-Host "Lotus installed in $Prefix. Run: lotus run hello.lt"
