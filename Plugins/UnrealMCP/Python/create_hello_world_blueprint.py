#!/usr/bin/env python3
"""
Create Hello World Blueprint via UnrealMCP

This script demonstrates how Claude can create Blueprint files in Unreal Engine
by communicating with the UnrealMCP server. It creates an Actor Blueprint with
a BeginPlay event that prints "Hello World" to the screen.

Usage:
    python create_hello_world_blueprint.py

Requirements:
    - Unreal Editor must be running with the UnrealMCP plugin enabled
    - The MCP server starts automatically when the editor loads
"""

import socket
import json
import time
import sys
from typing import Dict, Any, Optional


# Configuration
UNREAL_HOST = "127.0.0.1"
UNREAL_PORT = 55557
CONNECT_TIMEOUT = 10
RECV_TIMEOUT = 60  # Increased timeout for complex operations
MAX_RETRIES = 3


class UnrealMCPClient:
    """Simple client for communicating with UnrealMCP server."""

    def __init__(self, host: str = UNREAL_HOST, port: int = UNREAL_PORT):
        self.host = host
        self.port = port
        self.socket: Optional[socket.socket] = None

    def connect(self) -> bool:
        """Connect to the UnrealMCP server."""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(CONNECT_TIMEOUT)
            self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            self.socket.connect((self.host, self.port))
            print(f"Connected to UnrealMCP at {self.host}:{self.port}")
            return True
        except Exception as e:
            print(f"Failed to connect: {e}")
            return False

    def disconnect(self):
        """Disconnect from the server."""
        if self.socket:
            try:
                self.socket.shutdown(socket.SHUT_RDWR)
            except:
                pass
            try:
                self.socket.close()
            except:
                pass
            self.socket = None

    def send_command(self, command_type: str, params: Dict[str, Any] = None) -> Dict[str, Any]:
        """Send a command and receive the response with retry logic."""
        last_error = None

        for attempt in range(MAX_RETRIES):
            try:
                result = self._send_command_once(command_type, params)
                return result
            except Exception as e:
                last_error = str(e)
                print(f"  Attempt {attempt + 1}/{MAX_RETRIES} failed: {e}")
                if attempt < MAX_RETRIES - 1:
                    delay = 0.5 * (2 ** attempt)  # Exponential backoff
                    print(f"  Retrying in {delay:.1f}s...")
                    time.sleep(delay)

        return {"status": "error", "error": f"Failed after {MAX_RETRIES} attempts: {last_error}"}

    def _send_command_once(self, command_type: str, params: Dict[str, Any] = None) -> Dict[str, Any]:
        """Send a command and receive the response (single attempt)."""
        if not self.connect():
            raise ConnectionError("Failed to connect")

        try:
            # Build command
            command = {
                "type": command_type,
                "params": params or {}
            }
            command_json = json.dumps(command)

            print(f"Sending: {command_type}")

            # Send
            self.socket.settimeout(10)
            self.socket.sendall(command_json.encode('utf-8'))

            # Receive
            self.socket.settimeout(RECV_TIMEOUT)
            chunks = []
            while True:
                try:
                    chunk = self.socket.recv(8192)
                    if not chunk:
                        break
                    chunks.append(chunk)

                    # Try to parse as complete JSON
                    data = b''.join(chunks)
                    try:
                        response = json.loads(data.decode('utf-8'))
                        return response
                    except json.JSONDecodeError:
                        continue
                except socket.timeout:
                    if chunks:
                        data = b''.join(chunks)
                        try:
                            return json.loads(data.decode('utf-8'))
                        except:
                            pass
                    raise

            if chunks:
                data = b''.join(chunks)
                return json.loads(data.decode('utf-8'))

            raise ConnectionError("No response received")

        finally:
            self.disconnect()


