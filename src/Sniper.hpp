#include "HttpClient.hpp"
#include <fstream>
#include <deque>
#include <stdexcept>
#include <typeinfo>
#include <chrono>
#include <ctime>
#include <thread>
#include <map>
#include <random>
#include <fstream>

using namespace nlohmann;

namespace uuid {
    static std::random_device              rd;
    static std::mt19937                    gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    std::string generate_uuid_v4() {
        std::stringstream ss;
        int i;
        ss << std::hex;
        for (i = 0; i < 8; i++) {
            ss << dis(gen);
        }
        ss << "-";
        for (i = 0; i < 4; i++) {
            ss << dis(gen);
        }
        ss << "-4";
        for (i = 0; i < 3; i++) {
            ss << dis(gen);
        }
        ss << "-";
        ss << dis2(gen);
        for (i = 0; i < 3; i++) {
            ss << dis(gen);
        }
        ss << "-";
        for (i = 0; i < 12; i++) {
            ss << dis(gen);
        };
        return ss.str();
    }
}

class sniper {
    char* clear = (system("echo") == 0) ? "cls" : "clear";

    std::deque<std::string> error_logs;
    std::deque<std::string> buy_logs;
    std::deque<std::string> search_logs;

    long total_searches = 0;

    unsigned long long user_id;
    std::string cookie;
    std::string x_token;

    json items;
    int global_max_price;


public:
    void exit() {
        system("pause");
        std::exit(0);
    }

    sniper() {
        load_config();
        CURL* curl = createSession(cookie);
        get_user_id(curl);
        get_xtoken(curl);
        searcher(curl);
        closeSession(curl);
    }

