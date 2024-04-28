//
// Created by vitek on 28.4.24.
//

#ifndef WDC_VARIANT_HELPERS_H
#define WDC_VARIANT_HELPERS_H

#include <variant>

namespace utils {

    /** Concatenates variants */
    template<typename T, typename... Args>
    struct cat_variant;

    template<typename... Args0, typename... Args1>
    struct cat_variant<std::variant<Args0...>, Args1...> {
        using type = std::variant<Args0..., Args1...>;
    };

/** Casts variant to a different one with superset of types. Likely does not move properly :(. Source: https://stackoverflow.com/a/47204507/8435335 */
    template<class... Args>
    struct variant_cast_proxy {
        std::variant<Args...> v;

        template<class... ToArgs>
        operator std::variant<ToArgs...>() const {
          return std::visit([](auto &&arg) -> std::variant<ToArgs...> { return arg; },
                            v);
        }
    };

    template<class... Args>
    auto variant_cast(const std::variant<Args...> &v) -> variant_cast_proxy<Args...> {
      return {v};
    }
}

#endif //WDC_VARIANT_HELPERS_H
