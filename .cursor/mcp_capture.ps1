# Captures the Unreal editor viewport through the unreal-mcp bridge and writes a PNG.
#
#   .\mcp_capture.ps1 -Out shot.png
#   .\mcp_capture.ps1 -Out shot.png -ArgumentsFile _cap.json
#
# Base64 image data never reaches stdout; only the saved path and size are printed.

param(
    [Parameter(Mandatory = $true)][string]$Out,
    [string]$ArgumentsFile,
    [string]$Tool = 'CaptureViewport',
    [string]$Toolset = 'EditorToolset.EditorAppToolset',
    [string]$Url = 'http://127.0.0.1:8000/mcp',
    [int]$TimeoutSec = 300
)

$ErrorActionPreference = 'Stop'

$toolArgs = if ($ArgumentsFile) { [System.IO.File]::ReadAllText((Resolve-Path $ArgumentsFile)) } else { '{}' }

function Send-Raw {
    param([string]$JsonBody, [string]$SessionId)
    $headers = @{ 'Accept' = 'application/json, text/event-stream' }
    if ($SessionId) { $headers['Mcp-Session-Id'] = $SessionId }
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($JsonBody)
    $resp = Invoke-WebRequest -Uri $Url -Method Post -Headers $headers `
        -ContentType 'application/json; charset=utf-8' -Body $bytes -TimeoutSec $TimeoutSec
    $text = $resp.Content
    if ($text -match '(?m)^data:') {
        $text = ($text -split "`n" | Where-Object { $_ -like 'data:*' } |
            ForEach-Object { $_.Substring(5).Trim() }) -join ''
    }
    return @{ Text = $text; Headers = $resp.Headers }
}

$init = Send-Raw -JsonBody '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"cursor-shell","version":"1.0"}}}'
$session = $init.Headers['Mcp-Session-Id']
if ($session -is [array]) { $session = $session[0] }
if ($session) {
    try { Send-Raw -JsonBody '{"jsonrpc":"2.0","method":"notifications/initialized"}' -SessionId $session | Out-Null } catch { }
}

$inner = '{"toolset_name":"' + $Toolset + '","tool_name":"' + $Tool + '","arguments":' + $toolArgs + '}'
$body = '{"jsonrpc":"2.0","id":11,"method":"tools/call","params":{"name":"call_tool","arguments":' + $inner + '}}'

$parsed = (Send-Raw -JsonBody $body -SessionId $session).Text | ConvertFrom-Json
if ($parsed.error) { throw "MCP error: $($parsed.error.message)" }

$b64 = $null
foreach ($block in $parsed.result.content) {
    if ($block.type -eq 'image' -and $block.data) { $b64 = $block.data; break }
}
if (-not $b64) {
    $raw = $parsed.result.content[0].text
    try { $obj = $raw | ConvertFrom-Json }
    catch { throw "Unparseable response: $($raw.Substring(0, [Math]::Min(600, $raw.Length)))" }
    $rv = if ($obj.PSObject.Properties.Name -contains 'returnValue') { $obj.returnValue } else { $obj }
    if ($rv.image -and $rv.image.data) { $b64 = $rv.image.data }
    if ($rv.cameraLocation) {
        "camera: $($rv.cameraLocation | ConvertTo-Json -Compress)"
    }
}
if (-not $b64) { throw 'No image data in response.' }

$outPath = if ([System.IO.Path]::IsPathRooted($Out)) { $Out } else { Join-Path $PSScriptRoot $Out }
[System.IO.File]::WriteAllBytes($outPath, [System.Convert]::FromBase64String($b64))
"saved: $outPath ($([System.IO.FileInfo]::new($outPath).Length) bytes)"
