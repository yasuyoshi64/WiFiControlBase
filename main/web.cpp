#include <stdio.h>
#include <string.h>
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
    // WebServer* pThis = (WebServer*)req->user_ctx;
    const char resp[] = "<h1>Hello World</h1>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
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
    char *buffer;
    long file_size;
    size_t result;
    FILE* fd = fopen(path, "rb");
    if (fd != NULL) {
        fseek(fd, 0, SEEK_END);
        file_size = ftell(fd);
        rewind(fd);
        ESP_LOGI(TAG, "file_size = %ld", file_size);
        buffer = (char*)heap_caps_malloc(sizeof(char) * file_size, MALLOC_CAP_8BIT);
        if (buffer != NULL) {
            result = fread(buffer, 1, file_size, fd);
            httpd_resp_set_type(req, contentType.c_str());
            httpd_resp_send(req, buffer, result);
            heap_caps_free(buffer);
        } else {
            ESP_LOGI(TAG, "buffer = NULL");
            httpd_resp_send_404(req);
        }
        fclose(fd);
    } else {
        ESP_LOGI(TAG, "maji NOT FOUND");
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
        httpd_method_t methods[] = {HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_DELETE, HTTP_HEAD, HTTP_OPTIONS, HTTP_PATCH};
        for(int i=0; i<sizeof(methods); i++) {
            httpd_uri_t api = {
                .uri      = "/API",
                .method   = methods[i],
                .handler  = get_api,
                .user_ctx = this
            };
            httpd_register_uri_handler(m_server, &api);
        }
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
