/* ********************************************************************************************************* *
 *
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * The BSD-3-Clause license for this file can be found in the LICENSE.txt file included with this distribution
 * or at https://spdx.org/licenses/BSD-3-Clause.html#licenseText
 *
 * ********************************************************************************************************* */
#include "Logger.hpp"
namespace wix {
    Logger::Logger(const std::function<void(const std::string &)> &handler)
        : handler(handler) {
    }
    Logger::~Logger() {
        this->writeData();
    }
    Logger &Logger::operator<<(std::ostream &(*item)(std::ostream &)) {
        if (item == static_cast<std::ostream &(*)(std::ostream &)>(std::endl)) {
            this->stream << item;
            this->writeData();
            this->stream.str("");
            this->stream.clear();
        } else {
            this->stream << item;
        }
        return *this;
    }

    void Logger::writeData() {
        std::string data = this->stream.str();
        if (!data.empty()) {
            this->handler(data);
        }
    }
}  // namespace wix
