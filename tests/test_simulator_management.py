#!/usr/bin/env python3
"""Integration tests for the simulator-only Management AF_UNIX endpoint."""

import json
import os
from pathlib import Path
import socket
import stat
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]
MOCK = ROOT / "dev/simulator/mock_management.py"
ROUTE = "/fre3nder/api/v1/system/details"


class SimulatorManagementTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.sock_path = Path(self.tmp.name) / "management.sock"
        self.process = subprocess.Popen(
            [sys.executable, str(MOCK), "--socket", str(self.sock_path)],
            stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
        )
        self.addCleanup(self.stop)
        for _ in range(100):
            if self.sock_path.is_socket():
                try:
                    self.request("GET", ROUTE)
                    break
                except (ConnectionRefusedError, FileNotFoundError, OSError):
                    pass
            if self.process.poll() is not None:
                self.fail(f"mock exited: {self.process.stderr.read().decode()}")
            time.sleep(0.02)
        else:
            self.fail("mock failed to bind within 2 seconds")

    def stop(self):
        if self.process.poll() is None:
            self.process.terminate()
        try:
            self.process.communicate(timeout=3)
        except subprocess.TimeoutExpired:
            self.process.kill()
            self.process.communicate()

    def request(self, method, route, body=b"", extra_headers=None):
        if isinstance(body, str):
            body = body.encode()
        headers = f"{method} {route} HTTP/1.0\r\nHost: localhost\r\n"
        if method == "POST":
            headers += f"Content-Length: {len(body)}\r\n"
            headers += "Content-Type: application/json\r\n"
        for key, value in (extra_headers or {}).items():
            headers += f"{key}: {value}\r\n"
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
            sock.settimeout(2)
            sock.connect(str(self.sock_path))
            sock.sendall((headers + "\r\n").encode() + body)
            data = bytearray()
            while True:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                data.extend(chunk)
        header, separator, body = bytes(data).partition(b"\r\n\r\n")
        self.assertTrue(separator, "missing HTTP header terminator")
        status = int(header.split(b"\r\n", 1)[0].split()[1])
        headers = dict(line.split(b": ", 1) for line in header.split(b"\r\n")[1:] if b": " in line)
        self.assertEqual(int(headers[b"Content-Length"]), len(body))
        self.assertEqual(headers[b"Content-Type"], b"application/json; charset=utf-8")
        return status, json.loads(body)

    def test_system_details_contract_and_synthetic_content(self):
        self.assertEqual(stat.S_IMODE(self.sock_path.stat().st_mode), 0o600)
        code, data = self.request("GET", ROUTE)
        self.assertEqual(code, 200)
        self.assertTrue(data["ok"])
        self.assertEqual(data["api_version"], 1)
        self.assertEqual(data["operation"], "system-details")
        info = data["system"]
        self.assertEqual(info["platform"]["architecture"], "mipsel")
        self.assertIn("simulator", info["platform"]["version"])
        self.assertEqual([item["role"] for item in info["storage"]], ["SYS", "HOME"])
        self.assertEqual(info["memory"]["used_bytes"] + info["memory"]["available_bytes"],
                         info["memory"]["total_bytes"])
        self.assertEqual(info["network"]["default_ipv4_route"]["interface"], "eth0")
        self.assertFalse(info["maintenance"]["enabled"])
        self.assertEqual(info["applications"]["display_frontend"]["name"], "fre3nderscreen")
        self.assertGreater(len(info["services"]), 3)

    def test_apps_status_details_and_simulated_removal(self):
        code, data = self.request("GET", "/fre3nder/api/v1/apps/status")
        self.assertEqual(code, 200)
        self.assertEqual(data["operation"], "apps-status")
        self.assertEqual(len(data["apps"]["installed"]), 3)
        self.assertEqual(data["apps"]["display"]["active"], "fre3nderscreen")
        self.assertEqual(self.request("GET", "/fre3nder/api/v1/apps/detail?name=fluidd")[0], 200)
        self.assertEqual(self.request("GET", "/fre3nder/api/v1/apps/detail?name=missing")[0], 404)
        code, data = self.request("POST", "/fre3nder/api/v1/apps/remove", '{"name":"demo-app"}')
        self.assertEqual(code, 202)
        self.assertEqual(data["operation"], "apps-accepted")
        apps = self.request("GET", "/fre3nder/api/v1/apps/status")[1]["apps"]
        self.assertNotIn("demo-app", [a["name"] for a in apps["installed"]])
        self.assertEqual(self.request("POST", "/fre3nder/api/v1/apps/remove",
                                      '{"name":"fre3nderscreen"}')[0], 409)
        self.assertEqual(self.request("POST", "/fre3nder/api/v1/ota/write")[0], 409)
        self.assertEqual(self.request("POST", "/fre3nder/api/v1/maintenance/enable")[0], 405)

    def test_simulated_usb_upload_then_install(self):
        route = "/fre3nder/api/v1/apps/upload"
        code, data = self.request("POST", route, b"dummy-archive", {
            "Content-Type": "application/octet-stream",
            "X-Fre3nder-Filename": "sample-app.fre3app",
        })
        self.assertEqual(code, 202)
        self.assertEqual(data["operation"], "apps-accepted")
        apps = self.request("GET", "/fre3nder/api/v1/apps/status")[1]["apps"]
        package_id = apps["packages"][0]["id"]
        self.assertIn(package_id, apps["verified_packages"])
        code, data = self.request("POST", "/fre3nder/api/v1/apps/install",
            json.dumps({"package_id": package_id, "storage": "home"}))
        self.assertEqual(code, 202)
        apps = self.request("GET", "/fre3nder/api/v1/apps/status")[1]["apps"]
        installed = next(a for a in apps["installed"] if a["name"] == "sample-app")
        self.assertEqual(installed["storage"], "home")
        self.assertEqual(apps["packages"], [])

    def test_mutations_are_simulator_only_and_unknown_rejected(self):
        self.assertEqual(self.request("POST", "/fre3nder/api/v1/apps/arbitrary", "{}")[0], 404)
        self.assertEqual(self.request("POST", "/fre3nder/api/v1/apps/acknowledge", "{}")[0], 409)

    def test_termination_removes_socket(self):
        self.stop()
        self.assertFalse(self.sock_path.exists())


if __name__ == "__main__":
    unittest.main()
