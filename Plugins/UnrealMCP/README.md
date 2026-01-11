# UnrealMCP Plugin for Unduinocpp

This plugin enables AI clients (Claude Desktop, Cursor, Windsurf) to control Unreal Engine through the Model Context Protocol (MCP).

## Quick Start

### Prerequisites
- Unreal Engine 5.5+ (this project uses 5.7)
- Python 3.12+
- uv package manager (installed at `~/.local/bin/uv`)

### Installation Status
- Plugin installed in: `Plugins/UnrealMCP/`
- Python server at: `Plugins/UnrealMCP/Python/`
- MCP config at: `.cursor/mcp.json` and `~/.config/claude-desktop/mcp.json`

### Running the MCP Server

1. **Start Unreal Editor** with this project (the plugin will auto-load)

2. **Start the MCP Python server**:
   ```bash
   cd /home/developer/project/Plugins/UnrealMCP/Python
   ~/.local/bin/uv run unreal_mcp_server_advanced.py
   ```

3. **Connect your AI client** (Cursor, Claude Desktop, etc.)
   - The MCP configuration is already set up in `.cursor/mcp.json`

### Capabilities

The MCP server provides tools for:
- **Blueprint Visual Scripting** - Create nodes, variables, functions
- **World Building** - Generate towns, castles, mansions, mazes
- **Actor Management** - Spawn, delete, transform actors
- **Physics & Materials** - Apply materials, set physics properties
- **Level Design** - Create walls, pyramids, staircases

### Example Commands

Once connected, you can use natural language commands like:
- "Create a medieval castle with towers"
- "Generate a 10x10 maze"
- "Add a point light at position (100, 200, 300)"
- "Create a Blueprint with a health system"

### Troubleshooting

See `DEBUGGING.md` for common issues and solutions.

### Blueprint Guide

See `Guides/blueprint-graph-guide.md` for detailed Blueprint programming information.

## Source Repository

https://github.com/flopperam/unreal-engine-mcp
