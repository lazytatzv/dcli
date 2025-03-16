#include <iostream>
#include <fstream>
#include <string>
#include <curl/curl.h>
#include <getopt.h>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <memory>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <set>
#include <limits>
#include <map>
#include <algorithm>

using json = nlohmann::json;

// ヘルパークラス：リソース自動管理
struct CurlHandleDeleter {
    void operator()(CURL* curl) const { curl_easy_cleanup(curl); }
};
using CurlHandle = std::unique_ptr<CURL, CurlHandleDeleter>;

struct CurlSlistDeleter {
    void operator()(curl_slist* list) const { curl_slist_free_all(list); }
};
using CurlSlist = std::unique_ptr<curl_slist, CurlSlistDeleter>;

// 設定ファイルのディレクトリとパスを取得
std::string get_config_dir() {
    const char* home = getenv("HOME");
    if (!home) throw std::runtime_error("HOME environment variable not set");
    return std::string(home) + "/.config/dcli/";
}

std::string get_config_path() {
    return get_config_dir() + "config.json";
}

// 設定を保存
void save_config(const std::string& token, const std::map<std::string, std::string>& channels, const std::string& last_used_channel) {
    try {
        const std::string config_dir = get_config_dir();
        mkdir(config_dir.c_str(), 0700);

        json config;
        config["token"] = token;
        config["channels"] = channels;
        config["last_used_channel"] = last_used_channel;

        std::ofstream file(get_config_path());
        if (!file) throw std::runtime_error("Cannot open config file");

        file << config.dump(4);

        chmod(get_config_path().c_str(), 0600);
        std::cout << "Config saved successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error saving config: " << e.what() << std::endl;
        throw;
    }
}

// 設定を読み込み
std::tuple<std::string, std::map<std::string, std::string>, std::string> load_config() {
    try {
        std::ifstream file(get_config_path());
        if (!file) throw std::runtime_error("Config file not found");

        json config;
        file >> config;

        std::string token = config["token"];
        std::map<std::string, std::string> channels = config["channels"].get<std::map<std::string, std::string>>();
        std::string last_used_channel = config.value("last_used_channel", "");

        return {token, channels, last_used_channel};
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        throw;
    }
}

// ヘルプメッセージを表示
void print_help() {
    std::cout << "Usage: dcli [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --token <TOKEN>            Set Discord Bot Token\n";
    std::cout << "  --switch                   Switch to a different channel by name\n";
    std::cout << "  --add                      Add a new channel ID and name\n";
    std::cout << "  --remove                   Remove a channel ID and name\n";
    std::cout << "  --help                     Show this help message\n";
}

// コールバック関数を定義
size_t WriteCallback(void* ptr, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

// Function to retrieve recent messages from a specific user
void get_recent_messages(CurlHandle& curl, const std::string& url, CurlSlist& headers, const std::string& username) {
    std::string readBuffer;

    curl_easy_setopt(curl.get(), CURLOPT_URL, (url + "?limit=100").c_str());
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &readBuffer);

    CURLcode res = curl_easy_perform(curl.get());
    if (res != CURLE_OK) {
        std::cerr << "Request failed: " << curl_easy_strerror(res) << std::endl;
    } else {
        long response_code;
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code != 200) {
            std::cerr << "API Error [" << response_code << "]: " << readBuffer << std::endl;
        } else {
            try {
                auto messages = json::parse(readBuffer);
                std::vector<std::string> user_messages;
                for (const auto& message : messages) {
                    if (message["author"]["username"] == username) {
                        user_messages.push_back(message["content"]);
                    }
                }
                if (user_messages.empty()) {
                    std::cout << "No messages found for user: " << username << "\n";
                } else {
                    std::reverse(user_messages.begin(), user_messages.end()); // Reverse the order of user messages
                    for (const auto& msg : user_messages) {
                        std::cout << msg << "\n";
                    }
                }
            } catch (const json::parse_error& e) {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
            }
        }
    }
}

// Function to list recent usernames
std::vector<std::string> list_recent_usernames(CurlHandle& curl, const std::string& url, CurlSlist& headers) {
    std::string readBuffer;

    curl_easy_setopt(curl.get(), CURLOPT_URL, (url + "?limit=100").c_str());
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &readBuffer);

    CURLcode res = curl_easy_perform(curl.get());
    if (res != CURLE_OK) {
        std::cerr << "Request failed: " << curl_easy_strerror(res) << std::endl;
        return {};
    } else {
        long response_code;
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code != 200) {
            std::cerr << "API Error [" << response_code << "]: " << readBuffer << std::endl;
            return {};
        } else {
            try {
                auto messages = json::parse(readBuffer);
                std::set<std::string> usernames;
                for (const auto& message : messages) {
                    usernames.insert(message["author"]["username"]);
                }
                return std::vector<std::string>(usernames.begin(), usernames.end());
            } catch (const json::parse_error& e) {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
                return {};
            }
        }
    }
}

