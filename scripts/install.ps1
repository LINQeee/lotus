param(
    [string]$Prefix = "$env:LOCALAPPDATA\Lotus",
    [string]$Source = '',
    [string]$MsysRoot = "$env:SystemDrive\msys64",
    [switch]$NoDeps,
    [switch]$NoPath
)
$ErrorActionPreference = 'Stop'
$interactive = -not [Console]::IsOutputRedirected -and $env:GITHUB_ACTIONS -ne 'true'

function Invoke-SetupStep($Title, $File, $Arguments, [switch]$AllowRestart) {
    $stdout = [IO.Path]::GetTempFileName()
    $stderr = [IO.Path]::GetTempFileName()
    $timer = [Diagnostics.Stopwatch]::StartNew()
    $process = $null
    if (-not $interactive) { Write-Host "  > $Title" -ForegroundColor Cyan }
    try {
        $process = Start-Process $File -ArgumentList $Arguments -NoNewWindow -PassThru `
            -RedirectStandardOutput $stdout -RedirectStandardError $stderr
        $frames = '|/-\'
        $frame = 0
        while (-not $process.HasExited) {
            if ($interactive) {
                Write-Host ("`r  {0} {1}  ({2}s)   " -f $frames[$frame++ % 4], $Title, [int]$timer.Elapsed.TotalSeconds) -NoNewline -ForegroundColor Cyan
            }
            Start-Sleep -Milliseconds 200
        }
        $process.WaitForExit()
        if ($interactive) { Write-Host "`r$(' ' * ($Title.Length + 30))`r" -NoNewline }
        if ($process.ExitCode -ne 0 -and -not $AllowRestart) {
            Get-Content $stdout, $stderr -Tail 20 | Out-Host
            throw "$Title failed (exit $($process.ExitCode))"
        }
        if ($process.ExitCode -ne 0) {
            Write-Host '  > Continue MSYS2 update in a new shell' -ForegroundColor Yellow
        } else {
            Write-Host ("  {0} {1}  ({2}s)" -f [char]0x2713, $Title, [int]$timer.Elapsed.TotalSeconds) -ForegroundColor Green
        }
    } finally {
        if ($process -and -not $process.HasExited) { $process.Kill() }
        Remove-Item $stdout, $stderr -ErrorAction SilentlyContinue
    }
}

Write-Host "`n  LOTUS  /  Windows setup" -ForegroundColor Cyan
Write-Host "  Prepare MSYS2, install Lotus, register the launcher`n"
$Prefix = [IO.Path]::GetFullPath($Prefix)
if (-not [Environment]::Is64BitOperatingSystem) { throw 'Lotus requires 64-bit Windows' }
Write-Host '  [1/3] Windows toolchain' -ForegroundColor Cyan
$bash = Join-Path $MsysRoot 'usr\bin\bash.exe'
if (-not (Test-Path $bash)) {
    if ($NoDeps) { throw "MSYS2 not found at $MsysRoot" }
    Invoke-SetupStep 'Install MSYS2' winget ('install --id MSYS2.MSYS2 --exact --location "{0}" --silent --disable-interactivity --accept-package-agreements --accept-source-agreements' -f $MsysRoot.TrimEnd('\'))
}
$env:MSYSTEM = 'CLANG64'
$env:CHERE_INVOKING = '1'
$env:LOTUS_HOME = [IO.Path]::GetFullPath($Prefix)
$env:LOTUS_SOURCE = if ($Source) { [IO.Path]::GetFullPath($Source) } else { '' }
$env:LOTUS_NO_DEPS = if ($NoDeps) { '1' } else { '0' }
$env:LOTUS_TERMINAL = if ($interactive) { '1' } else { '0' }
$invokeBash = {
    param([string]$Command, [string]$Title = '', [switch]$AllowRestart)
    $commandFile = Join-Path ([IO.Path]::GetTempPath()) ([IO.Path]::GetRandomFileName() + '.sh')
    try {
        [IO.File]::WriteAllText($commandFile, $Command.Replace("`r", ''), [Text.UTF8Encoding]::new($false))
        if ($Title) {
            Invoke-SetupStep $Title $bash ('--login "{0}"' -f ($commandFile -replace '\\', '/')) -AllowRestart:$AllowRestart
        } else {
            & $bash --login ($commandFile -replace '\\', '/')
            if ($LASTEXITCODE -ne 0) { throw 'Lotus setup failed; see the output above' }
        }
    } finally {
        Remove-Item $commandFile -ErrorAction SilentlyContinue
    }
}
if (-not $NoDeps) {
    # MSYS2 may terminate its first shell when updating the runtime
    & $invokeBash 'pacman -Syu --noconfirm' 'Update MSYS2 runtime' -AllowRestart
    & $invokeBash 'pacman -Syu --noconfirm' 'Update MSYS2 packages'
}
Write-Host '  [2/3] Build and install Lotus' -ForegroundColor Cyan
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
Write-Host '  [3/3] Register Windows launcher' -ForegroundColor Cyan
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
Write-Host "`n  OK  Lotus is ready" -ForegroundColor Green
Write-Host "  Installed in $Prefix"
if ($NoPath) {
    Write-Host "  PATH unchanged. Run: & '$bin\lotus.cmd' run hello.lt"
} else {
    Write-Host '  Open a new terminal, then run:'
    Write-Host "`n    lotus run hello.lt" -ForegroundColor Cyan
}
Write-Host ''
