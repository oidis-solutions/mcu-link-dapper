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
import io
import os.path
import unittest
from unittest.mock import patch

from python.mock_dapper import MockDapper


class DapperAPITest(unittest.TestCase):

    def __init__(self, methodName: str = "runTest"):
        super().__init__(methodName)
        self.dapper: MockDapper

    def test_void_void(self) -> None:
        with patch("sys.stdout", new=io.StringIO()) as mock_stdout:
            self.dapper.test_void_void()
            self.assertEqual("void_void\n", mock_stdout.getvalue())

    def test_int_void(self) -> None:
        rv = self.dapper.test_int_void()
        self.assertEqual(rv, 666)

    def test_int_int(self) -> None:
        rv = self.dapper.test_int_int(12)
        self.assertEqual(rv, 24)

    def test_bool_void(self) -> None:
        rv = self.dapper.test_bool_void()
        self.assertEqual(rv, True)

    def test_bool_bool(self) -> None:
        rv = self.dapper.test_bool_bool(True)
        self.assertEqual(rv, False)

    def test_float_void(self) -> None:
        rv = self.dapper.test_float_void()
        self.assertEqual(round(rv, 6), 1.230000)

    def test_float_float(self) -> None:
        rv = self.dapper.test_float_float(0.56)
        self.assertEqual(round(rv, 6), 0.56 * 2)

    def test_str_void(self) -> None:
        rv = self.dapper.test_str_void()
        self.assertEqual(rv, "hello there")

    def test_str_str(self) -> None:
        rv = self.dapper.test_str_str("eddie")
        self.assertEqual(rv, "inner: eddie")

    def test_struct_void(self) -> None:
        rv = self.dapper.test_struct_void()
        self.assertEqual(
            rv,
            {
                "struct_1": {
                    "struct_bool": True,
                    "struct_float": 12.130000114440918,
                    "struct_int": 11,
                    "struct_string": "struct 1 string",
                },
                "struct_2": {
                    "struct_bool": False,
                    "struct_float": 54.38999938964844,
                    "struct_int": 44,
                    "struct_string": "struct 2 string",
                },
                "struct_string": "struct parent",
            },
        )

    def test_fcn_void_void(self) -> None:
        with patch("sys.stdout", new=io.StringIO()) as mock_stdout:
            self.dapper.test_fcn_void_void()
            self.assertEqual("py_fcn_void_void called\n", mock_stdout.getvalue())

    def test_fcn_async_void_void(self) -> None:
        with patch("sys.stdout", new=io.StringIO()) as mock_stdout:
            self.dapper.test_fcn_async_void_void()
            self.assertEqual("py_fcn_async_void_void called\n", mock_stdout.getvalue())

    def test_fcn_int_int(self) -> None:
        with patch("sys.stdout", new=io.StringIO()) as mock_stdout:
            rv = self.dapper.test_fcn_int_int(7)
            self.assertEqual("py_fcn_int_int called with: 14\n", mock_stdout.getvalue())
            self.assertEqual(rv, 7 * 2 * 2 * 2)

    def test_fcn_async_int_int(self) -> None:
        with patch("sys.stdout", new=io.StringIO()) as mock_stdout:
            rv = self.dapper.test_fcn_async_int_int(7)
            self.assertEqual("py_fcn_async_int_int called with: 14\n", mock_stdout.getvalue())
            self.assertEqual(rv, 7 * 2 * 2 * 2)

    def test_fcn_str_str(self) -> None:
        with patch("sys.stdout", new=io.StringIO()) as mock_stdout:
            rv = self.dapper.test_fcn_str_str("hello")
            self.assertEqual(
                "py_fcn_str_str called with: hello,tester,before\n", mock_stdout.getvalue()
            )
            self.assertEqual("hello,tester,before,_python,after,tester-done", rv)

    def test_read_data(self) -> None:
        rv = self.dapper.test_read_data()
        self.assertEqual("test mock read data: nothing special", rv)

    def test_write_data(self) -> None:
        with patch("sys.stdout", new=io.StringIO()) as mock_stdout:
            self.dapper.test_write_data()
            self.assertEqual("rec: b'hello from wasm write'\n", mock_stdout.getvalue())

    def test_throw_exception(self) -> None:
        err = None
        try:
            self.dapper.test_throw_exception()
        except Exception as e:
            err = e
        self.assertEqual(f"{err}", "Exception from wasm")

    def test_fcn_throw_exception(self) -> None:
        err = None
        try:
            self.dapper.test_fcn_throw_exception()
        except Exception as e:
            err = e
        self.assertEqual(f"{err}", "Exception from Python handler")

    @classmethod
    def setUpClass(cls) -> None:
        super().setUpClass()
        cls.dapper = MockDapper(
            os.path.abspath(
                os.path.dirname(__file__) + "/../../../build/build_wasm/test-dapper-wasm.wasm"
            )
        )


if __name__ == "__main__":
    unittest.main()
