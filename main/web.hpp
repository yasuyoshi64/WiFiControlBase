/**
 * Webサーバー
*/
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_http_server.h"

class WebServer {
    public:
        WebServer();
    
    public:
        void init();
        void start();
        void stop();

    private:
        void clear();
        // タスク
        static void task(void* arg);
        //
        void webInit();
        void webStart();
        void webStop();

    private:
        TaskHandle_t m_xHandle;     // タスクハンドル
        QueueHandle_t m_xQueue;     // メッセージキュー
        httpd_handle_t m_server;    // httpdサーバー
};
