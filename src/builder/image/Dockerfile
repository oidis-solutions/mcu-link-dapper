 #/* ********************************************************************************************************* *
 # *
 # * Copyright 2024 NXP
 # *
 # * SPDX-License-Identifier: BSD-3-Clause
 # * The BSD-3-Clause license for this file can be found in the LICENSE.txt file included with this distribution
 # * or at https://spdx.org/licenses/BSD-3-Clause.html#licenseText
 # *
 # * ********************************************************************************************************* */

FROM oidis/oidis-ubuntu-base:2022.3.1
MAINTAINER Michal Kelnar "michal.kelnar@nxp.com"

RUN apt-get update -y && \
    apt-get install -y git cmake build-essential python3 python3-pip gcc-9 g++-9 lsb-release wget software-properties-common gnupg

RUN wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 17 && \
    apt-get install -y clang-tidy-17 clang-format-17 && \
    ln -s $(which clang-tidy-17) /bin/clang-tidy && \
    ln -s $(which clang-format-17) /bin/clang-format \

WORKDIR "/"
RUN git clone https://github.com/emscripten-core/emsdk.git

ENV EMSDK=/emsdk \
    EMSDK_NODE=/emsdk/node/14.18.2_64bit/bin/node \
    PATH=/emsdk:/emsdk/upstream/emscripten:/emsdk/upstream/bin:/emsdk/node/14.18.2_64bit/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

RUN emsdk install 3.1.54
RUN emsdk activate 3.1.54

RUN mkdir -p "/var/webix"
WORKDIR "/var/webix"

CMD ["bash"]
