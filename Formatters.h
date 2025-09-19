#pragma once
#include <thread>

#include "Common.h"


// Custom formatter for the identifier block
template<>
struct std::formatter<t_piece_identifier> {

    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const t_piece_identifier& identifier, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{:3}({})", identifier.index, identifier.rotation);
    };
};


// Custom formatter for thread id
template<>
struct std::formatter<std::thread::id> {

    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const std::thread::id& identifier, std::format_context& ctx) const {
        std::ostringstream ss;
        ss << identifier;
        return std::format_to(ctx.out(), "{:5}", ss.str());
    };
};

std::string format_number_human_readable(int64_t number) {
    if (number < 1000)
        return std::to_string(number);
    if (number < 1'000'000)
        return std::format("{:6.2f}K", static_cast<double>(number) / 1'000.0);
    if (number < 1'000'000'000)
        return std::format("{:6.2f}M", static_cast<double>(number) / 1'000'000.0);
    if (number < 1'000'000'000'000)
        return std::format("{:6.2f}B", static_cast<double>(number) / 1'000'000'000.0);
    return std::format("{:6.2f}T", static_cast<double>(number) / 1'000'000'000'000.0);
}