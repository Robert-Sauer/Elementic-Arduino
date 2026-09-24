#!/usr/bin/env python3
"""Checks frozen released v1 wire and EEPROM layouts before Stage F changes."""

from __future__ import annotations

import json
from pathlib import Path
import unittest


FIXTURE = Path(__file__).parents[1] / "fixtures" / "released-layouts.json"


def decode_released_eeprom(blob: bytes) -> list[tuple[int, int, int, bytes]]:
    """Decode only the layout emitted by the tagged v1 writer.

    Released system records always carry a two-byte little-endian payload even
    though their length byte is zero. Other fixed-width fields likewise use
    their known width when the historical length byte is zero.
    """
    if len(blob) < 3 or blob[0:2] != b"\x01\x01":
        raise ValueError("not released storage schema 1")
    pos = 2
    records: list[tuple[int, int, int, bytes]] = []
    while pos < len(blob) and blob[pos] != 0xFF:
        if pos + 4 > len(blob):
            raise ValueError("truncated record header")
        category, field, item, declared = blob[pos : pos + 4]
        pos += 4
        if category == 3:
            if item != 0 or declared != 0:
                raise ValueError("released system record must have zero id/length")
            width = 2
        elif declared:
            width = declared
        elif category == 0 and field in {0, 1, 2, 3, 4, 5, 8}:
            width = 1
        elif category == 1 and field == 0:
            width = 1
        else:
            raise ValueError("ambiguous zero-length record")
        if pos + width > len(blob):
            raise ValueError("truncated record payload")
        payload = blob[pos : pos + width]
        pos += width
        records.append((category, field, item, payload))
    if pos >= len(blob) or blob[pos] != 0xFF:
        raise ValueError("missing terminator")
    return records


class ReleasedFixtureTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.data = json.loads(FIXTURE.read_text(encoding="utf-8"))

    def test_all_released_full_syncs_use_ascii_version_one(self) -> None:
        full = self.data["wire_v1"][0]
        raw = bytes.fromhex(full["hex"])
        self.assertEqual(raw[:5], b"S0111")
        self.assertEqual(raw[4], full["version_byte"])

    def test_directional_integer_fixtures_remain_distinct(self) -> None:
        device = bytes.fromhex(self.data["wire_v1"][2]["hex"])
        app = bytes.fromhex(self.data["wire_v1"][3]["hex"])
        self.assertIn(b"\x12\x34", device)
        self.assertIn(b"\x34\x12", app)

    def test_released_system_record_has_two_payload_bytes(self) -> None:
        item = self.data["eeprom_v1"][0]
        records = decode_released_eeprom(bytes.fromhex(item["hex"]))
        self.assertEqual(records, [(3, 13, 0, b"\x13\x00")])
        self.assertEqual(int.from_bytes(records[0][3], "little"), 19)

    def test_reader_stays_aligned_after_system_record(self) -> None:
        item = self.data["eeprom_v1"][1]
        records = decode_released_eeprom(bytes.fromhex(item["hex"]))
        self.assertEqual(records[0], (3, 10, 0, b"\x01\x00"))
        self.assertEqual(records[1], (0, 0, 1, b"\x07"))

    def test_unreleased_four_byte_shape_is_rejected(self) -> None:
        raw = bytes.fromhex(self.data["unsupported_layouts"][0]["hex"])
        with self.assertRaises(ValueError):
            decode_released_eeprom(raw)


if __name__ == "__main__":
    unittest.main()
