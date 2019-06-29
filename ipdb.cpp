//
// Created by root on 9/4/18.
//

#include "ipdb.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <fstream>
#include <sstream>
#include <arpa/inet.h>

using namespace std;
using namespace rapidjson;

void ipdb::MetaData::Parse(const string &json) {
    Document d;
    d.Parse(json.c_str());
    Build = d["build"].GetUint64();
    IPVersion = static_cast<uint16_t>(d["ip_version"].GetUint());
    NodeCount = d["node_count"].GetInt();
    TotalSize = d["total_size"].GetInt();
    auto fields = d["fields"].GetArray();
    for (auto &field : fields) {
        Fields.emplace_back(field.GetString());
    }
    auto languages = d["languages"].GetObject();
    for (auto &language:languages) {
        Languages[language.name.GetString()] = language.value.GetInt();
    }
}

int ipdb::Reader::readNode(int node, int index) const {
    auto off = node * 8 + index * 4;
    return ntohl(static_cast<uint32_t>(*(int *) &data.get()[off]));
}

string ipdb::Reader::resolve(int node) {
    auto resolved = node - meta.NodeCount + meta.NodeCount * 8;
    if (resolved >= fileSize) {
        throw ErrDatabaseError;
    }

    auto size = (data.get()[resolved] << 8) | data.get()[resolved + 1];
    if ((resolved + 2 + size) > dataSize) {
        throw ErrDatabaseError;
    }
    string bytes = (const char *) data.get() + resolved + 2;
    return bytes;
}

int ipdb::Reader::search(const u_char *ip, int bitCount) const {
    int node = 0;

    if (bitCount == 32) {
        node = v4offset;
    } else {
        node = 0;
    }

    for (auto i = 0; i < bitCount; ++i) {
        if (node > meta.NodeCount) {
            break;
        }

        node = readNode(node, ((0xFF & int(ip[i >> 3])) >> uint(7 - (i % 8))) & 1);
    }

    if (node > meta.NodeCount) {
        return node;
    }
    throw ErrDataNotExists;
}

string ipdb::Reader::find0(const string &addr) {
    int node = 0;
    struct in_addr addr4{};
    struct in6_addr addr6{};
    if (inet_pton(AF_INET, addr.c_str(), &addr4)) {
        if (!IsIPv4Support()) {
            throw ErrNoSupportIPv4;
        }
        node = search((const u_char *) &addr4.s_addr, 32);
    } else if (inet_pton(AF_INET6, addr.c_str(), &addr6)) {
        if (!IsIPv6Support()) {
            throw ErrNoSupportIPv6;
        }
        node = search((const u_char *) &addr6.s6_addr, 128);
    } else {
        throw ErrIPFormat;
    }
    auto body = resolve(node);

    return body;
}

vector<string> split(const string &s, const string &sp) {
    vector<string> output;

    string::size_type prev_pos = 0, pos = 0;

    while ((pos = s.find(sp, pos)) != string::npos) {
        string substring(s.substr(prev_pos, pos - prev_pos));

        output.emplace_back(substring);

        prev_pos = ++pos;
    }

    output.emplace_back(s.substr(prev_pos, pos - prev_pos)); // Last word

    return output;
}

vector<string> ipdb::Reader::find1(const string &addr, const string &language) {
    if (meta.Languages.find(language) == meta.Languages.end()) {
        throw ErrNoSupportLanguage;
    }
    auto off = meta.Languages[language];

    auto body = find0(addr);


    auto tmp = split(body, "\t");

    if (off + meta.Fields.size() > tmp.size()) {
        throw ErrDatabaseError;
    }

    vector<string> result;
    for (auto i = tmp.begin() + off; i != tmp.begin() + off + meta.Fields.size(); ++i) {
        result.emplace_back(*i);
    }
    return result;
}

map<string, string> ipdb::Reader::FindMap(const string &addr, const string &language) {

    auto data = find1(addr, language);
    map<string, string> info;
    auto k = 0;
    for (auto &v : data) {
        info[meta.Fields[k++]] = v;
    }

    return info;
}

vector<string> ipdb::Reader::Find(const string &addr, const string &language) {
    return find1(addr, language);
}

bool ipdb::Reader::IsIPv6Support() {
    return (((int) meta.IPVersion) & IPv6) == IPv6;
}

bool ipdb::Reader::IsIPv4Support() {
    return (((int) meta.IPVersion) & IPv4) == IPv4;
}

