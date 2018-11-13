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

IPv4 Support: 0
IPv6 Support: 1
Build Time: 1535451865
Languages: CN EN 
...
```

## Example
```c++
#include "ipdb.h"
#include <iostream>

int main() {
    try {
        auto db = std::make_shared<ipdb::City>("/root/cpp/ipdb-cplusplus/mydata6vipday4.ipdb");
        std::cout << "IPv4 Support: " << db->IsIPv4Support() << std::endl; // check database support ip type
        std::cout << "IPv6 Support: " << db->IsIPv6Support() << std::endl; // check database support ip type
        std::cout << "Build Time: " << db->BuildTime() << std::endl; // database build time
        std::cout << "Languages: "; // database support language
        for (const auto &i:db->Languages())
            std::cout << i << " ";
        std::cout << std::endl; // database support fields
        std::cout << "Fields: ";
        for (const auto &i:db->Fields())
            std::cout << i << " ";
        std::cout << std::endl;

        std::cout << db->FindInfo("2001:250:200::", "CN").str() << std::endl; // return CityInfo
        for (const auto &i:db->Find("1.1.1.1", "CN")) // return std::vector<std::string>
            std::cout << i << ", ";
        std::cout << std::endl;
        for (const auto &i:db->FindMap("118.28.8.8", "CN")) // return std::map<std::string, std::string>
            std::cout << i.first << ": " << i.second << ", ";
        std::cout << std::endl;
        std::cout << db->FindInfo("127.0.0.1", "CN").str() << std::endl; // return CityInfo

    } catch (const char *e) {
        std::cout << e << std::endl;
    }
    return 0;
}
```