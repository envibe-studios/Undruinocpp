# Runs a batch of unreal-mcp tool calls over a single session.
#
# Input: a JSON file containing an array of calls:
#   [ { "label": "...", "toolset": "...", "tool": "...", "args": { ... } }, ... ]
# Omit "toolset" to call a top-level MCP tool.
#
#   .\mcp_batch.ps1 -File batch.json
#
# Prints "== label ==" followed by the unwrapped returnValue payload for each
# call, so results stay readable instead of drowning in JSON-RPC envelopes.
# A failed call prints its error and, unless -ContinueOnError, stops the batch.

param(
    [Parameter(Mandatory = $true)][string]$File,
    [string]$Url = 'http://127.0.0.1:8000/mcp',
    [int]$TimeoutSec = 300,
    [switch]$ContinueOnError
)

$ErrorActionPreference = 'Stop'

$calls = [System.IO.File]::ReadAllText((Resolve-Path $File)) | ConvertFrom-Json
if ($calls -isnot [array]) { $calls = @($calls) }

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

# --- One handshake for the whole batch ---
$init = Send-Raw -JsonBody '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"cursor-shell","version":"1.0"}}}'
$session = $init.Headers['Mcp-Session-Id']
if ($session -is [array]) { $session = $session[0] }
if ($session) {
    try { Send-Raw -JsonBody '{"jsonrpc":"2.0","method":"notifications/initialized"}' -SessionId $session | Out-Null } catch { }
}

$id = 10
foreach ($call in $calls) {
    $id++
    $label = if ($call.label) { $call.label } else { $call.tool }

    $inner = @{ tool_name = $call.tool }
    if ($call.toolset) { $inner['toolset_name'] = $call.toolset }
    $inner['arguments'] = if ($null -ne $call.args) { $call.args } else { @{} }

    $params = @{ name = 'call_tool'; arguments = $inner }
    $body = @{ jsonrpc = '2.0'; id = $id; method = 'tools/call'; params = $params } |
        ConvertTo-Json -Depth 40 -Compress

    "== $label =="
    try {
        $parsed = (Send-Raw -JsonBody $body -SessionId $session).Text | ConvertFrom-Json

        if ($parsed.error) {
            "ERROR: $($parsed.error.message)"
            if (-not $ContinueOnError) { throw "Batch stopped at '$label'" }
        }
        else {
            $inner = $parsed.result.content[0].text
            # Tool results arrive as JSON embedded in a text block; unwrap the
            # single-key {"returnValue": ...} envelope when present.
            try {
                $obj = $inner | ConvertFrom-Json
                if ($obj.PSObject.Properties.Name -contains 'returnValue') {
                    $obj.returnValue | ConvertTo-Json -Depth 40 -Compress
                }
                else { $inner }
            }
            catch { $inner }

            if ($parsed.result.isError) { "(tool reported isError)" }
        }
    }
    catch {
        "EXCEPTION: $($_.Exception.Message)"
        if (-not $ContinueOnError) { throw }
    }
    ''
}
