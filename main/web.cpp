#include <stdio.h>
#include <string.h>
#include <regex>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include "web.hpp"

#define TAG "Web"

// メッセージ種別 (メッセージキュー用)
enum class WebMessage {
    Init,       // 初期化
    Start,      // 開始
    Stop,       // 停止
    Quit        // 終了
};

WebServer::WebServer() {
    clear();
}

void WebServer::clear() {
    m_xHandle = NULL;
    m_xQueue = NULL;
    m_server = NULL;
    for(int i=0; i<m_apiCallbacks.size(); i++) {
        delete (ST_API_CALLBACK_DATA*)m_apiCallbacks[i];
    }
    m_apiCallbacks.clear();
}

void WebServer::init() {
    ESP_LOGI(TAG, "Init(S)");

    // タスク作成
    xTaskCreate(WebServer::task, TAG, configMINIMAL_STACK_SIZE * 2, (void*)this, tskIDLE_PRIORITY, &m_xHandle);

    // メッセージキューの初期化
    m_xQueue = xQueueCreate(10, sizeof(WebMessage));

    // 初期化用メッセージポスト
    WebMessage msg = WebMessage::Init;
    xQueueSend(m_xQueue, &msg, portMAX_DELAY);

    ESP_LOGI(TAG, "Init(E)");
}

void WebServer::start(const char* ipAddress, const char* root) {
    m_ipAddress = ipAddress;
    m_root = root;
    WebMessage msg = WebMessage::Start;
    xQueueSend(m_xQueue, &msg, portMAX_DELAY);
}

void WebServer::stop() {
    WebMessage msg = WebMessage::Stop;
    xQueueSend(m_xQueue, &msg, portMAX_DELAY);
}

// タスク
void WebServer::task(void* arg) {
    WebServer* pThis = (WebServer*)arg;
    WebMessage msg;
    bool loop = true;
    while(loop) {
        // メツセージキュー読み取り
        if (pThis->m_xQueue != NULL && xQueueReceive(pThis->m_xQueue, (void*)&msg, portMAX_DELAY) == pdTRUE) {
            switch(msg) {
                case WebMessage::Init:          // 初期化
                    pThis->webInit();
                    break;
                case WebMessage::Start:         // 開始
                    pThis->webStart();
                    break;
                case WebMessage::Stop:          // 停止
                    pThis->webStop();
                    break;
                case WebMessage::Quit:          // 終了
                    loop = false;
                    break;
            }
        }
    }
    // 終了処理
    pThis->stop();
    pThis->clear();
    vTaskDelete(NULL);
}

void WebServer::webInit() {
}

// GET "/API" ハンドラ
esp_err_t WebServer::get_api(httpd_req_t *req) {
    WebServer* pThis = (WebServer*)req->user_ctx;
    ESP_LOGI(TAG, "request uri : %s", req->uri);
    // コールバックを検索/実行
    bool isCall = false;
    std::string path = req->uri;
    std::regex pattern(R"(^.*/API/)");
    std::regex pattern2(R"(&.*)");
    path = std::regex_replace(path, pattern, "");
    path = std::regex_replace(path, pattern2, "");
    ESP_LOGI(TAG, "API request path=%s", path.c_str());
    for(int i=0; i<pThis->m_apiCallbacks.size(); i++) {
        ST_API_CALLBACK_DATA* v = pThis->m_apiCallbacks[i];
        if (v != NULL) {
            ESP_LOGI(TAG, "API setting path=%s", v->path.c_str());
            if (req->method == v->method && path == v->path) {
                v->callback(req, v->context);
                isCall = true;
                break;
            }
        }
    }
    if (isCall == false) {
        ESP_LOGI(TAG, "API NOT FOUND");
        httpd_resp_send_404(req);
    }
    return ESP_OK;
}

esp_err_t WebServer::get_api_get(httpd_req_t *req) {
    return get_api(req);
}

esp_err_t WebServer::get_api_post(httpd_req_t *req) {
    return get_api(req);
}

esp_err_t WebServer::get_api_put(httpd_req_t *req) {
    return get_api(req);
}

esp_err_t WebServer::get_api_delete(httpd_req_t *req) {
    return get_api(req);
}

esp_err_t WebServer::get_api_head(httpd_req_t *req) {
    return get_api(req);
}

esp_err_t WebServer::get_api_options(httpd_req_t *req) {
    return get_api(req);
}

esp_err_t WebServer::get_api_patch(httpd_req_t *req) {
    return get_api(req);
}

