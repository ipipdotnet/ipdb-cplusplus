//
// Created by root on 9/4/18.
//

#include "ipdb.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <fstream>
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
    return ntohl(static_cast<uint32_t>(*(int *) &data[off]));
}

string ipdb::Reader::resolve(int node) {
    auto resolved = node - meta.NodeCount + meta.NodeCount * 8;
    if (resolved >= fileSize) {
        throw ErrDatabaseError;
    }

    auto size = (data[resolved] << 8) | data[resolved + 2];
    if ((resolved + 2 + size) > dataSize) {
        throw ErrDatabaseError;
    }
    string bytes = (const char *) data + resolved + 2;
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

string ipdb::Reader::FindInfo(const string &addr, const string &language) {
    auto data = FindMap(addr, language);
    Document doc;
    doc.SetObject();
    auto &allocator = doc.GetAllocator();
    for (auto &i : data) {
        doc.AddMember(rapidjson::StringRef(i.first.c_str()), rapidjson::StringRef(i.second.c_str()), allocator);
    }
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
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

shared_ptr<ipdb::Reader> ipdb::Reader::New(const string &file) {
    ifstream fs(file, ios::binary | ios::out);
    if (fs.tellg() == -1) {
        throw ErrFileSize;
    }
    auto reader = make_shared<Reader>();
    fs.seekg(0, ios::end);
    auto fileSize = fs.tellg();
    uint32_t metaLength = 0ul;
    fs.seekg(0, ios::beg);
    fs.read((char *) &metaLength, 4);
    metaLength = ntohl(metaLength);
    string bb;
    bb.resize(metaLength);
    fs.read(&bb[0], metaLength);

    reader->meta.Parse(bb);

    if (reader->meta.Languages.empty() || reader->meta.Fields.empty()) {
        throw ErrMetaData;
    }

    if (fileSize != (4 + metaLength + reader->meta.TotalSize)) {
        throw ErrFileSize;
    }
    reader->fileSize = (int) fileSize;
    auto dataLen = (int) fileSize - 4 - metaLength;
    reader->data = new u_char[dataLen];
    fs.read((char *) reader->data, dataLen);
    reader->dataSize = dataLen;
    auto node = 0;
    for (auto i = 0; i < 96 && node < reader->meta.NodeCount; ++i) {
        if (i >= 80) {
            node = reader->readNode(node, 1);
        } else {
            node = reader->readNode(node, 0);
        }
    }
    reader->v4offset = node;
    return reader;
}

ipdb::Reader::~Reader() {
    delete[]data;
}

uint64_t ipdb::Reader::Build() {
    return meta.Build;
}

vector<string> ipdb::Reader::Languages() {
    vector<string> ls;
    for (auto i:meta.Languages) {
        ls.emplace_back(i.first);
    }
    return ls;
}
