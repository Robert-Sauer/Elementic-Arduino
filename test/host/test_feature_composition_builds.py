"""Check real AVR binaries after building test/compile/feature-composition."""

import os
from pathlib import Path
import re
import shutil
import subprocess
import unittest


ROOT = Path(__file__).parents[2]
BUILD = ROOT / "test/compile/feature-composition/.pio/build"
ENVIRONMENTS = (
    "offline_v1", "offline_v2", "ethernet_only",
    "ethernet_mqtt_v1", "ethernet_mqtt_v2", "soft_disabled",
)
MQTT_STORAGE = ("mqttClient", "MQTT_Input", "MQTT_Output", "MQTTUpdate")


class FeatureCompositionBuildTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.nm = os.environ.get("ELEMENTIC_AVR_NM") or shutil.which("avr-nm")
        if not cls.nm:
            raise unittest.SkipTest("Set ELEMENTIC_AVR_NM to run binary checks")
        missing = [env for env in ENVIRONMENTS if not (BUILD / env / "firmware.elf").exists()]
        if missing:
            raise AssertionError("Build the complete composition matrix first: " + ", ".join(missing))
        cls.symbols = {
            env: subprocess.check_output(
                [cls.nm, "-S", "-C", str(BUILD / env / "firmware.elf")], text=True,
            ) for env in ENVIRONMENTS
        }

    def test_hard_off_removes_mqtt_objects_and_buffers(self):
        for env in ("offline_v1", "offline_v2", "ethernet_only"):
            with self.subTest(env=env):
                for name in MQTT_STORAGE:
                    self.assertIsNone(re.search(r"\b" + name + r"$", self.symbols[env], re.M))
                self.assertNotIn("PubSubClient::", self.symbols[env])

    def test_offline_builds_remove_ethernet_driver_and_client(self):
        for env in ("offline_v1", "offline_v2"):
            with self.subTest(env=env):
                self.assertNotIn("EthernetClass::", self.symbols[env])
                self.assertNotIn("EthernetClient::", self.symbols[env])
                self.assertNotIn("netClient", self.symbols[env])

    def test_soft_off_retains_mqtt_storage(self):
        for env in ("ethernet_mqtt_v1", "ethernet_mqtt_v2", "soft_disabled"):
            with self.subTest(env=env):
                for name in MQTT_STORAGE:
                    self.assertIsNotNone(re.search(r"\b" + name + r"$", self.symbols[env], re.M))
                self.assertIn("PubSubClient::", self.symbols[env])

    def test_soft_on_and_off_keep_identical_topic_array_sizes(self):
        def size(env, name):
            match = re.search(r"^[0-9a-fA-F]+\s+([0-9a-fA-F]+)\s+\w\s+" + name + r"$",
                              self.symbols[env], re.M)
            self.assertIsNotNone(match)
            return int(match[1], 16)
        for name in ("MQTT_Input", "MQTT_Output", "MQTTUpdate"):
            self.assertEqual(size("ethernet_mqtt_v2", name), size("soft_disabled", name))


if __name__ == "__main__":
    unittest.main()
