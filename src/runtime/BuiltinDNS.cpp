/**
 * nova:dns - DNS Module Implementation
 *
 * Provides DNS resolution for Nova programs.
 * Compatible with Node.js dns module.
 */

#include "nova/runtime/BuiltinModules.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windns.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "dnsapi.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <resolv.h>
#endif

namespace nova {
namespace runtime {
namespace dns {

// Helper to allocate and copy string
static char* allocString(const std::string& str) {
    char* result = (char*)malloc(str.length() + 1);
    if (result) {
        strcpy(result, str.c_str());
    }
    return result;
}

#ifdef _WIN32
// Helper to convert wide string to narrow string
static char* allocWideString(const wchar_t* wstr) {
    if (!wstr) return nullptr;
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return nullptr;
    char* result = (char*)malloc(len);
    if (result) {
        WideCharToMultiByte(CP_UTF8, 0, wstr, -1, result, len, nullptr, nullptr);
    }
    return result;
}
#endif

// ============================================================================
// DNS Servers Configuration
// ============================================================================

static std::vector<std::string> customServers;
static int defaultResultOrder = 0;  // 0 = ipv4first, 1 = verbatim

// ============================================================================
// Error Codes (compatible with Node.js)
// ============================================================================

static const int DNS_NODATA = 1;
static const int DNS_FORMERR = 2;
static const int DNS_SERVFAIL = 3;
static const int DNS_NOTFOUND = 4;
static const int DNS_NOTIMP = 5;
static const int DNS_REFUSED = 6;
static const int DNS_BADQUERY = 7;
static const int DNS_BADNAME = 8;
static const int DNS_BADFAMILY = 9;
static const int DNS_BADRESP = 10;
static const int DNS_CONNREFUSED = 11;
static const int DNS_TIMEOUT = 12;
static const int DNS_EOF = 13;
static const int DNS_FILE = 14;
static const int DNS_NOMEM = 15;
static const int DNS_DESTRUCTION = 16;
static const int DNS_BADSTR = 17;
static const int DNS_BADFLAGS = 18;
static const int DNS_NONAME = 19;
static const int DNS_BADHINTS = 20;
static const int DNS_NOTINITIALIZED = 21;
static const int DNS_LOADIPHLPAPI = 22;
static const int DNS_ADDRGETNETWORKPARAMS = 23;
static const int DNS_CANCELLED = 24;

#ifdef _WIN32
static bool wsaInitialized = false;

static void initWinsock() {
    if (!wsaInitialized) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        wsaInitialized = true;
    }
}
#endif

extern "C" {

// ============================================================================
// Error Code Constants
// ============================================================================

int nova_dns_NODATA() { return DNS_NODATA; }
int nova_dns_FORMERR() { return DNS_FORMERR; }
int nova_dns_SERVFAIL() { return DNS_SERVFAIL; }
int nova_dns_NOTFOUND() { return DNS_NOTFOUND; }
int nova_dns_NOTIMP() { return DNS_NOTIMP; }
int nova_dns_REFUSED() { return DNS_REFUSED; }
int nova_dns_BADQUERY() { return DNS_BADQUERY; }
int nova_dns_BADNAME() { return DNS_BADNAME; }
int nova_dns_BADFAMILY() { return DNS_BADFAMILY; }
int nova_dns_BADRESP() { return DNS_BADRESP; }
int nova_dns_CONNREFUSED() { return DNS_CONNREFUSED; }
int nova_dns_TIMEOUT() { return DNS_TIMEOUT; }
int nova_dns_EOF() { return DNS_EOF; }
int nova_dns_FILE() { return DNS_FILE; }
int nova_dns_NOMEM() { return DNS_NOMEM; }
int nova_dns_DESTRUCTION() { return DNS_DESTRUCTION; }
int nova_dns_BADSTR() { return DNS_BADSTR; }
int nova_dns_BADFLAGS() { return DNS_BADFLAGS; }
int nova_dns_NONAME() { return DNS_NONAME; }
int nova_dns_BADHINTS() { return DNS_BADHINTS; }
int nova_dns_NOTINITIALIZED() { return DNS_NOTINITIALIZED; }
int nova_dns_LOADIPHLPAPI() { return DNS_LOADIPHLPAPI; }
int nova_dns_ADDRGETNETWORKPARAMS() { return DNS_ADDRGETNETWORKPARAMS; }
int nova_dns_CANCELLED() { return DNS_CANCELLED; }

// ============================================================================
// dns.lookup() - Resolve hostname to IP address
// ============================================================================

// Synchronous lookup
char* nova_dns_lookup(const char* hostname, int family, int* errorCode) {
    if (!hostname) {
        if (errorCode) *errorCode = DNS_BADNAME;
        return nullptr;
    }

#ifdef _WIN32
    initWinsock();
#endif

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));

    if (family == 4) {
        hints.ai_family = AF_INET;
    } else if (family == 6) {
        hints.ai_family = AF_INET6;
    } else {
        hints.ai_family = AF_UNSPEC;
    }
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(hostname, nullptr, &hints, &res);
    if (status != 0) {
        if (errorCode) *errorCode = DNS_NOTFOUND;
        return nullptr;
    }

