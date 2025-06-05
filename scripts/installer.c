#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define MARKETPLACE_API "https://marketplace.visualstudio.com/_apis/public/gallery/extensionquery"
#define DOWNLOAD_SUFFIX "/Microsoft.VisualStudio.Services.VSIXPackage"
#define USER_AGENT "VSCode C Installer"

struct Memory {
    char *data;
    size_t size;
};

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    struct Memory *mem = (struct Memory *)userp;
    char *ptr = realloc(mem->data, mem->size + total + 1);
    if (!ptr) return 0;
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, total);
    mem->size += total;
    mem->data[mem->size] = '\0';
    return total;
}

char *http_post(const char *extension_id) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    char payload[1024];
    snprintf(payload, sizeof(payload),
        "{ \"filters\": [ { \"criteria\": [ { \"filterType\": 7, \"value\": \"%s\" } ] } ],"
        " \"flags\": 103 }", extension_id);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json;api-version=3.0-preview.1");

    struct Memory response = { .data = malloc(1), .size = 0 };

    curl_easy_setopt(curl, CURLOPT_URL, MARKETPLACE_API);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        free(response.data);
        return NULL;
    }

    return response.data;
}

char *extract_vsix_url(const char *json) {
    const char *asset_marker = "\"assetUri\":\"";
    char *start = strstr(json, asset_marker);
    if (!start) return NULL;

    start += strlen(asset_marker);
    char *end = strchr(start, '"');
    if (!end) return NULL;

    size_t len = end - start;
    char *url = malloc(len + strlen(DOWNLOAD_SUFFIX) + 1);
    strncpy(url, start, len);
    url[len] = '\0';
    strcat(url, DOWNLOAD_SUFFIX);
    return url;
}

int download_file(const char *url, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) return -1;

    CURL *curl = curl_easy_init();
    if (!curl) {
        fclose(fp);
        return -2;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(fp);

    return res == CURLE_OK ? 0 : -3;
}

const char *detect_code_command() {
    const char *cmds[] = {"code", "codium", "vscodium"};
    static char path[64];
    for (int i = 0; i < 3; ++i) {
        snprintf(path, sizeof(path), "which %s > /dev/null 2>&1", cmds[i]);
        if (system(path) == 0) return cmds[i];
    }
    return NULL;
}

int install_extension(const char *cmd, const char *filename) {
    char full_cmd[256];
    snprintf(full_cmd, sizeof(full_cmd), "%s --install-extension %s", cmd, filename);
    return system(full_cmd);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s [-i] <publisher.extension>\n", argv[0]);
        return 1;
    }

    int install_flag = 0;
    char *ext_id = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-i") == 0) install_flag = 1;
        else if (strchr(argv[i], '.')) ext_id = argv[i];
    }

    if (!ext_id) {
        fprintf(stderr, "Missing extension id (e.g. publisher.extension)\n");
        return 1;
    }

    printf("Fetching extension metadata for %s...\n", ext_id);
    char *json = http_post(ext_id);
    if (!json) {
        fprintf(stderr, "Failed to fetch extension info.\n");
        return 1;
    }

    printf("Extracting VSIX URL...\n");
    char *vsix_url = extract_vsix_url(json);
    free(json);

    if (!vsix_url) {
        fprintf(stderr, "Could not extract VSIX URL.\n");
        return 1;
    }

    char filename[128];
    snprintf(filename, sizeof(filename), "%s.vsix", strchr(ext_id, '.') + 1);

    printf("Downloading VSIX to %s...\n", filename);
    if (download_file(vsix_url, filename) != 0) {
        fprintf(stderr, "Download failed.\n");
        free(vsix_url);
        return 1;
    }

    free(vsix_url);
    printf("Download complete.\n");

    if (install_flag) {
        const char *cmd = detect_code_command();
        if (!cmd) {
            fprintf(stderr, "No VS Code CLI tool found (code/codium/vscodium).\n");
            return 1;
        }
        printf("Installing using %s...\n", cmd);
        if (install_extension(cmd, filename) != 0) {
            fprintf(stderr, "Extension installation failed.\n");
            return 1;
        }
        printf("Installation complete!\n");
    } else {
        printf("Use -i to install the extension after download.\n");
    }

    return 0;
}
