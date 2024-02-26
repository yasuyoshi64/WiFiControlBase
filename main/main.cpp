#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <map>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "main.hpp"

#define TAG "Application"
#define ROOT "/mnt"

Application app;

// メッセージ種別 (メッセージキュー用)
enum class AppMessage {
    UpdateDisplay,      // ディスプレイに現在状態表示
    WIFIConnection,     // Wi-Fi接続
    WIFIDisconnection,  // Wi-Fi切断
    Quit                // 終了
};

Application::Application() {
    m_LedState = false;
    m_xHandle = NULL;
    m_xQueue = NULL;
}

// 初期化
void Application::init() {
    ESP_LOGI(TAG, "Init(S)");

    // LED初期化
    gpio_reset_pin((gpio_num_t)CONFIG_LED_PIN);
    gpio_set_direction((gpio_num_t)CONFIG_LED_PIN, GPIO_MODE_OUTPUT);
    led(0);

    // タスク作成
    xTaskCreate(Application::app_task, TAG, configMINIMAL_STACK_SIZE * 2, (void*)this, tskIDLE_PRIORITY, &m_xHandle);

    // メッセージキューの初期化
    m_xQueue = xQueueCreate(10, sizeof(AppMessage));

    // SDカード初期化
    m_sd_card.init(ROOT);
    m_sd_card.setMountCallback(mountFunc, this);

    // OLED(SSD1306)ディスプレイ初期化
    m_oled.init(dispInitCompFunc, this);

    // Wi-Fi初期化
    m_wifi.init(wifiConnectFunc, this);

    // Webサーバー初期化
    m_web.init();

    ESP_LOGI(TAG, "Init(E)");
}

// タスク
void Application::app_task(void* arg) {
    Application* pThis = (Application*)arg;
    AppMessage msg;
    bool loop = true;
    while(loop) {
        // メッセージキュー読み取り
        if (pThis->m_xQueue != NULL && xQueueReceive(pThis->m_xQueue, (void*)&msg, portMAX_DELAY) == pdTRUE) {
            switch(msg) {
                case AppMessage::UpdateDisplay:     // ディスプレイに現在状態表示
                    pThis->updateDisplay();
                    break;
                case AppMessage::WIFIConnection:    // Wi-Fi接続
                    pThis->wifiConnection();
                    break;
                case AppMessage::WIFIDisconnection: // Wi-Fi切断
                    pThis->wifiDisconnection();
                    break;
                case AppMessage::Quit:              // 終了
                    loop = false;
                    break;
            }
        }
    }
    // 終了処理
    vTaskDelete(NULL);
}

// LED点灯制御
//  state   : 1=点灯, 0=消灯
void Application::led(int state) {
    m_LedState = state;
    gpio_set_level((gpio_num_t)CONFIG_LED_PIN, m_LedState);
}

// SDカードマウントコールバック
void Application::mountFunc(bool isMount, void* context) {
    ESP_LOGI(TAG, "SD Card mount : %d", isMount);
    Application* pThis = (Application*)context;
    AppMessage msg = AppMessage::UpdateDisplay;
    xQueueSend(pThis->m_xQueue, &msg, portMAX_DELAY);
    if (isMount && pThis->getConfig(ROOT)) {
        AppMessage msg = AppMessage::WIFIConnection;
        xQueueSend(pThis->m_xQueue, &msg, portMAX_DELAY);
    } else {
        AppMessage msg = AppMessage::WIFIDisconnection;
        xQueueSend(pThis->m_xQueue, &msg, portMAX_DELAY);
    }
}

// ファイル一覧コールバック
bool Application::fileFunc(bool isFile, const char* name, void* context) {
    Application* pThis = (Application*)context;
    ESP_LOGI(TAG, "%s : %s", isFile ? "File" : "Directory", name);
    return true;
}

// ディスプレイ初期化完了コールバック
void Application::dispInitCompFunc(void* context) {
    Application* pThis = (Application*)context;
    AppMessage msg = AppMessage::UpdateDisplay;
    xQueueSend(pThis->m_xQueue, &msg, portMAX_DELAY);
}

// Wi-Fi接続コールバック
void Application::wifiConnectFunc(bool isConnect, void* context) {
    Application* pThis = (Application*)context;
    if (isConnect) {
        const char* ipAddress = pThis->m_wifi.getIPAddress();
        ESP_LOGI(TAG, "IP Address: %s", ipAddress);
        char url[256];
        sprintf(url, "http://%s/", ipAddress);
        pThis->m_oled.dispQRCode(url);
        pThis->led(0);
        pThis->m_web.start();   // Webサーバー開始
    }
}

// ディスプレイに現在状態表示
void Application::updateDisplay() {
    if (!m_oled.isInitialize())
        return;
    if (m_sd_card.isMount()) {
        m_oled.dispString("Wi-Fi Connecting");
        led(1);
    } else {
        m_oled.dispString("No File");
        led(0);
    }
}

// Wi-Fi接続
void Application::wifiConnection() {
    wifiDisconnection();
    if (m_configMap.find("ssid") == m_configMap.end() || m_configMap.find("pass") == m_configMap.end())
        return; // CONFIGファイルにssidまたはpassの設定がない
    const char* ssid = m_configMap["ssid"].c_str();
    const char* pass = m_configMap["pass"].c_str();
    m_wifi.connect(ssid, pass);
}

// Wi-Fi切断
void Application::wifiDisconnection() {
    m_web.stop();
    m_wifi.disconnect();
}

extern "C" void app_main(void)
{
    nvs_flash_init();     // Flash初期化  (お約束のようなもの)

    app.init();
}