ipdb::Reader::Reader(const string &file) {
    ifstream fs(file, ios::binary | ios::out);
    if (fs.tellg() == -1) {
        throw ErrFileSize;
    }
    fs.seekg(0, ios::end);
    auto fsize = fs.tellg();
    uint32_t metaLength = 0ul;
    fs.seekg(0, ios::beg);
    fs.read((char *) &metaLength, 4);
    metaLength = ntohl(metaLength);
    string bb;
    bb.resize(metaLength);
    fs.read(&bb[0], metaLength);

    meta.Parse(bb);

    if (meta.Languages.empty() || meta.Fields.empty()) {
        throw ErrMetaData;
    }

    if (fsize != (4 + metaLength + meta.TotalSize)) {
        throw ErrFileSize;
    }
    fileSize = (int) fsize;
    auto dataLen = (int) fsize - 4 - metaLength;
    data = shared_ptr<u_char>(new u_char[dataLen], std::default_delete<u_char[]>());;
    fs.read((char *) data.get(), dataLen);
    dataSize = dataLen;
    auto node = 0;
    for (auto i = 0; i < 96 && node < meta.NodeCount; ++i) {
        if (i >= 80) {
            node = readNode(node, 1);
        } else {
            node = readNode(node, 0);
        }
    }
    v4offset = node;
}

ipdb::Reader::~Reader() = default;

uint64_t ipdb::Reader::BuildTime() {
    return meta.Build;
}

vector<string> ipdb::Reader::Languages() {
    vector<string> ls;
    for (auto i:meta.Languages) {
        ls.emplace_back(i.first);
    }
    return ls;
}

vector<string> ipdb::Reader::Fields() {
    return meta.Fields;
}

ipdb::CityInfo::CityInfo(const vector<string> &data) {
    _data = data;
}

string ipdb::CityInfo::CountryName() {
    return _data[0];
}

string ipdb::CityInfo::RegionName() {
    return _data[1];
}

string ipdb::CityInfo::CityName() {
    return _data[2];
}

string ipdb::CityInfo::OwnerDomain() {
    return _data[3];
}

string ipdb::CityInfo::IspDomain() {
    return _data[4];
}

string ipdb::CityInfo::Latitude() {
    return _data[5];
}

string ipdb::CityInfo::Longitude() {
    return _data[6];
}

string ipdb::CityInfo::Timezone() {
    return _data[7];
}

string ipdb::CityInfo::UtcOffset() {
    return _data[8];
}

string ipdb::CityInfo::ChinaAdminCode() {
    return _data[9];
}

string ipdb::CityInfo::IddCode() {
    return _data[10];
}

string ipdb::CityInfo::CountryCode() {
    return _data[11];
}

string ipdb::CityInfo::ContinentCode() {
    return _data[12];
}

string ipdb::CityInfo::IDC() {
    return _data[13];
}

string ipdb::CityInfo::BaseStation() {
    return _data[14];
}

string ipdb::CityInfo::CountryCode3() {
    return _data[15];
}

string ipdb::CityInfo::EuropeanUnion() {
    return _data[16];
}

string ipdb::CityInfo::CurrencyCode() {
    return _data[17];
}

string ipdb::CityInfo::CurrencyName() {
    return _data[18];
}

string ipdb::CityInfo::Anycast() {
    return _data[19];
}

string ipdb::CityInfo::str() {
    stringstream sb;
    sb << "country_name:";
    sb << CountryName();
    sb << "\n";
    sb << "region_name:";
    sb << RegionName();
    sb << "\n";
    sb << "city_name:";
    sb << CityName();
    sb << "\n";
    sb << "owner_domain:";
    sb << OwnerDomain();
    sb << "\n";
    sb << "isp_domain:";
    sb << IspDomain();
    sb << "\n";
    sb << "latitude:";
    sb << Latitude();
    sb << "\n";
    sb << "longitude:";
    sb << Longitude();
    sb << "\n";

    sb << "timezone:";
    sb << Timezone();
    sb << "\n";

    sb << "utc_offset:";
    sb << UtcOffset();
    sb << "\n";

    sb << "china_admin_code:";
    sb << ChinaAdminCode();
    sb << "\n";

    sb << "idd_code:";
    sb << IddCode();
    sb << "\n";

    sb << "country_code:";
    sb << CountryCode();
    sb << "\n";

    sb << "continent_code:";
    sb << ContinentCode();
    sb << "\n";

    sb << "idc:";
    sb << IDC();
    sb << "\n";

    sb << "base_station:";
    sb << BaseStation();
    sb << "\n";

    sb << "country_code3:";
    sb << CountryCode3();
    sb << "\n";

    sb << "european_union:";
    sb << EuropeanUnion();
    sb << "\n";

    sb << "currency_code:";
    sb << CurrencyCode();
    sb << "\n";

    sb << "currency_name:";
    sb << CurrencyName();
    sb << "\n";

    sb << "anycast:";
    sb << Anycast();

    return sb.str();
}

