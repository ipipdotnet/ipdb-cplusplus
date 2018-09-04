# ipdb-cplusplus
IPIP.net officially supported IP database ipdb format parsing library

## Dependence
```sh
dnf install rapidjson-devel
```

## Build & Run & Output
```sh
g++ -std=c++11 main.cpp ipdb.cpp -o main

./main

IPv6 Support:1
IPv4 Support:0
Languages:CN EN 
Build Time:1535451865
{"anycast":"","base_station":"","china_admin_code":"810000","city_name":"","continent_code":"AP","country_code":"HK","country_code3":"CHN","country_name":"中国","currency_code":"CNY","currency_name":"Yuan Renminbi","european_union":"0","idc":"","idd_code":"86","isp_domain":"snapserv.net","latitude":"22.396428","longitude":"114.109497","owner_domain":"skyshe.cn","region_name":"香港","timezone":"Asia/Hong_Kong","utc_offset":"UTC+8"}
{"anycast":"","base_station":"","china_admin_code":"810000","city_name":"","continent_code":"AP","country_code":"HK","country_code3":"CHN","country_name":"China","currency_code":"CNY","currency_name":"Yuan Renminbi","european_union":"0","idc":"","idd_code":"86","isp_domain":"snapserv.net","latitude":"22.396428","longitude":"114.109497","owner_domain":"skyshe.cn","region_name":"Hong Kong","timezone":"Asia/Hong_Kong","utc_offset":"UTC+8"}

```

## Example
```c++
#include "ipdb.h"
#include <iostream>

int main() {
    try {
        auto reader = ipdb::Reader::New("/root/cpp/ipdb-cplusplus/mydata6vipday4.ipdb");
        std::cout << "IPv6 Support:" << reader->IsIPv6Support() << std::endl; // Whether IPv6 is supported
        std::cout << "IPv4 Support:" << reader->IsIPv4Support() << std::endl; // Whether IPv4 is supported
        std::cout << "Languages:"; // Supported language items
        for (const auto &l:reader->Languages())
            std::cout << l << " ";
        std::cout << std::endl;
        std::cout << "Build Time:" << reader->Build() << std::endl;            // Build Time

        auto cn = reader->FindInfo("2a06:e881:3800::", "CN");
        std::cout << cn << std::endl;
        auto en = reader->FindInfo("2a06:e881:3800::", "EN");
        std::cout << en << std::endl;
    } catch (const char *e) {
        std::cout << e << std::endl;
    }
    return 0;
}
```