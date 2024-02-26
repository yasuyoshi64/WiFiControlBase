/**
 * Webサーバー
*/
#pragma once

#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_http_server.h"

class WebServer {
    public:
        WebServer();
    
    public:
        void init();
        void start(const char* ipAddress, const char* root);
        void stop();

    private:
        void clear();
        // タスク
        static void task(void* arg);
        //
        void webInit();
        void webStart();
        void webStop();
        // Webリクエストハンドラ
        static esp_err_t get_api(httpd_req_t *req);
        static esp_err_t get_root(httpd_req_t *req);
        static bool custom_uri_matcher(const char* reference_uri, const char* uri_to_match, size_t match_upto);
        //
        const char* getContentType(const char* uri);

    private:
        TaskHandle_t m_xHandle;     // タスクハンドル
        QueueHandle_t m_xQueue;     // メッセージキュー
        httpd_handle_t m_server;    // httpdサーバー
        std::string m_ipAddress;    // IPアドレス
        std::string m_root;         // ファイルシステムルートパス
};