ipdb::City::City(const string &file) : Reader(file) {

}

ipdb::CityInfo ipdb::City::FindInfo(const string &addr, const string &language) {
    return CityInfo(Find(addr, language));
}

ipdb::BaseStationInfo::BaseStationInfo(const vector<string> &data) {
    _data = data;
}

string ipdb::BaseStationInfo::CountryName() {
    return _data[0];
}

string ipdb::BaseStationInfo::RegionName() {
    return _data[1];
}

string ipdb::BaseStationInfo::CityName() {
    return _data[2];
}

string ipdb::BaseStationInfo::OwnerDomain() {
    return _data[3];
}

string ipdb::BaseStationInfo::IspDomain() {
    return _data[4];
}

string ipdb::BaseStationInfo::BaseStation() {
    return _data[5];
}

string ipdb::BaseStationInfo::str() {
    stringstream sb;
    sb << "country_name:";
    sb << CountryName();
    sb << "\n";
    sb << "region_name:";
    sb << RegionName();
    sb << "\n";
    sb << "city_name:";
    sb << CityName();
    sb << "\n";
    sb << "owner_domain:";
    sb << OwnerDomain();
    sb << "\n";
    sb << "isp_domain:";
    sb << IspDomain();
    sb << "\n";
    sb << "base_station:";
    sb << BaseStation();

    return sb.str();
}

ipdb::BaseStation::BaseStation(const string &file) : Reader(file) {

}

ipdb::BaseStationInfo ipdb::BaseStation::FindInfo(const string &addr, const string &language) {
    return BaseStationInfo(Find(addr, language));
}

ipdb::DistrictInfo::DistrictInfo(const vector<string> &data) {
    _data = data;
}

string ipdb::DistrictInfo::CountryName() {
    return _data[0];
}

string ipdb::DistrictInfo::RegionName() {
    return _data[1];
}

string ipdb::DistrictInfo::CityName() {
    return _data[2];
}

string ipdb::DistrictInfo::DistrictName() {
    return _data[3];
}

string ipdb::DistrictInfo::ChinaAdminCode() {
    return _data[4];
}

string ipdb::DistrictInfo::CoveringRadius() {
    return _data[5];
}

string ipdb::DistrictInfo::Latitude() {
    return _data[6];
}

string ipdb::DistrictInfo::Longitude() {
    return _data[7];
}

string ipdb::DistrictInfo::str() {
    stringstream sb;
    sb << "country_name:";
    sb << CountryName();
    sb << "\n";
    sb << "region_name:";
    sb << RegionName();
    sb << "\n";
    sb << "city_name:";
    sb << CityName();
    sb << "\n";
    sb << "district_name:";
    sb << DistrictName();
    sb << "\n";
    sb << "china_admin_code:";
    sb << ChinaAdminCode();
    sb << "\n";
    sb << "covering_radius:";
    sb << CoveringRadius();
    sb << "\n";
    sb << "latitude:";
    sb << Latitude();
    sb << "\n";
    sb << "longitude:";
    sb << Longitude();

    return sb.str();
}

ipdb::District::District(const string &file) : Reader(file) {

}

ipdb::DistrictInfo ipdb::District::FindInfo(const string &addr, const string &language) {
    return DistrictInfo(Find(addr, language));
}

ipdb::IDCInfo::IDCInfo(const vector<string> &data) {
    _data = data;
}

string ipdb::IDCInfo::CountryName() {
    return _data[0];
}

string ipdb::IDCInfo::RegionName() {
    return _data[1];
}

string ipdb::IDCInfo::CityName() {
    return _data[2];
}

string ipdb::IDCInfo::OwnerDomain() {
    return _data[3];
}

string ipdb::IDCInfo::IspDomain() {
    return _data[4];
}

string ipdb::IDCInfo::IDC() {
    return _data[5];
}

string ipdb::IDCInfo::str() {
    stringstream sb;
    sb << "country_name:";
    sb << CountryName();
    sb << "\n";
    sb << "region_name:";
    sb << RegionName();
    sb << "\n";
    sb << "city_name:";
    sb << CityName();
    sb << "\n";
    sb << "owner_domain:";
    sb << OwnerDomain();
    sb << "\n";
    sb << "isp_domain:";
    sb << IspDomain();
    sb << "\n";
    sb << "idc:";
    sb << IDC();

    return sb.str();
}

ipdb::IDC::IDC(const string &file) : Reader(file) {

}

ipdb::IDCInfo ipdb::IDC::FindInfo(const string &addr, const string &language) {
    return IDCInfo(Find(addr, language));
}
