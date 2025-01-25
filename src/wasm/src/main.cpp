/* ********************************************************************************************************* *
 *
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * The BSD-3-Clause license for this file can be found in the LICENSE.txt file included with this distribution
 * or at https://spdx.org/licenses/BSD-3-Clause.html#licenseText
 *
 * ********************************************************************************************************* */

#ifndef NATIVE_BUILD

#include <emscripten.h>
#include <emscripten/bind.h>

#define UINT32_EXTRACT(p, i) (p[0 + i] + (p[1 + i] << 8) + (p[2 + i] << 16) + (p[3 + i] << 24))
#define UINT16_EXTRACT(p, i) (p[0 + i] + (p[1 + i] << 8))
#define UINT8_EXTRACT(p, i) (p[0 + i])

#define UINT32_INSERT(v, p, i)                                                                                                             \
    {                                                                                                                                      \
        p[0 + i] = (v) & 0xff;                                                                                                             \
        p[1 + i] = (v >> 8) & 0xff;                                                                                                        \
        p[2 + i] = (v >> 16) & 0xff;                                                                                                       \
        p[3 + i] = (v >> 24) & 0xff;                                                                                                       \
    }
#define UINT16_INSERT(v, p, i)                                                                                                             \
    {                                                                                                                                      \
        p[0 + i] = (v) & 0xff;                                                                                                             \
        p[1 + i] = (v >> 8) & 0xff;                                                                                                        \
    }
#define UINT8_INSERT(v, p, i)                                                                                                              \
    {                                                                                                                                      \
        p[0 + i] = (v) & 0xff;                                                                                                             \
    }

#endif
#include "Logger.hpp"
#include <iomanip>

#ifndef NATIVE_BUILD

void stdoutHandler(const std::string &data) {
    emscripten::val::global("stdout")(emscripten::val(data));
}

void stderrHandler(const std::string &data) {
    emscripten::val::global("stderr")(emscripten::val(data));
}

namespace wix {
    static Logger cout(stdoutHandler);
    static Logger cerr(stderrHandler);
}  // namespace wix

emscripten::val getSupportedVendorIDs() {
    // read it from embedded probetable.csv or find different way
    // experimental: mculink/dap hardcoded for now: ARM-vid, NXP-vid
    int probeIDs[] = {0xD28, 0x1fc9};
    return emscripten::val(emscripten::typed_memory_view(2, probeIDs));
}

// read from probetable.csv
const int packetSize = 64;
uint32_t last_ap = 0xffffffff;

unsigned int rxBufferSize = packetSize;
uint8_t *rxBuffer = new uint8_t[rxBufferSize];

unsigned int txBufferSize = packetSize;
uint8_t *txBuffer = new uint8_t[txBufferSize];

inline void readProbeData() {
    auto input = emscripten::val::global("readData")().await();
    auto size = input["length"].as<unsigned int>();
    if (rxBufferSize < size) {
        delete[] rxBuffer;
        rxBuffer = new uint8_t[size];
        rxBufferSize = size;
    }
    auto memory = emscripten::val::module_property("HEAPU8")["buffer"];
    auto memoryView = input["constructor"].new_(memory, reinterpret_cast<uintptr_t>(rxBuffer), size);
    memoryView.call<void>("set", input);
}

inline void writeProbeData() {
    emscripten::val::global("writeData")(emscripten::val(emscripten::typed_memory_view(txBufferSize, txBuffer))).await();
}

inline void writeReadProbeData() {
    writeProbeData();
    readProbeData();
};

struct DAPCapabilities {
    bool swd;
    bool jtag;
    bool manchester;
    int swoTraceBufferSize;
    bool atomic;
    bool swoStreaming;
};

struct DAPFirmwareInfo {
    std::string firmwareVersion;
    std::string productId;
    int maxPacketCount;
    int maxPacketSize;
};

struct DAPInfo {
    std::string vendorId;
    std::string productId;
    std::string serialNo;
    std::string firmwareVer;
    std::string targetVendor;
    std::string targetName;
    std::string boardVendor;
    std::string boardName;
    std::string productFwVer;
    DAPCapabilities capabilities{};
    DAPFirmwareInfo firmwareInfo{};
};