    char ipstr[INET6_ADDRSTRLEN];
    void* addr;

    if (res->ai_family == AF_INET) {
        struct sockaddr_in* ipv4 = (struct sockaddr_in*)res->ai_addr;
        addr = &(ipv4->sin_addr);
    } else {
        struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)res->ai_addr;
        addr = &(ipv6->sin6_addr);
    }

    inet_ntop(res->ai_family, addr, ipstr, sizeof(ipstr));
    freeaddrinfo(res);

    if (errorCode) *errorCode = 0;
    return allocString(ipstr);
}

// Get family of resolved address
int nova_dns_lookup_family(const char* hostname) {
    if (!hostname) return 0;

#ifdef _WIN32
    initWinsock();
#endif

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(hostname, nullptr, &hints, &res);
    if (status != 0) return 0;

    int family = (res->ai_family == AF_INET) ? 4 : 6;
    freeaddrinfo(res);
    return family;
}

// Lookup all addresses
char** nova_dns_lookupAll(const char* hostname, int family, int* count, int* errorCode) {
    if (!hostname) {
        if (errorCode) *errorCode = DNS_BADNAME;
        *count = 0;
        return nullptr;
    }

#ifdef _WIN32
    initWinsock();
#endif

    struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof(hints));

    if (family == 4) {
        hints.ai_family = AF_INET;
    } else if (family == 6) {
        hints.ai_family = AF_INET6;
    } else {
        hints.ai_family = AF_UNSPEC;
    }
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(hostname, nullptr, &hints, &res);
    if (status != 0) {
        if (errorCode) *errorCode = DNS_NOTFOUND;
        *count = 0;
        return nullptr;
    }

    // Count results
    int numResults = 0;
    for (p = res; p != nullptr; p = p->ai_next) {
        numResults++;
    }

    char** results = (char**)malloc(numResults * sizeof(char*));
    char ipstr[INET6_ADDRSTRLEN];
    int i = 0;

    for (p = res; p != nullptr; p = p->ai_next) {
        void* addr;
        if (p->ai_family == AF_INET) {
            struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
            addr = &(ipv4->sin_addr);
        } else {
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
            addr = &(ipv6->sin6_addr);
        }
        inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
        results[i++] = allocString(ipstr);
    }

    freeaddrinfo(res);
    *count = numResults;
    if (errorCode) *errorCode = 0;
    return results;
}

// ============================================================================
// dns.lookupService() - Reverse lookup for address and port
// ============================================================================

char* nova_dns_lookupService_hostname(const char* address, int port, int* errorCode) {
    if (!address) {
        if (errorCode) *errorCode = DNS_BADNAME;
        return nullptr;
    }

#ifdef _WIN32
    initWinsock();
#endif

    struct sockaddr_in sa;
    struct sockaddr_in6 sa6;
    struct sockaddr* saddr;
    socklen_t slen;

    memset(&sa, 0, sizeof(sa));
    memset(&sa6, 0, sizeof(sa6));

    // Try IPv4 first
    if (inet_pton(AF_INET, address, &sa.sin_addr) == 1) {
        sa.sin_family = AF_INET;
        sa.sin_port = htons((uint16_t)port);
        saddr = (struct sockaddr*)&sa;
        slen = sizeof(sa);
    } else if (inet_pton(AF_INET6, address, &sa6.sin6_addr) == 1) {
        sa6.sin6_family = AF_INET6;
        sa6.sin6_port = htons((uint16_t)port);
        saddr = (struct sockaddr*)&sa6;
        slen = sizeof(sa6);
    } else {
        if (errorCode) *errorCode = DNS_BADNAME;
        return nullptr;
    }

    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];

    int status = getnameinfo(saddr, slen, host, sizeof(host), serv, sizeof(serv), 0);
    if (status != 0) {
        if (errorCode) *errorCode = DNS_NOTFOUND;
        return nullptr;
    }

    if (errorCode) *errorCode = 0;
    return allocString(host);
}

char* nova_dns_lookupService_service(const char* address, int port, int* errorCode) {
    if (!address) {
        if (errorCode) *errorCode = DNS_BADNAME;
        return nullptr;
    }

#ifdef _WIN32
    initWinsock();
#endif

    struct sockaddr_in sa;
    struct sockaddr_in6 sa6;
    struct sockaddr* saddr;
    socklen_t slen;

    memset(&sa, 0, sizeof(sa));
    memset(&sa6, 0, sizeof(sa6));

    if (inet_pton(AF_INET, address, &sa.sin_addr) == 1) {
        sa.sin_family = AF_INET;
        sa.sin_port = htons((uint16_t)port);
        saddr = (struct sockaddr*)&sa;
        slen = sizeof(sa);
    } else if (inet_pton(AF_INET6, address, &sa6.sin6_addr) == 1) {
        sa6.sin6_family = AF_INET6;
        sa6.sin6_port = htons((uint16_t)port);
        saddr = (struct sockaddr*)&sa6;
        slen = sizeof(sa6);
    } else {
        if (errorCode) *errorCode = DNS_BADNAME;
        return nullptr;
    }

    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];

    int status = getnameinfo(saddr, slen, host, sizeof(host), serv, sizeof(serv), 0);
    if (status != 0) {
        if (errorCode) *errorCode = DNS_NOTFOUND;
        return nullptr;
    }

    if (errorCode) *errorCode = 0;
    return allocString(serv);
}

