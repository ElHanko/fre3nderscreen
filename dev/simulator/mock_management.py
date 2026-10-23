#!/usr/bin/env python3
"""Local Fre3nder Management API simulator fixture.

This process is exclusively for Fre3nderScreen's Docker/SDL simulator. It implements the existing Unix-socket HTTP/JSON contract using synthetic
state. App mutations affect this simulator process only (never the host). No
TCP listener, real packages, signatures, or production access are involved.
"""

import argparse
import datetime
import http.server
import json
import os
import re
import secrets
from pathlib import Path
import signal
import socketserver
import stat
import time

ROUTE = "/fre3nder/api/v1/system/details"
APPS_PREFIX = "/fre3nder/api/v1/apps/"
STARTED = time.monotonic()


def system_snapshot():
    """Shape compatible with managementd's system/details response."""
    mib = 1024 * 1024
    return {
        "api_version": 1,
        "ok": True,
        "operation": "system-details",
        "system": {
            "platform": {
                "version": "2026.5-simulator",
                "hardware_model": "Ender-3 V3 KE (simulated)",
                "cpu_model": "Ingenic X2000 (simulated)",
                "architecture": "mipsel",
                "kernel_version": "6.6.x-rt (simulated)",
                "hostname": "fre3nder-demo",
            },
            "runtime": {
                "uptime_seconds": round(3 * 3600 + time.monotonic() - STARTED, 1),
                "system_time": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                "load_average": [0.12, 0.18, 0.09],
            },
            "memory": {
                "total_bytes": 244 * mib,
                "available_bytes": 84 * mib,
                "used_bytes": 160 * mib,
                "swap_total_bytes": 0,
                "swap_free_bytes": 0,
                "swap_used_bytes": 0,
            },
            "storage": [
                {"role": "SYS", "mountpoint": "/run/fre3nder-root/system",
                 "device": "/dev/mock-sys", "filesystem": "ext4",
                 "total_bytes": 2048 * mib, "used_bytes": 740 * mib,
                 "available_bytes": 1200 * mib},
                {"role": "HOME", "mountpoint": "/home",
                 "device": "/dev/mock-home", "filesystem": "ext4",
                 "total_bytes": 4096 * mib, "used_bytes": 1680 * mib,
                 "available_bytes": 2300 * mib},
            ],
            "boot": {"active_slot": "A", "active_rootfs": "rootfs-a",
                     "root_status": "active"},
            "network": {
                "interfaces": [
                    {"name": "eth0", "kind": "ethernet", "state": "up",
                     "carrier": True, "addresses": ["192.0.2.10"]},
                    {"name": "wlan0", "kind": "wlan", "state": "down",
                     "carrier": False, "addresses": []},
                ],
                "default_ipv4_routes": [
                    {"interface": "eth0", "gateway": "192.0.2.1", "metric": 0}
                ],
                "default_ipv4_route": {"interface": "eth0",
                                       "gateway": "192.0.2.1", "metric": 0},
            },
            "usb": [
                {"port": "1-1", "parent": "usb1", "manufacturer": "Simulator",
                 "product": "Demo USB device", "vendor_id": "0000",
                 "product_id": "0001", "driver": "usbhid",
                 "interfaces": [{"interface": "1-1:1.0", "driver": "usbhid"}]},
            ],
            "services": [
                {"name": "Klipper", "status": "active",
                 "recorded_status": "active", "process_running": True},
                {"name": "Moonraker", "status": "active",
                 "recorded_status": "active", "process_running": True},
                {"name": "Management", "status": "active",
                 "recorded_status": "active", "process_running": True},
                {"name": "Web server", "status": "active",
                 "recorded_status": "active", "process_running": True},
                {"name": "Maintenance web server", "status": "disabled",
                 "recorded_status": "disabled", "process_running": False},
                {"name": "Camera", "status": "not-running",
                 "recorded_status": "stopped", "process_running": False},
                {"name": "SSH", "status": "active",
                 "recorded_status": "active", "process_running": True},
            ],
            "applications": {
                "installed_count": 2,
                "web_frontend": {"status": "selected", "name": "fluidd"},
                "display_frontend": {"status": "selected", "name": "fre3nderscreen"},
                "display_runtime_status": "active",
            },
            "maintenance": {
                "enabled": False,
                "web_status": "disabled",
                "ota": None,
                "execution": None,
            },
        },
    }


