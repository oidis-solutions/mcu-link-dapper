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

const int packetSize = 64;

unsigned int rxBufferSize = packetSize;
uint8_t *rxBuffer = new uint8_t[rxBufferSize];

unsigned int txBufferSize = packetSize;
uint8_t *txBuffer = new uint8_t[txBufferSize];

void readProbeData() {
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

void writeProbeData() {
    emscripten::val::global("writeData")(emscripten::val(emscripten::typed_memory_view(txBufferSize, txBuffer))).await();
}

void writeReadProbeData() {
    writeProbeData();
    readProbeData();
};

struct Struct_A {
    bool struct_bool;
    int struct_int;
    float struct_float;
    std::string struct_string;
};

struct Struct_B {
    Struct_A struct_1;
    Struct_A struct_2;
    std::string struct_string;
};

void test_void_void() {
    printf("void_void\n");
}

int test_int_void() {
    return 666;
}

int test_int_int(int i) {
    return 2 * i;
}

bool test_bool_void() {
    return true;
}

bool test_bool_bool(bool i) {
    return !i;
}

float test_float_void() {
    return 1.23;
}

float test_float_float(float i) {
    return 2 * i;
}

std::string test_str_void() {
    return "hello there";
}

std::string test_str_str(const std::string &s) {
    return "inner: " + s;
}

Struct_B test_struct_void() {
    Struct_B info{};
    info.struct_string = "struct parent";
    info.struct_1 = {};
    info.struct_1.struct_bool = true;
    info.struct_1.struct_int = 11;
    info.struct_1.struct_float = 12.13;
    info.struct_1.struct_string = "struct 1 string";

    info.struct_2 = {};
    info.struct_2.struct_bool = false;
    info.struct_2.struct_int = 44;
    info.struct_2.struct_float = 54.39;
    info.struct_2.struct_string = "struct 2 string";

    return info;
}

void fcn_void_void() {
    emscripten::val::global("fcn_void_void")();
}
void test_fcn_void_void() {
    fcn_void_void();
}

void fcn_async_void_void() {
    emscripten::val::global("fcn_async_void_void")().await();
}
void test_fcn_async_void_void() {
    fcn_async_void_void();
}

int fcn_int_int(int a1) {
    auto retVal = emscripten::val::global("fcn_int_int")(a1 * 2);
    return retVal.as<int>() * 2;
}
int test_fcn_int_int(int a1) {
    return fcn_int_int(a1);
}
int fcn_async_int_int(int a1) {
    auto retVal = emscripten::val::global("fcn_async_int_int")(a1 * 2).await();
    return retVal.as<int>() * 2;
}
int test_fcn_async_int_int(int a1) {
    return fcn_async_int_int(a1);
}

std::string fcn_str_str(const std::string &s1) {
    auto retVal = emscripten::val::global("fcn_str_str")(s1 + ",before");
    return retVal.as<std::string>() + ",after";
}
std::string test_fcn_str_str(const std::string &a1) {
    return fcn_str_str(a1 + ",tester") + ",tester-done";
}

std::string test_read_data() {
    auto input = emscripten::val::global("test_cb_read_data")().await();
    auto size = input["length"].as<unsigned int>();
    if (rxBufferSize < size) {
        delete[] rxBuffer;
        rxBuffer = new uint8_t[size];
        rxBufferSize = size;
    }
    auto memory = emscripten::val::module_property("HEAPU8")["buffer"];
    auto memoryView = input["constructor"].new_(memory, reinterpret_cast<uintptr_t>(rxBuffer), size);
    memoryView.call<void>("set", input);

    return {reinterpret_cast<char *>(rxBuffer), size};
}

void test_write_data() {
    auto memView = emscripten::typed_memory_view(txBufferSize, txBuffer);
    auto val = emscripten::val(memView);
    std::string s("hello from wasm write");
    for (size_t i = 0; i < s.size(); ++i) {
        txBuffer[i] = s[i];
    }

    emscripten::val::global("test_cb_write_data")(val).await();
}

void test_throw_exception() {
    throw std::runtime_error("Exception from wasm");
}

void test_fcn_throw_exception() {
    emscripten::val::global("fcn_throw_exception")();
}

// @formatter:off
EMSCRIPTEN_BINDINGS(module) {
    /* types declaration */
    emscripten::value_object<Struct_A>("Struct_A")
            .field("struct_bool", &Struct_A::struct_bool)
            .field("struct_int", &Struct_A::struct_int)
            .field("struct_float", &Struct_A::struct_float)
            .field("struct_string", &Struct_A::struct_string);

    emscripten::value_object<Struct_B>("Struct_B")
            .field("struct_string", &Struct_B::struct_string)
            .field("struct_1", &Struct_B::struct_1)
            .field("struct_2", &Struct_B::struct_2);

    /* api declaration */
    emscripten::function("test_void_void", &test_void_void);
    emscripten::function("test_int_void", &test_int_void);
    emscripten::function("test_int_int", &test_int_int);
    emscripten::function("test_bool_void", &test_bool_void);
    emscripten::function("test_bool_bool", &test_bool_bool);
    emscripten::function("test_float_void", &test_float_void);
    emscripten::function("test_float_float", &test_float_float);
    emscripten::function("test_str_void", &test_str_void);
    emscripten::function("test_str_str", &test_str_str);
    emscripten::function("test_struct_void", &test_struct_void);
    emscripten::function("test_fcn_void_void", &test_fcn_void_void);
    emscripten::function("test_fcn_async_void_void", &test_fcn_async_void_void);
    emscripten::function("test_fcn_int_int", &test_fcn_int_int);
    emscripten::function("test_fcn_async_int_int", &test_fcn_async_int_int);
    emscripten::function("test_fcn_str_str", &test_fcn_str_str);

    emscripten::function("test_read_data", &test_read_data);
    emscripten::function("test_write_data", &test_write_data);
    emscripten::function("test_struct_void", &test_struct_void);

    emscripten::function("test_throw_exception", test_throw_exception);
    emscripten::function("test_fcn_throw_exception", test_fcn_throw_exception);
}
// @formatter:on
#else

int main() {
    wix::cout << "Hello from dapper CLI interface! There is nothing to dapperize yet, please come back and try me later :-)" << std::endl;
}

#endif
