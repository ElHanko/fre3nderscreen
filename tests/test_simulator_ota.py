#!/usr/bin/env python3
"""OTA simulator contract tests: mock state only, no real package or device."""
import json
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]
MOCK = ROOT / "dev/simulator/mock_management.py"
PREFIX = "/fre3nder/api/v1/ota/"


class SimulatorOtaTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.sock = Path(self.temp.name) / "management.sock"
        self.process = subprocess.Popen([sys.executable, str(MOCK), "--socket", str(self.sock)],
                                        stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        self.addCleanup(self.stop)
        for _ in range(100):
            if self.sock.is_socket():
                break
            if self.process.poll() is not None:
                self.fail(self.process.stderr.read().decode())
            time.sleep(.02)
        else:
            self.fail("Management mock did not start")

    def stop(self):
        if self.process.poll() is None:
            self.process.terminate()
        self.process.communicate(timeout=3)

    def request(self, method, suffix, data=None, filename=None):
        if data is None:
            content = b""
        elif isinstance(data, bytes):
            content = data
        else:
            content = json.dumps(data).encode()
        header = f"{method} {PREFIX}{suffix} HTTP/1.0\r\nHost: localhost\r\nConnection: close\r\n"
        if method == "POST":
            header += ("Content-Type: application/octet-stream\r\n" if filename else
                       "Content-Type: application/json\r\n")
            header += f"Content-Length: {len(content)}\r\n"
            if filename:
                header += f"X-Fre3nder-Filename: {filename}\r\n"
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as conn:
            conn.settimeout(3)
            conn.connect(str(self.sock))
            conn.sendall((header + "\r\n").encode() + content)
            chunks = []
            while True:
                piece = conn.recv(16384)
                if not piece:
                    break
                chunks.append(piece)
        raw = b"".join(chunks)
        statusline, sep, body = raw.partition(b"\r\n\r\n")
        self.assertTrue(sep)
        return int(statusline.split()[1]), json.loads(body)

    def status(self):
        code, value = self.request("GET", "status")
        self.assertEqual(code, 200)
        self.assertEqual(value["operation"], "ota-status")
        return value["ota"]

    def upload(self):
        code, value = self.request("POST", "upload", b"simulator-only fake OTA", "demo.ota")
        self.assertEqual(code, 202)
        self.assertEqual(value["execution"]["operation"], "verify")
        package_id = value["execution"]["package_id"]
        self.assertEqual(self.status()["packages"][0]["id"], package_id)
        return package_id

    def post(self, step, payload):
        code, value = self.request("POST", step, payload)
        self.assertEqual(code, 202, (step, value))
        self.assertEqual(value["operation"], "ota-accepted")
        self.assertEqual(value["execution"]["operation"], step)
        self.assertEqual(self.status()["execution"]["state"], "succeeded")
        return value["execution"]["result"]

    def test_initial_state_and_rejected_mutations(self):
        state = self.status()
        self.assertIsNone(state["core"]["transaction"])
        self.assertFalse(state["core"]["busy"])
        self.assertEqual(self.request("POST", "unknown", {})[0], 404)
        self.assertEqual(self.request("POST", "write", {"package_id": "a" * 32})[0], 409)
        self.assertEqual(self.request("POST", "upload", b"invalid", "../path.ota")[0], 400)

    def test_manual_update_backup_readback_and_reboot_contract(self):
        pid = self.upload()
        preflight = self.post("preflight", {"package_id": pid})
        self.assertEqual(preflight["target"]["slot"], "B")
        self.assertEqual(preflight["backup_offer"]["home"]["targets"], ["usb"])
        choices = {"package_id": pid, "home": "usb", "sys": "home"}
        self.post("backup-plan", choices)
        created = self.post("backup", choices)
        tid = created["confirmation"]["transaction_id"]
        self.assertTrue(created["backup_result"]["home"]["verified"])
        self.assertEqual(self.status()["core"]["transaction"]["state"], "awaiting-confirmation")
        self.assertEqual(self.request("POST", "write", {"package_id": pid, "transaction_id": tid})[0], 409)
        for op, wanted in [("confirm", "confirmed"), ("write", "written-awaiting-readback"),
                           ("readback", "readback-verified"), ("prepare-activation", "activation-prepared")]:
            self.post(op, {"package_id": pid, "transaction_id": tid})
            self.assertEqual(self.status()["core"]["transaction"]["state"], wanted)
        self.assertIsNotNone(self.status()["core"]["activation"])
        self.assertEqual(self.request("POST", "reboot", {"transaction_id": "b" * 32})[0], 409)
        requested = self.post("reboot", {"transaction_id": tid})
        self.assertEqual(requested["state"], "reboot-requested")
        # Simulator must not perform a real reboot or mark a new known-good boot.
        self.assertEqual(self.status()["core"]["known_good"]["version"], "2026.4-demo")

    def test_explicitly_declined_backups(self):
        pid = self.upload()
        selections = {"package_id": pid, "home": "none", "sys": "none"}
        plan = self.post("backup-plan", selections)
        self.assertIsNone(plan["backup_selection"]["home"]["target"])
        self.assertIsNone(plan["backup_selection"]["sys"]["target"])
        result = self.post("backup", selections)
        self.assertFalse(result["backup_result"]["home"]["selected"])
        self.assertFalse(result["backup_result"]["sys"]["selected"])


if __name__ == "__main__":
    unittest.main()