class SimulatedApps:
    """Isolated, deterministic app records, not a package/signature verifier."""
    def __init__(self):
        self.installed = [
            {"name": "fre3nderscreen", "version": "2026.5-dev", "storage": "sys",
             "publisher": "fre3nder-official", "release_serial": 0,
             "web_frontend": False, "display_frontend": True, "display_api": 1,
             "autostart": False},
            {"name": "fluidd", "version": "1.36.0-demo", "storage": "sys",
             "publisher": "fre3nder-official", "release_serial": 1,
             "web_frontend": True, "display_frontend": False, "display_api": None,
             "autostart": False},
            {"name": "demo-app", "version": "1.0.0", "storage": "home",
             "publisher": "simulator", "release_serial": 1,
             "web_frontend": False, "display_frontend": False, "display_api": None,
             "autostart": True},
        ]
        self.installed.append({
            "name": "fre3nder-app-update", "version": "1.0-demo", "storage": "home",
            "publisher": "fre3nder-official", "release_serial": 1,
            "web_frontend": False, "display_frontend": False, "display_api": None,
            "autostart": True})
        # Preselected API output only: the simulator does not compare versions.
        self.repository = {
            "client_installed": True, "configuration": "configured", "check": "ok",
            "availability": "checked", "error": None,
            "identity": {"name": "Simulator repository", "key_fingerprint": "c" * 64,
                         "base_url": "https://repository.example.invalid/"},
            "revision": 7, "last_attempt": "2026-10-20T10:00:00+00:00",
            "last_success": "2026-10-20T10:00:00+00:00",
            "updates": [{"name": "fluidd", "publisher": {
                "id": "fre3nder-official", "key_fingerprint": "a" * 64},
                "installed": {"version": "1.36.0-demo", "release_serial": 1},
                "offered": {"version": "1.38.0", "release_serial": 2, "channel": "stable"}}]}
        self.display = "fre3nderscreen"
        self.web = "fluidd"
        self.staged = {}
        self.execution = None

    def overview(self):
        mib = 1024 * 1024
        return {
            "installed": list(self.installed),
            "web": {"active": self.web, "document_root": None},
            "display": {"active": self.display, "apps": [
                {**app, "active": app["name"] == self.display}
                for app in self.installed if app["display_frontend"]]},
            "packages": [{"id": key, "size": value["size"]}
                         for key, value in sorted(self.staged.items())],
            "verified_packages": sorted(key for key, value in self.staged.items()
                                        if value["verified"]),
            "execution": self.execution,
            "repository": self.repository,
            "storage": {"available_bytes": 2048 * mib,
                        "max_package_bytes": 256 * mib, "reserve_bytes": 64 * mib,
                        "sys": {"available_bytes": 1024 * mib},
                        "home": {"available_bytes": 2048 * mib}},
        }

    def detail(self, name):
        app = next((a for a in self.installed if a["name"] == name), None)
        return None if app is None else {**app, "running": app["name"] == self.display
                                                   or bool(app["autostart"])}

    def run(self, operation, payload):
        """Mutation affects only in-process demo lists and receipts."""
        if not isinstance(payload, dict):
            return None
        package_id = payload.get("package_id")
        name = payload.get("name")
        if operation in ("verify", "install", "update", "discard") and package_id not in self.staged:
            return None
        if operation in ("remove", "display/select", "update") and not isinstance(name, str):
            return None
        if operation == "verify":
            self.staged[package_id]["verified"] = True
        elif operation == "install":
            if not self.staged[package_id]["verified"] or payload.get("storage", "sys") not in ("sys", "home"):
                return None
            name = self.staged[package_id]["name"]
            if self.detail(name):
                return None
            self.installed.append({"name": name, "version": "1.0-demo",
                "release_serial": 1, "publisher": "simulator", "storage": payload.get("storage", "sys"),
                "web_frontend": False, "display_frontend": False, "display_api": None,
                "autostart": False})
            del self.staged[package_id]
        elif operation == "update":
            if not self.staged[package_id]["verified"] or self.staged[package_id]["name"] != name:
                return None
            app = self.detail(name)
            if app is None:
                return None
            for item in self.installed:
                if item["name"] == name:
                    item["version"] = "2.0-demo"
            del self.staged[package_id]
        elif operation == "remove":
            if name == self.display or not self.detail(name):
                return None
            self.installed = [a for a in self.installed if a["name"] != name]
            if name == self.web:
                self.web = None
        elif operation == "display-select":
            app = self.detail(name)
            if not app or not app["display_frontend"]:
                return None
            self.display = name
        elif operation == "display-disable":
            self.display = None
        elif operation == "discard":
            del self.staged[package_id]
            return "discard"
        else:
            return None
        self.execution = {"id": secrets.token_hex(16), "operation": operation,
                          "state": "succeeded", "name": name, "package_id": package_id,
                          "result": {"api_version": 1, "ok": True, "operation": operation}}
        return self.execution

