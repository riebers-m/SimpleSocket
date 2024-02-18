//
// Created by max on 11.02.24.
//

#ifndef SIMPLESOCKET_UNIQUE_VALUE_HPP
#define SIMPLESOCKET_UNIQUE_VALUE_HPP

#include <optional>
#include <functional>

namespace simple {
    template<typename T, typename Deleter=std::function<void(T)>>
    class UniqueValue {
    private:
        std::optional<T> m_value;
        Deleter m_deleter;

    public:
        explicit UniqueValue(T value) : UniqueValue{std::move(value), Deleter{}} {}

        UniqueValue(T value, Deleter deleter) : m_value{std::move(value)}, m_deleter{std::move(deleter)} {}

        UniqueValue(UniqueValue const &) = delete;

        UniqueValue &operator=(UniqueValue const &) = delete;

        UniqueValue(UniqueValue &&other) noexcept:
                m_value{std::exchange(other.m_value, std::nullopt)}, m_deleter{std::move(other.m_deleter)} {}

        UniqueValue &operator=(UniqueValue &&other) noexcept {
            if (this == std::addressof(other)) {
                return *this;
            }
            using std::swap;
            swap(m_value, other.m_value);
            swap(m_deleter, other.m_deleter);
            return *this;
        }

        ~UniqueValue() {
            if (m_value.has_value()) {
                m_deleter(m_value.value());
            }
        }

        [[nodiscard]] bool has_value() const {
            return m_value.has_value();
        }

        [[nodiscard]] T const &value() const {
            return m_value.value();
        }

        [[nodiscard]] T &value() {
            return m_value.value();
        }

    };
};
#endif //SIMPLESOCKET_UNIQUE_VALUE_HPP
