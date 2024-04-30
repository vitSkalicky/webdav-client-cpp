//
// Created by vitek on 16.2.24.
//

#ifndef WEBDAV_UTILS_H
#define WEBDAV_UTILS_H

#include <chrono>
#include <string>

#define UNUSED(expr) do { (void)(expr); } while (0)

namespace utils {
    std::optional<std::chrono::system_clock::time_point> parse_tp_rfc2616(const std::string &input){
        const std::string rfc822_1 = "%a, %d %b %Y %H:%M:%S";
        const std::string rfc822_2 = "%d %b %Y %H:%M:%S";
        const std::string rfc850_1 = "%a, %d-%b-%y %H:%M:%S";
        const std::string rfc850_2 = "%d-%b-%y %H:%M:%S";
        const std::string ansi_1 = "%a %b %d %H:%M:%S %Y";
        const std::string ansi_2 = "%a %b  %d %H:%M:%S %Y";

        const std::string formats[] = {rfc822_1, rfc822_2, rfc850_1, rfc850_2, ansi_1, ansi_2};

        std::string inp = input;

        // detect timezone
        size_t last_space = inp.rfind(' ', inp.length());
        std::string tz_str = inp.substr(last_space + 1, inp.length() - last_space - 1);
        const std::chrono::time_zone *tz_ptr = nullptr;
        UNUSED(tz_ptr);
        try {
            tz_ptr = std::chrono::locate_zone(tz_str);
            inp = inp.substr(0, last_space);
        }catch (std::runtime_error &ignored){}

        std::tm tm = {};
        std::stringstream ss(inp);

        //todo use the timezone, so far it is ignored
        // todo properly return none if invalid

        for (auto&& format : formats) {
            ss >> std::get_time(&tm, format.c_str());
            if (!ss.fail()){
                break;
            }
            ss.str(inp);
            ss.clear();
        }

        auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        return tp;
    }
}

#endif //WEBDAV_UTILS_H
