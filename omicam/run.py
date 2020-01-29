#!/usr/bin/python3
# This script ensures the reliability of Omicam during the competition. It shouldn't be used as a way to completely ignore
# segfaults and crashes, rather to force stability in case the app crashes during a game.

import subprocess
import sys
import os
import shutil
import uuid

print("[Runner] Running Omicam...")
os.chdir("cmake-build-release-clang")

for i in range(10):
    try:
        subprocess.run("./omicam", check=True)
        print("[Runner] Omicam terminated normally")
        sys.exit(0)
    except subprocess.CalledProcessError as e:
        # make sure I'm not a dumbass and don't overlook errors
        print("[Runner] ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION!")
        print(f"[Runner] Omicam crashed with exit code: {e.returncode}, restarting...", file=sys.stderr)

        # copy log file for later analysis
        log_file = os.path.expanduser("~/Documents/TeamOmicron/Omicam/omicam.log")
        new_name = os.path.expanduser(f"~/Documents/TeamOmicron/Omicam/omicam_crash_{uuid.uuid4().hex}.log")
        print(f"[Runner] Copying current Omicam log file to {new_name} for later analysis")
        shutil.copy(log_file, new_name)
    except KeyboardInterrupt:
        print("[Runner] Terminating normally due to keyboard interrupt")
        sys.exit(0)

