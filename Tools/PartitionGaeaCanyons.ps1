<#
.SYNOPSIS
    Convert Content/Level/GaeaCanyons.umap to a World Partition level and (optionally) build HLODs.

.DESCRIPTION
    Runs the Unreal Editor commandlets headlessly so the editor never has to load
    the full 840 MB map into memory at once. All output is teed to a timestamped
    log file under Saved/Logs/.

    Recommended order:
        1) .\Tools\PartitionGaeaCanyons.ps1                       # Convert (safe, creates GaeaCanyons_WP)
        2)  -- open editor, set up Landscape grid, save --
        3) .\Tools\PartitionGaeaCanyons.ps1 -BuildHLODs           # Build HLODs on GaeaCanyons_WP
        4) .\Tools\PartitionGaeaCanyons.ps1 -Finalize             # Replace original with _WP

.PARAMETER UeRoot
    Root of the Unreal Engine 5.7 install. Auto-detected by default.

.PARAMETER MapName
    Map name without extension. Defaults to GaeaCanyons.

.PARAMETER BuildHLODs
    Skip conversion and only build HLODs on <MapName>_WP.

.PARAMETER Finalize
    Skip conversion and only swap <MapName>_WP into <MapName>.
    Original is preserved as <MapName>.PRECONVERT.umap.bak.

.PARAMETER SkipBackup
    Skip the initial .bak copy of the original .umap. Not recommended.

.PARAMETER LowMemory
    Use -nullrhi and limit shader compile workers (recommended on 16 GB RAM).
    REQUIRED for GaeaCanyons if conversion OOMs while building LandscapeNaniteMesh_*.
#>

[CmdletBinding()]
param(
    [string]$UeRoot   = 'D:\UnrealEngines\UE_5.7',
    [string]$MapName  = 'GaeaCanyons',
    [switch]$BuildHLODs,
    [switch]$Finalize,
    [switch]$SkipBackup,
    [switch]$LowMemory
)

$ErrorActionPreference = 'Stop'

# ---- paths -----------------------------------------------------------------
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Uproject    = Join-Path $ProjectRoot 'Unduinocpp.uproject'
$LevelDir    = Join-Path $ProjectRoot 'Content\Level'
$OrigUmap    = Join-Path $LevelDir   "$MapName.umap"
$WpUmap      = Join-Path $LevelDir   "${MapName}_WP.umap"
$BackupUmap  = Join-Path $LevelDir   "$MapName.PRECONVERT.umap.bak"
$LogsDir     = Join-Path $ProjectRoot 'Saved\Logs'
$Stamp       = Get-Date -Format 'yyyyMMdd_HHmmss'
$LogFile     = Join-Path $LogsDir "PartitionGaeaCanyons_${Stamp}.log"

$UeCmd       = Join-Path $UeRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'

# ---- preflight -------------------------------------------------------------
if (-not (Test-Path $UeCmd))     { throw "UnrealEditor-Cmd.exe not found at: $UeCmd. Pass -UeRoot <path>." }
if (-not (Test-Path $Uproject))  { throw ".uproject not found at: $Uproject" }
if (-not (Test-Path $LogsDir))   { New-Item -ItemType Directory -Force -Path $LogsDir | Out-Null }

function Write-Step { param([string]$Msg) Write-Host "`n=== $Msg ===" -ForegroundColor Cyan }

function Invoke-UeCmd {
    param([string[]]$Args, [string]$StageName)
    Write-Step "$StageName"
    Write-Host "CMD : $UeCmd $($Args -join ' ')" -ForegroundColor DarkGray
    Write-Host "LOG : $LogFile" -ForegroundColor DarkGray

    # Native programs (UE / embedded Chromium) write benign messages to stderr.
    # With $ErrorActionPreference='Stop' + 2>&1, PowerShell would treat those
    # as terminating errors and kill the pipeline before the commandlet runs.
    # Relax it for just this call and rely on the real exit code instead.
    $prevEAP = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $exitCode = 1
    try {
        & $UeCmd @Args 2>&1 | Tee-Object -FilePath $LogFile -Append
        $exitCode = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $prevEAP
    }

    if ($exitCode -ne 0) { throw "$StageName failed with exit code $exitCode. See $LogFile" }
}