std::string readInfoParam(int code) {
    txBuffer[0] = 0x00;
    txBuffer[1] = code;
    writeReadProbeData();
    if (rxBuffer[0] != 0x00) {
        throw std::runtime_error("HWIF transfer error");
    }
    if (rxBuffer[1] == 0) {
        return {"N/A"};
    } else if (rxBuffer[1] == 1) {
        return "";
    }
    return {reinterpret_cast<char *>(rxBuffer + 2), static_cast<std::size_t>(rxBuffer[1] - 1)};
}

int readInfoValue(int code) {
    txBuffer[0] = 0x00;
    txBuffer[1] = code;
    writeReadProbeData();
    int value = 0;
    switch (rxBuffer[1]) {
        case 0:
            break;
        case 1: {
            value = static_cast<int>(rxBuffer[2]);
            break;
        }
        case 2:
            value = UINT16_EXTRACT(rxBuffer, 2);
            break;
    }
    return value;
}

DAPFirmwareInfo getFirmwareInfo() {
    DAPFirmwareInfo firmwareInfo{};
    firmwareInfo.firmwareVersion = readInfoParam(0x04);
    firmwareInfo.productId = readInfoParam(0x02);
    firmwareInfo.maxPacketCount = readInfoValue(0xfe);
    firmwareInfo.maxPacketSize = readInfoValue(0xff);
    return firmwareInfo;
}

DAPCapabilities getProbeDAPCap() {
    DAPCapabilities capabilities{};
    txBuffer[0] = 0x00;
    txBuffer[1] = 0xf0;
    writeReadProbeData();
    //  uint8_t bytes = UINT8_EXTRACT(rxBuffer, 1);
    uint8_t info0 = UINT8_EXTRACT(rxBuffer, 2);
    capabilities.swd = (info0 & 0x01) != 0;
    capabilities.jtag = (info0 & 0x02) != 0;
    capabilities.manchester = (info0 & 0x08) != 0;
    capabilities.atomic = (info0 & 0x10) != 0;
    capabilities.swoStreaming = (info0 & 0x40) != 0;
    capabilities.swoTraceBufferSize = readInfoValue(0xFD);
    return capabilities;
}

DAPInfo getProbeDAPInfo() {
    DAPInfo info{};
    getFirmwareInfo();
    info.targetName = readInfoParam(0x06);
    info.targetVendor = readInfoParam(0x05);
    info.boardName = readInfoParam(0x08);
    info.boardVendor = readInfoParam(0x07);
    info.productId = readInfoParam(0x02);
    info.vendorId = readInfoParam(0x01);
    info.firmwareVer = readInfoParam(0x04);
    info.productFwVer = readInfoParam(0x09);
    info.serialNo = readInfoParam(0x03);
    info.capabilities = getProbeDAPCap();
    info.firmwareInfo = getFirmwareInfo();
    return info;
}

uint8_t swjPinStatus(uint8_t pin, uint8_t mask) {
    txBuffer[0] = 0x10;
    txBuffer[1] = pin;
    txBuffer[2] = mask;
    UINT32_INSERT(5000, txBuffer, 3);
    writeReadProbeData();
    if (rxBuffer[0] != 0x10) {
        throw std::runtime_error("HIF transfer error");
    }
    return rxBuffer[1];
}

void holdReset(int value) {
    uint8_t pin = 0x00;
    uint8_t mask = 0x00;
    uint8_t state = swjPinStatus(pin, mask);
    uint8_t resetBit = (1 << 7);
    int retries = 2;
    mask = 0x80;
    do {
        pin = state;
        if ((value & 1) == 0) {
            pin &= ~resetBit;
        } else {
            pin |= resetBit;
        }
        state = swjPinStatus(pin, mask);
        retries--;
    } while (retries && (((state & resetBit) >> 7) != value));
}

int SWJSequence(int bitcount, uint8_t *data) {
    txBuffer[0] = 0x12;
    txBuffer[1] = static_cast<uint8_t>(bitcount >= 256 ? 0 : bitcount);
    memcpy(&(txBuffer[2]), data, (bitcount + 7) / 8);
    writeReadProbeData();
    if (rxBuffer[0] != 0x12) {
        return 0x83;
    } else if (rxBuffer[1] != 0) {
        return 255;
    }
    return 0;
}

