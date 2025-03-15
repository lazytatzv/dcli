#include <iostream>
#include <fstream>
#include <string>
#include <curl/curl.h>
#include <getopt.h>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <memory>

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
    return get_config_dir() + "config.txt";
}

// 設定を保存
void save_config(const std::string& token, const std::string& channel_id) {
    try {
        const std::string config_dir = get_config_dir();
        mkdir(config_dir.c_str(), 0700);

        std::ofstream file(get_config_path());
        if (!file) throw std::runtime_error("Cannot open config file");

        file << token << std::endl;
        file << channel_id << std::endl;

        chmod(get_config_path().c_str(), 0600);
        std::cout << "Config saved successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error saving config: " << e.what() << std::endl;
        throw;
    }
}

// 設定を読み込み
std::pair<std::string, std::string> load_config() {
    try {
        std::ifstream file(get_config_path());
        if (!file) throw std::runtime_error("Config file not found");

        std::string token, channel_id;
        std::getline(file, token);
        std::getline(file, channel_id);

        return {token, channel_id};
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
    std::cout << "  --channel-id <CHANNEL_ID>  Set Discord Channel ID\n";
    std::cout << "  --help                     Show this help message\n";
}

// コールバック関数を定義
size_t WriteCallback(void* ptr, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

// メイン関数
int main(int argc, char *argv[]) {
    try {
        std::string token, channel_id;

        // コマンドライン引数の処理
        struct option long_options[] = {
            {"token", required_argument, nullptr, 't'},
            {"channel-id", required_argument, nullptr, 'c'},
            {"help", no_argument, nullptr, 'h'},
            {nullptr, 0, nullptr, 0}
        };

        int opt;
        while ((opt = getopt_long(argc, argv, "t:c:h", long_options, nullptr)) != -1) {
            switch (opt) {
                case 't':
                    token = optarg;
                    break;
                case 'c':
                    channel_id = optarg;
                    break;
                case 'h':
                    print_help();
                    return 0;
                default:
                    print_help();
                    return 1;
            }
        }

        // 設定ファイルからトークンとチャンネルIDを読み込み
        if (token.empty() || channel_id.empty()) {
            try {
                auto config = load_config();
                token = config.first;
                channel_id = config.second;
            } catch (const std::exception& e) {
                std::cerr << "Failed to load config: " << e.what() << std::endl;
            }
        }

        // トークンとチャンネルIDが設定されていない場合、初期設定を求める
        if (token.empty() || channel_id.empty()) {
            std::cout << "Discord Bot Token and Channel ID are required.\n";
            if (token.empty()) {
                std::cout << "Token: ";
                std::getline(std::cin, token);
            }
            if (channel_id.empty()) {
                std::cout << "Channel ID: ";
                std::getline(std::cin, channel_id);
            }
            save_config(token, channel_id);
        }

        // Discord APIとの通信処理
        CurlHandle curl(curl_easy_init());
        if (curl) {
            // ヘッダーの正しい構築
            CurlSlist headers;
            headers.reset(curl_slist_append(nullptr, "Content-Type: application/json"));
            headers.reset(curl_slist_append(headers.release(), ("Authorization: " + token).c_str()));

            std::string url = "https://discord.com/api/v10/channels/" + channel_id + "/messages";
            bool continue_input = true;
            // メインループ内の修正
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

                if (message == "/exit") {
                    continue_input = false;
                    break;
                }

                std::string payload = "{\"content\": \"" + message + "\"}";
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