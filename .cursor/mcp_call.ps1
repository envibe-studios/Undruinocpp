# Minimal MCP (streamable HTTP) client for the unreal-mcp bridge.
# Used by AI agents when the bridge is not registered as a native tool.
#
#   .\mcp_call.ps1 -Method tools/list
#   .\mcp_call.ps1 -ToolName list_toolsets
#   .\mcp_call.ps1 -ToolName call_tool -Arguments '{"toolset_name":"actor","tool_name":"get_actors_in_level","arguments":{}}'
#
# Arguments is passed through as raw JSON so it works on Windows PowerShell 5.1
# (which lacks ConvertFrom-Json -AsHashtable).

param(
    [string]$Method = 'tools/call',
    [string]$ToolName,
    [string]$Arguments = '{}',
    # Path to a UTF-8 JSON file holding the arguments object. Preferred for any
    # payload with nested quotes, since shell quoting mangles those.
    [string]$ArgumentsFile,
    [string]$Url = 'http://127.0.0.1:8000/mcp',
    [int]$TimeoutSec = 300
)

$ErrorActionPreference = 'Stop'
$sessionFile = Join-Path $PSScriptRoot '_mcp_session.txt'

if ($ArgumentsFile) {
    $Arguments = [System.IO.File]::ReadAllText((Resolve-Path $ArgumentsFile))
}

function Invoke-Mcp {
    param([string]$JsonBody, [string]$SessionId)

    $headers = @{ 'Accept' = 'application/json, text/event-stream' }
    if ($SessionId) { $headers['Mcp-Session-Id'] = $SessionId }

    # Send as UTF-8 bytes so non-ASCII payloads survive the round trip.
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($JsonBody)
    return Invoke-WebRequest -Uri $Url -Method Post -Headers $headers `
        -ContentType 'application/json; charset=utf-8' -Body $bytes -TimeoutSec $TimeoutSec
}

# --- Handshake: obtain a session id ---
$initBody = '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"cursor-shell","version":"1.0"}}}'
$initResp = Invoke-Mcp -JsonBody $initBody

$session = $initResp.Headers['Mcp-Session-Id']
if ($session -is [array]) { $session = $session[0] }

if ($session) {
    Set-Content -Path $sessionFile -Value $session -NoNewline
    # Streamable HTTP requires notifications/initialized before normal requests.
    try {
        Invoke-Mcp -JsonBody '{"jsonrpc":"2.0","method":"notifications/initialized"}' -SessionId $session | Out-Null
    }
    catch { }
}

# --- Actual request ---
if ($Method -eq 'tools/call') {
    if (-not $ToolName) { throw 'ToolName is required for tools/call' }
    $params = '{"name":"' + $ToolName + '","arguments":' + $Arguments + '}'
}
else {
    $params = '{}'
}

$body = '{"jsonrpc":"2.0","id":2,"method":"' + $Method + '","params":' + $params + '}'

$resp = Invoke-Mcp -JsonBody $body -SessionId $session
$text = $resp.Content

# Streamable HTTP may answer as SSE; strip the event framing to recover the JSON.
if ($text -match '(?m)^data:') {
    $text = ($text -split "`n" | Where-Object { $_ -like 'data:*' } | ForEach-Object { $_.Substring(5).Trim() }) -join ''
}

$text