inline void WriteDPAP(int tap, uint8_t address, uint32_t data) {
    txBuffer[0] = 0x05;
    txBuffer[1] = tap;
    txBuffer[2] = 1;
    txBuffer[3] = address;

    UINT32_INSERT(data, txBuffer, 4);

    // wix::cout < "write: ";
    for (int i = 0; i < 8; ++i) {
        // wix::cout < std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(txBuffer[i]);
    }
    // wix::cout < " (" << data << ")" << std::endl;

    writeReadProbeData();
    if (rxBuffer[0] != 0x05) {
        throw std::runtime_error("HWIF transfer error");
    }
    if (rxBuffer[2] != 0x01) {
        if (rxBuffer[2] == 0x02) {
            throw std::runtime_error("WIRE ACK WAIT");
        } else {
            throw std::runtime_error("WIRE ACK FAULT");
        }
    } else if (rxBuffer[1] != 1) {
        throw std::runtime_error("Status fail");
    }
}

void WriteBlockDPAP(int tap, uint8_t address, uint32_t size, uint32_t *data) {
    uint32_t maxRepeatBlockPayload = (512 - 5) / 4;  // packet size 512
    uint32_t payloadPerReport;
    uint32_t index = 0;

    //  address &= 0x0d;  // write address

    if (size <= 0) {
        throw std::runtime_error("Invalid block data size 1");
    }
    while (size) {
        if (size >= maxRepeatBlockPayload) {
            payloadPerReport = maxRepeatBlockPayload;
        } else {
            payloadPerReport = size;
        }
        txBuffer[0] = 0x06;
        txBuffer[1] = tap;
        UINT16_INSERT(payloadPerReport, txBuffer, 2);
        txBuffer[4] = address;

        memcpy(&txBuffer[5], &data[index], payloadPerReport * sizeof(uint32_t));

        // wix::cout < "write-block: ";
        for (int i = 0; i < payloadPerReport + 4; ++i) {
            // wix::cout < std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(txBuffer[i]);
        }
        // wix::cout < std::endl;

        writeReadProbeData();

        if (rxBuffer[0] != 0x06) {
            throw std::runtime_error("HWIF transfer error");
        }
        if (rxBuffer[3] != 0x01) {  // ACK
            if (rxBuffer[3] == 0x02) {
                throw std::runtime_error("WIRE ACK WAIT");
            } else {
                throw std::runtime_error("WIRE ACK FAULT");
            }
        }
        if ((rxBuffer[1] != txBuffer[2]) || (rxBuffer[2] != txBuffer[3])) {
            throw std::runtime_error("Status fail");
        }
        // wix::cout < "response-block: ";
        for (int i = 0; i < UINT16_EXTRACT(rxBuffer, 1) + 1; ++i) {
            // wix::cout < std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(rxBuffer[i]);
        }
        // wix::cout < std::endl;
        size -= payloadPerReport;
        index += payloadPerReport;
    }
}

inline uint32_t ReadDPAP(int tap, uint8_t address) {
    txBuffer[0] = 0x05;
    txBuffer[1] = tap;
    txBuffer[2] = 1;
    txBuffer[3] = address;

    // wix::cout < "read-request: ";
    //  for (int i = 0; i < 4; ++i) {
    // wix::cout < std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(txBuffer[i]);
    //  }
    // wix::cout < std::endl;
    writeReadProbeData();
    if (rxBuffer[0] != 0x05) {
        throw std::runtime_error("HWIF transfer error");
    }
    if (rxBuffer[2] != 0x01) {
        if (rxBuffer[2] == 0x02) {
            throw std::runtime_error("WIRE ACK WAIT");
        } else {
            throw std::runtime_error("WIRE ACK FAULT");
        }
    } else if (rxBuffer[1] != 1) {
        throw std::runtime_error("Status fail");
    }
    // wix::cout < "read-data: ";
    for (int i = 0; i < 4; ++i) {
        // wix::cout < std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(rxBuffer[i]);
    }
    // wix::cout < std::endl;
    return UINT32_EXTRACT(rxBuffer, 3);
}