// ============================================================================
// dns.resolve4() - Resolve A records (IPv4)
// ============================================================================

char** nova_dns_resolve4(const char* hostname, int* count, int* errorCode) {
    return nova_dns_lookupAll(hostname, 4, count, errorCode);
}

// With TTL
char** nova_dns_resolve4_ttl(const char* hostname, int* count, int* ttls, int* errorCode) {
    // TTL info requires platform-specific DNS query
    // For now, return addresses without TTL
    char** result = nova_dns_lookupAll(hostname, 4, count, errorCode);
    if (result && ttls) {
        for (int i = 0; i < *count; i++) {
            ttls[i] = 300;  // Default TTL
        }
    }
    return result;
}

// ============================================================================
// dns.resolve6() - Resolve AAAA records (IPv6)
// ============================================================================

char** nova_dns_resolve6(const char* hostname, int* count, int* errorCode) {
    return nova_dns_lookupAll(hostname, 6, count, errorCode);
}

char** nova_dns_resolve6_ttl(const char* hostname, int* count, int* ttls, int* errorCode) {
    char** result = nova_dns_lookupAll(hostname, 6, count, errorCode);
    if (result && ttls) {
        for (int i = 0; i < *count; i++) {
            ttls[i] = 300;
        }
    }
    return result;
}

// ============================================================================
// dns.resolve() - Generic resolver
// ============================================================================

char** nova_dns_resolve(const char* hostname, const char* rrtype, int* count, int* errorCode) {
    if (!rrtype || strcmp(rrtype, "A") == 0) {
        return nova_dns_resolve4(hostname, count, errorCode);
    } else if (strcmp(rrtype, "AAAA") == 0) {
        return nova_dns_resolve6(hostname, count, errorCode);
    }
    // Other record types would require platform-specific DNS queries
    if (errorCode) *errorCode = DNS_NOTIMP;
    *count = 0;
    return nullptr;
}

// ============================================================================
// dns.resolveCname() - Resolve CNAME records
// ============================================================================

char** nova_dns_resolveCname(const char* hostname, int* count, int* errorCode) {
#ifdef _WIN32
    initWinsock();

    PDNS_RECORD pDnsRecord = nullptr;
    DNS_STATUS status = DnsQuery_A(hostname, DNS_TYPE_CNAME, DNS_QUERY_STANDARD, nullptr, &pDnsRecord, nullptr);

    if (status != 0 || !pDnsRecord) {
        if (errorCode) *errorCode = DNS_NOTFOUND;
        *count = 0;
        return nullptr;
    }

    // Count records
    int numRecords = 0;
    PDNS_RECORD p = pDnsRecord;
    while (p) {
        if (p->wType == DNS_TYPE_CNAME) numRecords++;
        p = p->pNext;
    }

    char** results = (char**)malloc(numRecords * sizeof(char*));
    p = pDnsRecord;
    int i = 0;
    while (p) {
        if (p->wType == DNS_TYPE_CNAME) {
            results[i++] = allocWideString(p->Data.CNAME.pNameHost);
        }
        p = p->pNext;
    }

    DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
    *count = numRecords;
    if (errorCode) *errorCode = 0;
    return results;
#else
    // Unix: use res_query (simplified)
    if (errorCode) *errorCode = DNS_NOTIMP;
    *count = 0;
    return nullptr;
#endif
}

// ============================================================================
// dns.resolveMx() - Resolve MX records
// ============================================================================

struct MxRecord {
    char* exchange;
    int priority;
};

void* nova_dns_resolveMx(const char* hostname, int* count, int* errorCode) {
#ifdef _WIN32
    initWinsock();

    PDNS_RECORD pDnsRecord = nullptr;
    DNS_STATUS status = DnsQuery_A(hostname, DNS_TYPE_MX, DNS_QUERY_STANDARD, nullptr, &pDnsRecord, nullptr);

    if (status != 0 || !pDnsRecord) {
        if (errorCode) *errorCode = DNS_NOTFOUND;
        *count = 0;
        return nullptr;
    }

    // Count MX records
    int numRecords = 0;
    PDNS_RECORD p = pDnsRecord;
    while (p) {
        if (p->wType == DNS_TYPE_MX) numRecords++;
        p = p->pNext;
    }

    MxRecord* results = (MxRecord*)malloc(numRecords * sizeof(MxRecord));
    p = pDnsRecord;
    int i = 0;
    while (p) {
        if (p->wType == DNS_TYPE_MX) {
            results[i].exchange = allocWideString(p->Data.MX.pNameExchange);
            results[i].priority = p->Data.MX.wPreference;
            i++;
        }
        p = p->pNext;
    }

    DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
    *count = numRecords;
    if (errorCode) *errorCode = 0;
    return results;
#else
    if (errorCode) *errorCode = DNS_NOTIMP;
    *count = 0;
    return nullptr;
#endif
}

