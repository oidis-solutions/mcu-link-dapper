/*
 * Copyright 2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * The BSD-3-Clause license for this file can be found in the LICENSE.txt file included with this distribution
 * or at https://spdx.org/licenses/BSD-3-Clause.html#licenseText
 */
import loadWasm from "../../build/build_wasm/test-dapper-wasm.js";

let stdoutHandler = () => {
    // override me
};

let stderrHandler = () => {
    // override me
};

let readDataHandler = async () => {
    // override me
};

let writeDataHandler = async () => {
    // override me
};

async function readData() {
    return readDataHandler();
}

async function writeData(data) {
    await writeDataHandler(data);
}

function stdout(data) {
    stdoutHandler(data);
}

function stderr(data) {
    stderrHandler(data);
}

function extendBuffer(data, packetSize) {
    function isView(source) {
        return source.buffer !== undefined;
    }

    const arrayBuffer = isView(data) ? data.buffer : data;
    if (arrayBuffer.byteLength === packetSize) {
        return isView(data) ? data : new Uint8Array(data);
    }
    const length = Math.min(arrayBuffer.byteLength, packetSize);
    const result = new Uint8Array(length);
    result.set(new Uint8Array(arrayBuffer));

    return result;
}

export class ProbeInfo {
    vendorId;
    productId;
    serialNo;
    firmwareVer;
    targetVendor;
    targetName;
    boardVendor;
    boardName;
    productFwVer;
}

export class WebixDapper {
    module = null;
    device = null;

    interfaceNumber = undefined;
    endpointIn = null;
    endpointOut = null;
    packetSize = 64;

    alwaysControlTransfer = false;
    trace = false;
    traceData = {inbound: [], outbound: []};

    constructor() {
        this.setStdoutHandler(null);
        this.setStderrHandler(null);
        globalThis.readData = readData;
        globalThis.writeData = writeData;
        globalThis.stdout = stdout;
        globalThis.stderr = stderr;
    }

    /**
     * Register handler for stdout data processing, default is console.log(..)
     * @param handler Specify string processing function.
     */
    setStdoutHandler(handler) {
        if (typeof handler === "function") {
            stdoutHandler = handler;
        } else {
            stdoutHandler = (data) => {
                console.log(data);
            };
        }
    }

    /**
     * Register handler for stderr data processing, default is console.error(..)
     * @param handler Specify string processing function.
     */
    setStderrHandler(handler) {
        if (typeof handler === "function") {
            stderrHandler = handler;
        } else {
            stderrHandler = (data) => {
                console.error(data);
            };
        }
    }

    setReadDataHandler(handler) {
        if (typeof handler === "function") {
            readDataHandler = handler;
        } else {
            readDataHandler = async () => {
                if (this.interfaceNumber === undefined) {
                    throw new Error("Device needs to be opened first.");
                }
                let result;
                if (this.endpointIn) {
                    result = await this.device.transferIn(
                        this.endpointIn.endpointNumber,
                        this.packetSize
                    );
                } else {
                    result = await this.device.controlTransferIn(
                        {
                            requestType: "class",
                            recipient: "interface",
                            request: 0x01,
                            value: 0x100,
                            index: this.interfaceNumber
                        },
                        this.packetSize
                    );
                }
                const data = new Uint8Array(result.data.buffer);
                if (this.trace) {
                    this.traceData.inbound.push(data);
                }
                return data;
            };
        }
    }

    setWriteDataHandler(handler) {
        if (typeof handler === "function") {
            writeDataHandler = handler;
        } else {
            writeDataHandler = async (data) => {
                if (this.interfaceNumber === undefined) {
                    throw new Error("Device needs to be opened first.");
                }

                // TODO(mkelnar) data from memory view contains complete HEAP8 data, so copy only array content
                //  can we replace this copy by some parametrisation?
                const dataBuff = new Uint8Array(data);
                if (this.trace) {
                    this.traceData.outbound.push(dataBuff);
                }
                const buffer = extendBuffer(dataBuff, this.packetSize);

                if (this.endpointOut) {
                    await this.device.transferOut(
                        this.endpointOut.endpointNumber,
                        buffer
                    );
                } else {
                    await this.device.controlTransferOut(
                        {
                            requestType: 'class',
                            recipient: 'interface',
                            request: 0x09,
                            value: 0x200,
                            index: this.interfaceNumber
                        },
                        buffer
                    );
                }
            };
        }
    }

    /**
     * Call init before any other operation! Main purpose is to instantiate WASM module
     * @return {Promise<void>}
     */
    async Init() {
        if (!this.module) {
            this.module = await loadWasm();
        }
    }

    async test_void_void() {
        return this.module.test_void_void();
    }

    async test_int_void() {
        return this.module.test_int_void();
    }

    async test_int_int(arg0) {
        return this.module.test_int_int(arg0);
    }

    async test_bool_void() {
        return this.module.test_bool_void();
    }

    async test_bool_bool(arg0) {
        return this.module.test_bool_bool(arg0);
    }

    async test_float_void() {
        return this.module.test_float_void();
    }

    async test_float_float(arg0) {
        return this.module.test_float_float(arg0);
    }

    async test_str_void() {
        return this.module.test_str_void();
    }

    async test_str_str(arg0) {
        return this.module.test_str_str(arg0);
    }

    async test_struct_void() {
        return this.module.test_struct_void();
    }

    async test_fcn_void_void() {
        return this.module.test_fcn_void_void();
    }

    async test_fcn_async_void_void() {
        return this.module.test_fcn_async_void_void();
    }

    async test_fcn_int_int(arg0) {
        return this.module.test_fcn_int_int(arg0);
    }

    async test_fcn_async_int_int(arg0) {
        return this.module.test_fcn_async_int_int(arg0);
    }

    async test_fcn_str_str(arg0) {
        return this.module.test_fcn_str_str(arg0);
    }

    async test_read_data() {
        return this.module.test_read_data();
    }

    async test_write_data() {
        return this.module.test_write_data();
    }

    async test_throw_exception() {
        return this.module.test_throw_exception();
    }

    async test_fcn_throw_exception() {
        return this.module.test_fcn_throw_exception();
    }
}