APPS = SimulatedApps()

# Synthetic OTA state machine: UI/transport fixture ONLY; never validates
# signatures or touches a real kernel, RootFS, selector, or backup medium.
OTA_PREFIX = "/fre3nder/api/v1/ota/"


class SimulatedOta:
    def __init__(self):
        self.packages = {}
        self.execution = None
        self.transaction = None
        self.activation = None
        self.known_good = {"state": "known-good", "version": "2026.4-demo", "slot": "A"}

    def overview(self):
        mib = 1024 * 1024
        return {
            "core": {"api_version": 1, "ok": True, "operation": "status",
                     "busy": False, "transaction": self.transaction,
                     "activation": self.activation, "known_good": self.known_good},
            "execution": self.execution,
            "packages": [{"id": k, "size": v["size"]} for k, v in self.packages.items()],
            "storage": {"available_bytes": 600 * mib, "max_package_bytes": 96 * mib,
                        "reserve_bytes": 32 * mib, "proxy_copy_reserved": True,
                        "proxy_available_bytes": 1024 * mib},
        }

    def context(self, package_id):
        return {"package": {"path": f"/ota/packages/{package_id}.ota",
                            "version": "2026.5-demo", "platform": "ender3-v3-ke",
                            "status": "valid", "artifact_mode": "development"},
                "runtime": {"installed_version": "2026.4-demo", "active_slot": "A",
                            "active_root": "/dev/mmcblk0p7"},
                "target": {"slot": "B", "kernel": "/dev/mmcblk0p6",
                           "rootfs": "/dev/mmcblk0p8"}}

    def offer(self):
        return {"home": {"offered": True, "required": False, "targets": ["usb"]},
                "sys": {"offered": True, "required": False, "targets": ["usb", "home"]}}

    def run(self, operation, payload):
        if not isinstance(payload, dict):
            return None
        pid = payload.get("package_id")
        if operation == "reboot":
            if (self.transaction is None or self.transaction["state"] != "activation-prepared"
                    or payload != {"transaction_id": self.transaction["transaction_id"]}):
                return None
            result = {"state": "reboot-requested", "transaction_id": self.transaction["transaction_id"]}
        elif not isinstance(pid, str) or pid not in self.packages:
            return None
        elif operation in ("verify", "preflight"):
            if payload != {"package_id": pid} or self.transaction or self.activation:
                return None
            result = self.context(pid) if operation == "preflight" else {"package": self.context(pid)["package"]}
            if operation == "preflight":
                result["backup_offer"] = self.offer()
        elif operation in ("backup-plan", "backup"):
            if set(payload) != {"package_id", "home", "sys"} or self.transaction or self.activation:
                return None
            home, sys = payload["home"], payload["sys"]
            if home not in ("usb", "none") or sys not in ("usb", "home", "none"):
                return None
            selection = {role: {"selected": value != "none",
                                "target": value if value != "none" else None}
                         for role, value in (("home", home), ("sys", sys))}
            result = {**self.context(pid), "backup_offer": self.offer(),
                      "backup_selection": selection}
            if operation == "backup":
                result["backup_result"] = {
                    role: {"selected": value != "none", "verified": value != "none",
                           "archive": f"/mock/{role}.tar", "size": 1234, "sha256": "a" * 64}
                    for role, value in (("home", home), ("sys", sys))}
                tx = secrets.token_hex(16)
                self.transaction = {"api_version": 1, "state": "awaiting-confirmation",
                                    "transaction_id": tx, **self.context(pid),
                                    "backup_selection": selection,
                                    "backup_result": result["backup_result"]}
                result["confirmation"] = {"required": True, "state": "awaiting-confirmation",
                                          "transaction_id": tx}
        else:
            if not self.transaction or payload != {
                "package_id": pid, "transaction_id": self.transaction["transaction_id"]}:
                return None
            phases = {"confirm": ("awaiting-confirmation", "confirmed"),
                      "write": ("confirmed", "written-awaiting-readback"),
                      "readback": ("written-awaiting-readback", "readback-verified"),
                      "prepare-activation": ("readback-verified", "activation-prepared")}
            if operation not in phases or self.transaction["state"] != phases[operation][0]:
                return None
            self.transaction["state"] = phases[operation][1]
            if operation == "prepare-activation":
                self.activation = {"state": "activation-prepared",
                                   "transaction_id": self.transaction["transaction_id"]}
            result = {"transaction": {"state": self.transaction["state"],
                                       "transaction_id": self.transaction["transaction_id"]}}
        result = {"api_version": 1, "ok": True, "operation": operation, **result}
        self.execution = {"id": secrets.token_hex(16), "state": "succeeded",
                          "operation": operation, **payload, "result": result}
        return self.execution

    def upload(self, filename, size):
        if not re.fullmatch(r"[A-Za-z0-9._-]{1,124}\.ota", filename) or size <= 0 or size > 96 * 1024 * 1024:
            return None
        if self.transaction or self.activation:
            return None
        pid = secrets.token_hex(16)
        self.packages[pid] = {"size": size}
        return self.run("verify", {"package_id": pid})


