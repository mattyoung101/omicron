#!/usr/bin/python3
# Script to run Omicam and restart it if an error occurs

import subprocess
import sys
import os

print("[Runner] Running Omicam...")
os.chdir("cmake-build-release-clang")

for i in range(10):
    try:
        subprocess.run("./omicam", check=True)
        print("[Runner] Omicam terminated normally")
        sys.exit(0)
    except subprocess.CalledProcessError as e:
        print(f"[Runner] Omicam crashed with exit code: {e.returncode}, restarting...")
    except KeyboardInterrupt:
        print("[Runner] Terminating due to keyboard interrupt")
        sys.exit(0)