    void load_config() {
        std::ifstream file("config.json");
        json data = json::parse((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        cookie = data["cookie"];
        items = data["items"]["list"];
        global_max_price = data["items"]["global_max_price"];
    }

    void get_user_id(CURL* curl) {
        user_id = json::parse(performGetRequest(curl, "https://users.roblox.com/v1/users/authenticated").body)["id"].get<unsigned long long>();
    }

    void get_xtoken(CURL* curl) {
        x_token = convertHeadersToJson(performPostRequest(curl, "https://economy.roblox.com/", "").headers)["x-csrf-token"].get<std::string>();

        updateCsrfToken(curl, x_token, cookie);
    }

    std::string get_time() {
        auto now = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(now);
        std::tm local_time_s = *std::localtime(&time);
        std::ostringstream formatted_time;
        formatted_time << std::put_time(&local_time_s, "%H:%M:%S");
        return formatted_time.str();
    }

    void add_error(std::string error) {
        auto current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        error_logs.push_back("[" + get_time() + "] [ERROR] " + error);
        if (error_logs.size() > 5) {
            error_logs.pop_front();
        }
    }

    std::string return_error() {
        std::string error_logs_string = "";
        for (const std::string& buy : error_logs) {
            error_logs_string += buy + "\n";
        }
        return error_logs_string;
    }

    void add_search(std::string search) {
        auto current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        search_logs.push_back("[" + get_time() + "] [SEARCH] " + search);
        if (search_logs.size() > 5) {
            search_logs.pop_front();
        }
    }

    std::string return_search() {
        std::string search_logs_string = "";
        for (const std::string& buy : search_logs) {
            search_logs_string += buy + "\n";
        }
        return search_logs_string;
    }

    void add_buy(std::string buy) {
        auto current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        buy_logs.push_back("[" + get_time() + "] [BUY] " + buy);
    }

    std::string return_buy() {
        std::string buy_logs_string = "";
        for (const std::string& buy : buy_logs) {
            buy_logs_string += buy + "\n";
        }
        return buy_logs_string;
    }

    void close() {
        system(clear);
        std::cout << "No more items left closing programm..." << std::endl;
        exit();
    }
    void print_stats() {
        std::cout << "Total Searchers: " + std::to_string(total_searches) << "\n\n";
        std::cout << "Search logs: \n" << return_search() << "\n\n";
        std::cout << "Buy logs: \n" << return_buy() << "\n\n";
        std::cout << "Error logs: \n" << return_error() << std::endl;
    }

    // searchers
    json convertToMatrix(const json& input_list) {
        int max_value = 120;
        json result_matrix;

        for (size_t i = 0; i < input_list.size(); i += max_value) {
            size_t end_index = i + max_value;
            if (end_index > input_list.size()) {
                end_index = input_list.size();
            }
            result_matrix.push_back(json(input_list.begin() + i, input_list.begin() + end_index));

            if (end_index == input_list.size()) {
                i = 0;
                max_value -= end_index;
            }
        }

        return result_matrix;
    }

    std::string get_keys(const json& input_json) {
        json result = {{"items", json::array()}};

        for (const auto& entry : input_json.items()) {
            result["items"].push_back(json::parse("{\"itemType\": \"Asset\", \"id\": \"" + std::string(entry.value()) + "\"}"));
        }
        return result.dump();
    }

    struct ItemInfo {
        int price;
        std::string collectible_item_id;
        unsigned long long item_id;
        std::string productid_data;
        unsigned long long creator;
        std::string collectible_item_instance_id;
    };

    void buy_item(CURL* curl, ItemInfo item) {
        json data = {
            {"collectibleItemId", item.collectible_item_id},
            {"expectedCurrency", 1},
            {"expectedPrice", item.price},
            {"expectedPurchaserId", user_id},
            {"expectedPurchaserType", "User"},
            {"expectedSellerId", item.creator},
            {"expectedSellerType", "User"},
            {"idempotencyKey", uuid::generate_uuid_v4()},
            {"collectibleProductId", item.productid_data},
            {"collectibleItemInstanceId", item.collectible_item_instance_id}
        };
        ResponseData raw = performPostRequest(curl, "https://apis.roblox.com/marketplace-sales/v1/item/" + item.collectible_item_id + "/purchase-resale", data.dump());
        int status_code = get_response_status_code(raw.headers);
        if (status_code == 200) {
            json response = json::parse(raw.body);
            if (response.contains("purchased") && !response["purchased"]) {
                add_error("Failed to buy item " + std::to_string(item.item_id) + " reason: " + response["errorMessage"].get<std::string>());
            }
            else {
                add_buy("Bought item " + std::to_string(item.item_id) + " for a price of " + std::to_string(item.price));
            }
        }
        else {
            add_error("Failed to buy item " + std::to_string(item.item_id));
        }
    }

    void handle_items(json body) {
        CURL* curl = createSession(cookie);
        updateCsrfToken(curl, x_token, cookie);
        for (const auto& item : body["data"]) {
            if (item.contains("priceStatus") && item["priceStatus"] == "Off Sale") {
                items.erase(std::to_string(static_cast<unsigned long long>(item["id"])));
                continue;
            }

            ItemInfo info {
                item["lowestPrice"],
                item["collectibleItemId"],
                item["id"]
            };

            if (!item.contains("hasResellers") || info.price > global_max_price || info.price > items[std::to_string(info.item_id)]) {
                continue;
            }
            ResponseData raw = performGetRequest(curl, "https://apis.roblox.com/marketplace-sales/v1/item/0fcc875b-967b-44e4-8bfb-3ca1f05979c3/resellers?limit=1");
            int status_code = get_response_status_code(raw.headers);
            if (status_code == 200) {
                json data_product = json::parse(raw.body)["data"][0];
                info.price = data_product["price"];
                info.productid_data = data_product["collectibleProductId"];
                info.creator = data_product["seller"]["sellerId"];
                info.collectible_item_instance_id = data_product["collectibleItemInstanceId"];
                buy_item(curl, info);
            }
            else if (status_code == 429) {
                add_error("Ratelimit hit");
                continue;
            }
            else {
                continue;
            }
        }
        closeSession(curl);
    }

    void searcher(CURL* curl) {
        while (true) {
            if (items.size() == 0) {
                close();
            }
            std::vector<std::string> keys;
            for (const auto& entry : items.items()) {
                keys.push_back(entry.key());
            }

            auto result = convertToMatrix(keys);
            for (const json& item : result) {
                try {
                    ResponseData raw = performPostRequest(curl, "https://catalog.roblox.com/v1/catalog/items/details", get_keys(item));
                    int status_code = get_response_status_code(raw.headers);
                    switch (status_code) {
                    case 200:
                        handle_items(json::parse(raw.body));
                        break;
                    case 403:
                        if (json::parse(raw.body)["message"] == "Token Validation Failed") {
                            get_xtoken(curl);
                            add_error("Invalid xtoken -> refreshed");
                        }
                        break;
                    case 429:
                        add_error("Ratelimit hit");
                        break;
                    default:
                        break;
                    }

                }
                catch (const std::exception& ex) {
                    std::cout << std::string(ex.what()) << std::endl;
                    add_error("Caught exception: " + std::string(ex.what()));
                }
                catch (...) {
                    add_error("Caught unknown exception.");
                }
                system(clear);
                total_searches += item.size();
                add_search("Searched " + std::to_string(item.size()) + " items");
                print_stats();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
};