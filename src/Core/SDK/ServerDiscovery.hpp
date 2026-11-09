#pragma once
#include <cstdint>
#include <istream>
#include <map>
#include <regex>
#include <string>

namespace ServerDiscovery {
// FiveM crashometry stores little-endian key length, value length, then bytes.
inline bool Length(std::istream& stream, std::uint32_t& value) {
    unsigned char bytes[4]{};
    if (!stream.read(reinterpret_cast<char*>(bytes),4)) return false;
    value=std::uint32_t(bytes[0]) | (std::uint32_t(bytes[1])<<8) |
        (std::uint32_t(bytes[2])<<16) | (std::uint32_t(bytes[3])<<24);
    return true;
}
inline std::map<std::string,std::string> Read(std::istream& stream) {
    std::map<std::string,std::string> records;
    std::uint32_t keySize=0,valueSize=0;
    std::size_t total=0;
    while (Length(stream,keySize) && Length(stream,valueSize)) {
        if (!keySize || keySize>4096 || valueSize>65536 || total+keySize+valueSize>4*1024*1024) break;
        std::string key(keySize,'\0'),value(valueSize,'\0');
        if (!stream.read(&key[0],keySize) || (valueSize && !stream.read(&value[0],valueSize))) break;
        total+=keySize+valueSize;
        if (key.find('\0')!=std::string::npos || value.find('\0')!=std::string::npos) continue;
        records[key]=value;
    }
    return records;
}
inline std::string Token(const std::string& url) {
    static const std::regex join(R"(^https?://cfx\.re/join/([A-Za-z0-9]+)/?(?:\?[^\s]*)?$)");
    std::smatch match;
    return std::regex_match(url,match,join)?match[1].str():std::string{};
}
inline std::string Endpoint(const std::string& value) {
    static const std::regex endpoint(R"(^(https?://)?([A-Za-z0-9.-]+|\[[0-9A-Fa-f:]+\]):([0-9]{1,5})/?$)");
    std::smatch match;
    if (!std::regex_match(value,match,endpoint)) return {};
    const int port=std::stoi(match[3].str());
    if (port<1 || port>65535) return {};
    return (match[1].matched?match[1].str():"http://")+match[2].str()+":"+match[3].str();
}
}