# ---- mode: -Finalize -------------------------------------------------------
if ($Finalize) {
    Write-Step "FINALIZE: replace $MapName with ${MapName}_WP"
    if (-not (Test-Path $WpUmap)) { throw "Expected $WpUmap to exist. Run conversion first." }

    if (Test-Path $OrigUmap) {
        Write-Host "Renaming original -> $BackupUmap"
        if (Test-Path $BackupUmap) { Remove-Item $BackupUmap -Force }
        Rename-Item -Path $OrigUmap -NewName (Split-Path $BackupUmap -Leaf)
    }

    Write-Host "Renaming ${MapName}_WP.umap -> $MapName.umap"
    Rename-Item -Path $WpUmap -NewName "$MapName.umap"

    # External actor / object folders use the map name -> rename them too.
    $extActorsOld = Join-Path $ProjectRoot "Content\__ExternalActors__\Level\${MapName}_WP"
    $extActorsNew = Join-Path $ProjectRoot "Content\__ExternalActors__\Level\$MapName"
    $extObjsOld   = Join-Path $ProjectRoot "Content\__ExternalObjects__\Level\${MapName}_WP"
    $extObjsNew   = Join-Path $ProjectRoot "Content\__ExternalObjects__\Level\$MapName"
    foreach ($pair in @(@{Old=$extActorsOld; New=$extActorsNew}, @{Old=$extObjsOld; New=$extObjsNew})) {
        if (Test-Path $pair.Old) {
            if (Test-Path $pair.New) { Remove-Item $pair.New -Recurse -Force }
            Write-Host "Renaming $($pair.Old) -> $($pair.New)"
            Rename-Item -Path $pair.Old -NewName (Split-Path $pair.New -Leaf)
        }
    }

    Write-Host "`nDone. Reopen the editor to see GaeaCanyons as a World Partition level." -ForegroundColor Green
    return
}

# ---- mode: -BuildHLODs -----------------------------------------------------
if ($BuildHLODs) {
    $TargetMap = if (Test-Path $WpUmap) { "${MapName}_WP" } else { $MapName }
    Invoke-UeCmd -StageName "Build HLODs on /Game/Level/$TargetMap" -Args @(
        "`"$Uproject`""
        '-run=WorldPartitionBuilderCommandlet'
        "/Game/Level/$TargetMap"
        '-Builder=WorldPartitionHLODsBuilder'
        '-AllowCommandletRendering'
        '-SCCProvider=None'
        '-unattended'
        '-nop4'
    )
    Write-Host "`nHLODs built. Open the level and verify in the World Partition Editor." -ForegroundColor Green
    return
}

# ---- mode: default = CONVERT ----------------------------------------------
if (-not (Test-Path $OrigUmap)) { throw "Source map not found: $OrigUmap" }

if (-not $SkipBackup) {
    if (-not (Test-Path $BackupUmap)) {
        Write-Step "Backing up original -> $BackupUmap"
        Copy-Item $OrigUmap $BackupUmap
    } else {
        Write-Host "Backup already exists at $BackupUmap (skipping copy)" -ForegroundColor Yellow
    }
}

if (Test-Path $WpUmap) {
    Write-Host "WARNING: $WpUmap already exists. Delete it before re-running, or this commandlet may fail." -ForegroundColor Yellow
}

$convertArgs = @(
    "`"$Uproject`""
    '-run=WorldPartitionConvertCommandlet'
    "/Game/Level/$MapName"
    '-ConversionSuffix'
    '-SCCProvider=None'
    '-unattended'
    '-nop4'
    '-nosound'
)
# Editor UI passes -AllowCommandletRendering which spawns 13 shader workers and can OOM
# on 16 GB RAM. Don't use that flag here. We also CANNOT use -nullrhi (WorldPartition
# convert needs the RHI for landscape work and will hang silently without it).
# Instead, cap shader workers + Nanite memory via DPCVars + raise GC timing.
if ($LowMemory) {
    Write-Host 'Low-memory mode: cap shader workers to 2, raise -StallDelay, force GC pressure. Landscape Nanite MUST be disabled first.' -ForegroundColor Yellow
    $convertArgs += '-DPCVars=r.ShaderCompiler.NumLocalWorkers=2,r.ShaderCompiler.MaxShaderJobBatchSize=1,gc.TimeBetweenPurgingPendingKillObjects=10,r.Nanite.MaxNaniteMeshBuildMemoryMB=1024'
    $convertArgs += '-MaxShaderJobs=2'
} else {
    Write-Host 'Tip: on 16 GB RAM, re-run with -LowMemory after disabling Landscape Nanite.' -ForegroundColor DarkYellow
}

Invoke-UeCmd -StageName "Convert /Game/Level/$MapName to World Partition (suffix _WP)" -Args $convertArgs

Write-Host "`nConversion complete." -ForegroundColor Green
Write-Host "Next steps:"
Write-Host "  1. Open the editor and load /Game/Level/${MapName}_WP."
Write-Host "  2. Select the Landscape actor and set its World Partition Grid Size, then run"
Write-Host "     Build > World Partition > Build Landscape Streaming Proxies."
Write-Host "  3. Save All."
Write-Host "  4. Run:  .\Tools\PartitionGaeaCanyons.ps1 -BuildHLODs"
Write-Host "  5. Run:  .\Tools\PartitionGaeaCanyons.ps1 -Finalize"
