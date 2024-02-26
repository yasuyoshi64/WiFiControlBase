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

void WebServer::start() {
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

// GET "/" ハンドラ
esp_err_t get_root(httpd_req_t *req) {
    const char resp[] = "<h1>Hello World</h1>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// URLマップ "/"
httpd_uri_t root = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = get_root,
    .user_ctx = NULL
};

void WebServer::webStart() {
    webStop();
    // Webサーバー開始
    httpd_config_t conf = HTTPD_DEFAULT_CONFIG();
    if (httpd_start(&m_server, &conf) == ESP_OK) {
        // URLマップハンドラ登録
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