char* nova_dns_MxRecord_exchange(void* record) {
    if (!record) return nullptr;
    return allocString(((MxRecord*)record)->exchange);
}

int nova_dns_MxRecord_priority(void* record) {
    if (!record) return 0;
    return ((MxRecord*)record)->priority;
}

// ============================================================================
// dns.resolveNs() - Resolve NS records
// ============================================================================

char** nova_dns_resolveNs(const char* hostname, int* count, int* errorCode) {
#ifdef _WIN32
    initWinsock();

    PDNS_RECORD pDnsRecord = nullptr;
    DNS_STATUS status = DnsQuery_A(hostname, DNS_TYPE_NS, DNS_QUERY_STANDARD, nullptr, &pDnsRecord, nullptr);

    if (status != 0 || !pDnsRecord) {
        if (errorCode) *errorCode = DNS_NOTFOUND;
        *count = 0;
        return nullptr;
    }

    int numRecords = 0;
    PDNS_RECORD p = pDnsRecord;
    while (p) {
        if (p->wType == DNS_TYPE_NS) numRecords++;
        p = p->pNext;
    }

    char** results = (char**)malloc(numRecords * sizeof(char*));
    p = pDnsRecord;
    int i = 0;
    while (p) {
        if (p->wType == DNS_TYPE_NS) {
            results[i++] = allocWideString(p->Data.NS.pNameHost);
        }
        p = p->pNext;
    }

    DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
    *count = numRecords;
    if (errorCode) *errorCode = 0;
    return results;
#else
    if (errorCode) *errorCode = DNS_NOTIMP;
    *count = 0;
    return nullptr;
#endif
}

// ============================================================================
// dns.resolveTxt() - Resolve TXT records
// ============================================================================

char** nova_dns_resolveTxt(const char* hostname, int* count, int* errorCode) {
#ifdef _WIN32
    initWinsock();

    PDNS_RECORD pDnsRecord = nullptr;
    DNS_STATUS status = DnsQuery_A(hostname, DNS_TYPE_TEXT, DNS_QUERY_STANDARD, nullptr, &pDnsRecord, nullptr);

    if (status != 0 || !pDnsRecord) {
        if (errorCode) *errorCode = DNS_NOTFOUND;
        *count = 0;
        return nullptr;
    }

    int numRecords = 0;
    PDNS_RECORD p = pDnsRecord;
    while (p) {
        if (p->wType == DNS_TYPE_TEXT) numRecords++;
        p = p->pNext;
    }

    char** results = (char**)malloc(numRecords * sizeof(char*));
    p = pDnsRecord;
    int i = 0;
    while (p) {
        if (p->wType == DNS_TYPE_TEXT && p->Data.TXT.dwStringCount > 0) {
            results[i++] = allocWideString(p->Data.TXT.pStringArray[0]);
        }
        p = p->pNext;
    }

    DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
    *count = numRecords;
    if (errorCode) *errorCode = 0;
    return results;
#else
    if (errorCode) *errorCode = DNS_NOTIMP;
    *count = 0;
    return nullptr;
#endif
}

// ============================================================================
// dns.resolveSrv() - Resolve SRV records
// ============================================================================

struct SrvRecord {
    char* name;
    int port;
    int priority;
    int weight;
};

void* nova_dns_resolveSrv(const char* hostname, int* count, int* errorCode) {
#ifdef _WIN32
    initWinsock();

    PDNS_RECORD pDnsRecord = nullptr;
    DNS_STATUS status = DnsQuery_A(hostname, DNS_TYPE_SRV, DNS_QUERY_STANDARD, nullptr, &pDnsRecord, nullptr);

    if (status != 0 || !pDnsRecord) {
        if (errorCode) *errorCode = DNS_NOTFOUND;
        *count = 0;
        return nullptr;
    }

    int numRecords = 0;
    PDNS_RECORD p = pDnsRecord;
    while (p) {
        if (p->wType == DNS_TYPE_SRV) numRecords++;
        p = p->pNext;
    }

    SrvRecord* results = (SrvRecord*)malloc(numRecords * sizeof(SrvRecord));
    p = pDnsRecord;
    int i = 0;
    while (p) {
        if (p->wType == DNS_TYPE_SRV) {
            results[i].name = allocWideString(p->Data.SRV.pNameTarget);
            results[i].port = p->Data.SRV.wPort;
            results[i].priority = p->Data.SRV.wPriority;
            results[i].weight = p->Data.SRV.wWeight;
            i++;
        }
        p = p->pNext;
    }

    DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
    *count = numRecords;
    if (errorCode) *errorCode = 0;
    return results;
#else
    if (errorCode) *errorCode = DNS_NOTIMP;
    *count = 0;
    return nullptr;
#endif
}