inline void ReadBlockDPAP(int tap, uint32_t address, uint32_t *size, uint32_t *data) {
    uint32_t maxRepeatBlockPayload = (512 - 5) / 4;  // packet size 512
    uint32_t payloadPerReport;
    uint32_t index = 0;
    if (*size <= 0) {
        throw std::runtime_error("Invalid block data size 2");
    }
    while (*size) {
        if (*size >= maxRepeatBlockPayload) {
            payloadPerReport = maxRepeatBlockPayload;
        } else {
            payloadPerReport = *size;
        }
        txBuffer[0] = 0x06;
        txBuffer[1] = tap;
        UINT16_INSERT(1, txBuffer, 2);
        txBuffer[4] = address;
        // wix::cout < "read-block: ";
        for (int i = 0; i < 5; ++i) {
            // wix::cout < std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(txBuffer[i]);
        }
        // wix::cout < std::endl;
        writeReadProbeData();
        if (rxBuffer[0] != 0x06) {
            throw std::runtime_error("HWIF transfer error");
        }
        if (rxBuffer[3] != 0x01) {  // ACK
            if (rxBuffer[3] == 0x02) {
                throw std::runtime_error("WIRE ACK WAIT");
            } else {
                throw std::runtime_error("WIRE ACK FAULT");
            }
        }
        if ((rxBuffer[1] != txBuffer[2]) || (rxBuffer[2] != txBuffer[3])) {
            throw std::runtime_error("Status fail");
        }
        memcpy(&data[index], &rxBuffer[4], payloadPerReport * sizeof(uint32_t));
        *size -= payloadPerReport;
        index += payloadPerReport;
        int size2 = UINT16_EXTRACT(rxBuffer, 1);
        // wix::cout < "read-block-data (" << size2 << "): ";
        for (int i = 0; i < size2 * sizeof(uint32_t); ++i) {
            // wix::cout < std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(rxBuffer[4 + i]);
        }
        // wix::cout < std::endl;
    }
}

std::map<std::pair<uint8_t, uint8_t>, uint8_t> REG_ADDR_TO_ID_MAP = {
    {{0, 0x0}, 0x0},
    {{0, 0x4}, 0x1},
    {{0, 0x8}, 0x2},
    {{0, 0xC}, 0x3},
    {{1, 0x0}, 0x4},
    {{1, 0x4}, 0x5},
    {{1, 0x8}, 0x6},
    {{1, 0xC}, 0x7}
};

inline uint32_t read_reg(int regID) {
    uint8_t request = 1 << 1;
    if (regID < 4) {
        request |= (0 << 0);
    } else {
        request |= (1 << 0);
    }
    request |= (regID % 4) << 2;
    return ReadDPAP(0, request);
}

inline void write_reg(uint8_t regID, uint32_t value) {
    uint8_t request = 0 << 1;
    if (regID < 4) {
        request |= (0 << 0);
    } else {
        request |= (1 << 0);
    }
    request |= (regID % 4) * 4;
    WriteDPAP(0, request, value);
}

inline void write_ap(uint8_t address, uint32_t data) {
    uint8_t apReg = REG_ADDR_TO_ID_MAP.at({0x01, address & 0x0000000c});  // 0x0000000c = A32
    write_reg(apReg, data);
}

inline uint32_t read_ap(uint8_t address) {
    uint8_t apReg = REG_ADDR_TO_ID_MAP.at({0x01, address & 0x0000000c});
    return read_reg(apReg);
}

inline uint32_t read_dp(uint8_t address) {
    uint8_t dpReg = REG_ADDR_TO_ID_MAP.at({0x00, address});
    return read_reg(dpReg);
}

inline void write_dp(uint8_t address, uint32_t data) {
    uint8_t dpReg = REG_ADDR_TO_ID_MAP.at({0x00, address});
    write_reg(dpReg, data);
}

uint32_t coresight_reg_read(bool accessPort, uint32_t address);
void coresight_reg_write(bool accessPort, uint32_t address, uint32_t data);

