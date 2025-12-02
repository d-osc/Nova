// URL Runtime Implementation for Nova Compiler
// Web APIs: URL, URLSearchParams

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>

// ============================================================================
// URL Structure (outside extern "C" due to C++ members)
// ============================================================================

struct NovaURL {
    char* href;
    char* protocol;
    char* username;
    char* password;
    char* host;
    char* hostname;
    char* port;
    char* pathname;
    char* search;
    char* hash;
    char* origin;
    void* searchParams;  // NovaURLSearchParams*
};

// ============================================================================
// URLSearchParams Structure
// ============================================================================

struct NovaURLSearchParamsEntry {
    std::string key;
    std::string value;
};

struct NovaURLSearchParams {
    std::vector<NovaURLSearchParamsEntry>* entries;
};

// ============================================================================
// Helper: Duplicate string
// ============================================================================

static char* dupString(const std::string& s) {
    char* dup = new char[s.length() + 1];
    strcpy(dup, s.c_str());
    return dup;
}

// ============================================================================
// Helper: URL percent-encode
// ============================================================================

static std::string percentEncode(const std::string& s) {
    std::ostringstream encoded;
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c;
        } else {
            encoded << '%' << std::uppercase << std::hex << (int)c;
        }
    }
    return encoded.str();
}

static std::string percentDecode(const std::string& s) {
    std::string decoded;
    for (size_t i = 0; i < s.length(); i++) {
        if (s[i] == '%' && i + 2 < s.length()) {
            int value;
            std::istringstream iss(s.substr(i + 1, 2));
            if (iss >> std::hex >> value) {
                decoded += static_cast<char>(value);
                i += 2;
            } else {
                decoded += s[i];
            }
        } else if (s[i] == '+') {
            decoded += ' ';
        } else {
            decoded += s[i];
        }
    }
    return decoded;
}

// Forward declarations
extern "C" void* nova_urlsearchparams_create(const char* init);
extern "C" void nova_url_destroy(void* urlPtr);
extern "C" const char* nova_urlsearchparams_toString(void* paramsPtr);
extern "C" void nova_urlsearchparams_destroy(void* paramsPtr);

// ============================================================================
// Helper: Parse URL
// ============================================================================

static void parseURL(NovaURL* url, const std::string& href) {
    std::string remaining = href;

    // Extract protocol
    size_t protocolEnd = remaining.find("://");
    std::string protocol = "";
    if (protocolEnd != std::string::npos) {
        protocol = remaining.substr(0, protocolEnd + 1);  // includes ':'
        remaining = remaining.substr(protocolEnd + 3);
    }
    url->protocol = dupString(protocol);

    // Extract hash
    std::string hash = "";
    size_t hashPos = remaining.find('#');
    if (hashPos != std::string::npos) {
        hash = remaining.substr(hashPos);
        remaining = remaining.substr(0, hashPos);
    }
    url->hash = dupString(hash);

    // Extract search
    std::string search = "";
    size_t searchPos = remaining.find('?');
    if (searchPos != std::string::npos) {
        search = remaining.substr(searchPos);
        remaining = remaining.substr(0, searchPos);
    }
    url->search = dupString(search);

    // Extract pathname
    std::string pathname = "/";
    size_t pathStart = remaining.find('/');
    if (pathStart != std::string::npos) {
        pathname = remaining.substr(pathStart);
        remaining = remaining.substr(0, pathStart);
    }
    url->pathname = dupString(pathname);

    // Extract userinfo (username:password@)
    std::string username = "";
    std::string password = "";
    size_t atPos = remaining.find('@');
    if (atPos != std::string::npos) {
        std::string userinfo = remaining.substr(0, atPos);
        remaining = remaining.substr(atPos + 1);

        size_t colonPos = userinfo.find(':');
        if (colonPos != std::string::npos) {
            username = userinfo.substr(0, colonPos);
            password = userinfo.substr(colonPos + 1);
        } else {
            username = userinfo;
        }
    }
    url->username = dupString(username);
    url->password = dupString(password);

    // Extract host (hostname:port)
    std::string hostname = remaining;
    std::string port = "";
    size_t portPos = remaining.rfind(':');
    if (portPos != std::string::npos) {
        // Check if it's actually a port (all digits after colon)
        std::string potentialPort = remaining.substr(portPos + 1);
        bool isPort = !potentialPort.empty();
        for (char c : potentialPort) {
            if (!isdigit(c)) {
                isPort = false;
                break;
            }
        }
        if (isPort) {
            hostname = remaining.substr(0, portPos);
            port = potentialPort;
        }
    }
    url->hostname = dupString(hostname);
    url->port = dupString(port);

    // Construct host
    std::string host = hostname;
    if (!port.empty()) {
        host += ":" + port;
    }
    url->host = dupString(host);

    // Construct origin
    std::string origin = protocol + "//" + host;
    url->origin = dupString(origin);

    // Construct full href
    std::ostringstream hrefBuilder;
    hrefBuilder << protocol << "//";
    if (!username.empty()) {
        hrefBuilder << username;
        if (!password.empty()) {
            hrefBuilder << ":" << password;
        }
        hrefBuilder << "@";
    }
    hrefBuilder << host << pathname << search << hash;
    url->href = dupString(hrefBuilder.str());

    // Create searchParams
    if (search.length() > 1) {
        url->searchParams = nova_urlsearchparams_create(search.c_str() + 1);  // Skip '?'
    } else {
        url->searchParams = nova_urlsearchparams_create("");
    }
}