char* nova_dns_SrvRecord_name(void* record) {
    if (!record) return nullptr;
    return allocString(((SrvRecord*)record)->name);
}

int nova_dns_SrvRecord_port(void* record) {
    if (!record) return 0;
    return ((SrvRecord*)record)->port;
}

int nova_dns_SrvRecord_priority(void* record) {
    if (!record) return 0;
    return ((SrvRecord*)record)->priority;
}

int nova_dns_SrvRecord_weight(void* record) {
    if (!record) return 0;
    return ((SrvRecord*)record)->weight;
}

// ============================================================================
// dns.resolvePtr() - Resolve PTR records (reverse DNS)
// ============================================================================

char** nova_dns_resolvePtr(const char* ip, int* count, int* errorCode) {
#ifdef _WIN32
    initWinsock();

    // Convert IP to reverse DNS format
    std::string reverseIp;
    struct in_addr addr;
    if (inet_pton(AF_INET, ip, &addr) == 1) {
        unsigned char* bytes = (unsigned char*)&addr.s_addr;
        char buf[64];
        snprintf(buf, sizeof(buf), "%d.%d.%d.%d.in-addr.arpa",
                bytes[3], bytes[2], bytes[1], bytes[0]);
        reverseIp = buf;
    } else {
        if (errorCode) *errorCode = DNS_BADNAME;
        *count = 0;
        return nullptr;
    }

    PDNS_RECORD pDnsRecord = nullptr;
    DNS_STATUS status = DnsQuery_A(reverseIp.c_str(), DNS_TYPE_PTR, DNS_QUERY_STANDARD, nullptr, &pDnsRecord, nullptr);

    if (status != 0 || !pDnsRecord) {
        if (errorCode) *errorCode = DNS_NOTFOUND;
        *count = 0;
        return nullptr;
    }

    int numRecords = 0;
    PDNS_RECORD p = pDnsRecord;
    while (p) {
        if (p->wType == DNS_TYPE_PTR) numRecords++;
        p = p->pNext;
    }

    char** results = (char**)malloc(numRecords * sizeof(char*));
    p = pDnsRecord;
    int i = 0;
    while (p) {
        if (p->wType == DNS_TYPE_PTR) {
            results[i++] = allocWideString(p->Data.PTR.pNameHost);
        }
        p = p->pNext;
    }

    DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
    *count = numRecords;
    if (errorCode) *errorCode = 0;
    return results;
#else
    if (errorCode) *errorCode = DNS_NOTIMP;
    *count = 0;
    return nullptr;
#endif
}

// ============================================================================
// dns.reverse() - Reverse DNS lookup
// ============================================================================

char** nova_dns_reverse(const char* ip, int* count, int* errorCode) {
    return nova_dns_resolvePtr(ip, count, errorCode);
}

// ============================================================================
// dns.resolveSoa() - Resolve SOA records
// ============================================================================

struct SoaRecord {
    char* nsname;
    char* hostmaster;
    int serial;
    int refresh;
    int retry;
    int expire;
    int minttl;
};

void* nova_dns_resolveSoa(const char* hostname, int* errorCode) {
#ifdef _WIN32
    initWinsock();

    PDNS_RECORD pDnsRecord = nullptr;
    DNS_STATUS status = DnsQuery_A(hostname, DNS_TYPE_SOA, DNS_QUERY_STANDARD, nullptr, &pDnsRecord, nullptr);

    if (status != 0 || !pDnsRecord) {
        if (errorCode) *errorCode = DNS_NOTFOUND;
        return nullptr;
    }

    SoaRecord* result = nullptr;
    PDNS_RECORD p = pDnsRecord;
    while (p) {
        if (p->wType == DNS_TYPE_SOA) {
            result = (SoaRecord*)malloc(sizeof(SoaRecord));
            result->nsname = allocWideString(p->Data.SOA.pNamePrimaryServer);
            result->hostmaster = allocWideString(p->Data.SOA.pNameAdministrator);
            result->serial = p->Data.SOA.dwSerialNo;
            result->refresh = p->Data.SOA.dwRefresh;
            result->retry = p->Data.SOA.dwRetry;
            result->expire = p->Data.SOA.dwExpire;
            result->minttl = p->Data.SOA.dwDefaultTtl;
            break;
        }
        p = p->pNext;
    }

    DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
    if (errorCode) *errorCode = result ? 0 : DNS_NOTFOUND;
    return result;
#else
    if (errorCode) *errorCode = DNS_NOTIMP;
    return nullptr;
#endif
}

char* nova_dns_SoaRecord_nsname(void* record) {
    if (!record) return nullptr;
    return allocString(((SoaRecord*)record)->nsname);
}

char* nova_dns_SoaRecord_hostmaster(void* record) {
    if (!record) return nullptr;
    return allocString(((SoaRecord*)record)->hostmaster);
}