inline void select_ap(uint32_t address) {
    uint32_t addr = address & (0xFF000000 | 0x000000F0);
    if (last_ap != addr) {
        last_ap = addr;
        coresight_reg_write(false, 0x08, addr);
        wix::cout << "Selected AP: " << ((addr & 0xFF000000) >> 24) << ", Bank: " << std::hex << ((addr & 0x000000F0) >> 4) << std::endl;
    }
}

inline uint32_t coresight_reg_read(bool accessPort, uint32_t address) {
    uint32_t data = 0;
    if (accessPort) {
        select_ap(address);

        address = address & 0x0f;
        data = read_ap(address);
    } else {
        data = read_dp(address);
    }
    // wix::cout < "Coresight read " << (accessPort ? "AP" : "DP") << ", address: 0x" << std::hex << std::setw(8) << std::setfill('0')
    //         << static_cast<int>(address) << ", data: " << data << std::endl;
    return data;
}

inline void coresight_reg_write(bool accessPort, uint32_t address, uint32_t data) {
    // wix::cout < "Coresight write " << (accessPort ? "AP" : "DP") << ", address: 0x" << std::hex << std::setw(8) << std::setfill('0')
    //          << static_cast<int>(address) << ", data: " << data << std::endl;
    if (accessPort) {
        select_ap(address);

        address = address & 0x0f;
        write_ap(address, data);
    } else {
        write_dp(address, data);
    }
}

void WireConnect() {
    last_ap = 0xffffffff;
    txBuffer[0] = 0x02;  // Connect
    txBuffer[1] = 1;  // 1 = swd
    writeReadProbeData();
    if (rxBuffer[0] != 0x02) {
        throw std::runtime_error("HWIF transfer error");
    } else if (rxBuffer[1] != 1) {
        throw std::runtime_error("Status fail");
    }
    wix::cout << "SWD connected" << std::endl;

    // SWJ clock
    txBuffer[0] = 0x11;  // SWJ_Clock
    UINT32_INSERT(1000000, txBuffer, 1);  // INITIAL_WIRE_SPEED 10000000, HID - 1000000
    writeReadProbeData();
    if (rxBuffer[0] != 0x11) {
        throw std::runtime_error("HWIF transfer error");
    } else if (rxBuffer[1] != 0) {
        throw std::runtime_error("Status fail");
    }
    wix::cout << "SWD clock" << std::endl;

    txBuffer[0] = 0x04;
    txBuffer[1] = 0x02;  // idle_cycles
    UINT16_INSERT(0x0050, txBuffer, 2);
    UINT16_INSERT(0x0000, txBuffer, 4);
    writeReadProbeData();
    if (rxBuffer[0] != 0x04) {
        throw std::runtime_error("HWIF transfer error");
    } else if (rxBuffer[1] != 0) {
        throw std::runtime_error("Status fail");
    }
    wix::cout << "SWD transfere configured" << std::endl;

    // Transfer configure
    txBuffer[0] = 0x13;  // ?? wtf, it should be 0x04 for transfer configure, this is swd configure, transfere missing!
    txBuffer[1] = 0x00;
    writeReadProbeData();
    if (rxBuffer[0] != 0x13) {
        throw std::runtime_error("HWIF transfer error");
    } else if (rxBuffer[1] != 0) {
        throw std::runtime_error("Status fail");
    }
    wix::cout << "SWD configured" << std::endl;

    // line reset
    uint8_t data[32];
    int bitcount = sizeof(data) * 8;
    for (auto &index: data) {
        index = 0xff;
    }
    auto status = SWJSequence(bitcount, data);

    UINT16_INSERT(0xE79E, data, 0)
    status = SWJSequence(16, data);
    for (auto &index: data) {
        index = 0xff;
    }
    status = SWJSequence(bitcount, data);

    data[0] = 0;
    status = SWJSequence(8, data);

    if (!status) {
        //  status = CoreReadIdCode(0);
        //  wix::cout << "CoreID: " << status << std::endl;
        wix::cout << "status: " << status << std::endl;
    }

    uint32_t size = 25;
    uint32_t buff[25];
    ReadBlockDPAP(0, 0x02, &size, buff);
    auto idr = buff[0];
    wix::cout << "DPIDR(idr=" << std::dec << idr << ", partno=" << std::dec << static_cast<int>((idr & 0x0ff00000) >> 20)
              << ", version=" << static_cast<int>((idr & 0x0000f000) >> 12) << ", revision=" << static_cast<int>((idr & 0xf0000000) >> 28)
              << ", mindp=" << ((idr & 0x00010000) != 0 ? "true" : "false") << std::endl;

    size = 25;
    ReadBlockDPAP(0, 0x06, &size, buff);
    wix::cout << "Checked Sticky Errors: " << std::hex << std::setw(8) << std::setfill('0') << buff[0] << std::endl;
}

