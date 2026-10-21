"""Use gitignored local OTA credentials for builds and uploads."""
import hashlib
import os
import sys
from SCons.Script import COMMAND_LINE_TARGETS

Import("env")
sys.path.insert(0, os.path.join(env.subst("$PROJECT_DIR"), "scripts"))
from ota_config import read_ota_settings

password, upload_password = read_ota_settings(env.subst("$PROJECT_DIR"), os.environ)
if password is not None:
    digest = hashlib.md5(password.encode("utf-8")).hexdigest()
    env.Append(CPPDEFINES=[("CAFE_OTA_PASSWORD_HASH", r'\"%s\"' % digest)])

if env.GetProjectOption("upload_protocol", "") == "espota":
    if "upload" in COMMAND_LINE_TARGETS and not upload_password:
        raise RuntimeError("Create ota.local.json or set CAFE_OTA_PASSWORD before OTA upload.")
    if upload_password:
        env.Append(UPLOAD_FLAGS=["--auth=" + upload_password])
