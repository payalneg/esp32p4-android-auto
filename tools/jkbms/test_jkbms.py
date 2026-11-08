"""Decoder tests using frames captured from real JK-BMS units.

Sources: syssi/esphome-jk-bms, components/jk_bms_ble/jk_bms_ble.cpp (MIT).
Run with:  python -m unittest -v
"""

import unittest

import jkbms

# JK-B2A24S15P, 16 cells, 24S layout, SOC 84 %, 68.494/81.000 Ah, 53.251 V.
CELL_INFO_24S = bytes.fromhex(
    "55AAEB90028CFF0C010D010DFF0C010D010DFF0C010D010D010D010DFF0C"
    "010D010D010D010D00000000000000000000000000000000FFFF0000000D"
    "000000009D0196018C018701840184018301840185018101830186018201"
    "8201830185010000000000000000000000000000000000000000000003D0"
    "00000000000000000000BE00BF00D2000000000000548E0B0100683C0100"
    "000000003D04000064007904CA0310000101AA0600000000000000000000"
    "0000070001000000D50200000000AED63B400000000058AAFDFF00000001"
    "00020000ECE64F0000000000000000000000000000000000000000000000"
    "000000000000000000000000000000000000000000000000000000000000"
    "0000000000000000000000000000000000000000000000000000000000CD"
)

DEVICE_INFO = bytes.fromhex(
    "55AAEB90039F4A4B2D4232413234533135500000000031302E5857000000"
    "31302E303700000040AF0100060000004A4B2D4232413234533135500000"
    "000031323334000000000000000000000000323230343037000032303231"
    "363032303936003030303000496E70757420557365726461746100003132"
    "333435360000000000000000000000000000000000000000000000000000"
    "000000000000000000000000000000000000000000000000000000000000"
    "000000000000000000000000000000000000000000000000000000000000"
    "000000000000000000000000000000000000000000000000000000000000"
    "000000000000000000000000000000000000000000000000000000000000"
    "000000000000000000000000000000000000000000000000000000000065"
)


class DecodeTests(unittest.TestCase):
    def test_24s_cell_info(self):
        st = jkbms.parse_cell_info(CELL_INFO_24S)
        self.assertEqual(st.variant, "JK02_24S")
        self.assertAlmostEqual(st.voltage, 53.251)
        self.assertAlmostEqual(st.soc, 84)
        self.assertAlmostEqual(st.remaining_ah, 68.494)
        self.assertAlmostEqual(st.full_ah, 81.0)
        self.assertEqual(st.cycles, 0)
        self.assertEqual(st.soh, 100)
        self.assertAlmostEqual(st.temp1, 19.0)
        self.assertAlmostEqual(st.temp2, 19.1)
        self.assertAlmostEqual(st.temp_mos, 21.0)
        self.assertEqual(len(st.cells), 24)
        self.assertAlmostEqual(st.cells[0], 3.327)
        self.assertAlmostEqual(st.cell_min, 3.327)
        self.assertAlmostEqual(st.cell_max, 3.329)
        self.assertAlmostEqual(st.cell_delta, 0.002)
        self.assertTrue(st.charging_mos)
        self.assertTrue(st.discharging_mos)

    def test_32s_layout(self):
        frame = bytearray(300)
        frame[0:5] = bytes([0x55, 0xAA, 0xEB, 0x90, 0x02])
        for i in range(32):
            mv = 3300 + i
            frame[6 + 2 * i : 8 + 2 * i] = mv.to_bytes(2, "little")
        frame[144:146] = (305).to_bytes(2, "little", signed=True)          # MOS temp
        frame[150:154] = (53300).to_bytes(4, "little")                     # voltage
        frame[158:162] = (-1234).to_bytes(4, "little", signed=True)        # current
        frame[162:164] = (250).to_bytes(2, "little", signed=True)          # T1
        frame[164:166] = (260).to_bytes(2, "little", signed=True)          # T2
        frame[173] = 77                                                    # SOC
        frame[174:178] = (50500).to_bytes(4, "little")                     # remaining
        frame[178:182] = (60000).to_bytes(4, "little")                     # full
        frame[182:186] = (12).to_bytes(4, "little")                        # cycles
        frame[190] = 99                                                    # SOH
        frame[198] = 1                                                     # charge MOS
        frame[199] = 1                                                     # discharge MOS
        frame[299] = jkbms.checksum(frame[:299])

        self.assertTrue(jkbms.frame_is_valid(bytes(frame)))
        st = jkbms.parse_cell_info(bytes(frame))
        self.assertEqual(st.variant, "JK02_32S")
        self.assertEqual(len(st.cells), 32)
        self.assertAlmostEqual(st.voltage, 53.3)
        self.assertAlmostEqual(st.current, -1.234)
        self.assertAlmostEqual(st.soc, 77)
        self.assertAlmostEqual(st.remaining_ah, 50.5)
        self.assertAlmostEqual(st.full_ah, 60.0)
        self.assertEqual(st.cycles, 12)
        self.assertEqual(st.soh, 99)
        self.assertAlmostEqual(st.temp_mos, 30.5)
        self.assertAlmostEqual(st.temp1, 25.0)
        self.assertAlmostEqual(st.temp2, 26.0)

    def test_device_info(self):
        info = jkbms.parse_device_info(DEVICE_INFO)
        self.assertEqual(info["model"], "JK-B2A24S15P")
        self.assertEqual(info["hardware_version"], "10.XW")
        self.assertEqual(info["software_version"], "10.07")
        self.assertEqual(info["uptime_s"], 110400)
        self.assertEqual(info["power_on_count"], 6)
        self.assertEqual(info["manufacturing_date"], "220407")

    def test_assembler_handles_split_and_garbage(self):
        fa = jkbms.FrameAssembler()
        blob = b"\x00" * 7 + CELL_INFO_24S
        out = []
        for off in range(0, len(blob), 128):
            out.extend(fa.feed(blob[off : off + 128]))
        self.assertEqual(len(out), 1)
        self.assertEqual(out[0], CELL_INFO_24S)

    def test_bad_crc_is_dropped(self):
        broken = bytearray(CELL_INFO_24S)
        broken[100] ^= 0xFF
        fa = jkbms.FrameAssembler()
        self.assertEqual(fa.feed(bytes(broken)), [])

    def test_request_frames(self):
        req = jkbms.build_request(jkbms.CMD_CELL_INFO)
        self.assertEqual(len(req), 20)
        self.assertEqual(req[:5], b"\xAA\x55\x90\xEB\x96")
        self.assertEqual(req[19], 0x10)
        self.assertEqual(
            jkbms.build_request(jkbms.CMD_DEVICE_INFO)[19], 0x11
        )

    def test_unit_checksum(self):
        self.assertEqual(jkbms.checksum(b"\x01\x02\x03"), 6)
        self.assertEqual(jkbms.checksum(b"\xff\xff"), 0xFE)


if __name__ == "__main__":
    unittest.main()
