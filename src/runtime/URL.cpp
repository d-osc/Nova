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

} // extern "C"
