#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# * ********************************************************************************************************* *
# *
# * Copyright 2024 NXP
# *
# * SPDX-License-Identifier: BSD-3-Clause
# * The BSD-3-Clause license for this file can be found in the LICENSE.txt file included with this distribution
# * or at https://spdx.org/licenses/BSD-3-Clause.html#licenseText
# *
# * ********************************************************************************************************* *
import json
import os.path
import unittest

from python.mock_dapper import MockDapper


class DapperIntegrationTest(unittest.TestCase):

    def test_supported_vendor_ids(self) -> None:
        rv = self.dapper.supported_vendor_ids()
        self.assertEqual([0xD28, 0x1FC9], rv)

    def test_mcxa153(self) -> None:
        file_path = os.path.abspath(
            os.path.dirname(__file__) + "/../../resources/traces/trace_mcxa153.json"
        )
        with open(file_path, "r") as file:
            data = json.load(file)

        self.assertIsInstance(data.get("inbound"), list, "Expected 'inbound' to be a list")
        self.assertIsInstance(data.get("outbound"), list, "Expected 'outbound' to be a list")

        self.dapper.trace_data = data
        self.dapper.init()
        self.dapper.open(None)

        rv = self.dapper.get_probe_dap_info()
        self.assertEqual(
            {
                "board_name": "FRDM-MCXA153",
                "board_vendor": "NXP",
                "firmware_ver": "2.1.1",
                "product_fw_ver": "3.128",
                "product_id": "MCU-Link CMSIS-DAP V3.128",
                "serial_no": "WAZHTY5YQ2IUB",
                "target_name": "MCXA153VLH",
                "target_vendor": "NXP",
                "vendor_id": "NXP Semiconductors",
            },
            rv.__dict__,
        )

        try:
            self.dapper.dpap_test(self.dapper)
        except Exception as e:
            self.fail(f"Test failed with: {e}")

        self.assertEqual(self.dapper.outbound_index, len(data["outbound"]))
        self.assertEqual(self.dapper.write_data_trace, data["outbound"])

    def test_mcxn947(self) -> None:
        file_path = os.path.abspath(
            os.path.dirname(__file__) + "/../../resources/traces/trace_mcxn947.json"
        )
        with open(file_path, "r") as file:
            data = json.load(file)

        self.assertIsInstance(data.get("inbound"), list, "Expected 'inbound' to be a list")
        self.assertIsInstance(data.get("outbound"), list, "Expected 'outbound' to be a list")

        self.dapper.trace_data = data
        self.dapper.init()
        self.dapper.open(None)

        rv = self.dapper.get_probe_dap_info()
        self.assertEqual(
            {
                "board_name": "FRDM-MCXN947",
                "board_vendor": "NXP",
                "firmware_ver": "2.1.1",
                "product_fw_ver": "3.128",
                "product_id": "MCU-Link CMSIS-DAP V3.128",
                "serial_no": "VNNTUE4SZEZCJ",
                "target_name": "MCXN947VDFT",
                "target_vendor": "NXP",
                "vendor_id": "NXP Semiconductors",
            },
            rv.__dict__,
        )

        try:
            self.dapper.dpap_test(self.dapper)
        except Exception as e:
            self.fail(f"Test failed with: {e}")

        self.assertEqual(self.dapper.outbound_index, len(data["outbound"]))
        self.assertEqual(self.dapper.write_data_trace, data["outbound"])

    def test_rt1060(self) -> None:
        file_path = os.path.abspath(
            os.path.dirname(__file__) + "/../../resources/traces/trace_rt1060_evk.json"
        )
        with open(file_path, "r") as file:
            data = json.load(file)

        self.assertIsInstance(data.get("inbound"), list, "Expected 'inbound' to be a list")
        self.assertIsInstance(data.get("outbound"), list, "Expected 'outbound' to be a list")

        self.dapper.trace_data = data
        self.dapper.init()
        self.dapper.open(None)

        rv = self.dapper.get_probe_dap_info()
        self.assertEqual(
            {
                "board_name": "N/A",
                "board_vendor": "N/A",
                "firmware_ver": "1.10",
                "product_fw_ver": "N/A",
                "product_id": "N/A",
                "serial_no": "N/A",
                "target_name": "N/A",
                "target_vendor": "N/A",
                "vendor_id": "N/A",
            },
            rv.__dict__,
        )

        try:
            self.dapper.dpap_test(self.dapper)
        except Exception as e:
            self.fail(f"Test failed with: {e}")

        self.assertEqual(self.dapper.outbound_index, len(data["outbound"]))
        self.assertEqual(self.dapper.write_data_trace, data["outbound"])

    @classmethod
    def setUpClass(cls) -> None:
        super().setUpClass()

    def setUp(self) -> None:
        super().setUp()
        self.dapper = MockDapper()


if __name__ == "__main__":
    unittest.main()
