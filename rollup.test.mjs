/*
 * Copyright 2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * The BSD-3-Clause license for this file can be found in the LICENSE.txt file included with this distribution
 * or at https://spdx.org/licenses/BSD-3-Clause.html#licenseText
 */

import commonjs from '@rollup/plugin-commonjs';
import copy from "rollup-plugin-copy";
import {nodeResolve} from '@rollup/plugin-node-resolve';

export default [
    {
        input: ["test/js/mock-dapper.mjs"],
        // suppress eval warnings because emscripten needs DYNAMIC_EXECUTION=1 in current configuration
        onwarn(warning, warn) {
            if (warning.code === 'EVAL') {
                return;
            }
            warn(warning);
        },
        output: [
            {
                dir: "build/target/test-dapper",
                format: "esm",
                sourcemap: true
            }
        ],
        plugins: [
            nodeResolve(),
            commonjs(),
            copy({
                targets: [
                    {
                        src: "src/wasm/build_wasm/*.wasm",
                        dest: "build/target/test-dapper"
                    },
                    {
                        src: "test/data/dataset.mjs",
                        dest: "build/target/data",
                        transform: contents => contents.toString().replaceAll("../data/dataset", "./data/dataset")
                    }
                ]
            })
        ],
        watch: {
            clearScreen: false
        }
    }
];