OTA = SimulatedOta()



class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.0"
    server_version = "Fre3nderManagementSimulator/1"
    sys_version = ""

    def address_string(self):
        return "local"

    def log_message(self, _fmt, *_args):
        pass

    def send_json(self, status, data):
        body = (json.dumps(data, separators=(",", ":"), sort_keys=True) + "\n").encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path == ROUTE:
            self.send_json(200, system_snapshot())
        elif self.path == OTA_PREFIX + "status":
            self.send_json(200, {"api_version": 1, "ok": True,
                                 "operation": "ota-status", "ota": OTA.overview()})
        elif self.path == APPS_PREFIX + "status":
            self.send_json(200, {"api_version": 1, "ok": True,
                                 "operation": "apps-status", "apps": APPS.overview()})
        elif self.path.startswith(APPS_PREFIX + "detail?name="):
            name = self.path[len(APPS_PREFIX + "detail?name="):]
            app = APPS.detail(name) if re.fullmatch(r"[a-z][a-z0-9_-]{0,63}", name) else None
            if app is None:
                self.send_json(404, {"api_version": 1, "ok": False,
                                     "error": {"code": "not-found", "message": "App not installed"}})
            else:
                self.send_json(200, {"api_version": 1, "ok": True,
                                     "operation": "apps-detail", "app": app})
        else:
            self.send_json(404, {"api_version": 1, "ok": False,
                                 "error": {"code": "not-found"}})

    def do_POST(self):
        if self.path.startswith(OTA_PREFIX):
            self.handle_ota_post()
            return
        if not self.path.startswith(APPS_PREFIX):
            self.send_json(405, {"api_version": 1, "ok": False,
                                 "error": {"code": "simulator-only"}})
            return
        operation = self.path[len(APPS_PREFIX):]
        if operation not in ("upload", "verify", "install", "update", "remove",
                             "display/select", "display/disable", "discard", "acknowledge"):
            self.send_json(404, {"api_version": 1, "ok": False,
                                 "error": {"code": "not-found"}})
            return
        try:
            length = int(self.headers.get("Content-Length", "-1"))
        except ValueError:
            length = -1
        if length < 0 or length > 256 * 1024 * 1024:
            self.send_json(413, {"api_version": 1, "ok": False,
                                 "error": {"code": "request-too-large"}})
            return
        if operation == "upload":
            filename = self.headers.get("X-Fre3nder-Filename", "")
            if (not re.fullmatch(r"[A-Za-z0-9._-]{1,128}\.fre3app", filename)
                    or length <= 0):
                self.send_json(400, {"api_version": 1, "ok": False,
                                     "error": {"code": "invalid-package"}})
                return
            # Read and discard upload bytes: no real archive or signature handling.
            remaining = length
            while remaining:
                data = self.rfile.read(min(65536, remaining))
                if not data:
                    return
                remaining -= len(data)
            ident = secrets.token_hex(16)
            name = filename[:-8]
            if not re.fullmatch(r"[a-z][a-z0-9_-]{0,63}", name):
                name = "sample-app"
            APPS.staged[ident] = {"name": name, "size": length, "verified": True}
            job = APPS.run("verify", {"package_id": ident})
            self.send_json(202, {"api_version": 1, "ok": True,
                                 "operation": "apps-accepted", "execution": job})
            return
        try:
            payload = json.loads(self.rfile.read(length))
        except (ValueError, UnicodeDecodeError):
            payload = None
        if operation == "acknowledge":
            # Uncertain operations are not generated by this deterministic fixture.
            self.send_json(409, {"api_version": 1, "ok": False,
                                 "error": {"code": "no-unknown-operation",
                                           "message": "No unknown simulator execution"}})
            return
        actual_op = "display-select" if operation == "display/select" else (
            "display-disable" if operation == "display/disable" else operation)
        job = APPS.run(actual_op, payload)
        if job is None:
            self.send_json(409, {"api_version": 1, "ok": False,
                                 "error": {"code": "simulated-rejection",
                                           "message": "Operation rejected by simulator"}})
        elif job == "discard":
            self.send_json(200, {"api_version": 1, "ok": True,
                                 "operation": "apps-discard", "discarded": {}})
        else:
            self.send_json(202, {"api_version": 1, "ok": True,
                                 "operation": "apps-accepted", "execution": job})


    def handle_ota_post(self):
        operation = self.path[len(OTA_PREFIX):]
        allowed = {"upload", "verify", "preflight", "backup-plan", "backup", "confirm",
                   "write", "readback", "prepare-activation", "reboot"}
        if operation not in allowed:
            self.send_json(404, {"api_version": 1, "ok": False,
                                 "error": {"code": "not-found", "message": "Unknown OTA route"}})
            return
        try:
            length = int(self.headers.get("Content-Length", "-1"))
        except ValueError:
            length = -1
        if length < 0 or length > 96 * 1024 * 1024:
            self.send_json(413, {"api_version": 1, "ok": False,
                                 "error": {"code": "request-too-large", "message": "Invalid length"}})
            return
        if operation == "upload":
            if self.headers.get("Content-Type") != "application/octet-stream":
                self.send_json(415, {"api_version": 1, "ok": False,
                                     "error": {"code": "invalid-type"}})
                return
            name = self.headers.get("X-Fre3nder-Filename", "")
            if not re.fullmatch(r"[A-Za-z0-9._-]{1,124}\.ota", name) or length == 0:
                self.send_json(400, {"api_version": 1, "ok": False,
                                     "error": {"code": "invalid-filename"}})
                return
            remaining = length
            while remaining:
                block = self.rfile.read(min(65536, remaining))
                if not block:
                    return
                remaining -= len(block)
            job = OTA.upload(name, length)
        else:
            if self.headers.get("Content-Type") != "application/json" or length > 256:
                self.send_json(415, {"api_version": 1, "ok": False,
                                     "error": {"code": "invalid-body"}})
                return
            try:
                payload = json.loads(self.rfile.read(length))
            except (ValueError, UnicodeDecodeError):
                payload = None
            job = OTA.run(operation, payload)
        if job is None:
            self.send_json(409, {"api_version": 1, "ok": False,
                                 "error": {"code": "invalid-transition",
                                           "message": "Simulator rejects invalid OTA transition"}})
            return
        self.send_json(202, {"api_version": 1, "ok": True,
                             "operation": "ota-accepted", "execution": job})


