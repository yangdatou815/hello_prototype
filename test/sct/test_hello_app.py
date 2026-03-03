import socket
import time

from sct_helpers import AppSimulator, PythonClient, print_tcp_state


# ─────────────────────────────────────────────
# Test cases
# ─────────────────────────────────────────────
def test_hello_exception(app_path):
    """
    Tests that the application handles the --throw flag correctly.
    The app should print an error to stderr and exit with a non-zero code.
    """
    print("\n[SCT] ===== test_hello_exception =====")
    simulator = AppSimulator(app_path)
    simulator.start(args=["--throw"])
    error_output = simulator.get_error_output()
    print(f"[SCT] stderr: {error_output}")
    print(f"[SCT] returncode: {simulator.process.returncode}")
    assert "Caught exception: Exception from Greeter" in error_output
    assert simulator.process.returncode != 0
    print("[SCT] test_hello_exception PASSED")


def test_no_role_prints_usage(app_path):
    """
    Tests that running the app with no arguments prints a usage message.
    """
    print("\n[SCT] ===== test_no_role_prints_usage =====")
    simulator = AppSimulator(app_path)
    simulator.start()
    error_output = simulator.get_error_output()
    print(f"[SCT] stderr: {error_output}")
    print(f"[SCT] returncode: {simulator.process.returncode}")
    assert "Usage:" in error_output
    assert simulator.process.returncode != 0
    print("[SCT] test_no_role_prints_usage PASSED")


def test_client_server_handshake(app_path):
    """
    Tests the full client-server hello handshake:
    1. Start the server in background.
    2. Start the client.
    3. Verify the client reaches ONLINE state after receiving ACK from server.
    The client maintains a persistent connection after handshake,
    so we read stdout line-by-line with a timeout and then terminate.
    """
    print("\n[SCT] ===== test_client_server_handshake =====")
    server = AppSimulator(app_path)
    client = AppSimulator(app_path)

    try:
        # Step 1: Start server
        print("[SCT] Step 1: Starting server...")
        server.start(args=["--role", "server"])
        print(f"[SCT] Waiting 0.5s for server to bind port...")
        time.sleep(0.5)
        print("[SCT] Server should be ready.")
        print_tcp_state("after server start (LISTEN expected)")

        # Step 2: Start client
        print("[SCT] Step 2: Starting client...")
        client.start(args=["--role", "client"])
        time.sleep(0.2)
        print_tcp_state("after client start (SYN/ESTABLISHED expected)")

        # Step 3: Read client stdout line by line until handshake complete or timeout
        print("[SCT] Step 3: Reading client stdout (timeout=5s)...")
        collected = []
        deadline = time.time() + 5.0
        while time.time() < deadline:
            line = client.process.stdout.readline()
            if line:
                collected.append(line.strip())
                print(f"[SCT][Client stdout] {line.strip()}")
                if "Handshake complete" in line:
                    print("[SCT] Handshake complete line detected, stopping read.")
                    print_tcp_state("after handshake complete (ESTABLISHED expected)")
                    break
        else:
            print("[SCT] WARNING: Timeout reached before 'Handshake complete' was seen.")

        client_output = "\n".join(collected)

        # Step 4: Assert
        print("[SCT] Step 4: Asserting client output...")
        assert "State changed to: ONLINE" in client_output, \
            f"Expected 'State changed to: ONLINE', got:\n{client_output}"
        assert "Handshake complete" in client_output, \
            f"Expected 'Handshake complete', got:\n{client_output}"
        print("[SCT] All assertions passed.")

    finally:
        print("[SCT] Cleanup: stopping client and server...")
        client.stop()
        print_tcp_state("after client stop (TIME_WAIT/CLOSE_WAIT expected)")
        server.stop()
        print_tcp_state("after server stop (no connections expected)")
        print("[SCT] test_client_server_handshake DONE")


def test_server_receives_hello_and_replies_ack(app_path):
    """
    SCT acts as the client:
    1. Start the real Server process.
    2. SCT connects via a raw Python socket (PythonClient).
    3. SCT sends 'Hello, World!' to the Server.
    4. Verify Server replies with 'ACK: Hello, World!'.
    5. Send an unknown message and verify Server does NOT reply
       (connection stays open, no data within timeout).
    """
    print("\n[SCT] ===== test_server_receives_hello_and_replies_ack =====")

    server = AppSimulator(app_path)
    py_client = PythonClient(host="127.0.0.1", port=9000)

    try:
        # Step 1: Start the real server
        print("[SCT] Step 1: Starting server process...")
        server.start(args=["--role", "server"])
        print("[SCT] Waiting 0.5s for server to bind port...")
        time.sleep(0.5)
        print("[SCT] Server should be ready.")
        print_tcp_state("after server start (LISTEN expected)")

        # Step 2: SCT connects as a fake client
        print("[SCT] Step 2: PythonClient connecting...")
        py_client.connect()
        time.sleep(0.1)
        print_tcp_state("after PythonClient connect (ESTABLISHED expected)")

        # Step 3: Send the handshake message
        print("[SCT] Step 3: Sending 'Hello, World!'...")
        py_client.send("Hello, World!")

        # Step 4: Verify ACK reply
        print("[SCT] Step 4: Waiting for ACK...")
        reply = py_client.receive()
        print(f"[SCT] Server replied: \"{reply}\"")
        print_tcp_state("after ACK received (ESTABLISHED expected)")
        assert reply == "ACK: Hello, World!", \
            f"Expected 'ACK: Hello, World!', got: '{reply}'"
        print("[SCT] ACK assertion passed.")

        # Step 5: Send an unknown message, expect no reply (timeout)
        print("[SCT] Step 5: Sending unknown message, expecting no reply...")
        py_client.send("UNKNOWN_MSG")
        try:
            unexpected = py_client.receive()
            assert False, \
                f"Server should not reply to unknown message, but got: '{unexpected}'"
        except (TimeoutError, socket.timeout):
            print("[SCT] Correctly got no reply for unknown message (socket timeout).")

        print("[SCT] All assertions passed.")

    finally:
        print("[SCT] Cleanup: closing PythonClient and stopping server...")
        py_client.close()
        time.sleep(0.1)
        print_tcp_state("after PythonClient close (TIME_WAIT/CLOSE_WAIT expected)")
        server.stop()
        print_tcp_state("after server stop (no connections expected)")
        print("[SCT] test_server_receives_hello_and_replies_ack DONE")