void WireDisconnect() {
    txBuffer[0] = 0x03;
    writeReadProbeData();
    if (rxBuffer[0] != 0x03) {
        throw std::runtime_error("HIF transfer error");
    } else if (rxBuffer[1] != 0) {
        throw std::runtime_error("Status error");
    }
}

void ProbeReset() {
    last_ap = 0xffffffff;
    txBuffer[0] = 0x81;  // ID_DAP_INFO: Vendor1
    txBuffer[1] = 0;  // 1 for ISP reset
    writeReadProbeData();
    if (rxBuffer[0] != 0x81) {
        throw std::runtime_error("HIF transfer error");
    } else if (rxBuffer[1] != 0) {
        throw std::runtime_error("REDLINK status fail");
    } else if (rxBuffer[2] <= 0) {
        throw std::runtime_error("REDLINK status fail 2");
    }
}

void Reset() {
    last_ap = 0xffffffff;
    wix::cout << "Reset target" << std::endl;
    holdReset(0);
    emscripten_sleep(50 - 3);  // -3ms for USB latency
    holdReset(1);
}

// @formatter:off
EMSCRIPTEN_BINDINGS(module) {
    /** General API **/
    emscripten::value_object<DAPCapabilities>("DAPCapabilities")
            .field("swd", &DAPCapabilities::swd)
            .field("jtag", &DAPCapabilities::jtag)
            .field("manchester", &DAPCapabilities::manchester)
            .field("swoTraceBufferSize", &DAPCapabilities::swoTraceBufferSize)
            .field("atomic", &DAPCapabilities::atomic)
            .field("swoStreaming", &DAPCapabilities::swoStreaming);

    emscripten::value_object<DAPFirmwareInfo>("getFirmwareInfo")
            .field("firmwareVersion", &DAPFirmwareInfo::firmwareVersion)
            .field("productId", &DAPFirmwareInfo::productId)
            .field("maxPacketCount", &DAPFirmwareInfo::maxPacketCount)
            .field("maxPacketSize", &DAPFirmwareInfo::maxPacketSize);

    emscripten::value_object<DAPInfo>("DAPInfo")
            .field("vendorId", &DAPInfo::vendorId)
            .field("productId", &DAPInfo::productId)
            .field("serialNo", &DAPInfo::serialNo)
            .field("firmwareVer", &DAPInfo::firmwareVer)
            .field("targetVendor", &DAPInfo::targetVendor)
            .field("targetName", &DAPInfo::targetName)
            .field("boardVendor", &DAPInfo::boardVendor)
            .field("boardName", &DAPInfo::boardName)
            .field("productFwVer", &DAPInfo::productFwVer)
            .field("capabilities", &DAPInfo::capabilities)
            .field("firmwareInfo", &DAPInfo::firmwareInfo);
    emscripten::function("getProbeDAPInfo", &getProbeDAPInfo);
    emscripten::function("getSupportedVendorIDs", &getSupportedVendorIDs);
    emscripten::function("reset", &Reset);
    emscripten::function("probeReset", &ProbeReset);

    /** Debugger API **/
    emscripten::function("connect", WireConnect);
    emscripten::function("disconnect", WireDisconnect);
    emscripten::function("coreSightRead", coresight_reg_read);
    emscripten::function("coreSightWrite", coresight_reg_write);
}
// @formatter:on
#else

int main() {
    // wix::cout < "Hello from dapper CLI interface! There is nothing to dapperize yet, please come back and try me later :-)" << std::endl;
}

#endif
