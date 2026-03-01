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
  $segments = $env:PATH -split ";" | Where-Object { $_ -and $_.Trim() -ne "" }
  $exists = $false
  foreach ($seg in $segments) {
    if ($seg.TrimEnd("\").ToLowerInvariant() -eq $normalized.ToLowerInvariant()) {
      $exists = $true
      break
    }
  }

  if (-not $exists) {
    $env:PATH = "$normalized;$env:PATH"
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

  if (-not (Test-Path $qmakePath)) {
    throw "qmake not found: $qmakePath`nSet QTDIR correctly or install Qt MinGW toolchain."
  }

  if (-not (Test-Path $makePath)) {
    throw "mingw32-make not found: $makePath`nSet MINGW_ROOT correctly or install MinGW tools from Qt."
  }

  Ensure-PathPrefix (Join-Path $qtdir "bin")
  Ensure-PathPrefix (Join-Path $mingwRoot "bin")

  return @{
    QMake = $qmakePath
    Make = $makePath
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
    & $tools.QMake "..\theme-demo.pro" "CONFIG+=$configLower"

    if (-not (Test-Path $makefileName)) {
      throw "$makefileName not generated under $buildDir"
    }

    $jobs = [Math]::Max([Environment]::ProcessorCount, 1)
    Write-Host "[run-theme-demo] make -f $makefileName -j$jobs"
    & $tools.Make "-f" $makefileName "-j$jobs"
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
