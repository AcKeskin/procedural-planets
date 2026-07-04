#!/usr/bin/env python3
"""MCP server that drives a running procedural-planets app over its --live TCP/JSON socket.

The planet app (launched with `--live [port]`) listens on 127.0.0.1:<port> and accepts one
JSON command per line, replying with one line. This server is a thin bridge: each MCP tool
builds a command dict, sends it, and returns the app's reply. All rendering/state lives in
the app on its render thread; this process only forwards.

Connection is lazy and sticky: the first tool call opens the socket, later calls reuse it,
and a dropped connection is transparently reopened on the next call.
"""

import json
import os
import socket
import threading
from typing import Any

from mcp.server.fastmcp import FastMCP

mcp = FastMCP("planet-driver")

# The app's --live port; override with PLANET_LIVE_PORT. Host is always loopback.
_HOST = "127.0.0.1"
_PORT = int(os.environ.get("PLANET_LIVE_PORT", "8787"))

_lock = threading.Lock()
_conn: socket.socket | None = None
_rx = b""  # leftover bytes between line reads


def _connect() -> socket.socket:
    s = socket.create_connection((_HOST, _PORT), timeout=30)
    s.settimeout(60)
    return s


def _send(cmd: dict[str, Any]) -> str:
    """Send one command line and return the app's single reply line.

    Serialized under a lock so concurrent tool calls can't interleave on the one socket.
    Reconnects once if the existing connection is dead.
    """
    global _conn, _rx
    line = (json.dumps(cmd) + "\n").encode()

    with _lock:
        for attempt in range(2):
            try:
                if _conn is None:
                    _conn = _connect()
                    _rx = b""
                _conn.sendall(line)
                # read until newline
                while b"\n" not in _rx:
                    chunk = _conn.recv(4096)
                    if not chunk:
                        raise ConnectionError("app closed the connection")
                    _rx += chunk
                reply, _, _rx = _rx.partition(b"\n")
                return reply.decode(errors="replace").rstrip("\r")
            except (OSError, ConnectionError) as e:
                # drop the dead socket and retry once with a fresh one
                if _conn is not None:
                    try:
                        _conn.close()
                    except OSError:
                        pass
                _conn = None
                _rx = b""
                if attempt == 1:
                    return f"error: cannot reach planet app on {_HOST}:{_PORT} ({e}). " \
                           f"Is it running with --live {_PORT}?"
    return "error: unreachable"


def _vec3(x: float, y: float, z: float) -> list[float]:
    return [x, y, z]


@mcp.tool()
def move_camera(
    mode: str,
    distance: float = 0.0,
    distance_radii: float = 0.0,
    yaw: float = -90.0,
    pitch: float = 0.0,
    eye_x: float = 0.0,
    eye_y: float = 0.0,
    eye_z: float = 0.0,
    target_x: float = 0.0,
    target_y: float = 0.0,
    target_z: float = 0.0,
) -> str:
    """Position the camera on the running planet.

    mode:
      "orbit"   — orbit the target at `distance` world units (or `distance_radii` planet
                  radii, which wins when >0), seated at yaw/pitch (degrees). Prefer
                  `distance_radii`: ~2.5 frames the whole disc on ANY body regardless of its
                  radius; absolute `distance` only frames the body it was tuned for.
      "lookat"  — free-fly: place the eye at (eye_x,eye_y,eye_z), look at (target_x,..).
      "freefly" — place the eye and aim by yaw/pitch (degrees) instead of a target point.
    Returns the app's reply line.
    """
    m = mode.lower()
    if m == "orbit":
        cmd: dict[str, Any] = {
            "cmd": "camera", "mode": "orbit",
            "target": _vec3(target_x, target_y, target_z),
            "distance": distance, "distanceRadii": distance_radii,
            "yaw": yaw, "pitch": pitch,
        }
    elif m == "lookat":
        cmd = {
            "cmd": "camera", "mode": "lookat",
            "eye": _vec3(eye_x, eye_y, eye_z),
            "target": _vec3(target_x, target_y, target_z),
        }
    elif m == "freefly":
        cmd = {
            "cmd": "camera", "mode": "freefly",
            "eye": _vec3(eye_x, eye_y, eye_z), "yaw": yaw, "pitch": pitch,
        }
    else:
        return f"error: unknown camera mode '{mode}' (use orbit | lookat | freefly)"
    return _send(cmd)


@mcp.tool()
def fly_path(waypoints_json: str, frames: int, pattern: str, every: int = 1,
            ease: str = "catmull-rom") -> str:
    """Fly a keyframed camera path and capture a frame burst along it.

    waypoints_json: a JSON array of {"t": seconds, "eye": [x,y,z], "target": [x,y,z]} points
                    (>=2). Arms the path, then renders `frames` frames capturing every
                    `every`-th to `pattern` (a printf path with one %0Nd, e.g. out/fly_%03d.png).
    Returns the burst reply (frame count).
    """
    try:
        wps = json.loads(waypoints_json)
        if not isinstance(wps, list) or len(wps) < 2:
            return "error: waypoints_json must be a JSON array of >=2 waypoints"
    except json.JSONDecodeError as e:
        return f"error: bad waypoints_json ({e})"
    arm = _send({"cmd": "camera", "mode": "path", "ease": ease, "waypoints": wps})
    if arm.startswith("error"):
        return arm
    return _send({"cmd": "burst", "frames": frames, "every": max(1, every), "pattern": pattern})


@mcp.tool()
def set_param(key: str, value: float) -> str:
    """Set a scalar live parameter by dotted key (e.g. "body.shading.snowLine",
    "scene.lighting.sunIntensity", "ocean.waveStrength", "lod.splitThreshold").
    Real-time params apply on the next frame; terrain/LOD params regenerate before the next
    capture. Use set_param_vec3 for color/direction (vec3) keys."""
    return _send({"cmd": "setParam", "key": key, "value": value})


@mcp.tool()
def set_param_vec3(key: str, x: float, y: float, z: float) -> str:
    """Set a vec3 live parameter by dotted key (e.g. "scene.lightDir",
    "ocean.deepColor", "body.shading.colorSnow")."""
    return _send({"cmd": "setParam", "key": key, "value": _vec3(x, y, z)})


@mcp.tool()
def set_body(body: str) -> str:
    """Switch the planet body type: "earth", "volcanic", or "crystalline". Regenerates the
    world; the camera framing you last set is preserved."""
    return _send({"cmd": "setBody", "body": body})


@mcp.tool()
def set_seed(seed: int) -> str:
    """Set the generation seed (regenerates terrain on the next capture)."""
    return _send({"cmd": "setSeed", "seed": seed})


@mcp.tool()
def capture(path: str) -> str:
    """Render one frame and write it to `path` (relative to the app's working dir, e.g.
    "out/shot.png"). Returns the written path (read it to see the result)."""
    return _send({"cmd": "capture", "path": path})


@mcp.tool()
def wait_frames(frames: int) -> str:
    """Advance `frames` fixed-dt frames (drives shader time + any armed camera path) without
    capturing. Use to let motion/ocean advance between shots."""
    return _send({"cmd": "wait", "frames": frames})


if __name__ == "__main__":
    mcp.run()
