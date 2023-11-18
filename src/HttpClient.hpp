#include <iostream>
#include <string>
#include <curl/curl.h>
#include "json.hpp"
#include <sstream>
#include <regex>

nlohmann::json convertHeadersToJson(const std::string& headers) {
    nlohmann::json headersJson;

    std::istringstream headersStream(headers);
    std::string line;
    while (std::getline(headersStream, line)) {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            key.erase(0, key.find_first_not_of(" \r"));
            key.erase(key.find_last_not_of(" \r") + 1);
            value.erase(0, value.find_first_not_of(" \r"));
            value.erase(value.find_last_not_of(" \r") + 1);
            headersJson[key] = value;
        }
    }

    return headersJson;
}

int get_response_status_code(const std::string& headers) {
    std::regex status_line_regex(R"(HTTP/\d+\.\d+ (\d+))");
    std::smatch match;
    if (std::regex_search(headers, match, status_line_regex)) {
        if (match.size() == 2) {
            return std::stoi(match[1].str());
        }
    }

    return -1;
}


struct ResponseData {
    std::string headers;
    std::string body;
};

size_t WriteCallback(void* contents, size_t size, size_t nmemb, ResponseData* data) {
    size_t total_size = size * nmemb;
    if (data != nullptr) {
        // Body
        data->body.append((char*)contents, total_size);
    }
    return total_size;
}

static size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t total_size = size * nitems;
    if (userdata != nullptr) {
        std::string& headers = *static_cast<std::string*>(userdata);
        headers.append(buffer, total_size);
    }
    return total_size;
}

CURL* createSession(const std::string& cookie) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL* curl = curl_easy_init();

    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Cookie: .ROBLOSECURITY=" + cookie).c_str());
        headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 0L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 0L);
    }
    return curl;
}

    void closeSession(CURL* curl) {
        if (curl) {
            curl_easy_cleanup(curl);
    }
}

void updateCsrfToken(CURL* curl, const std::string& newCsrfToken, std::string cookie) {
    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Cookie: .ROBLOSECURITY=" + cookie).c_str());
        headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("x-csrf-token: " + newCsrfToken).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
}

ResponseData performGetRequest(CURL* curl, const std::string& url) {
    ResponseData responseData;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseData.headers);
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
    }
    return responseData;
}


ResponseData performPostRequest(CURL* curl, const std::string& url, const std::string& postData) {
    CURLcode res;

    ResponseData responseData;

    if (curl) {

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseData.headers);
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cout << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

    }
    return responseData;
}