class Server(socketserver.UnixStreamServer):
    allow_reuse_address = False


def serve(socket_path):
    path = Path(socket_path)
    if not path.parent.is_dir():
        raise ValueError("Management simulator socket parent does not exist")
    if path.exists() or path.is_symlink():
        info = path.lstat()
        if not stat.S_ISSOCK(info.st_mode) or info.st_uid != os.geteuid():
            raise ValueError("Refusing to replace a non-owned/non-socket path")
        path.unlink()  # Socket left by an interrupted previous simulator run.

    previous_umask = os.umask(0o077)
    try:
        server = Server(str(path), Handler)
    finally:
        os.umask(previous_umask)
    os.chmod(path, 0o600)
    bound = path.lstat()
    stopped = False

    def stop(_signum, _frame):
        nonlocal stopped
        stopped = True

    signal.signal(signal.SIGTERM, stop)
    signal.signal(signal.SIGINT, stop)
    server.timeout = 0.2
    try:
        while not stopped:
            server.handle_request()
    finally:
        server.server_close()
        try:
            current = path.lstat()
            if current.st_dev == bound.st_dev and current.st_ino == bound.st_ino:
                path.unlink()
        except FileNotFoundError:
            pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--socket", required=True)
    args = parser.parse_args()
    serve(args.socket)


if __name__ == "__main__":
    main()