int nova_dns_SoaRecord_serial(void* record) {
    if (!record) return 0;
    return ((SoaRecord*)record)->serial;
}

int nova_dns_SoaRecord_refresh(void* record) {
    if (!record) return 0;
    return ((SoaRecord*)record)->refresh;
}

int nova_dns_SoaRecord_retry(void* record) {
    if (!record) return 0;
    return ((SoaRecord*)record)->retry;
}

int nova_dns_SoaRecord_expire(void* record) {
    if (!record) return 0;
    return ((SoaRecord*)record)->expire;
}

int nova_dns_SoaRecord_minttl(void* record) {
    if (!record) return 0;
    return ((SoaRecord*)record)->minttl;
}

// ============================================================================
// dns.resolveCaa() - Resolve CAA records
// ============================================================================

struct CaaRecord {
    int critical;
    char* issue;   // or issuewild, iodef
    char* value;
};

void* nova_dns_resolveCaa(const char* hostname, int* count, int* errorCode) {
#ifdef _WIN32
    initWinsock();

    // CAA record type is 257
    PDNS_RECORD pDnsRecord = nullptr;
    DNS_STATUS status = DnsQuery_A(hostname, 257, DNS_QUERY_STANDARD, nullptr, &pDnsRecord, nullptr);

    if (status != 0 || !pDnsRecord) {
        if (errorCode) *errorCode = DNS_NOTFOUND;
        *count = 0;
        return nullptr;
    }

    // Count CAA records
    int numRecords = 0;
    PDNS_RECORD p = pDnsRecord;
    while (p) {
        if (p->wType == 257) numRecords++;
        p = p->pNext;
    }

    if (numRecords == 0) {
        DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
        if (errorCode) *errorCode = DNS_NODATA;
        *count = 0;
        return nullptr;
    }

    CaaRecord* results = (CaaRecord*)malloc(numRecords * sizeof(CaaRecord));
    p = pDnsRecord;
    int i = 0;
    while (p) {
        if (p->wType == 257) {
            // CAA record format: flags (1 byte) + tag length (1 byte) + tag + value
            BYTE* data = (BYTE*)&p->Data;
            results[i].critical = data[0] & 0x80 ? 1 : 0;
            int tagLen = data[1];
            char tag[256] = {0};
            memcpy(tag, &data[2], tagLen);
            results[i].issue = allocString(tag);
            results[i].value = allocString((char*)&data[2 + tagLen]);
            i++;
        }
        p = p->pNext;
    }

    DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
    *count = numRecords;
    if (errorCode) *errorCode = 0;
    return results;
#else
    // Unix implementation using res_query
    if (errorCode) *errorCode = DNS_NOTIMP;
    *count = 0;
    return nullptr;
#endif
}

// ============================================================================
// dns.resolveNaptr() - Resolve NAPTR records
// ============================================================================

struct NaptrRecord {
    char* flags;
    char* service;
    char* regexp;
    char* replacement;
    int order;
    int preference;
};

void* nova_dns_resolveNaptr(const char* hostname, int* count, int* errorCode) {
#ifdef _WIN32
    initWinsock();

    PDNS_RECORD pDnsRecord = nullptr;
    DNS_STATUS status = DnsQuery_A(hostname, DNS_TYPE_NAPTR, DNS_QUERY_STANDARD, nullptr, &pDnsRecord, nullptr);

    if (status != 0 || !pDnsRecord) {
        if (errorCode) *errorCode = DNS_NOTFOUND;
        *count = 0;
        return nullptr;
    }

    // Count NAPTR records
    int numRecords = 0;
    PDNS_RECORD p = pDnsRecord;
    while (p) {
        if (p->wType == DNS_TYPE_NAPTR) numRecords++;
        p = p->pNext;
    }

    if (numRecords == 0) {
        DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
        if (errorCode) *errorCode = DNS_NODATA;
        *count = 0;
        return nullptr;
    }

    NaptrRecord* results = (NaptrRecord*)malloc(numRecords * sizeof(NaptrRecord));
    p = pDnsRecord;
    int i = 0;
    while (p) {
        if (p->wType == DNS_TYPE_NAPTR) {
            results[i].order = p->Data.NAPTR.wOrder;
            results[i].preference = p->Data.NAPTR.wPreference;
            results[i].flags = allocWideString(p->Data.NAPTR.pFlags);
            results[i].service = allocWideString(p->Data.NAPTR.pService);
            results[i].regexp = allocWideString(p->Data.NAPTR.pRegularExpression);
            results[i].replacement = allocWideString(p->Data.NAPTR.pReplacement);
            i++;
        }
        p = p->pNext;
    }

    DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
    *count = numRecords;
    if (errorCode) *errorCode = 0;
    return results;
#else
    if (errorCode) *errorCode = DNS_NOTIMP;
    *count = 0;
    return nullptr;
#endif
}

// Accessors for NaptrRecord
char* nova_dns_NaptrRecord_flags(void* record) {
    if (!record) return nullptr;
    return allocString(((NaptrRecord*)record)->flags);
}

