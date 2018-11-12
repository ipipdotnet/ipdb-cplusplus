#include "ipdb.h"
#include <iostream>

int main() {
    try {
        auto reader = std::make_shared<ipdb::Reader>("/root/cpp/ipdb-cplusplus/mydata6vipday4.ipdb");
        std::cout << "IPv6 Support:" << reader->IsIPv6Support() << std::endl;   // Whether IPv6 is supported
        std::cout << "IPv4 Support:" << reader->IsIPv4Support() << std::endl;   // Whether IPv4 is supported
        std::cout << "Languages:";                                              // Supported language items
        for (const auto &l:reader->Languages())
            std::cout << l << " ";
        std::cout << std::endl;
        std::cout << "Build Time:" << reader->Build() << std::endl;             // Build Time

        auto cn = reader->FindInfo("2a06:e881:3800::", "CN");
        std::cout << cn << std::endl;
        auto en = reader->FindInfo("2a06:e881:3800::", "EN");
        std::cout << en << std::endl;
    } catch (const char *e) {
        std::cout << e << std::endl;
    }
    return 0;
}