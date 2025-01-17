#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Cấu trúc dữ liệu nhận
typedef struct {
    int node_id;
    float temperature;
    float humidity;
} sensor_data_t;

sensor_data_t node_data[9];
int node_count = 0;

// Cập nhật dữ liệu node
void update_node_data(sensor_data_t *data) {
    for (int i = 0; i < node_count; i++) {
        if (node_data[i].node_id == data->node_id) {
            node_data[i] = *data;
            return;
        }
    }
    if (node_count < 9) {
        node_data[node_count++] = *data;
    }
}

// Callback khi nhận dữ liệu
void on_data_recv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    if (len == sizeof(sensor_data_t)) {
        sensor_data_t received_data;
        memcpy(&received_data, data, sizeof(received_data));
        update_node_data(&received_data);
    }
}

// Deep Sleep sau khi nhận đủ dữ liệu
void enter_deep_sleep() {
    printf("Chuyển sang chế độ Deep Sleep. Sẽ đánh thức sau 2 phút.\n");
    esp_sleep_enable_timer_wakeup(120000000); // Đánh thức sau 2 phút (2 * 60 * 1000000 us)
    esp_deep_sleep_start();
}

// Khởi tạo NVS
void initialize_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    printf("Khởi tạo NVS thành công.\n");
}

// Khởi tạo WiFi
void initialize_wifi() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // Chuyển sang chế độ Station
    ESP_ERROR_CHECK(esp_wifi_start());
    printf("Khởi tạo WiFi ở chế độ Station thành công.\n");
}

// In dữ liệu theo format CSV
void print_data_as_csv() {
    printf("node_id,temperature,humidity\n");
    for (int i = 0; i < node_count; i++) {
        printf("%d,%.2f,%.2f\n",
               node_data[i].node_id,
               node_data[i].temperature,
               node_data[i].humidity);
    }
}

// Main Task
void app_main() {
    initialize_nvs();
    initialize_wifi();
    esp_now_init();
    esp_now_register_recv_cb(on_data_recv);

    while (1) {
        if (node_count == 9) {
            printf("Đã nhận đủ dữ liệu từ tất cả các node:\n");
            print_data_as_csv(); // In dữ liệu theo format CSV
            node_count = 0;
            enter_deep_sleep(); // Chuyển sang Deep Sleep
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Chờ thêm dữ liệu
    }
}