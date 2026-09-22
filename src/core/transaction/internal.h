#pragma once
#include "foopodbridge/core/transaction/transaction.h"
#include <Windows.h>
#include <bcrypt.h>
#include <array>
#include <limits>

namespace foopodbridge::core::transaction::detail {
class hash final {
public:
    hash() {
        if (BCryptOpenAlgorithmProvider(&algorithm_, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
            throw failure("hash_provider");
        if (BCryptCreateHash(algorithm_, &hash_, nullptr, 0, nullptr, 0, 0) < 0) {
            BCryptCloseAlgorithmProvider(algorithm_, 0);
            throw failure("hash_create");
        }
    }
    ~hash() { BCryptDestroyHash(hash_); BCryptCloseAlgorithmProvider(algorithm_, 0); }
    hash(const hash&) = delete;
    hash& operator=(const hash&) = delete;
    void add(std::span<const std::byte> data) {
        if (data.size() > std::numeric_limits<ULONG>::max() ||
            BCryptHashData(hash_, reinterpret_cast<PUCHAR>(const_cast<std::byte*>(data.data())),
                           static_cast<ULONG>(data.size()), 0) < 0) throw failure("hash_update");
    }
    std::string finish() {
        std::array<unsigned char, 32> data{};
        if (BCryptFinishHash(hash_, data.data(), static_cast<ULONG>(data.size()), 0) < 0)
            throw failure("hash_finish");
        constexpr char hex[] = "0123456789abcdef";
        std::string result;
        for (const auto value : data) { result += hex[value >> 4]; result += hex[value & 15]; }
        return result;
    }
private:
    BCRYPT_ALG_HANDLE algorithm_{};
    BCRYPT_HASH_HANDLE hash_{};
};
inline bool key(const std::string& text) {
    return !text.empty() && text.size() <= 80 && text.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_") == std::string::npos;
}
inline bool digest(const std::string& text) {
    return text.size() == 64 && text.find_first_not_of("0123456789abcdef") == std::string::npos;
}
inline bytes encode(const std::string& text) {
    return bytes(reinterpret_cast<const std::byte*>(text.data()), reinterpret_cast<const std::byte*>(text.data() + text.size()));
}
inline std::string decode(const bytes& data) {
    return std::string(reinterpret_cast<const char*>(data.data()), data.size());
}
inline void write(filesystem& fs, const std::string& path, std::span<const std::byte> data) {
    fs.create(path);
    fs.append(path, data);
    fs.flush(path);
    if (read_all(fs, path) != bytes(data.begin(), data.end())) throw failure("write_verification");
}
inline std::string seal(const std::string& body) { return body + sha256(encode(body)) + "\n"; }
inline std::string unseal(const bytes& data) {
    const auto text = decode(data);
    if (text.size() < 65 || text.back() != '\n') throw failure("record_invalid");
    const auto body = text.substr(0, text.size() - 65);
    if (text.substr(text.size() - 65, 64) != sha256(encode(body))) throw failure("record_checksum");
    return body;
}
} // namespace foopodbridge::core::transaction::detail