char* nova_dns_NaptrRecord_service(void* record) {
    if (!record) return nullptr;
    return allocString(((NaptrRecord*)record)->service);
}

char* nova_dns_NaptrRecord_regexp(void* record) {
    if (!record) return nullptr;
    return allocString(((NaptrRecord*)record)->regexp);
}

char* nova_dns_NaptrRecord_replacement(void* record) {
    if (!record) return nullptr;
    return allocString(((NaptrRecord*)record)->replacement);
}

int nova_dns_NaptrRecord_order(void* record) {
    if (!record) return 0;
    return ((NaptrRecord*)record)->order;
}

int nova_dns_NaptrRecord_preference(void* record) {
    if (!record) return 0;
    return ((NaptrRecord*)record)->preference;
}

// Free NAPTR records
void nova_dns_freeNaptrRecords(void* records, int count) {
    if (records) {
        NaptrRecord* naptr = (NaptrRecord*)records;
        for (int i = 0; i < count; i++) {
            if (naptr[i].flags) free(naptr[i].flags);
            if (naptr[i].service) free(naptr[i].service);
            if (naptr[i].regexp) free(naptr[i].regexp);
            if (naptr[i].replacement) free(naptr[i].replacement);
        }
        free(records);
    }
}

// Accessors for CaaRecord
int nova_dns_CaaRecord_critical(void* record) {
    if (!record) return 0;
    return ((CaaRecord*)record)->critical;
}

char* nova_dns_CaaRecord_issue(void* record) {
    if (!record) return nullptr;
    return allocString(((CaaRecord*)record)->issue);
}

char* nova_dns_CaaRecord_value(void* record) {
    if (!record) return nullptr;
    return allocString(((CaaRecord*)record)->value);
}

// Free CAA records
void nova_dns_freeCaaRecords(void* records, int count) {
    if (records) {
        CaaRecord* caa = (CaaRecord*)records;
        for (int i = 0; i < count; i++) {
            if (caa[i].issue) free(caa[i].issue);
            if (caa[i].value) free(caa[i].value);
        }
        free(records);
    }
}

// ============================================================================
// dns.resolveAny() - Resolve any record type
// ============================================================================

// Structure to hold any record type result
struct AnyRecord {
    int type;          // Record type (A=1, AAAA=28, CNAME=5, MX=15, NS=2, TXT=16, etc)
    char* value;       // String representation of the value
    char* typeName;    // Type name string
    int ttl;           // TTL in seconds
};

void* nova_dns_resolveAny(const char* hostname, int* count, int* errorCode) {
#ifdef _WIN32
    initWinsock();

    // Query ALL record types
    PDNS_RECORD pDnsRecord = nullptr;
    DNS_STATUS status = DnsQuery_A(hostname, DNS_TYPE_ALL, DNS_QUERY_STANDARD, nullptr, &pDnsRecord, nullptr);

    if (status != 0 || !pDnsRecord) {
        if (errorCode) *errorCode = DNS_NOTFOUND;
        *count = 0;
        return nullptr;
    }

    // Count all records
    int numRecords = 0;
    PDNS_RECORD p = pDnsRecord;
    while (p) {
        numRecords++;
        p = p->pNext;
    }

    if (numRecords == 0) {
        DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
        if (errorCode) *errorCode = DNS_NODATA;
        *count = 0;
        return nullptr;
    }

    AnyRecord* results = (AnyRecord*)malloc(numRecords * sizeof(AnyRecord));
    p = pDnsRecord;
    int i = 0;
    char buf[512];

    while (p) {
        results[i].type = p->wType;
        results[i].ttl = p->dwTtl;

        switch (p->wType) {
            case DNS_TYPE_A: {
                struct in_addr addr;
                addr.s_addr = p->Data.A.IpAddress;
                inet_ntop(AF_INET, &addr, buf, sizeof(buf));
                results[i].value = allocString(buf);
                results[i].typeName = allocString("A");
                break;
            }
            case DNS_TYPE_AAAA: {
                inet_ntop(AF_INET6, &p->Data.AAAA.Ip6Address, buf, sizeof(buf));
                results[i].value = allocString(buf);
                results[i].typeName = allocString("AAAA");
                break;
            }
            case DNS_TYPE_CNAME:
                results[i].value = allocWideString(p->Data.CNAME.pNameHost);
                results[i].typeName = allocString("CNAME");
                break;
            case DNS_TYPE_MX:
                snprintf(buf, sizeof(buf), "%d %S", p->Data.MX.wPreference, p->Data.MX.pNameExchange);
                results[i].value = allocString(buf);
                results[i].typeName = allocString("MX");
                break;
            case DNS_TYPE_NS:
                results[i].value = allocWideString(p->Data.NS.pNameHost);
                results[i].typeName = allocString("NS");
                break;
            case DNS_TYPE_TEXT:
                if (p->Data.TXT.dwStringCount > 0) {
                    results[i].value = allocWideString(p->Data.TXT.pStringArray[0]);
                } else {
                    results[i].value = allocString("");
                }
                results[i].typeName = allocString("TXT");
                break;
            case DNS_TYPE_SOA:
                snprintf(buf, sizeof(buf), "%S %S %u %u %u %u %u",
                    p->Data.SOA.pNamePrimaryServer,
                    p->Data.SOA.pNameAdministrator,
                    p->Data.SOA.dwSerialNo,
                    p->Data.SOA.dwRefresh,
                    p->Data.SOA.dwRetry,
                    p->Data.SOA.dwExpire,
                    p->Data.SOA.dwDefaultTtl);
                results[i].value = allocString(buf);
                results[i].typeName = allocString("SOA");
                break;
            case DNS_TYPE_PTR:
                results[i].value = allocWideString(p->Data.PTR.pNameHost);
                results[i].typeName = allocString("PTR");
                break;
            case DNS_TYPE_SRV:
                snprintf(buf, sizeof(buf), "%d %d %d %S",
                    p->Data.SRV.wPriority,
                    p->Data.SRV.wWeight,
                    p->Data.SRV.wPort,
                    p->Data.SRV.pNameTarget);
                results[i].value = allocString(buf);
                results[i].typeName = allocString("SRV");
                break;
            default:
                snprintf(buf, sizeof(buf), "TYPE%d", p->wType);
                results[i].value = allocString("");
                results[i].typeName = allocString(buf);
                break;
        }
        i++;
        p = p->pNext;
    }

    DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
    *count = numRecords;
    if (errorCode) *errorCode = 0;
    return results;
#else
    if (errorCode) *errorCode = DNS_NOTIMP;
    *count = 0;
    return nullptr;
#endif
}

