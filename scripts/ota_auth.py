"""Keep OTA credentials out of platformio.ini and GitHub."""
import os
from SCons.Script import COMMAND_LINE_TARGETS

Import("env")

if env.GetProjectOption("upload_protocol", "") == "espota":
    password = os.environ.get("CAFE_OTA_PASSWORD", "")
    if "upload" in COMMAND_LINE_TARGETS and not password:
        raise RuntimeError("Set CAFE_OTA_PASSWORD to the device admin password before OTA upload.")
    if password:
        env.Append(UPLOAD_FLAGS=["--auth=" + password])
