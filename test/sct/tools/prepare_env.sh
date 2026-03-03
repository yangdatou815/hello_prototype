#!/bin/bash
set -e
echo "Preparing SCT environment..."
# In a real project, this would generate code, e.g., from Viper/Protobuf/IM
# For this prototype, we just create a placeholder.
mkdir -p ../../src/generated
touch ../../src/generated/api.py
echo "SCT environment ready."