// Accessors for AnyRecord
int nova_dns_AnyRecord_type(void* record) {
    if (!record) return 0;
    return ((AnyRecord*)record)->type;
}

char* nova_dns_AnyRecord_value(void* record) {
    if (!record) return nullptr;
    return allocString(((AnyRecord*)record)->value);
}

char* nova_dns_AnyRecord_typeName(void* record) {
    if (!record) return nullptr;
    return allocString(((AnyRecord*)record)->typeName);
}

int nova_dns_AnyRecord_ttl(void* record) {
    if (!record) return 0;
    return ((AnyRecord*)record)->ttl;
}

// Free AnyRecords
void nova_dns_freeAnyRecords(void* records, int count) {
    if (records) {
        AnyRecord* any = (AnyRecord*)records;
        for (int i = 0; i < count; i++) {
            if (any[i].value) free(any[i].value);
            if (any[i].typeName) free(any[i].typeName);
        }
        free(records);
    }
}

// ============================================================================
// Server Configuration
// ============================================================================

// Set DNS servers
void nova_dns_setServers(const char** servers, int count) {
    customServers.clear();
    for (int i = 0; i < count; i++) {
        if (servers[i]) {
            customServers.push_back(servers[i]);
        }
    }
}

// Get DNS servers
char** nova_dns_getServers(int* count) {
    if (customServers.empty()) {
        // Return system default (simplified)
        *count = 0;
        return nullptr;
    }

    *count = (int)customServers.size();
    char** result = (char**)malloc(*count * sizeof(char*));
    for (int i = 0; i < *count; i++) {
        result[i] = allocString(customServers[i]);
    }
    return result;
}

// Set default result order
void nova_dns_setDefaultResultOrder(const char* order) {
    if (order) {
        if (strcmp(order, "ipv4first") == 0) {
            defaultResultOrder = 0;
        } else if (strcmp(order, "verbatim") == 0) {
            defaultResultOrder = 1;
        }
    }
}

// Get default result order
char* nova_dns_getDefaultResultOrder() {
    return allocString(defaultResultOrder == 0 ? "ipv4first" : "verbatim");
}

// ============================================================================
// Utility Functions
// ============================================================================

// Free string array
void nova_dns_freeStringArray(char** arr, int count) {
    if (arr) {
        for (int i = 0; i < count; i++) {
            if (arr[i]) free(arr[i]);
        }
        free(arr);
    }
}

// Free MX records
void nova_dns_freeMxRecords(void* records, int count) {
    if (records) {
        MxRecord* mx = (MxRecord*)records;
        for (int i = 0; i < count; i++) {
            if (mx[i].exchange) free(mx[i].exchange);
        }
        free(records);
    }
}

// Free SRV records
void nova_dns_freeSrvRecords(void* records, int count) {
    if (records) {
        SrvRecord* srv = (SrvRecord*)records;
        for (int i = 0; i < count; i++) {
            if (srv[i].name) free(srv[i].name);
        }
        free(records);
    }
}

// Free SOA record
void nova_dns_freeSoaRecord(void* record) {
    if (record) {
        SoaRecord* soa = (SoaRecord*)record;
        if (soa->nsname) free(soa->nsname);
        if (soa->hostmaster) free(soa->hostmaster);
        free(record);
    }
}

} // extern "C"

} // namespace dns
} // namespace runtime
} // namespace nova
