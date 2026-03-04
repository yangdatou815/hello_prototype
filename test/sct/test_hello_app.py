import socket
import time

from sct_helpers import AppSimulator, PythonClient, print_tcp_state


# ─────────────────────────────────────────────
# Test cases
# ─────────────────────────────────────────────
def test_hello_exception(app_path):
    """Tests that --throw exits with error."""
    print("\n[SCT] ===== test_hello_exception =====")
    simulator = AppSimulator(app_path)
    simulator.start(args=["--throw"])
    error_output = simulator.get_error_output()
    print(f"[SCT] stderr: {error_output}")
    assert "Caught exception: Exception from Greeter" in error_output
    assert simulator.process.returncode != 0
    print("[SCT] test_hello_exception PASSED")


def test_no_role_prints_usage(app_path):
    """Tests that no args prints usage."""
    print("\n[SCT] ===== test_no_role_prints_usage =====")
    simulator = AppSimulator(app_path)
    simulator.start()
    error_output = simulator.get_error_output()
    print(f"[SCT] stderr: {error_output}")
    assert "Usage:" in error_output
    assert simulator.process.returncode != 0
    print("[SCT] test_no_role_prints_usage PASSED")


def test_client_server_handshake(app_path):
    """
    Tests the 3-step handshake:
      Client → DETECTED → Server
      Server → CONNECTED → Client
      Client → ONLINE   → Server
      Server → ACK: ONLINE → Client  (client reaches ONLINE state)
    """
    print("\n[SCT] ===== test_client_server_handshake =====")
    server = AppSimulator(app_path)
    client = AppSimulator(app_path)

    try:
        print("[SCT] Step 1: Starting server...")
        server.start(args=["--role", "server"])
        time.sleep(0.5)
        print_tcp_state("after server start (LISTEN expected)")

        print("[SCT] Step 2: Starting client...")
        client.start(args=["--role", "client"])
        time.sleep(0.2)
        print_tcp_state("after client start (ESTABLISHED expected)")

        print("[SCT] Step 3: Reading client stdout (timeout=8s)...")
        collected = []
        deadline = time.time() + 8.0
        while time.time() < deadline:
            line = client.process.stdout.readline()
            if line:
                collected.append(line.strip())
                print(f"[SCT][Client stdout] {line.strip()}")
                if "Handshake complete" in line:
                    print("[SCT] Handshake complete detected.")
                    print_tcp_state("after handshake complete (ESTABLISHED expected)")
                    break
        else:
            print("[SCT] WARNING: Timeout before 'Handshake complete'.")

        client_output = "\n".join(collected)

        print("[SCT] Step 4: Asserting...")
        assert "State changed to: DETECTED"  in client_output, \
            f"Missing DETECTED in:\n{client_output}"
        assert "State changed to: CONNECTED" in client_output, \
            f"Missing CONNECTED in:\n{client_output}"
        assert "State changed to: ONLINE"    in client_output, \
            f"Missing ONLINE in:\n{client_output}"
        assert "Handshake complete"          in client_output, \
            f"Missing 'Handshake complete' in:\n{client_output}"
        print("[SCT] All assertions passed.")

    finally:
        client.stop()
        print_tcp_state("after client stop")
        server.stop()
        print_tcp_state("after server stop")
        print("[SCT] test_client_server_handshake DONE")


def test_server_receives_handshake_and_replies(app_path):
    """
    SCT acts as fake client and drives the 3-step handshake manually:
      SCT  → DETECTED  → Server  (expects CONNECTED back)
      SCT  → ONLINE    → Server  (expects ACK: ONLINE back)
      SCT  → UNKNOWN   → Server  (expects no reply)
    """
    print("\n[SCT] ===== test_server_receives_handshake_and_replies =====")

    server = AppSimulator(app_path)
    py_client = PythonClient(host="127.0.0.1", port=9000)

    try:
        print("[SCT] Step 1: Starting server...")
        server.start(args=["--role", "server"])
        time.sleep(0.5)
        print_tcp_state("after server start (LISTEN expected)")

        print("[SCT] Step 2: PythonClient connecting...")
        py_client.connect()
        time.sleep(0.1)
        print_tcp_state("after PythonClient connect (ESTABLISHED expected)")

        # Handshake step 1
        print("[SCT] Step 3: Sending 'DETECTED'...")
        py_client.send("DETECTED")
        reply = py_client.receive()
        print(f"[SCT] Server replied: \"{reply}\"")
        print_tcp_state("after DETECTED/CONNECTED exchange")
        assert reply == "CONNECTED", \
            f"Expected 'CONNECTED', got: '{reply}'"
        print("[SCT] CONNECTED assertion passed.")

        # Handshake step 3
        print("[SCT] Step 4: Sending 'ONLINE'...")
        py_client.send("ONLINE")
        reply = py_client.receive()
        print(f"[SCT] Server replied: \"{reply}\"")
        print_tcp_state("after ONLINE/ACK:ONLINE exchange")
        assert reply == "ACK: ONLINE", \
            f"Expected 'ACK: ONLINE', got: '{reply}'"
        print("[SCT] ACK: ONLINE assertion passed.")

        # Unknown message — no reply expected
        print("[SCT] Step 5: Sending unknown message, expecting no reply...")
        py_client.send("UNKNOWN_MSG")
        try:
            unexpected = py_client.receive()
            assert False, f"Server should not reply, but got: '{unexpected}'"
        except (TimeoutError, socket.timeout):
            print("[SCT] Correctly got no reply (socket timeout).")

        print("[SCT] All assertions passed.")

    finally:
        py_client.close()
        time.sleep(0.1)
        print_tcp_state("after PythonClient close")
        server.stop()
        print_tcp_state("after server stop")
        print("[SCT] test_server_receives_handshake_and_replies DONE")
