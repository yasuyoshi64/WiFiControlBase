/**
 * Webサーバー
*/
#pragma once

#include <vector>
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_http_server.h"

typedef void (*CallbackWebAPIFunction)(httpd_req_t *req, void* context);

struct ST_API_CALLBACK_DATA {
    httpd_method_t method;
    std::string path;
    CallbackWebAPIFunction callback;
    void* context;
};

class WebServer {
    public:
        WebServer();
    
    public:
        void init();
        void start(const char* ipAddress, const char* root);
        void stop();

        // "/API"用コールバック
        int addHandler(httpd_method_t method, const char* path, CallbackWebAPIFunction callback, void* context);
        void removeHandler(int handle);

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
        static esp_err_t get_api_get(httpd_req_t *req);
        static esp_err_t get_api_post(httpd_req_t *req);
        static esp_err_t get_api_put(httpd_req_t *req);
        static esp_err_t get_api_delete(httpd_req_t *req);
        static esp_err_t get_api_head(httpd_req_t *req);
        static esp_err_t get_api_options(httpd_req_t *req);
        static esp_err_t get_api_patch(httpd_req_t *req);
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
        std::vector<ST_API_CALLBACK_DATA*> m_apiCallbacks;
};