// GET "/*" ハンドラ
esp_err_t WebServer::get_root(httpd_req_t *req) {
    WebServer* pThis = (WebServer*)req->user_ctx;
    std::string contentType = pThis->getContentType(req->uri);
    const char* docRoot = "/document";
    char* path = (char*)heap_caps_malloc(pThis->m_root.size() + strlen(docRoot) + strlen(req->uri) + strlen("/index.html") + 1, MALLOC_CAP_8BIT);
    if (req->uri[strlen(req->uri)-1] == '/') {
        sprintf(path, "%s%s%sindex.html", pThis->m_root.c_str(), docRoot, req->uri);
        contentType = "text/html";
    } else {
        sprintf(path, "%s%s%s", pThis->m_root.c_str(), docRoot, req->uri);
    }
    ESP_LOGI(TAG, "request path : %s", path);
    
    FILE* fd = fopen(path, "rb");
    if (fd != NULL) {
        char* buffer = new char[1000];
        int ret;
        httpd_resp_set_type(req, contentType.c_str());
        while ((ret = fread(buffer, 1, 1000, fd)) > 0) {
            httpd_resp_send_chunk(req, buffer, ret);
        }
        httpd_resp_send_chunk(req, NULL, 0);
        delete[] buffer;
        fclose(fd);
    } else {
        ESP_LOGI(TAG, "NOT FOUND");
        httpd_resp_send_404(req);
    }
    heap_caps_free(path);
    return ESP_OK;
}

// URIマッチャー
bool WebServer::custom_uri_matcher(const char* reference_uri, const char* uri_to_match, size_t match_upto) {
    if (reference_uri[strlen(reference_uri)-1] == '*') {
        return strncmp(reference_uri, uri_to_match, strlen(reference_uri) - 1) == 0;
    } else {
        return strcmp(reference_uri, uri_to_match) == 0;
    }
}

void WebServer::webStart() {
    webStop();
    // Webサーバー開始
    httpd_config_t conf = HTTPD_DEFAULT_CONFIG();
    conf.max_uri_handlers = 15;
    conf.uri_match_fn = custom_uri_matcher;
    if (httpd_start(&m_server, &conf) == ESP_OK) {
        // URLマップハンドラ登録 (API)
        httpd_uri_t api = {
            .uri      = "/API/*",
            .user_ctx = this
        };
        api.method = HTTP_GET;
        api.handler = get_api_get;      
        httpd_register_uri_handler(m_server, &api);
        api.method = HTTP_POST;
        api.handler = get_api_post;      
        httpd_register_uri_handler(m_server, &api);
        api.method = HTTP_PUT;
        api.handler = get_api_put;      
        httpd_register_uri_handler(m_server, &api);
        api.method = HTTP_DELETE;
        api.handler = get_api_delete;      
        httpd_register_uri_handler(m_server, &api);
        api.method = HTTP_HEAD;
        api.handler = get_api_head;      
        httpd_register_uri_handler(m_server, &api);
        api.method = HTTP_OPTIONS;
        api.handler = get_api_options;      
        httpd_register_uri_handler(m_server, &api);
        api.method = HTTP_PATCH;
        api.handler = get_api_patch;      
        httpd_register_uri_handler(m_server, &api);
        // URLマップハンドラ登録 (ワイルドカード)
        httpd_uri_t root = {
            .uri      = "/*",
            .method   = HTTP_GET,
            .handler  = get_root,
            .user_ctx = this
        };
        httpd_register_uri_handler(m_server, &root);
    }
}

void WebServer::webStop() {
    if (m_server == NULL)
        return;
    // Webサーバー停止
    httpd_stop(m_server);
    m_server = NULL;
}

// "/API"のハンドラを登録
int WebServer::addHandler(httpd_method_t method, const char* path, CallbackWebAPIFunction callback, void* context) {
    ST_API_CALLBACK_DATA* pCallback = new ST_API_CALLBACK_DATA{
        method,
        path,
        callback,
        context
    };
    m_apiCallbacks.push_back(pCallback);
    return m_apiCallbacks.size() - 1;
}

// "/APIのハンドラを削除"
void WebServer::removeHandler(int handle) {
    delete (ST_API_CALLBACK_DATA*)m_apiCallbacks[handle];
    m_apiCallbacks[handle] = NULL;
}

// URIから拡張子のみを返します
const char* get_file_extension(const char* uri) {
    const char* dot = strrchr(uri, '.');
    if (!dot || dot == uri) return "";
    return dot + 1;
}

const char* WebServer::getContentType(const char* uri) {
    std::string ext = get_file_extension(uri);
    if (ext == "htm") {
        return "text/html";
    } else if (ext == "html") {
        return "text/html";
    } else if (ext == "css") {
        return "text/css";
    } else if (ext == "js") {
        return "application/javascript";
    } else if (ext == "png") {
        return "image/png";
    } else if (ext == "gif") {
        return "image/gif";
    } else if (ext == "jpg") {
        return "image/jpeg";
    } else if (ext == "ico") {
        return "image/x-icon";
    } else if (ext == "xml") {
        return "text/xml";
    } else if (ext == "pdf") {
        return "application/x-pdf";
    } else if (ext == ".zip") {
        return "application/x-zip";
    } else if (ext == "gz") {
        return "application/x-gzip";
    }
    return "text/plain";
}
