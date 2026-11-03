# Prints the firmware version and build date flags, so the app shows which commit runs on the radio.
# A "+" after the commit means the build had uncommitted changes. build.sh sets its own flags: then print nothing.
import datetime
import os
import re
import subprocess

if "FIRMWARE_VERSION" not in os.environ.get("PLATFORMIO_BUILD_FLAGS", ""):
    def git(*args):
        return subprocess.run(["git", *args], capture_output=True, text=True).stdout.strip()

    with open("examples/companion_radio/MyMesh.h") as f:
        base = re.search(r'#define FIRMWARE_VERSION "([^"]+)"', f.read()).group(1)
    dirty = "+" if git("status", "--porcelain", "--untracked-files=no") else ""
    version = f"{base}-{git('rev-parse', '--short', 'HEAD')}{dirty}"
    date = datetime.date.today().strftime("%d %b %Y")
    print(f"-D FIRMWARE_VERSION='\"{version}\"' -D FIRMWARE_BUILD_DATE='\"{date}\"'")
