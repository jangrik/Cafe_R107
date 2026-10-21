import json
import sys
import tempfile
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from ota_config import read_ota_settings

class LocalOtaTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name)
    def write(self, value):
        (self.path / "ota.local.json").write_text(json.dumps(value), encoding="utf-8-sig")
    def test_without_file_keeps_admin_fallback_and_legacy_upload(self):
        self.assertEqual(read_ota_settings(self.path, {}), (None, ""))
        self.assertEqual(read_ota_settings(self.path, {"CAFE_OTA_PASSWORD": "legacy"}), (None, "legacy"))
    def test_local_file_controls_firmware_and_upload(self):
        self.write({"password": "local-test-password"})
        self.assertEqual(read_ota_settings(self.path, {"CAFE_OTA_PASSWORD": "ignored"}), ("local-test-password", "local-test-password"))
    def test_transition_upload_credential_does_not_change_new_firmware_credential(self):
        self.write({"password": "new-test-password", "uploadPassword": "old-test-password"})
        self.assertEqual(read_ota_settings(self.path, {}), ("new-test-password", "old-test-password"))
    def test_invalid_configuration_fails_without_disclosing_content(self):
        for value in ({}, {"password": ""}, {"password": 123}, {"password": "x", "uploadPassword": None}, []):
            self.write(value)
            with self.assertRaises(RuntimeError):read_ota_settings(self.path, {})
        (self.path / "ota.local.json").write_text('not-json-secret',encoding='utf-8')
        with self.assertRaises(RuntimeError) as error:read_ota_settings(self.path, {})
        self.assertNotIn('not-json-secret',str(error.exception))

if __name__ == '__main__':unittest.main()
