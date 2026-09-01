# Prints just the tool names (and optionally arg names) of an MCP toolset,
# so agents can discover an API without pulling the full multi-hundred-KB schema.
#
#   .\mcp_tools.ps1 editor_toolset.toolsets.actor.ActorTools
#   .\mcp_tools.ps1 editor_toolset.toolsets.actor.ActorTools -Filter component -ShowArgs

param(
    [Parameter(Mandatory = $true)][string]$Toolset,
    [string]$Filter = '',
    [switch]$ShowArgs
)

$ErrorActionPreference = 'Stop'

$raw = & (Join-Path $PSScriptRoot 'mcp_call.ps1') -ToolName describe_toolset `
    -Arguments ('{"toolset_name":"' + $Toolset + '"}')

$payload = ($raw | ConvertFrom-Json).result.content[0].text | ConvertFrom-Json

foreach ($tool in $payload.tools) {
    $short = $tool.name -replace [regex]::Escape($Toolset + '.'), ''
    if ($Filter -and $short -notlike "*$Filter*") { continue }

    if ($ShowArgs) {
        $props = @()
        if ($tool.inputSchema.properties) {
            $props = $tool.inputSchema.properties.PSObject.Properties.Name
        }
        $req = @()
        if ($tool.inputSchema.required) { $req = $tool.inputSchema.required }

        $sig = ($props | ForEach-Object { if ($req -contains $_) { $_ } else { "[$_]" } }) -join ', '
        "$short($sig)"
    }
    else {
        $short
    }
}