def create_hello_world_blueprint():
    """
    Create a Hello World Blueprint with BeginPlay event printing a message.

    Steps:
    1. Create a new Actor Blueprint called "BP_HelloWorld"
    2. Add a BeginPlay event node
    3. Add a Print String node with "Hello World" message
    4. Connect BeginPlay to Print String
    5. Compile the Blueprint
    6. Save all assets to disk
    """
    client = UnrealMCPClient()

    blueprint_name = "BP_HelloWorld"

    print("=" * 60)
    print("Creating Hello World Blueprint")
    print("=" * 60)

    # Step 1: Create the Blueprint in Content/Blueprints folder
    print("\n[Step 1] Creating Blueprint...")
    result = client.send_command("create_blueprint", {
        "name": blueprint_name,
        "parent_class": "Actor",
        "folder_path": "/Game/Blueprints/"
    })

    if result.get("success") == False:
        # Check if blueprint already exists
        if "already exists" in result.get("error", ""):
            print(f"Blueprint {blueprint_name} already exists, continuing...")
        else:
            print(f"Error creating blueprint: {result.get('error')}")
            return False
    else:
        print(f"Blueprint created: {result}")

    # Step 2: Add BeginPlay event node
    print("\n[Step 2] Adding BeginPlay event node...")
    result = client.send_command("add_event_node", {
        "blueprint_name": blueprint_name,
        "event_name": "ReceiveBeginPlay",
        "pos_x": 0,
        "pos_y": 0
    })

    if result.get("success") == False:
        print(f"Error adding event node: {result.get('error')}")
        # Continue anyway - might already exist
    else:
        print(f"Event node added: {result}")

    # Get the event node ID from the result (directly in the response, not nested in 'result')
    event_node_id = result.get("node_id")
    print(f"  Event node ID: {event_node_id}")

    # Step 3: Add Print String node
    print("\n[Step 3] Adding Print String node...")
    result = client.send_command("add_blueprint_node", {
        "blueprint_name": blueprint_name,
        "node_type": "Print",
        "node_params": {
            "pos_x": 300,
            "pos_y": 0,
            "message": "Hello World"
        }
    })

    if result.get("success") == False:
        print(f"Error adding print node: {result.get('error')}")
        return False
    else:
        print(f"Print node added: {result}")

    # Get the print node ID (directly in the response, not nested in 'result')
    print_node_id = result.get("node_id")
    print(f"  Print node ID: {print_node_id}")

    # Step 4: Connect the nodes
    if event_node_id and print_node_id:
        print("\n[Step 4] Connecting BeginPlay to Print String...")
        result = client.send_command("connect_nodes", {
            "blueprint_name": blueprint_name,
            "source_node_id": event_node_id,
            "source_pin_name": "then",  # Execution output pin from BeginPlay
            "target_node_id": print_node_id,
            "target_pin_name": "execute"  # Execution input pin
        })

        if result.get("success") == False:
            print(f"Error connecting nodes: {result.get('error')}")
            # Try alternative pin names
            print("Trying alternative pin names...")
            result = client.send_command("connect_nodes", {
                "blueprint_name": blueprint_name,
                "source_node_id": event_node_id,
                "source_pin_name": "Then",
                "target_node_id": print_node_id,
                "target_pin_name": "Execute"
            })
            if result.get("success") == False:
                print(f"Still failed: {result.get('error')}")
        else:
            print(f"Nodes connected: {result}")
    else:
        print("\n[Step 4] Skipping connection - missing node IDs")
        print(f"  Event node ID: {event_node_id}")
        print(f"  Print node ID: {print_node_id}")

    # Step 5: Compile the Blueprint
    print("\n[Step 5] Compiling Blueprint...")
    result = client.send_command("compile_blueprint", {
        "blueprint_name": blueprint_name
    })

    if result.get("success") == False:
        print(f"Error compiling: {result.get('error')}")
    else:
        print(f"Blueprint compiled: {result}")

    # Step 6: Save all modified assets to disk
    print("\n[Step 6] Saving all assets to disk...")
    result = client.send_command("save_all", {})

    if result.get("success") == False:
        print(f"Error saving: {result.get('error')}")
    else:
        print(f"Assets saved: {result}")

    print("\n" + "=" * 60)
    print("Blueprint creation complete!")
    print(f"Location: /Game/Blueprints/{blueprint_name}")
    print("=" * 60)

    return True


def test_connection():
    """Test the connection to UnrealMCP with a ping command."""
    print("Testing connection to UnrealMCP...")
    client = UnrealMCPClient()
    result = client.send_command("ping", {})

    if result.get("success") == True or result.get("message") == "pong":
        print("Connection successful! Unreal Editor is responding.")
        return True
    else:
        print(f"Connection failed: {result.get('error', 'Unknown error')}")
        print("\nMake sure:")
        print("  1. Unreal Editor is running")
        print("  2. The UnrealMCP plugin is enabled")
        print("  3. The MCP server is listening on port 55557")
        return False


if __name__ == "__main__":
    print("=" * 60)
    print("Claude Blueprint Creator - Hello World Demo")
    print("=" * 60)
    print()

    # Test connection first
    if not test_connection():
        print("\nExiting due to connection failure.")
        sys.exit(1)

    print()

    # Create the blueprint
    success = create_hello_world_blueprint()

    if success:
        print("\nSuccess! You can now:")
        print("  1. Open the Blueprint in Unreal Editor")
        print("  2. Place it in your level")
        print("  3. Play the level to see 'Hello World' printed on screen")
    else:
        print("\nBlueprint creation encountered errors.")
        sys.exit(1)
