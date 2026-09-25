#!/usr/bin/env python3
import asyncio
import json
import re
import time
from aiohttp import WSMsgType, web

HOST = "127.0.0.1"
PORT = 7125


class SimulatorState:
    def __init__(self):
        self.clients = set()
        self.started = time.monotonic()
        self.extruder_temp = 21.0
        self.extruder_target = 0.0
        self.bed_temp = 52.0
        self.bed_target = 0.0
        self.print_state = "standby"

    def objects(self):
        return [
            "extruder",
            "heater_bed",
            "print_stats",
            "toolhead",
            "gcode_move",
            "configfile",
            "pause_resume",
            "virtual_sdcard",
        ]

    def status(self):
        return {
            "extruder": {
                "temperature": round(self.extruder_temp, 2),
                "target": round(self.extruder_target, 2),
                "power": 0.0 if self.extruder_target <= 0 else 0.35,
                "can_extrude": self.extruder_temp >= 170,
            },
            "heater_bed": {
                "temperature": round(self.bed_temp, 2),
                "target": round(self.bed_target, 2),
                "power": 0.0 if self.bed_target <= 0 else 0.25,
            },
            "print_stats": {
                "state": self.print_state,
                "filename": "",
                "total_duration": 0.0,
                "print_duration": 0.0,
                "filament_used": 0.0,
                "message": "",
            },
            "toolhead": {
                "homed_axes": "xyz",
                "position": [110.0, 110.0, 10.0, 0.0],
                "max_velocity": 500.0,
                "max_accel": 8000.0,
                "square_corner_velocity": 5.0,
            },
            "gcode_move": {
                "speed_factor": 1.0,
                "extrude_factor": 1.0,
                "homing_origin": [0.0, 0.0, 0.0, 0.0],
            },
            "configfile": {
                "config": {},
                "settings": {
                    "printer": {
                        "max_velocity": 500,
                        "max_accel": 8000,
                        "square_corner_velocity": 5,
                    }
                },
            },
            "pause_resume": {"is_paused": False},
            "virtual_sdcard": {"progress": 0.0, "is_active": False, "file_position": 0},
        }

    def apply_gcode(self, script):
        match = re.search(
            r"SET_HEATER_TEMPERATURE\\s+HEATER=([^\\s]+)\\s+TARGET=([-+]?[0-9]*\\.?[0-9]+)",
            script,
        )
        if not match:
            return
        heater, target_text = match.groups()
        target = float(target_text)
        if heater == "extruder":
            self.extruder_target = target
        elif heater == "heater_bed":
            self.bed_target = target

    def step_temperatures(self):
        # Keep the default screen stable, but make temperature controls useful:
        # once a target is set, move toward it deterministically.
        def step(current, target, idle):
            wanted = target if target > 0 else idle
            delta = wanted - current
            if abs(delta) < 0.05:
                return wanted
            return current + max(-1.0, min(1.0, delta * 0.08))

        self.extruder_temp = step(self.extruder_temp, self.extruder_target, 21.0)
        self.bed_temp = step(self.bed_temp, self.bed_target, 52.0)


STATE = SimulatorState()