extern "C" {

// ============================================================================
// URL Constructor: new URL(url, base?)
// ============================================================================

void* nova_url_create(const char* urlStr) {
    NovaURL* url = new NovaURL();
    memset(url, 0, sizeof(NovaURL));

    if (urlStr) {
        parseURL(url, std::string(urlStr));
    }

    return url;
}

void* nova_url_create_with_base(const char* urlStr, const char* baseStr) {
    // Simple base URL resolution
    std::string fullUrl;
    std::string urlString = urlStr ? urlStr : "";
    std::string baseString = baseStr ? baseStr : "";

    if (urlString.find("://") != std::string::npos) {
        // Absolute URL, ignore base
        fullUrl = urlString;
    } else if (!urlString.empty() && urlString[0] == '/') {
        // Absolute path, use base origin
        NovaURL* baseUrl = static_cast<NovaURL*>(nova_url_create(baseStr));
        fullUrl = std::string(baseUrl->origin) + urlString;
        nova_url_destroy(baseUrl);
    } else {
        // Relative path
        NovaURL* baseUrl = static_cast<NovaURL*>(nova_url_create(baseStr));
        std::string basePath = baseUrl->pathname;
        size_t lastSlash = basePath.rfind('/');
        if (lastSlash != std::string::npos) {
            basePath = basePath.substr(0, lastSlash + 1);
        }
        fullUrl = std::string(baseUrl->origin) + basePath + urlString;
        nova_url_destroy(baseUrl);
    }

    return nova_url_create(fullUrl.c_str());
}

// ============================================================================
// URL Property Getters
// ============================================================================

const char* nova_url_get_href(void* urlPtr) {
    if (!urlPtr) return "";
    return static_cast<NovaURL*>(urlPtr)->href;
}

const char* nova_url_get_protocol(void* urlPtr) {
    if (!urlPtr) return "";
    return static_cast<NovaURL*>(urlPtr)->protocol;
}

const char* nova_url_get_username(void* urlPtr) {
    if (!urlPtr) return "";
    return static_cast<NovaURL*>(urlPtr)->username;
}

const char* nova_url_get_password(void* urlPtr) {
    if (!urlPtr) return "";
    return static_cast<NovaURL*>(urlPtr)->password;
}

const char* nova_url_get_host(void* urlPtr) {
    if (!urlPtr) return "";
    return static_cast<NovaURL*>(urlPtr)->host;
}

const char* nova_url_get_hostname(void* urlPtr) {
    if (!urlPtr) return "";
    return static_cast<NovaURL*>(urlPtr)->hostname;
}

const char* nova_url_get_port(void* urlPtr) {
    if (!urlPtr) return "";
    return static_cast<NovaURL*>(urlPtr)->port;
}

const char* nova_url_get_pathname(void* urlPtr) {
    if (!urlPtr) return "";
    return static_cast<NovaURL*>(urlPtr)->pathname;
}

const char* nova_url_get_search(void* urlPtr) {
    if (!urlPtr) return "";
    return static_cast<NovaURL*>(urlPtr)->search;
}

const char* nova_url_get_hash(void* urlPtr) {
    if (!urlPtr) return "";
    return static_cast<NovaURL*>(urlPtr)->hash;
}

const char* nova_url_get_origin(void* urlPtr) {
    if (!urlPtr) return "";
    return static_cast<NovaURL*>(urlPtr)->origin;
}

void* nova_url_get_searchParams(void* urlPtr) {
    if (!urlPtr) return nullptr;
    return static_cast<NovaURL*>(urlPtr)->searchParams;
}

// ============================================================================
// URL.prototype.toString(), toJSON()
// ============================================================================

const char* nova_url_toString(void* urlPtr) {
    return nova_url_get_href(urlPtr);
}

const char* nova_url_toJSON(void* urlPtr) {
    return nova_url_get_href(urlPtr);
}

// ============================================================================
// URL Property Setters
// ============================================================================

// Helper to rebuild href after property change
static void rebuildHref(NovaURL* url) {
    std::ostringstream hrefBuilder;
    hrefBuilder << url->protocol << "//";
    if (url->username && strlen(url->username) > 0) {
        hrefBuilder << url->username;
        if (url->password && strlen(url->password) > 0) {
            hrefBuilder << ":" << url->password;
        }
        hrefBuilder << "@";
    }
    hrefBuilder << url->host << url->pathname << url->search << url->hash;
    delete[] url->href;
    url->href = dupString(hrefBuilder.str());
}

void nova_url_set_href(void* urlPtr, const char* value) {
    if (!urlPtr || !value) return;
    NovaURL* url = static_cast<NovaURL*>(urlPtr);
    // Re-parse the entire URL
    delete[] url->href;
    delete[] url->protocol;
    delete[] url->username;
    delete[] url->password;
    delete[] url->host;
    delete[] url->hostname;
    delete[] url->port;
    delete[] url->pathname;
    delete[] url->search;
    delete[] url->hash;
    delete[] url->origin;
    parseURL(url, std::string(value));
}

void nova_url_set_protocol(void* urlPtr, const char* value) {
    if (!urlPtr || !value) return;
    NovaURL* url = static_cast<NovaURL*>(urlPtr);
    std::string proto = value;
    if (proto.back() != ':') proto += ':';
    delete[] url->protocol;
    url->protocol = dupString(proto);
    // Rebuild origin
    delete[] url->origin;
    url->origin = dupString(proto + "//" + url->host);
    rebuildHref(url);
}

void nova_url_set_username(void* urlPtr, const char* value) {
    if (!urlPtr) return;
    NovaURL* url = static_cast<NovaURL*>(urlPtr);
    delete[] url->username;
    url->username = dupString(value ? value : "");
    rebuildHref(url);
}

void nova_url_set_password(void* urlPtr, const char* value) {
    if (!urlPtr) return;
    NovaURL* url = static_cast<NovaURL*>(urlPtr);
    delete[] url->password;
    url->password = dupString(value ? value : "");
    rebuildHref(url);
}

void nova_url_set_host(void* urlPtr, const char* value) {
    if (!urlPtr || !value) return;
    NovaURL* url = static_cast<NovaURL*>(urlPtr);
    std::string hostStr = value;
    delete[] url->host;
    url->host = dupString(hostStr);

    // Parse hostname and port
    size_t colonPos = hostStr.rfind(':');
    if (colonPos != std::string::npos) {
        delete[] url->hostname;
        url->hostname = dupString(hostStr.substr(0, colonPos));
        delete[] url->port;
        url->port = dupString(hostStr.substr(colonPos + 1));
    } else {
        delete[] url->hostname;
        url->hostname = dupString(hostStr);
        delete[] url->port;
        url->port = dupString("");
    }
    // Rebuild origin
    delete[] url->origin;
    url->origin = dupString(std::string(url->protocol) + "//" + url->host);
    rebuildHref(url);
}

void nova_url_set_hostname(void* urlPtr, const char* value) {
    if (!urlPtr || !value) return;
    NovaURL* url = static_cast<NovaURL*>(urlPtr);
    delete[] url->hostname;
    url->hostname = dupString(value);
    // Rebuild host
    std::string host = value;
    if (url->port && strlen(url->port) > 0) {
        host += ":" + std::string(url->port);
    }
    delete[] url->host;
    url->host = dupString(host);
    // Rebuild origin
    delete[] url->origin;
    url->origin = dupString(std::string(url->protocol) + "//" + url->host);
    rebuildHref(url);
}

void nova_url_set_port(void* urlPtr, const char* value) {
    if (!urlPtr) return;
    NovaURL* url = static_cast<NovaURL*>(urlPtr);
    delete[] url->port;
    url->port = dupString(value ? value : "");
    // Rebuild host
    std::string host = url->hostname;
    if (value && strlen(value) > 0) {
        host += ":" + std::string(value);
    }
    delete[] url->host;
    url->host = dupString(host);
    // Rebuild origin
    delete[] url->origin;
    url->origin = dupString(std::string(url->protocol) + "//" + url->host);
    rebuildHref(url);
}

void nova_url_set_pathname(void* urlPtr, const char* value) {
    if (!urlPtr) return;
    NovaURL* url = static_cast<NovaURL*>(urlPtr);
    std::string path = value ? value : "/";
    if (path.empty() || path[0] != '/') path = "/" + path;
    delete[] url->pathname;
    url->pathname = dupString(path);
    rebuildHref(url);
}

void nova_url_set_search(void* urlPtr, const char* value) {
    if (!urlPtr) return;
    NovaURL* url = static_cast<NovaURL*>(urlPtr);
    std::string search = value ? value : "";
    if (!search.empty() && search[0] != '?') search = "?" + search;
    delete[] url->search;
    url->search = dupString(search);
    // Update searchParams
    if (url->searchParams) {
        nova_urlsearchparams_destroy(url->searchParams);
    }
    url->searchParams = nova_urlsearchparams_create(search.length() > 1 ? search.c_str() + 1 : "");
    rebuildHref(url);
}

void nova_url_set_hash(void* urlPtr, const char* value) {
    if (!urlPtr) return;
    NovaURL* url = static_cast<NovaURL*>(urlPtr);
    std::string hash = value ? value : "";
    if (!hash.empty() && hash[0] != '#') hash = "#" + hash;
    delete[] url->hash;
    url->hash = dupString(hash);
    rebuildHref(url);
}

// ============================================================================
// URL Static Methods: canParse(), parse()
// ============================================================================

int64_t nova_url_canParse(const char* urlStr) {
    if (!urlStr) return 0;
    std::string s(urlStr);
    // Basic validation: must have protocol
    return s.find("://") != std::string::npos ? 1 : 0;
}

int64_t nova_url_canParse_with_base(const char* urlStr, const char* baseStr) {
    if (urlStr && std::string(urlStr).find("://") != std::string::npos) {
        return 1;
    }
    if (baseStr && std::string(baseStr).find("://") != std::string::npos) {
        return 1;
    }
    return 0;
}

void* nova_url_parse(const char* urlStr) {
    if (!nova_url_canParse(urlStr)) return nullptr;
    return nova_url_create(urlStr);
}

void* nova_url_parse_with_base(const char* urlStr, const char* baseStr) {
    if (!nova_url_canParse_with_base(urlStr, baseStr)) return nullptr;
    return nova_url_create_with_base(urlStr, baseStr);
}

// ============================================================================
// URL Destructor
// ============================================================================

void nova_url_destroy(void* urlPtr) {
    if (!urlPtr) return;

    NovaURL* url = static_cast<NovaURL*>(urlPtr);
    delete[] url->href;
    delete[] url->protocol;
    delete[] url->username;
    delete[] url->password;
    delete[] url->host;
    delete[] url->hostname;
    delete[] url->port;
    delete[] url->pathname;
    delete[] url->search;
    delete[] url->hash;
    delete[] url->origin;
    // Don't delete searchParams here - managed separately
    delete url;
}

// ============================================================================
// URLSearchParams Constructor
// ============================================================================

void* nova_urlsearchparams_create(const char* init) {
    NovaURLSearchParams* params = new NovaURLSearchParams();
    params->entries = new std::vector<NovaURLSearchParamsEntry>();

    if (init && strlen(init) > 0) {
        std::string s = init;
        // Skip leading '?' if present
        if (s[0] == '?') s = s.substr(1);

        // Parse key=value pairs
        std::istringstream stream(s);
        std::string pair;
        while (std::getline(stream, pair, '&')) {
            if (pair.empty()) continue;

            size_t eqPos = pair.find('=');
            NovaURLSearchParamsEntry entry;
            if (eqPos != std::string::npos) {
                entry.key = percentDecode(pair.substr(0, eqPos));
                entry.value = percentDecode(pair.substr(eqPos + 1));
            } else {
                entry.key = percentDecode(pair);
                entry.value = "";
            }
            params->entries->push_back(entry);
        }
    }

    return params;
}

// ============================================================================
// URLSearchParams.prototype.append(name, value)
// ============================================================================

void nova_urlsearchparams_append(void* paramsPtr, const char* name, const char* value) {
    if (!paramsPtr || !name) return;

    NovaURLSearchParams* params = static_cast<NovaURLSearchParams*>(paramsPtr);
    NovaURLSearchParamsEntry entry;
    entry.key = name;
    entry.value = value ? value : "";
    params->entries->push_back(entry);
}

// ============================================================================
// URLSearchParams.prototype.delete(name, value?)
// ============================================================================

void nova_urlsearchparams_delete(void* paramsPtr, const char* name) {
    if (!paramsPtr || !name) return;

    NovaURLSearchParams* params = static_cast<NovaURLSearchParams*>(paramsPtr);
    std::string key = name;

    auto& entries = *params->entries;
    entries.erase(
        std::remove_if(entries.begin(), entries.end(),
            [&key](const NovaURLSearchParamsEntry& e) { return e.key == key; }),
        entries.end()
    );
}

void nova_urlsearchparams_delete_value(void* paramsPtr, const char* name, const char* value) {
    if (!paramsPtr || !name) return;

    NovaURLSearchParams* params = static_cast<NovaURLSearchParams*>(paramsPtr);
    std::string key = name;
    std::string val = value ? value : "";

    auto& entries = *params->entries;
    entries.erase(
        std::remove_if(entries.begin(), entries.end(),
            [&key, &val](const NovaURLSearchParamsEntry& e) {
                return e.key == key && e.value == val;
            }),
        entries.end()
    );
}

// ============================================================================
// URLSearchParams.prototype.get(name)
// ============================================================================

const char* nova_urlsearchparams_get(void* paramsPtr, const char* name) {
    if (!paramsPtr || !name) return nullptr;

    NovaURLSearchParams* params = static_cast<NovaURLSearchParams*>(paramsPtr);
    std::string key = name;

    for (const auto& entry : *params->entries) {
        if (entry.key == key) {
            // Return a persistent string
            static thread_local std::string result;
            result = entry.value;
            return result.c_str();
        }
    }
    return nullptr;
}

// ============================================================================
// URLSearchParams.prototype.getAll(name)
// Returns concatenated values separated by commas
// ============================================================================

const char* nova_urlsearchparams_getAll(void* paramsPtr, const char* name) {
    if (!paramsPtr || !name) return "";

    NovaURLSearchParams* params = static_cast<NovaURLSearchParams*>(paramsPtr);
    std::string key = name;

    static thread_local std::string result;
    result.clear();

    bool first = true;
    for (const auto& entry : *params->entries) {
        if (entry.key == key) {
            if (!first) result += ",";
            result += entry.value;
            first = false;
        }
    }
    return result.c_str();
}

// ============================================================================
// URLSearchParams.prototype.has(name, value?)
// ============================================================================

int64_t nova_urlsearchparams_has(void* paramsPtr, const char* name) {
    if (!paramsPtr || !name) return 0;

    NovaURLSearchParams* params = static_cast<NovaURLSearchParams*>(paramsPtr);
    std::string key = name;

    for (const auto& entry : *params->entries) {
        if (entry.key == key) return 1;
    }
    return 0;
}

int64_t nova_urlsearchparams_has_value(void* paramsPtr, const char* name, const char* value) {
    if (!paramsPtr || !name) return 0;

    NovaURLSearchParams* params = static_cast<NovaURLSearchParams*>(paramsPtr);
    std::string key = name;
    std::string val = value ? value : "";

    for (const auto& entry : *params->entries) {
        if (entry.key == key && entry.value == val) return 1;
    }
    return 0;
}

// ============================================================================
// URLSearchParams.prototype.set(name, value)
// ============================================================================

void nova_urlsearchparams_set(void* paramsPtr, const char* name, const char* value) {
    if (!paramsPtr || !name) return;

    NovaURLSearchParams* params = static_cast<NovaURLSearchParams*>(paramsPtr);
    std::string key = name;
    std::string val = value ? value : "";

    // Remove all existing entries with this key
    auto& entries = *params->entries;
    bool found = false;

    for (auto& entry : entries) {
        if (entry.key == key) {
            if (!found) {
                entry.value = val;
                found = true;
            }
        }
    }

    // Remove duplicates, keeping first
    if (found) {
        bool seenFirst = false;
        entries.erase(
            std::remove_if(entries.begin(), entries.end(),
                [&key, &seenFirst](const NovaURLSearchParamsEntry& e) {
                    if (e.key == key) {
                        if (seenFirst) return true;
                        seenFirst = true;
                    }
                    return false;
                }),
            entries.end()
        );
    } else {
        // Add new entry
        NovaURLSearchParamsEntry entry;
        entry.key = key;
        entry.value = val;
        entries.push_back(entry);
    }
}

// ============================================================================
// URLSearchParams.prototype.sort()
// ============================================================================

void nova_urlsearchparams_sort(void* paramsPtr) {
    if (!paramsPtr) return;

    NovaURLSearchParams* params = static_cast<NovaURLSearchParams*>(paramsPtr);
    std::stable_sort(params->entries->begin(), params->entries->end(),
        [](const NovaURLSearchParamsEntry& a, const NovaURLSearchParamsEntry& b) {
            return a.key < b.key;
        });
}

// ============================================================================
// URLSearchParams.prototype.toString()
// ============================================================================

const char* nova_urlsearchparams_toString(void* paramsPtr) {
    if (!paramsPtr) return "";

    NovaURLSearchParams* params = static_cast<NovaURLSearchParams*>(paramsPtr);

    static thread_local std::string result;
    result.clear();

    bool first = true;
    for (const auto& entry : *params->entries) {
        if (!first) result += "&";
        result += percentEncode(entry.key) + "=" + percentEncode(entry.value);
        first = false;
    }

    return result.c_str();
}

// ============================================================================
// URLSearchParams.prototype.size (getter)
// ============================================================================

int64_t nova_urlsearchparams_size(void* paramsPtr) {
    if (!paramsPtr) return 0;
    NovaURLSearchParams* params = static_cast<NovaURLSearchParams*>(paramsPtr);
    return (int64_t)params->entries->size();
}

// ============================================================================
// URLSearchParams.prototype.keys(), values(), entries() - returns as strings
// ============================================================================

const char* nova_urlsearchparams_keys(void* paramsPtr) {
    if (!paramsPtr) return "";

    NovaURLSearchParams* params = static_cast<NovaURLSearchParams*>(paramsPtr);

    static thread_local std::string result;
    result.clear();

    bool first = true;
    for (const auto& entry : *params->entries) {
        if (!first) result += ",";
        result += entry.key;
        first = false;
    }

    return result.c_str();
}

const char* nova_urlsearchparams_values(void* paramsPtr) {
    if (!paramsPtr) return "";

    NovaURLSearchParams* params = static_cast<NovaURLSearchParams*>(paramsPtr);

    static thread_local std::string result;
    result.clear();

    bool first = true;
    for (const auto& entry : *params->entries) {
        if (!first) result += ",";
        result += entry.value;
        first = false;
    }

    return result.c_str();
}

// ============================================================================
// Destructor
// ============================================================================

void nova_urlsearchparams_destroy(void* paramsPtr) {
    if (!paramsPtr) return;

    NovaURLSearchParams* params = static_cast<NovaURLSearchParams*>(paramsPtr);
    delete params->entries;
    delete params;
}

// ============================================================================
// URLSearchParams.prototype.entries() - returns JSON array of [key, value] pairs
// ============================================================================

const char* nova_urlsearchparams_entries(void* paramsPtr) {
    if (!paramsPtr) return "[]";

    NovaURLSearchParams* params = static_cast<NovaURLSearchParams*>(paramsPtr);

    static thread_local std::string result;
    result = "[";

    bool first = true;
    for (const auto& entry : *params->entries) {
        if (!first) result += ",";
        result += "[\"" + entry.key + "\",\"" + entry.value + "\"]";
        first = false;
    }
    result += "]";

    return result.c_str();
}

// ============================================================================
// URLSearchParams.prototype.forEach(callback)
// ============================================================================

typedef void (*URLSearchParamsForEachCallback)(const char* value, const char* key, void* params);

void nova_urlsearchparams_forEach(void* paramsPtr, URLSearchParamsForEachCallback callback) {
    if (!paramsPtr || !callback) return;

    NovaURLSearchParams* params = static_cast<NovaURLSearchParams*>(paramsPtr);
    for (const auto& entry : *params->entries) {
        callback(entry.value.c_str(), entry.key.c_str(), paramsPtr);
    }
}

// ============================================================================
// Legacy URL module: url.format(urlObject)
// ============================================================================

const char* nova_url_format(void* urlPtr) {
    return nova_url_get_href(urlPtr);
}

// Format from components
const char* nova_url_format_components(const char* protocol, const char* hostname,
                                        const char* port, const char* pathname,
                                        const char* search, const char* hash) {
    static thread_local std::string result;
    result.clear();

    if (protocol && strlen(protocol) > 0) {
        result += protocol;
        if (result.back() != ':') result += ':';
        result += "//";
    }

    if (hostname && strlen(hostname) > 0) {
        result += hostname;
    }

    if (port && strlen(port) > 0) {
        result += ":" + std::string(port);
    }

    if (pathname && strlen(pathname) > 0) {
        if (pathname[0] != '/') result += '/';
        result += pathname;
    } else {
        result += '/';
    }

    if (search && strlen(search) > 0) {
        if (search[0] != '?') result += '?';
        result += search;
    }

    if (hash && strlen(hash) > 0) {
        if (hash[0] != '#') result += '#';
        result += hash;
    }

    return result.c_str();
}

// ============================================================================
// Legacy URL module: url.resolve(from, to)
// ============================================================================

const char* nova_url_resolve(const char* from, const char* to) {
    if (!from || !to) return "";

    static thread_local std::string result;

    std::string toStr = to;

    // If 'to' is absolute, return it
    if (toStr.find("://") != std::string::npos) {
        result = toStr;
        return result.c_str();
    }

    void* fromUrl = nova_url_create(from);
    if (!fromUrl) {
        result = toStr;
        return result.c_str();
    }

    NovaURL* url = static_cast<NovaURL*>(fromUrl);

    if (!toStr.empty() && toStr[0] == '/') {
        // Absolute path
        result = std::string(url->origin) + toStr;
    } else {
        // Relative path
        std::string basePath = url->pathname;
        size_t lastSlash = basePath.rfind('/');
        if (lastSlash != std::string::npos) {
            basePath = basePath.substr(0, lastSlash + 1);
        } else {
            basePath = "/";
        }
        result = std::string(url->origin) + basePath + toStr;
    }

    nova_url_destroy(fromUrl);
    return result.c_str();
}

// ============================================================================
// url.fileURLToPath(url)
// ============================================================================

const char* nova_url_fileURLToPath(const char* urlStr) {
    if (!urlStr) return "";

    static thread_local std::string result;

    void* urlPtr = nova_url_create(urlStr);
    if (!urlPtr) {
        result = "";
        return result.c_str();
    }

    NovaURL* url = static_cast<NovaURL*>(urlPtr);

    // Check if file: protocol
    if (strcmp(url->protocol, "file:") != 0) {
        nova_url_destroy(urlPtr);
        result = "";
        return result.c_str();
    }

    result = percentDecode(url->pathname);

#ifdef _WIN32
    // Convert /C:/path to C:/path on Windows
    if (result.length() >= 3 && result[0] == '/' &&
        isalpha(result[1]) && result[2] == ':') {
        result = result.substr(1);
    }
    // Convert forward slashes to backslashes
    for (char& c : result) {
        if (c == '/') c = '\\';
    }
#endif

    nova_url_destroy(urlPtr);
    return result.c_str();
}

// ============================================================================
// url.pathToFileURL(path)
// ============================================================================

const char* nova_url_pathToFileURL(const char* path) {
    if (!path) return "";

    static thread_local std::string result;
    std::string pathStr = path;

#ifdef _WIN32
    // Convert backslashes to forward slashes
    for (char& c : pathStr) {
        if (c == '\\') c = '/';
    }
    // Add leading slash if Windows absolute path (C:/)
    if (pathStr.length() >= 2 && isalpha(pathStr[0]) && pathStr[1] == ':') {
        pathStr = "/" + pathStr;
    }
#endif

    // Ensure leading slash
    if (pathStr.empty() || pathStr[0] != '/') {
        pathStr = "/" + pathStr;
    }

    result = "file://" + percentEncode(pathStr);
    return result.c_str();
}

// ============================================================================
// url.domainToASCII(domain) - Punycode encoding
// ============================================================================

const char* nova_url_domainToASCII(const char* domain) {
    if (!domain) return "";
    // Simple implementation - just lowercase for ASCII domains
    static thread_local std::string result;
    result = domain;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result.c_str();
}

// ============================================================================
// url.domainToUnicode(domain) - Punycode decoding
// ============================================================================

const char* nova_url_domainToUnicode(const char* domain) {
    if (!domain) return "";
    // Simple implementation - return as-is for ASCII domains
    static thread_local std::string result;
    result = domain;
    return result.c_str();
}

// ============================================================================
// url.urlToHttpOptions(url)
// Returns JSON object with http options
// ============================================================================

const char* nova_url_urlToHttpOptions(void* urlPtr) {
    if (!urlPtr) return "{}";

    NovaURL* url = static_cast<NovaURL*>(urlPtr);

    static thread_local std::string result;
    std::ostringstream json;
    json << "{";
    json << "\"protocol\":\"" << (url->protocol ? url->protocol : "") << "\",";
    json << "\"hostname\":\"" << (url->hostname ? url->hostname : "") << "\",";
    json << "\"hash\":\"" << (url->hash ? url->hash : "") << "\",";
    json << "\"search\":\"" << (url->search ? url->search : "") << "\",";
    json << "\"pathname\":\"" << (url->pathname ? url->pathname : "") << "\",";
    json << "\"path\":\"" << (url->pathname ? url->pathname : "") << (url->search ? url->search : "") << "\",";
    json << "\"href\":\"" << (url->href ? url->href : "") << "\"";

    if (url->port && strlen(url->port) > 0) {
        json << ",\"port\":" << url->port;
    }
    if (url->username && strlen(url->username) > 0) {
        json << ",\"auth\":\"" << url->username;
        if (url->password && strlen(url->password) > 0) {
            json << ":" << url->password;
        }
        json << "\"";
    }
    json << "}";

    result = json.str();
    return result.c_str();
}

// From string
const char* nova_url_urlToHttpOptions_str(const char* urlStr) {
    if (!urlStr) return "{}";
    void* urlPtr = nova_url_create(urlStr);
    const char* result = nova_url_urlToHttpOptions(urlPtr);
    nova_url_destroy(urlPtr);
    return result;
}

// ============================================================================
// Additional utility functions
// ============================================================================

// Encode URI component
const char* nova_url_encodeURIComponent(const char* str) {
    if (!str) return "";
    static thread_local std::string result;
    result = percentEncode(str);
    return result.c_str();
}

// Decode URI component
const char* nova_url_decodeURIComponent(const char* str) {
    if (!str) return "";
    static thread_local std::string result;
    result = percentDecode(str);
    return result.c_str();
}

// Encode URI (less strict than component)
const char* nova_url_encodeURI(const char* str) {
    if (!str) return "";
    static thread_local std::string result;
    std::ostringstream encoded;
    for (unsigned char c : std::string(str)) {
        // Don't encode: A-Z a-z 0-9 - _ . ~ ! ' ( ) * ; , / ? : @ & = + $ #
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' ||
            c == '!' || c == '\'' || c == '(' || c == ')' || c == '*' ||
            c == ';' || c == ',' || c == '/' || c == '?' || c == ':' ||
            c == '@' || c == '&' || c == '=' || c == '+' || c == '$' || c == '#') {
            encoded << c;
        } else {
            encoded << '%' << std::uppercase << std::hex << (int)c;
        }
    }
    result = encoded.str();
    return result.c_str();
}

// Decode URI
const char* nova_url_decodeURI(const char* str) {
    if (!str) return "";
    static thread_local std::string result;
    result = percentDecode(str);
    return result.c_str();
}

} // extern "C"
