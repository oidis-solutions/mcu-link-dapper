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
import fg from "fast-glob";
import livereload from "rollup-plugin-livereload";
import {nodeResolve} from '@rollup/plugin-node-resolve';
import process from "node:process";
import serve from "rollup-plugin-serve";

const watch = process.env.ROLLUP_WATCH;

export default [
    {
        input: ["src/js/webix-dapper.mjs", "test/js/mock-dapper.mjs"],
        // suppress eval warnings because emscripten needs DYNAMIC_EXECUTION=1 in current configuration
        onwarn(warning, warn) {
            if (warning.code === 'EVAL') {
                return;
            }
            warn(warning);
        },
        output: [
            {
                dir: "build/target/webix-dapper",
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
                        src: "build/build_wasm/*.wasm",
                        dest: "build/target/webix-dapper"
                    },
                    {
                        src: "LICENSE.txt",
                        dest: "build/target/webix-dapper"
                    },
                    {
                        src: "test/html/index.html",
                        dest: "build/target",
                        transform: contents => contents.toString()
                            .replaceAll("../../src/js/webix-dapper.mjs", "./webix-dapper/webix-dapper.js")
                            .replaceAll("../data/dataset", "./data/dataset")
                    },
                    {
                        src: "test/html/test.html",
                        dest: "build/target",
                        transform: contents => contents.toString()
                            .replaceAll("../js/mock-dapper.mjs", "./webix-dapper/mock-dapper.js")
                            .replaceAll("../data/dataset", "./data/dataset")
                    },
                    {
                        src: "test/data/dataset.mjs",
                        dest: "build/target/data",
                        transform: contents => contents.toString().replaceAll("../data/dataset", "./data/dataset")
                    }
                ]
            }),
            {
                name: "watch-test-index",
                async buildStart() {
                    const files = await fg("test/html/*");
                    for (let file of files) {
                        this.addWatchFile(file);
                    }
                }
            },
            // builtins(),
            watch && serve({
                contentBase: "build/target/",
                port: 10002
                // open: true
                // openPage: "/test/html/test/index.html",
            }),
            watch && livereload({watch: "build/target"})
        ],
        watch: {
            clearScreen: false
        }
    }
];
