/*
 * Copyright 2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * The BSD-3-Clause license for this file can be found in the LICENSE.txt file included with this distribution
 * or at https://spdx.org/licenses/BSD-3-Clause.html#licenseText
 */

import {after, afterEach, before, beforeEach, describe, it} from "mocha";
import {Buffer} from "node:buffer";
import {WebixDapper} from "../../js/mock-dapper.mjs";
import assert from "assert";
import {createRequire} from 'node:module';
import fs from "node:fs";
import http from "node:http";
import os from "node:os";
import path from 'node:path';
import process from "node:process";

globalThis.ddd = path.dirname(import.meta.url);
globalThis.__dirname = path.dirname(path.dirname(
    path.dirname(import.meta.url).substring(os.platform() === "win32" ? 8 : 7))) + "/build/build_wasm";
globalThis.require = createRequire(import.meta.url);

let server;
let browser;

let lastLog = "";

class MockDapper extends WebixDapper {
    inboundIndex = 0;
    outboundIndex = 0;
    traceData = {};
    writeData = [];

    constructor() {
        super();
        this.setStdoutHandler(() => {
            // dummy
        });
        this.setStderrHandler(() => {
            // dummy
        });

        globalThis.fcn_void_void = () => {
            lastLog = "js_fcn_void_void called";
        };
        globalThis.fcn_async_void_void = async () => {
            // await new Promise((resolve)=>setTimeout(resolve,100));
            lastLog = "js_fcn_void_void called";
        };
        globalThis.fcn_int_int = (a1) => {
            return a1 * 2;
        };
        globalThis.fcn_async_int_int = async (a1) => {
            // await new Promise((resolve)=>setTimeout(resolve,100));
            return a1 * 2;
        };
        globalThis.fcn_str_str = (s1) => {
            return s1 + ",_js";
        };
        globalThis.test_cb_read_data = async () => {
            return new Buffer("test mock read data: nothing special");
        };
        globalThis.test_cb_write_data = async (data) => {
            const reg = "[\\x00]";
            lastLog = "rec: " + Buffer.from(data).toString("utf8").replace(new RegExp(reg, "g"), '');
        };
        globalThis.fcn_throw_exception = () => {
            throw new Error("Exception from JS handler");
        };
    }

    async Open(_device) {
        this.setReadDataHandler(() => {
            return new Uint8Array(this.traceData.inbound[this.inboundIndex++]);
        });
        this.setWriteDataHandler((data) => {
            this.outboundIndex++;
            this.writeData.push([...data]);
        });
    }
}

describe("test-wasm-api", function () {
    let dapper;
    let stdout;
    let originalLog;

    const mockLog = (...args) => {
        stdout += args[0];
    };

    it("test_void_void", async () => {
        process.stdout.write = mockLog;
        try {
            await dapper.test_void_void();
        } catch (e) {
            assert.fail(e);
        } finally {
            process.stdout.write = originalLog;
        }
        assert.deepEqual(stdout.trim(), "void_void");
    });

    it("test_int_void", async () => {
        assert.deepEqual(await dapper.test_int_void(), 666);
    });

    it("test_int_int", async () => {
        assert.deepEqual(await dapper.test_int_int(12), 24);
    });

    it("test_bool_void", async () => {
        assert.deepEqual(await dapper.test_bool_void(), true);
    });

    it("test_bool_bool", async () => {
        assert.deepEqual(await dapper.test_bool_bool(true), false);
    });

    it("test_float_void", async () => {
        assert.deepEqual(Math.round((await dapper.test_float_void() * 1000000)) / 1000000, 1.23);
    });

    it("test_float_float", async () => {
        assert.deepEqual(Math.round(await dapper.test_float_float(0.56) * 1000000) / 1000000, 0.56 * 2);
    });

    it("test_str_void", async () => {
        assert.deepEqual(await dapper.test_str_void(), "hello there");
    });

    it("test_str_str", async () => {
        assert.deepEqual(await dapper.test_str_str("eddie"), "inner: eddie");
    });

    it("test_struct_void", async () => {
        assert.deepEqual(await dapper.test_struct_void(), {
            'struct_1': {
                'struct_bool': true,
                'struct_float': 12.130000114440918,
                'struct_int': 11,
                'struct_string': 'struct 1 string'
            },
            'struct_2': {
                'struct_bool': false,
                'struct_float': 54.38999938964844,
                'struct_int': 44,
                'struct_string': 'struct 2 string'
            },
            'struct_string': 'struct parent'
        });
    });

    it("test_fcn_void_void", async () => {
        assert.doesNotThrow(async () => {
            await dapper.test_fcn_void_void();
        });
    });

    it("test_fcn_async_void_void", async () => {
        assert.doesNotThrow(async () => {
            await dapper.test_fcn_async_void_void();
        });
    });

    it("test_fcn_int_int", async () => {
        assert.deepEqual(await dapper.test_fcn_int_int(7), 7 * 2 * 2 * 2);
    });

    it("test_fcn_async_int_int", async () => {
        assert.deepEqual(await dapper.test_fcn_async_int_int(7), 7 * 2 * 2 * 2);
    });

    it("test_fcn_str_str", async () => {
        assert.deepEqual(await dapper.test_fcn_str_str("hello"), "hello,tester,before,_js,after,tester-done");
    });

    it("test_read_data", async () => {
        assert.deepEqual(await dapper.test_read_data(), "test mock read data: nothing special");
    });

    it("test_write_data", async () => {
        await dapper.test_write_data();
        assert.deepEqual(lastLog, "rec: hello from wasm write");
    });

    it("test_throw_exception", async () => {
        let error;
        try {
            await dapper.test_throw_exception();
        } catch (e) {
            error = e;
        }
        assert.deepEqual(error.message, "Exception from wasm");
    });

    it("test_fcn_throw_exception", async () => {
        let error;
        try {
            await dapper.test_fcn_throw_exception();
        } catch (e) {
            error = e;
        }
        assert.deepEqual(error.message, "Exception from JS handler");
    });

    beforeEach(() => {
        stdout = "";
        originalLog = process.stdout.write;
    });

    afterEach(async () => {
        process.stdout.write = originalLog;
        if (browser) {
            await browser.close();
        }
    });

    before(async () => {
        if (!fs.existsSync("./build/WASM-RUN")) {
            fs.mkdirSync("./build/WASM-RUN", {recursive: true});
        }

        server = http.createServer(($request, $response) => {
            if ($request.url === "/index.html") {
                $response.writeHeader(200, {"Content-Type": "text/html"});
                $response.write(fs.readFileSync(process.cwd() + "/build" + $request.url));
                $response.end();
            } else if ($request.url.startsWith("/") && ($request.url.endsWith(".js") || $request.url.endsWith(".mjs"))) {
                $response.writeHeader(200, {"Content-Type": "application/javascript"});
                $response.write(fs.readFileSync(process.cwd() + "/build" + $request.url));
                $response.end();
            } else if ($request.url.startsWith("/") && $request.url.endsWith(".wasm")) {
                $response.writeHeader(200, {"Content-Type": "application/wasm"});
                $response.write(fs.readFileSync(process.cwd() + "/build" + $request.url));
                $response.end();
            } else if ($request.url.startsWith("/") && $request.url.endsWith(".jpg")) {
                $response.writeHeader(200, {"Content-Type": "image/jpeg"});
                $response.write(fs.readFileSync(process.cwd() + "/build" + $request.url));
                $response.end();
            }
        });
        server.listen(31231);

        dapper = new MockDapper();
        await dapper.Init();
    });

    after(async () => {
        server.close();
    });
});
