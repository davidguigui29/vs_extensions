// Minimal C++ implementation for downloading and optionally installing a VS Code extension VSIX.
// Requires libcurl (`sudo apt install libcurl4-openssl-dev`) and a C++11+ compiler.
// Compile with: g++ -std=c++11 installer.cpp -lcurl -o installer

#include <iostream>
#include <regex>
#include <string>
#include <cstdlib>
#include <curl/curl.h>
#include <fstream>

using namespace std;

const string MARKETPLACE_URL = "https://marketplace.visualstudio.com/items?itemName=";

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

string http_get(const string& url) {
    CURL *curl = curl_easy_init();
    if (!curl) throw runtime_error("Failed to init curl");
    string readBuffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw runtime_error("curl_easy_perform() failed: " + string(curl_easy_strerror(res)));
    }
    curl_easy_cleanup(curl);
    return readBuffer;
}

string extract_vsix_url(const string& html) {
    regex asset_regex("\"assetUri\"\\s*:\\s*\"([^\"]+)\"");
    regex fallback_regex("\"fallbackAssetUri\"\\s*:\\s*\"([^\"]+)\"");

    smatch match;
    if (regex_search(html, match, asset_regex)) {
        return match[1].str() + "/Microsoft.VisualStudio.Services.VSIXPackage";
    } else if (regex_search(html, match, fallback_regex)) {
        return match[1].str() + "/Microsoft.VisualStudio.Services.VSIXPackage";
    }
    throw runtime_error("Could not find assetUri or fallbackAssetUri in page.");
}

void download_file(const string& url, const string& filename) {
    CURL *curl = curl_easy_init();
    if (!curl) throw runtime_error("Failed to init curl for file download");
    FILE *fp = fopen(filename.c_str(), "wb");
    if (!fp) throw runtime_error("Failed to open file for writing: " + filename);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw runtime_error("VSIX download failed: " + string(curl_easy_strerror(res)));
    }
    curl_easy_cleanup(curl);
}

string detect_code_command() {
    const char* cmds[] = {"code", "codium", "vscodium"};
    for (const char* cmd : cmds) {
        string which_cmd = "which ";
        which_cmd += cmd;
        if (system((which_cmd + " > /dev/null 2>&1").c_str()) == 0) {
            return cmd;
        }
    }
    throw runtime_error("No VS Code CLI tool found (tried 'code', 'codium', 'vscodium').");
}

void install_extension(const string& code_cmd, const string& filename) {
    string cmd = code_cmd + " --install-extension " + filename;
    if (system(cmd.c_str()) != 0) {
        throw runtime_error("Failed to install extension using " + code_cmd);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " [-i] <publisher.extension>\n";
        return 1;
    }

    bool install_flag = false;
    string extension_id;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-i") {
            install_flag = true;
        } else if (arg.find('.') != string::npos) {
            extension_id = arg;
        }
    }

    if (extension_id.empty()) {
        cerr << "Error: Missing extension name (e.g., publisher.extension)\n";
        return 1;
    }

    size_t dot = extension_id.find('.');
    string publisher = extension_id.substr(0, dot);
    string extension = extension_id.substr(dot + 1);
    string filename = extension + ".vsix";
    string url = MARKETPLACE_URL + publisher + "." + extension + "&ssr=false#overview";

    try {
        cout << "Fetching Marketplace page for " << publisher << "." << extension << "...\n";
        string html = http_get(url);

        string vsix_url = extract_vsix_url(html);
        cout << "Found VSIX URL: " << vsix_url << endl;

        cout << "Downloading VSIX to " << filename << "...\n";
        download_file(vsix_url, filename);
        cout << "Download complete.\n";

        if (install_flag) {
            string code_cmd = detect_code_command();
            cout << "Installing extension using " << code_cmd << "...\n";
            install_extension(code_cmd, filename);
            cout << "Installation complete!\n";
        } else {
            cout << "Skipping installation. Use '-i' to install.\n";
        }
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}