def rpc_result(method, params):
    if method == "printer.objects.list":
        return {"objects": STATE.objects()}
    if method == "printer.objects.subscribe":
        return {"eventtime": time.monotonic(), "status": STATE.status()}
    if method == "server.files.roots":
        return {"roots": [{"name": "gcodes", "path": "/work/runtime/gcodes", "permissions": "rw"}]}
    if method == "server.files.list":
        return [
            {"path": "3DBenchy.gcode", "modified": 1790337600},
            {"path": "Calibration/first-layer.gcode", "modified": 1790251200},
            {"path": "Calibration/pressure-advance.gcode", "modified": 1790164800},
        ]
    if method == "server.files.metadata":
        filename = params.get("filename", "") if isinstance(params, dict) else ""
        metadata = {
            "3DBenchy.gcode": {
                "modified": 1790337600,
                "estimated_time": 2573,
                "filament_weight_total": 13,
                "size": 3557740,
                "thumbnails": [],
            },
            "Calibration/first-layer.gcode": {
                "modified": 1790251200,
                "estimated_time": 420,
                "filament_weight_total": 2,
                "size": 812400,
                "thumbnails": [],
            },
            "Calibration/pressure-advance.gcode": {
                "modified": 1790164800,
                "estimated_time": 780,
                "filament_weight_total": 4,
                "size": 1240000,
                "thumbnails": [],
            },
        }
        return metadata.get(filename, {
            "modified": 1790337600,
            "estimated_time": 0,
            "filament_weight_total": 0,
            "size": 0,
            "thumbnails": [],
        })
    if method == "printer.info":
        return {
            "state": "ready",
            "state_message": "Printer is ready",
            "hostname": "fre3nder-simulator",
            "software_version": "simulator",
        }
    if method == "server.info":
        return {"components": []}
    if method == "server.database.get_item":
        namespace = params.get("namespace") if isinstance(params, dict) else None
        if namespace == "fluidd":
            return {"namespace": "fluidd", "key": "console", "value": {"commandHistory": []}}
        if namespace == "fre3nderscreen":
            return {"namespace": "fre3nderscreen", "value": {}}
        return {"namespace": namespace, "value": {}}
    if method == "printer.gcode.help":
        return {}
    if method == "machine.device_power.devices":
        return {"devices": {}}
    if method == "machine.device_power.status":
        return {}
    if method == "printer.gcode.script":
        if isinstance(params, dict):
            STATE.apply_gcode(str(params.get("script", "")))
        return "ok"
    if method == "printer.print.start":
        STATE.print_state = "printing"
        return "ok"
    if method in {"printer.print.pause", "printer.print.resume", "printer.print.cancel"}:
        if method.endswith("pause"):
            STATE.print_state = "paused"
        elif method.endswith("resume"):
            STATE.print_state = "printing"
        else:
            STATE.print_state = "cancelled"
        return "ok"
    return {}


async def websocket_handler(request):
    ws = web.WebSocketResponse(heartbeat=30)
    await ws.prepare(request)
    STATE.clients.add(ws)
    try:
        async for msg in ws:
            if msg.type != WSMsgType.TEXT:
                continue
            request_json = None
            try:
                request_json = json.loads(msg.data)
                method = request_json.get("method", "")
                params = request_json.get("params", {})
                result = rpc_result(method, params)
                if "id" in request_json:
                    await ws.send_json({
                        "jsonrpc": "2.0",
                        "id": request_json["id"],
                        "result": result,
                    })
            except Exception as exc:
                if isinstance(request_json, dict) and "id" in request_json:
                    await ws.send_json({
                        "jsonrpc": "2.0",
                        "id": request_json["id"],
                        "error": {"code": -32603, "message": str(exc)},
                    })
    finally:
        STATE.clients.discard(ws)
    return ws


async def status_updates(app):
    try:
        while True:
            await asyncio.sleep(1.0)
            STATE.step_temperatures()
            update = {
                "jsonrpc": "2.0",
                "method": "notify_status_update",
                "params": [
                    {
                        "extruder": STATE.status()["extruder"],
                        "heater_bed": STATE.status()["heater_bed"],
                        "print_stats": {"state": STATE.print_state},
                    },
                    time.monotonic(),
                ],
            }
            dead = []
            for ws in tuple(STATE.clients):
                try:
                    await ws.send_json(update)
                except Exception:
                    dead.append(ws)
            for ws in dead:
                STATE.clients.discard(ws)
    except asyncio.CancelledError:
        pass


async def start_background(app):
    app["status_task"] = asyncio.create_task(status_updates(app))


async def stop_background(app):
    task = app.get("status_task")
    if task is not None:
        task.cancel()
        await task


def main():
    app = web.Application()
    app.router.add_get("/websocket", websocket_handler)
    app.on_startup.append(start_background)
    app.on_cleanup.append(stop_background)
    web.run_app(app, host=HOST, port=PORT, print=None, access_log=None)


if __name__ == "__main__":
    main()
