"""Read local OTA credentials without printing them or adding them to Git."""
import json
from pathlib import Path


def read_ota_settings(project_dir, environ):
    path = Path(project_dir) / "ota.local.json"
    if not path.exists():
        return None, environ.get("CAFE_OTA_PASSWORD", "")
    try:
        data = json.loads(path.read_text(encoding="utf-8-sig"))
    except (OSError, UnicodeError, ValueError) as exc:
        raise RuntimeError("Cannot read ota.local.json: provide valid UTF-8 JSON.") from None
    if not isinstance(data, dict):
        raise RuntimeError("ota.local.json must contain a JSON object.")
    def credential(key):
        value = data.get(key)
        if not isinstance(value, str) or not value or len(value) > 128 or any(c in value for c in '\x00\r\n'):
            raise RuntimeError("ota.local.json: invalid " + key + " (nonempty string, max. 128 characters).")
        return value
    password = credential("password")
    upload = credential("uploadPassword") if "uploadPassword" in data else password
    return password, upload
