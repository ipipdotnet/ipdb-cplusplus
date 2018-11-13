//
// Created by root on 9/4/18.
//

#ifndef IPDB_IPDB_H
#define IPDB_IPDB_H

#include <memory>
#include <string>
#include <vector>
#include <map>

namespace ipdb {

#define  IPv4  0x01
#define  IPv6  0x02

#define ErrFileSize "IP Database file size error."
#define ErrMetaData "IP Database metadata error."
//#define ErrReadFull "IP Database ReadFull error."

#define ErrDatabaseError "database error"

#define ErrIPFormat "Query IP Format error."

#define ErrNoSupportLanguage "language not support"
#define ErrNoSupportIPv4 "IPv4 not support"
#define ErrNoSupportIPv6 "IPv6 not support"

#define ErrDataNotExists "data is not exists"

    using namespace std;

    class MetaData {
    public:
        uint64_t Build{};             //`json:"build"`
        uint16_t IPVersion{};         //`json:"ip_version"`
        map<string, int> Languages; //`json:"languages"`
        int NodeCount{};              //`json:"node_count"`
        int TotalSize{};              //`json:"total_size"`
        vector<string> Fields;      //`json:"fields"`

        void Parse(const string &json);
    };

    class Reader {
        MetaData meta;
        int v4offset = 0;
        int fileSize = 0;
        int dataSize = 0;
        shared_ptr<u_char> data = nullptr;

        int readNode(int node, int index) const;

        string resolve(int node);

        int search(const u_char *ip, int bitCount) const;

        string find0(const string &addr);

        vector<string> find1(const string &addr, const string &language);

    public:
        ~Reader();

        explicit Reader(const string &file);

        vector<string> Find(const string &addr, const string &language);

        map<string, string> FindMap(const string &addr, const string &language);

        bool IsIPv4Support();

        bool IsIPv6Support();

        uint64_t BuildTime();

        vector<string> Languages();

        vector<string> Fields();
    };

    class CityInfo {
        vector<string> _data;
    public:
        explicit CityInfo(const vector<string> &data);

        string CountryName();

        string RegionName();

        string CityName();

        string OwnerDomain();

        string IspDomain();

        string Latitude();

        string Longitude();

        string Timezone();

        string UtcOffset();

        string ChinaAdminCode();

        string IddCode();

        string CountryCode();

        string ContinentCode();

        string IDC();

        string BaseStation();

        string CountryCode3();

        string EuropeanUnion();

        string CurrencyCode();

        string CurrencyName();

        string Anycast();

        string str();
    };

    class City : public Reader {
    public:
        explicit City(const string &file);

        CityInfo FindInfo(const string &addr, const string &language);
    };

    class BaseStationInfo {
        vector<string> _data;
    public:
        explicit BaseStationInfo(const vector<string> &data);

        string CountryName();

        string RegionName();

        string CityName();

        string OwnerDomain();

        string IspDomain();

        string BaseStation();

        string str();
    };

    class BaseStation : public Reader {
    public:
        explicit BaseStation(const string &file);

        BaseStationInfo FindInfo(const string &addr, const string &language);
    };

    class DistrictInfo {
        vector<string> _data;
    public:
        explicit DistrictInfo(const vector<string> &data);

        string CountryName();

        string RegionName();

        string CityName();

        string DistrictName();

        string ChinaAdminCode();

        string CoveringRadius();

        string Latitude();

        string Longitude();

        string str();
    };

    class District : public Reader {
    public:
        explicit District(const string &file);

        DistrictInfo FindInfo(const string &addr, const string &language);
    };

    class IDCInfo {
        vector<string> _data;
    public:
        explicit IDCInfo(const vector<string> &data);

        string CountryName();

        string RegionName();

        string CityName();

        string OwnerDomain();

        string IspDomain();

        string IDC();

        string str();
    };

    class IDC : public Reader {
    public:
        explicit IDC(const string &file);

        IDCInfo FindInfo(const string &addr, const string &language);
    };
}


#endif //IPDB_IPDB_H
