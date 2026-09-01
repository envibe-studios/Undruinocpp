# Captures the Windows virtual desktop (all monitors) to a PNG.
# Used to observe the running PIE session, which the editor's viewport-capture
# tool cannot see because it renders the editor world instead.
#
#   .\screen_capture.ps1 -Out _screen.png
#   .\screen_capture.ps1 -Out _screen.png -Scale 0.5

param(
    [Parameter(Mandatory = $true)][string]$Out,
    [double]$Scale = 1.0,
    # Optional sub-rectangle of the virtual desktop, in virtual-screen coordinates.
    [int]$X, [int]$Y, [int]$W, [int]$H
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms, System.Drawing

$bounds = [System.Windows.Forms.SystemInformation]::VirtualScreen
if ($W -gt 0 -and $H -gt 0) {
    $bounds = New-Object System.Drawing.Rectangle $X, $Y, $W, $H
}
$bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
$gfx = [System.Drawing.Graphics]::FromImage($bmp)
$gfx.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
$gfx.Dispose()

$outPath = if ([System.IO.Path]::IsPathRooted($Out)) { $Out } else { Join-Path $PSScriptRoot $Out }

if ($Scale -ne 1.0) {
    $w = [int]($bounds.Width * $Scale)
    $h = [int]($bounds.Height * $Scale)
    $small = New-Object System.Drawing.Bitmap $w, $h
    $g2 = [System.Drawing.Graphics]::FromImage($small)
    $g2.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g2.DrawImage($bmp, 0, 0, $w, $h)
    $g2.Dispose()
    $bmp.Dispose()
    $bmp = $small
}

$bmp.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
"saved: $outPath ($($bmp.Width)x$($bmp.Height)) virtual screen $($bounds.Width)x$($bounds.Height) at $($bounds.X),$($bounds.Y)"
$bmp.Dispose()
