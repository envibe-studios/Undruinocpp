# Convert grayscale PNG heightmap to OBJ terrain mesh for GrayboxExpedition.
param(
    [string]$PngPath = "C:\Users\mattes\.cursor\projects\d-Unreals-Undruinocpp\assets\c__Users_mattes_AppData_Roaming_Cursor_User_workspaceStorage_empty-window_images_image-93c0fac0-7d59-4528-bc00-9b29208be099.png",
    [string]$OutDir = "d:\Unreals\Undruinocpp\Content\Level\Graybox",
    [int]$SampleStep = 2,
    [double]$XMin = -100000,
    [double]$XMax = 100000,
    [double]$YMin = -10000,
    [double]$YMax = 160000,
    [double]$ZMin = 0,
    [double]$ZMax = 4500,
    [double]$RoadBias = 0.42
)

Add-Type -AssemblyName System.Drawing
$img = [System.Drawing.Bitmap]::FromFile($PngPath)
$w = $img.Width
$h = $img.Height
Write-Host "Image ${w}x${h}"

# Sample grid
$xs = [Math]::Floor(($w - 1) / $SampleStep) + 1
$ys = [Math]::Floor(($h - 1) / $SampleStep) + 1

# Collect normalized heights (0..1), PNG Y=0 is top of image -> world +Y (base)
$heights = New-Object 'double[,]' $ys, $xs
$sum = 0.0
$count = 0
for ($j = 0; $j -lt $ys; $j++) {
    $py = [Math]::Min($j * $SampleStep, $h - 1)
    for ($i = 0; $i -lt $xs; $i++) {
        $px = [Math]::Min($i * $SampleStep, $w - 1)
        $c = $img.GetPixel($px, $py)
        # luminance
        $n = ($c.R + $c.G + $c.B) / (3.0 * 255.0)
        $heights[$j, $i] = $n
        $sum += $n
        $count++
    }
}
$img.Dispose()
$mean = $sum / $count
Write-Host "Samples ${xs}x${ys} meanHeight=$([Math]::Round($mean,3))"

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$objPath = Join-Path $OutDir "SM_GrayboxHeightmapTerrain.obj"
$sw = New-Object System.IO.StreamWriter($objPath, $false, [System.Text.UTF8Encoding]::new($false))
$sw.WriteLine("# Graybox heightmap terrain from user PNG")
$sw.WriteLine("# grid $xs x $ys stepPx=$SampleStep worldX=[$XMin,$XMax] worldY=[$YMin,$YMax] z=[$ZMin,$ZMax]")

# Vertices: image row j=0 (top) -> world Y = YMax.
# UE OBJ import flips Y, so bake negated Y into the file and place at scale (1,1,1).
for ($j = 0; $j -lt $ys; $j++) {
    $v = if ($ys -le 1) { 0.0 } else { $j / ($ys - 1.0) }
    $yWorld = $YMax + ($YMin - $YMax) * $v   # j=0 -> YMax, j=max -> YMin
    $y = -$yWorld
    for ($i = 0; $i -lt $xs; $i++) {
        $u = if ($xs -le 1) { 0.0 } else { $i / ($xs - 1.0) }
        $x = $XMin + ($XMax - $XMin) * $u
        $n = $heights[$j, $i]
        $z = $ZMin + $n * ($ZMax - $ZMin)
        $sw.WriteLine(("v {0:F3} {1:F3} {2:F3}" -f $x, $y, $z))
    }
}

# Winding chosen so after UE's Y flip, normals face +Z (up).
for ($j = 0; $j -lt ($ys - 1); $j++) {
    for ($i = 0; $i -lt ($xs - 1); $i++) {
        $i0 = $j * $xs + $i + 1
        $i1 = $i0 + 1
        $i2 = $i0 + $xs
        $i3 = $i2 + 1
        $sw.WriteLine("f $i0 $i3 $i1")
        $sw.WriteLine("f $i0 $i2 $i3")
    }
}
$sw.Close()

$meta = @{
    xs = $xs
    ys = $ys
    sample_step = $SampleStep
    image_w = $w
    image_h = $h
    mean = $mean
    x_min = $XMin; x_max = $XMax
    y_min = $YMin; y_max = $YMax
    z_min = $ZMin; z_max = $ZMax
    obj = $objPath
} | ConvertTo-Json
$metaPath = Join-Path $OutDir "SM_GrayboxHeightmapTerrain.json"
Set-Content -Path $metaPath -Value $meta -Encoding UTF8
Write-Host "Wrote $objPath"
Write-Host "Wrote $metaPath"