// Function to escape special characters in a JSON string
std::string escape_json(const std::string& s) {
    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
            case '"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b"; break;
            case '\f': o << "\\f"; break;
            case '\n': o << "\\n"; break;
            case '\r': o << "\\r"; break;
            case '\t': o << "\\t"; break;
            default:
                if ('\x00' <= *c && *c <= '\x1f') {
                    o << "\\u"
                      << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
                } else {
                    o << *c;
                }
        }
    }
    return o.str();
}

// Function to trim whitespace from both ends of a string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, last - first + 1);
}

// メイン関数
int main(int argc, char *argv[]) {
    try {
        std::string token;
        std::map<std::string, std::string> channels;
        std::string current_channel_id;
        std::string last_used_channel;

        // コマンドライン引数の処理
        struct option long_options[] = {
            {"token", required_argument, nullptr, 't'},
            {"switch", no_argument, nullptr, 's'},
            {"add", no_argument, nullptr, 'a'},
            {"remove", no_argument, nullptr, 'r'},
            {"help", no_argument, nullptr, 'h'},
            {nullptr, 0, nullptr, 0}
        };

        int opt;
        while ((opt = getopt_long(argc, argv, "t:sarh", long_options, nullptr)) != -1) {
            switch (opt) {
                case 't':
                    token = optarg;
                    break;
                case 's': {
                    auto config = load_config();
                    token = std::get<0>(config);
                    channels = std::get<1>(config);
                    std::cout << "Select a channel to use:\n";
                    int index = 1;
                    for (const auto& [name, id] : channels) {
                        std::cout << index++ << ". " << name << "\n";
                    }
                    std::cout << "Enter the number of the channel: ";
                    int channel_index;
                    std::cin >> channel_index;
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear the input buffer
                    if (channel_index > 0 && channel_index <= channels.size()) {
                        auto it = channels.begin();
                        std::advance(it, channel_index - 1);
                        current_channel_id = it->second;
                        last_used_channel = current_channel_id;
                        save_config(token, channels, last_used_channel);
                    } else {
                        std::cerr << "Invalid selection.\n";
                        return 1;
                    }
                    break;
                }
                case 'a': {
                    auto config = load_config();
                    token = std::get<0>(config);
                    channels = std::get<1>(config);
                    std::string channel_id, channel_name;
                    std::cout << "Enter Channel ID: ";
                    std::getline(std::cin, channel_id);
                    std::cout << "Enter Channel Name: ";
                    std::getline(std::cin, channel_name);
                    channels[channel_name] = channel_id;
                    save_config(token, channels, last_used_channel);
                    std::cout << "Channel added successfully.\n";
                    return 0;
                }
                case 'r': {
                    auto config = load_config();
                    token = std::get<0>(config);
                    channels = std::get<1>(config);
                    std::cout << "Select a channel to remove:\n";
                    int index = 1;
                    for (const auto& [name, id] : channels) {
                        std::cout << index++ << ". " << name << "\n";
                    }
                    std::cout << "Enter the number of the channel: ";
                    int channel_index;
                    std::cin >> channel_index;
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear the input buffer
                    if (channel_index > 0 && channel_index <= channels.size()) {
                        auto it = channels.begin();
                        std::advance(it, channel_index - 1);
                        channels.erase(it);
                        save_config(token, channels, last_used_channel);
                        std::cout << "Channel removed successfully.\n";
                    } else {
                        std::cerr << "Invalid selection.\n";
                        return 1;
                    }
                    return 0;
                }
                case 'h':
                    print_help();
                    return 0;
                default:
                    print_help();
                    return 1;
            }
        }

        // 設定ファイルからトークンとチャンネルIDを読み込み
        if (token.empty() || current_channel_id.empty()) {
            try {
                auto config = load_config();
                token = std::get<0>(config);
                channels = std::get<1>(config);
                last_used_channel = std::get<2>(config);
                if (current_channel_id.empty() && !last_used_channel.empty()) {
                    current_channel_id = last_used_channel;
                }
                std::cout << "Token and channel ID loaded successfully.\n";
            } catch (const std::exception& e) {
                std::cerr << "Failed to load config: " << e.what() << std::endl;
            }
        }

        // トークンとチャンネルIDが設定されていない場合、初期設定を求める
        if (token.empty() || current_channel_id.empty()) {
            std::cout << "Discord Bot Token and Channel ID are required.\n";
            if (token.empty()) {
                std::cout << "Token: ";
                std::getline(std::cin, token);
            }
            if (current_channel_id.empty()) {
                std::string channel_name;
                std::cout << "Channel ID: ";
                std::getline(std::cin, current_channel_id);
                std::cout << "Enter a name for this channel: ";
                std::getline(std::cin, channel_name);
                channels[channel_name] = current_channel_id;
                last_used_channel = current_channel_id;
            }
            save_config(token, channels, last_used_channel);
        }

        // Discord APIとの通信処理
        CurlHandle curl(curl_easy_init());
        if (curl) {
            // ヘッダーの正しい構築
            CurlSlist headers;
            headers.reset(curl_slist_append(nullptr, "Content-Type: application/json"));
            headers.reset(curl_slist_append(headers.release(), ("Authorization: " + token).c_str()));

            std::string url = "https://discord.com/api/v10/channels/" + current_channel_id + "/messages";
            bool continue_input = true;
            bool is_first_input = true; // 初回入力かどうかを判定するフラグ

            while (continue_input) {
                if (is_first_input) {
                    std::cout << "Enter message (or '/exit' to quit): ";
                    is_first_input = false; // 初回のみ表示
                } else {
                    std::cout << "> "; // 2回目以降は簡潔なプロンプト
                }
                std::string message;
                std::getline(std::cin, message);
                message = trim(message); // Trim whitespace from the input

                // Trim command and check for specific commands
                std::string command = message.substr(0, message.find(' '));
                command = trim(command);

                if (command == "/exit") {
                    continue_input = false;
                    break;
                } else if (command == "/get") {
                    auto usernames = list_recent_usernames(curl, url, headers);
                    if (usernames.empty()) {
                        std::cout << "No recent messages found.\n";
                        continue;
                    }

                    std::cout << "Select a user to retrieve messages:\n";
                    for (size_t i = 0; i < usernames.size(); ++i) {
                        std::cout << i + 1 << ". " << usernames[i] << "\n";
                    }

                    std::cout << "Enter the number of the user: ";
                    size_t user_index;
                    std::cin >> user_index;
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear the input buffer

                    if (user_index > 0 && user_index <= usernames.size()) {
                        get_recent_messages(curl, url, headers, usernames[user_index - 1]);
                    } else {
                        std::cout << "Invalid selection.\n";
                    }
                    continue;
                } else if (command == "/channel") {
                    // 現在のチャンネル名を表示
                    for (const auto& [name, id] : channels) {
                        if (id == current_channel_id) {
                            std::cout << "Current channel: " << name << "\n";
                            break;
                        }
                    }
                    continue;
                } else if (command == "/switch") {
                    // チャンネルを切り替える
                    std::cout << "Select a channel to use:\n";
                    int index = 1;
                    for (const auto& [name, id] : channels) {
                        std::cout << index++ << ". " << name << "\n";
                    }
                    std::cout << "Enter the number of the channel: ";
                    int channel_index;
                    std::cin >> channel_index;
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear the input buffer
                    if (channel_index > 0 && channel_index <= channels.size()) {
                        auto it = channels.begin();
                        std::advance(it, channel_index - 1);
                        current_channel_id = it->second;
                        last_used_channel = current_channel_id;
                        save_config(token, channels, last_used_channel);
                        url = "https://discord.com/api/v10/channels/" + current_channel_id + "/messages";
                        std::cout << "Switched to channel: " << it->first << "\n";
                    } else {
                        std::cerr << "Invalid selection.\n";
                    }
                    continue;
                }

                if (message.empty()) {
                    std::cerr << "Cannot send an empty message.\n";
                    continue;
                }

                std::string escaped_message = escape_json(message);
                std::string payload = "{\"content\": \"" + escaped_message + "\"}";
                std::string readBuffer;  // 毎回新規作成

                curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
                curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, payload.c_str());
                curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &readBuffer);

                CURLcode res = curl_easy_perform(curl.get());
                if (res != CURLE_OK) {
                    std::cerr << "Request failed: " << curl_easy_strerror(res) << std::endl;
                    std::cerr << "Response: " << readBuffer << std::endl;
                } else {
                    long response_code;
                    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &response_code);
                    if (response_code != 200) {
                        std::cerr << "API Error [" << response_code << "]: " << readBuffer << std::endl;
                    } else {
                        std::cout << "Message sent successfully!" << std::endl;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}