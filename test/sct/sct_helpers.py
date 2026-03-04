import base64
import socket
import subprocess

from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad

# ─────────────────────────────────────────────
# Shared AES-256-CBC key / IV
# Must match the hardcoded values in Crypto.cpp
# ─────────────────────────────────────────────
_AES_KEY = bytes(range(0x00, 0x20))          # 32 bytes: 0x00..0x1f
_AES_IV  = bytes(range(0xa0, 0xb0))          # 16 bytes: 0xa0..0xaf


def _aes_encrypt(plaintext: str) -> str:
    """AES-256-CBC encrypt, return Base64 string."""
    cipher = AES.new(_AES_KEY, AES.MODE_CBC, _AES_IV)
    ct = cipher.encrypt(pad(plaintext.encode(), AES.block_size))
    return base64.b64encode(ct).decode()


def _aes_decrypt(b64_ciphertext: str) -> str:
    """Base64 → AES-256-CBC decrypt, return plaintext string."""
    ct = base64.b64decode(b64_ciphertext)
    cipher = AES.new(_AES_KEY, AES.MODE_CBC, _AES_IV)
    return unpad(cipher.decrypt(ct), AES.block_size).decode()


# ─────────────────────────────────────────────
# TCP state helper
# Prints all TCP connections on port 9000 using
# the `ss` command, annotated with a label so
# the reader knows which test step triggered it.
# ─────────────────────────────────────────────
def print_tcp_state(label: str, port: int = 9000):
    print(f"\n[SCT][TCP] ── {label} ──")
    try:
        result = subprocess.run(
            ["ss", "-tnp", f"sport = :{port}", "or", f"dport = :{port}"],
            capture_output=True, text=True
        )
        output = result.stdout.strip()
        print(f"[SCT][TCP] ss -tnp (port {port}):")
        if output:
            for line in output.splitlines():
                print(f"[SCT][TCP]   {line}")
        else:
            print(f"[SCT][TCP]   (no connections on port {port})")
    except FileNotFoundError:
        print("[SCT][TCP]   'ss' not found, skipping TCP state dump.")
    print(f"[SCT][TCP] ────────────────────────────────────\n")


# ─────────────────────────────────────────────
# AppSimulator: wraps a hello_app subprocess
# ─────────────────────────────────────────────
class AppSimulator:
    def __init__(self, app_path):
        self._app_path = app_path
        self.process = None

    def start(self, args=None):
        cmd = [self._app_path]
        if args:
            cmd.extend(args)
        print(f"[SCT] Launching: {' '.join(cmd)}")
        self.process = subprocess.Popen(cmd,
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE,
                                        text=True)
        print(f"[SCT] Process started. PID={self.process.pid}")
        return self.process

    def stop(self):
        if self.process and self.process.poll() is None:
            print(f"[SCT] Terminating PID={self.process.pid}")
            self.process.terminate()
            self.process.wait()
            print(f"[SCT] PID={self.process.pid} terminated. returncode={self.process.returncode}")
        else:
            print(f"[SCT] Process already exited. returncode={self.process.returncode if self.process else 'N/A'}")

    def get_output(self):
        if self.process:
            stdout, _ = self.process.communicate()
            return stdout.strip()
        return ""

    def get_error_output(self):
        if self.process:
            _, stderr = self.process.communicate()
            return stderr.strip()
        return ""


# ─────────────────────────────────────────────
# PythonClient: SCT-side fake TCP client
# Connects directly to the server via TCP,
# sends a message and reads the reply.
# ─────────────────────────────────────────────
class PythonClient:
    DEFAULT_TIMEOUT = 5.0

    def __init__(self, host: str = "127.0.0.1", port: int = 9000):
        self._host = host
        self._port = port
        self._sock = None
        print(f"[SCT][PythonClient] Initialized. Target: {host}:{port}")

    def connect(self):
        print(f"[SCT][PythonClient] Connecting to {self._host}:{self._port}...")
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.settimeout(self.DEFAULT_TIMEOUT)
        self._sock.connect((self._host, self._port))
        print(f"[SCT][PythonClient] Connected.")

    def send(self, msg: str):
        """Encrypt msg with AES-256-CBC, send as Base64 line."""
        encrypted = _aes_encrypt(msg)
        raw = (encrypted + "\n").encode()
        print(f"[SCT][PythonClient] >> Sending (plain): \"{msg}\"")
        print(f"[SCT][PythonClient] >> Sending (B64):   \"{encrypted}\" ({len(raw)} bytes)")
        self._sock.sendall(raw)

    def receive(self) -> str:
        """Read one Base64 line from server, decrypt and return plaintext."""
        print(f"[SCT][PythonClient] << Waiting for response (timeout={self.DEFAULT_TIMEOUT}s)...")
        buf = b""
        while b"\n" not in buf:
            chunk = self._sock.recv(1024)
            if not chunk:
                raise ConnectionError("[SCT][PythonClient] Connection closed by server.")
            buf += chunk
        b64_line = buf.split(b"\n")[0].decode().strip()
        decrypted = _aes_decrypt(b64_line)
        print(f"[SCT][PythonClient] << Received (B64):   \"{b64_line}\"")
        print(f"[SCT][PythonClient] << Received (plain): \"{decrypted}\"")
        return decrypted

    def close(self):
        if self._sock:
            print("[SCT][PythonClient] Closing connection.")
            self._sock.close()
            self._sock = None
