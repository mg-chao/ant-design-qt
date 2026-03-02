param(
  [ValidateSet("Debug", "Release")]
  [string]$Config = "Debug",
  [switch]$Clean,
  [switch]$NoBuild,
  [switch]$Wait,
  [switch]$Detached
)

$ErrorActionPreference = "Stop"

if ($Wait -and $Detached) {
  throw "Use either -Wait or -Detached, not both."
}

function Resolve-RepoRoot {
  $scriptDir = $PSScriptRoot
  if (-not $scriptDir) {
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
  }
  if (-not $scriptDir) {
    throw "Failed to resolve script directory."
  }
  return (Resolve-Path (Join-Path $scriptDir "..")).Path
}

function Ensure-PathPrefix {
  param(
    [string]$PathValue
  )

  if (-not $PathValue) {
    return
  }

  $normalized = $PathValue.TrimEnd("\")
  $currentPath = [Environment]::GetEnvironmentVariable("Path", "Process")
  if (-not $currentPath) {
    $currentPath = $env:PATH
  }

  $segments = $currentPath -split ";" | Where-Object { $_ -and $_.Trim() -ne "" }
  $exists = $false
  foreach ($seg in $segments) {
    if ($seg.TrimEnd("\").ToLowerInvariant() -eq $normalized.ToLowerInvariant()) {
      $exists = $true
      break
    }
  }

  if (-not $exists) {
    if ($currentPath) {
      $currentPath = "$normalized;$currentPath"
    } else {
      $currentPath = $normalized
    }
  }

  # Keep Path/PATH synchronized for child processes launched by different shells.
  [Environment]::SetEnvironmentVariable("Path", $currentPath, "Process")
  $env:PATH = $currentPath
}

function Clear-GccEnvOverrides {
  $overrideNames = @(
    "GCC_EXEC_PREFIX",
    "COMPILER_PATH",
    "LIBRARY_PATH",
    "CPATH",
    "C_INCLUDE_PATH",
    "CPLUS_INCLUDE_PATH"
  )

  $cleared = @()
  foreach ($name in $overrideNames) {
    $value = [Environment]::GetEnvironmentVariable($name, "Process")
    if ($null -ne $value -and $value.Trim() -ne "") {
      [Environment]::SetEnvironmentVariable($name, $null, "Process")
      $cleared += $name
    }
  }

  if ($cleared.Count -gt 0) {
    Write-Host "[run-theme-demo] cleared GCC env overrides: $($cleared -join ', ')"
  }
}

function Test-GxxToolchain {
  param(
    [Parameter(Mandatory = $true)]
    [string]$GxxPath,
    [Parameter(Mandatory = $true)]
    [string]$Cc1PlusPath
  )

  $probeBase = Join-Path ([System.IO.Path]::GetTempPath()) ("theme-demo-gxx-probe-" + [Guid]::NewGuid().ToString("N"))
  $probeSource = "$probeBase.cpp"
  $probeObject = "$probeBase.o"
  Set-Content -Path $probeSource -Value "int main(){return 0;}" -Encoding ASCII

  try {
    & $GxxPath -x c++ -c $probeSource -o $probeObject
    $probeExitCode = $LASTEXITCODE
    if ($probeExitCode -ne 0) {
      throw "g++ probe failed with exit code $probeExitCode."
    }
  }
  catch {
    $pathHead = ([Environment]::GetEnvironmentVariable("Path", "Process") -split ";" |
      Where-Object { $_ -and $_.Trim() -ne "" } |
      Select-Object -First 6) -join ";"
    throw "g++ cannot execute cc1plus under current environment.`ncompiler: $GxxPath`ncc1plus: $Cc1PlusPath`nPATH(head): $pathHead`nThis is usually caused by broken MinGW installation or conflicting GCC_* environment variables."
  }
  finally {
    Remove-Item -Force -ErrorAction SilentlyContinue $probeSource, $probeObject
  }
}

function Find-QtTools {
  $qtdir = $env:QTDIR
  if (-not $qtdir) {
    $qtdir = "C:\Qt\6.10.2\mingw_64"
  }

  $mingwRoot = $env:MINGW_ROOT
  if (-not $mingwRoot) {
    $mingwRoot = "C:\Qt\Tools\mingw1310_64"
  }

  $qmakePath = Join-Path $qtdir "bin\qmake.exe"
  $makePath = Join-Path $mingwRoot "bin\mingw32-make.exe"
  $gxxPath = Join-Path $mingwRoot "bin\g++.exe"
  $gccPath = Join-Path $mingwRoot "bin\gcc.exe"

  if (-not (Test-Path $qmakePath)) {
    throw "qmake not found: $qmakePath`nSet QTDIR correctly or install Qt MinGW toolchain."
  }

  if (-not (Test-Path $makePath)) {
    throw "mingw32-make not found: $makePath`nSet MINGW_ROOT correctly or install MinGW tools from Qt."
  }

  if (-not (Test-Path $gxxPath)) {
    throw "g++ not found: $gxxPath`nYour MinGW toolchain is incomplete. Reinstall/repair Qt MinGW tools."
  }

  if (-not (Test-Path $gccPath)) {
    throw "gcc not found: $gccPath`nYour MinGW toolchain is incomplete. Reinstall/repair Qt MinGW tools."
  }

  $cc1plusMatches = Get-ChildItem -Path (Join-Path $mingwRoot "libexec\gcc") -Recurse `
    -Filter "cc1plus.exe" -ErrorAction SilentlyContinue

  if (-not $cc1plusMatches -or $cc1plusMatches.Count -eq 0) {
    throw "cc1plus.exe not found under $mingwRoot\libexec\gcc`nThis usually means a broken MinGW install."
  }

  Ensure-PathPrefix (Join-Path $qtdir "bin")
  Ensure-PathPrefix (Join-Path $mingwRoot "bin")
  Clear-GccEnvOverrides
  Test-GxxToolchain -GxxPath $gxxPath -Cc1PlusPath $cc1plusMatches[0].FullName

  return @{
    QMake = $qmakePath
    Make = $makePath
    Gxx = $gxxPath
    Gcc = $gccPath
    Cc1Plus = $cc1plusMatches[0].FullName
  }
}

function Invoke-CheckedTool {
  param(
    [Parameter(Mandatory = $true)]
    [string]$FilePath,
    [Parameter(Mandatory = $true)]
    [string[]]$Arguments,
    [Parameter(Mandatory = $true)]
    [string]$StepName
  )

  & $FilePath @Arguments
  $exitCode = $LASTEXITCODE
  if ($exitCode -ne 0) {
    throw "$StepName failed with exit code $exitCode."
  }
}

$repoRoot = Resolve-RepoRoot
$demoDir = Join-Path $repoRoot "examples\theme-demo"
$buildDir = Join-Path $demoDir "build-mingw"
$proFile = Join-Path $demoDir "theme-demo.pro"
$configLower = $Config.ToLowerInvariant()
$exePath = Join-Path $buildDir "$configLower\theme-demo.exe"
$makefileName = "Makefile.$Config"
$runDetached = $Detached.IsPresent

if (-not (Test-Path $proFile)) {
  throw "Demo project file not found: $proFile"
}

$tools = Find-QtTools

if ($Clean -and (Test-Path $buildDir)) {
  Write-Host "[run-theme-demo] clean build dir: $buildDir"
  Remove-Item -Recurse -Force $buildDir
}

if (-not $NoBuild) {
  New-Item -ItemType Directory -Path $buildDir -Force | Out-Null

  Push-Location $buildDir
  try {
    Write-Host "[run-theme-demo] qmake ($Config)"
    Invoke-CheckedTool -FilePath $tools.QMake `
      -Arguments @("..\theme-demo.pro", "CONFIG+=$configLower") `
      -StepName "qmake"

    if (-not (Test-Path $makefileName)) {
      throw "$makefileName not generated under $buildDir"
    }

    $jobs = [Math]::Max([Environment]::ProcessorCount, 1)
    Write-Host "[run-theme-demo] compiler: $($tools.Gxx)"
    Write-Host "[run-theme-demo] cc1plus: $($tools.Cc1Plus)"
    Write-Host "[run-theme-demo] make -f $makefileName -j$jobs"
    Invoke-CheckedTool -FilePath $tools.Make `
      -Arguments @("-f", $makefileName, "-j$jobs") `
      -StepName "mingw32-make"
  }
  finally {
    Pop-Location
  }
}

if (-not (Test-Path $exePath)) {
  throw "Demo executable not found: $exePath"
}

Write-Host "[run-theme-demo] launch: $exePath"
if ($Wait) {
  $runDetached = $false
}

if ($runDetached) {
  Start-Process -FilePath $exePath | Out-Null
} else {
  & $exePath
}
