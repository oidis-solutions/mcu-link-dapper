/* ********************************************************************************************************* *
 *
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * The BSD-3-Clause license for this file can be found in the LICENSE.txt file included with this distribution
 * or at https://spdx.org/licenses/BSD-3-Clause.html#licenseText
 *
 * ********************************************************************************************************* */

#ifndef WEBIX_DAPPER_LOGGER_HPP_
#define WEBIX_DAPPER_LOGGER_HPP_

#include <functional>
#include <sstream>

namespace wix {
    class Logger {
     public:
        explicit Logger(const std::function<void(const std::string &)> &handler);
        ~Logger();

        template<typename T>
        Logger &operator<<(const T &value) {
            this->stream << value;
            return *this;
        }

        Logger &operator<<(std::ostream &(*item)(std::ostream &));

     private:
        std::function<void(const std::string &)> handler;
        std::ostringstream stream;
        void writeData();
    };
}  // namespace wix

#endif  // WEBIX_DAPPER_LOGGER_HPP_
