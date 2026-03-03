import os
import pytest


# ─────────────────────────────────────────────
# Fixtures
# ─────────────────────────────────────────────
@pytest.fixture(scope="module")
def app_path():
    path = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../build/src/hello_app'))
    print(f"[SCT] app_path resolved to: {path}")
    if not os.path.exists(path):
        pytest.fail(f"Application not found at {path}. Please build the project first.")
    return path
