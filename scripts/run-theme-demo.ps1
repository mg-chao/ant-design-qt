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

function Repair-QmakeMakefileShadowMocPaths {
  param(
    [Parameter(Mandatory = $true)]
    [string]$MakefilePath,
    [Parameter(Mandatory = $true)]
    [string]$ConfigLower
  )

  if (-not (Test-Path $MakefilePath)) {
    return
  }

  $configPrefix = "$ConfigLower/"
  $relativePrefixPattern = "\.\./" + [regex]::Escape($ConfigLower) + "/([^\s\\]+\.moc)"
  $targetPattern = "^" + [regex]::Escape($configPrefix) + "(?<name>[^:\s]+\.moc):"
  $lines = [System.IO.File]::ReadAllLines($MakefilePath)
  $updatedLines = New-Object System.Collections.Generic.List[string]
  $currentMocTarget = $null
  $changed = $false

  foreach ($line in $lines) {
    $targetMatch = [regex]::Match($line, $targetPattern)
    if ($targetMatch.Success) {
      $currentMocTarget = $configPrefix + $targetMatch.Groups["name"].Value
    } elseif ([string]::IsNullOrWhiteSpace($line) -or $line -match '^\S[^:]*:') {
      $currentMocTarget = $null
    }

    $updatedLine = [regex]::Replace($line, $relativePrefixPattern, "$configPrefix`$1")
    $trimmedLine = $updatedLine.Trim()

    if ($currentMocTarget -and ($trimmedLine -eq "$currentMocTarget \" -or $trimmedLine -eq $currentMocTarget)) {
      $changed = $true
      continue
    }

    if ($updatedLine -ne $line) {
      $changed = $true
    }

    $updatedLines.Add($updatedLine)
  }

  if (-not $changed) {
    return
  }

  [System.IO.File]::WriteAllLines($MakefilePath, $updatedLines, [System.Text.Encoding]::ASCII)
  Write-Host "[run-theme-demo] patched qmake shadow MOC deps in $MakefilePath"
}

function Build-QmakeProject {
  param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectName,
    [Parameter(Mandatory = $true)]
    [string]$ProFile,
    [Parameter(Mandatory = $true)]
    [string]$BuildDir,
    [Parameter(Mandatory = $true)]
    [string]$ConfigLower,
    [Parameter(Mandatory = $true)]
    [hashtable]$Tools,
    [Parameter(Mandatory = $true)]
    [string]$ConfigName
  )

  New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
  $makefileName = "Makefile.$ConfigName"
  $jobs = [Math]::Max([Environment]::ProcessorCount, 1)

  Push-Location $BuildDir
  try {
    Write-Host "[run-theme-demo] qmake $ProjectName ($ConfigName)"
    Invoke-CheckedTool -FilePath $Tools.QMake `
      -Arguments @($ProFile, "CONFIG+=$ConfigLower") `
      -StepName "$ProjectName qmake"

    if (-not (Test-Path $makefileName)) {
      throw "$makefileName not generated under $BuildDir"
    }

    Repair-QmakeMakefileShadowMocPaths -MakefilePath (Join-Path $BuildDir $makefileName) `
      -ConfigLower $ConfigLower

    Write-Host "[run-theme-demo] compiler: $($Tools.Gxx)"
    Write-Host "[run-theme-demo] cc1plus: $($Tools.Cc1Plus)"
    Write-Host "[run-theme-demo] make $ProjectName -f $makefileName -j$jobs"
    Invoke-CheckedTool -FilePath $Tools.Make `
      -Arguments @("-f", $makefileName, "-j$jobs") `
      -StepName "$ProjectName mingw32-make"
  }
  finally {
    Pop-Location
  }
}

$repoRoot = Resolve-RepoRoot
$iconCoreDir = Join-Path $repoRoot "packages\adqt-icon-core"
$iconCoreBuildDir = Join-Path $iconCoreDir "build-mingw"
$iconCoreProFile = Join-Path $iconCoreDir "adqt-icon-core.pro"
$libraryDir = Join-Path $repoRoot "packages\ant-design-qt"
$libraryBuildDir = Join-Path $libraryDir "build-mingw"
$libraryProFile = Join-Path $libraryDir "ant-design-qt.pro"
$iconsDir = Join-Path $repoRoot "packages\ant-design-icons-qt"
$iconsBuildDir = Join-Path $iconsDir "build-mingw"
$iconsProFile = Join-Path $iconsDir "ant-design-icons-qt.pro"
$demoDir = Join-Path $repoRoot "examples\theme-demo"
$demoBuildDir = Join-Path $demoDir "build-mingw"
$demoProFile = Join-Path $demoDir "theme-demo.pro"
$configLower = $Config.ToLowerInvariant()
$exePath = Join-Path $demoBuildDir "$configLower\theme-demo.exe"
$runDetached = $Detached.IsPresent

if (-not (Test-Path $iconCoreProFile)) {
  throw "Icon core project file not found: $iconCoreProFile"
}

if (-not (Test-Path $libraryProFile)) {
  throw "Library project file not found: $libraryProFile"
}

if (-not (Test-Path $demoProFile)) {
  throw "Demo project file not found: $demoProFile"
}

if (-not (Test-Path $iconsProFile)) {
  throw "Icon project file not found: $iconsProFile"
}

$tools = Find-QtTools

if ($Clean -and (Test-Path $iconCoreBuildDir)) {
  Write-Host "[run-theme-demo] clean icon core build dir: $iconCoreBuildDir"
  Remove-Item -Recurse -Force $iconCoreBuildDir
}

if ($Clean -and (Test-Path $libraryBuildDir)) {
  Write-Host "[run-theme-demo] clean library build dir: $libraryBuildDir"
  Remove-Item -Recurse -Force $libraryBuildDir
}

if ($Clean -and (Test-Path $demoBuildDir)) {
  $runningDemo = Get-Process -Name "theme-demo" -ErrorAction SilentlyContinue
  if ($runningDemo) {
    Write-Host "[run-theme-demo] stop running theme-demo.exe before clean"
    Stop-Process -Name "theme-demo" -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 200
  }
  Write-Host "[run-theme-demo] clean demo build dir: $demoBuildDir"
  Remove-Item -Recurse -Force $demoBuildDir
}

if ($Clean -and (Test-Path $iconsBuildDir)) {
  Write-Host "[run-theme-demo] clean icons build dir: $iconsBuildDir"
  Remove-Item -Recurse -Force $iconsBuildDir
}

if (-not $NoBuild) {
  Build-QmakeProject `
    -ProjectName "adqt-icon-core" `
    -ProFile $iconCoreProFile `
    -BuildDir $iconCoreBuildDir `
    -ConfigLower $configLower `
    -Tools $tools `
    -ConfigName $Config

  Build-QmakeProject `
    -ProjectName "ant-design-icons-qt" `
    -ProFile $iconsProFile `
    -BuildDir $iconsBuildDir `
    -ConfigLower $configLower `
    -Tools $tools `
    -ConfigName $Config

  Build-QmakeProject `
    -ProjectName "ant-design-qt" `
    -ProFile $libraryProFile `
    -BuildDir $libraryBuildDir `
    -ConfigLower $configLower `
    -Tools $tools `
    -ConfigName $Config

  Build-QmakeProject `
    -ProjectName "theme-demo" `
    -ProFile $demoProFile `
    -BuildDir $demoBuildDir `
    -ConfigLower $configLower `
    -Tools $tools `
    -ConfigName $Config
}

if (-not (Test-Path $exePath)) {
  throw "Demo executable not found: $exePath"
}

if (Get-Process -Name "theme-demo" -ErrorAction SilentlyContinue) {
  Write-Host "[run-theme-demo] stop previous theme-demo.exe before launch"
  Stop-Process -Name "theme-demo" -Force -ErrorAction SilentlyContinue
  Start-Sleep -Milliseconds 200
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
