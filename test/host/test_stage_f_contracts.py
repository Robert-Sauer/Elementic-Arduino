#!/usr/bin/env python3
"""Independent checks for the frozen CR-0004 Stage F binary vectors."""

from __future__ import annotations

import binascii
import json
from pathlib import Path
import re
import unittest


ROOT = Path(__file__).parents[2]
FIXTURE = ROOT / "test" / "fixtures" / "stage-f-contract-vectors.json"


def crc32(data: bytes) -> int:
    return binascii.crc32(data) & 0xFFFFFFFF


def decode_v2(raw: bytes) -> tuple[int, int, bytes]:
    if not raw.startswith(b"S011\x02") or not raw.endswith(b"E01"):
        raise ValueError("prefix")
    decoded = bytearray()
    position = 5
    while position < len(raw) - 3:
        value = raw[position]
        position += 1
        if value == 0x7D:
            if position >= len(raw) - 3:
                raise ValueError("escape")
            value = raw[position] ^ 0x20
            position += 1
            if value not in (0x53, 0x45, 0x7D):
                raise ValueError("noncanonical escape")
        elif value in (0x53, 0x45):
            raise ValueError("unescaped marker")
        decoded.append(value)
    payload_length = int.from_bytes(decoded[4:6], "little")
    if len(decoded) != 10 + payload_length:
        raise ValueError("length")
    expected = crc32(b"1\x02" + decoded[: 6 + payload_length])
    if expected != int.from_bytes(decoded[-4:], "little"):
        raise ValueError("crc")
    return decoded[0], int.from_bytes(decoded[2:4], "little"), bytes(decoded[6:-4])


def normalize_mac(value: str) -> str | None:
    text = value.strip(" \t\r\n")
    if len(text) == 17:
        separator = text[2]
        if separator not in ":-" or any(text[index] != separator for index in (2, 5, 8, 11, 14)):
            return None
        text = text.replace(separator, "")
    if len(text) != 12 or not re.fullmatch(r"[0-9A-Fa-f]{12}", text):
        return None
    raw = bytes.fromhex(text)
    if raw == b"\0" * 6 or raw == b"\xff" * 6 or raw[0] & 1:
        return None
    return raw.hex().upper()


class StageFContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.data = json.loads(FIXTURE.read_text(encoding="utf-8"))

    def test_numeric_v1_changes_wire_version_only(self) -> None:
        raw = bytes.fromhex(self.data["wire"]["numeric_v1"])
        self.assertEqual(raw[:5], b"S011\x01")
        source = (ROOT / "src" / "serial.cpp").read_text(encoding="utf-8")
        self.assertIn("serialwrite(elementic::WIRE_V1)", source)
        self.assertIn("EEPROM.write(EP++, (byte)1)", source)

    def test_all_v2_vectors_have_canonical_crc_and_bounds(self) -> None:
        expected = {
            "v2_project_id": (0x02, 1),
            "v2_markers": (0x05, 1),
            "v2_empty_request": (0x01, 1),
        }
        for name, pair in expected.items():
            kind, sequence, payload = decode_v2(bytes.fromhex(self.data["wire"][name]))
            self.assertEqual((kind, sequence), pair)
            self.assertLessEqual(len(payload), 192)

    def test_project_identity_fixture_preserves_all_64_bits(self) -> None:
        _, _, payload = decode_v2(bytes.fromhex(self.data["wire"]["v2_project_id"]))
        self.assertEqual(payload[:6], bytes.fromhex("032300000800"))
        self.assertEqual(int.from_bytes(payload[6:], "little"), 0xD84A67B24F0113AC)

    def test_storage_schema2_crc_and_commit(self) -> None:
        raw = bytes.fromhex(self.data["storage"]["schema2"])
        payload_length = int.from_bytes(raw[12:16], "little")
        self.assertEqual(raw[:4], b"ECF2")
        self.assertEqual(raw[32 + payload_length], 0xA5)
        self.assertEqual(int.from_bytes(raw[28:32], "little"), crc32(raw[:28] + raw[32:32 + payload_length]))

    def test_provisioning_crc_and_commit(self) -> None:
        raw = bytes.fromhex(self.data["storage"]["provisioning"])
        self.assertEqual(len(raw), 53)
        self.assertEqual(raw[:4], b"EUP1")
        self.assertEqual(raw[-1], 0xA5)
        self.assertEqual(int.from_bytes(raw[48:52], "little"), crc32(raw[:48]))

    def test_mac_vectors(self) -> None:
        for value, expected in self.data["mac"]["valid"].items():
            self.assertEqual(normalize_mac(value), expected)
        for value in self.data["mac"]["invalid"]:
            self.assertIsNone(normalize_mac(value))


if __name__ == "__main__":
    unittest.main()
