"""An image upload replaces all LittleFS data, including live bookings."""
Import("env")
import os
from SCons.Script import COMMAND_LINE_TARGETS
if "uploadfs" in COMMAND_LINE_TARGETS and os.environ.get("CAFE_ALLOW_FS_ERASE") != "YES":
    print("STOP: uploadfs replaces ALL LittleFS data. Back up via ESPFMfGK first.")
    print("First installation only: set CAFE_ALLOW_FS_ERASE=YES and retry.")
    env.Exit(1)

