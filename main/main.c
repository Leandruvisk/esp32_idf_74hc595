#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "capture_data.h"

/* ================= GPIO ================= */

#define PIN_SER   GPIO_NUM_14
#define PIN_SCK   GPIO_NUM_12
#define PIN_RCK   GPIO_NUM_13

/* ================= Replay ================= */

#define REPLAY_DONE_BIT (1 << 0)

static const char *TAG = "74HC595_REPLAY";

static volatile uint32_t idx = 0;
static esp_timer_handle_t replay_timer;
static EventGroupHandle_t replay_event;

/* ================= Timer callback ================= */

static void replay_timer_cb(void *arg)
{
    /* aplica níveis lógicos do evento atual */
    gpio_set_level(PIN_SER, ser[idx]);
    gpio_set_level(PIN_SCK, sck[idx]);
    gpio_set_level(PIN_RCK, rck[idx]);

    idx++;

    /* fim do replay */
    if (idx >= SIGNAL_LEN) {
        xEventGroupSetBits(replay_event, REPLAY_DONE_BIT);
        return;
    }

    /* calcula delta real entre eventos (ns → us) */
    uint64_t delta_ns = timestamp_ns[idx] - timestamp_ns[idx - 1];
    uint64_t delta_us = delta_ns / 1000;

    if (delta_us == 0) {
        delta_us = 1; // esp_timer não aceita 0
    }

    esp_timer_start_once(replay_timer, delta_us);
}

/* ================= Start replay ================= */

static void start_replay(void)
{
    idx = 0;

    if (!replay_timer) {
        const esp_timer_create_args_t args = {
            .callback = replay_timer_cb,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "replay_timer"
        };
        ESP_ERROR_CHECK(esp_timer_create(&args, &replay_timer));
    }

    /* agenda o primeiro evento */
    uint64_t first_us = timestamp_ns[0] / 1000;
    if (first_us == 0) {
        first_us = 1;
    }

    ESP_ERROR_CHECK(esp_timer_start_once(replay_timer, first_us));
}

/* ================= Main ================= */

void app_main(void)
{
    gpio_config_t cfg = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask =
            (1ULL << PIN_SER) |
            (1ULL << PIN_SCK) |
            (1ULL << PIN_RCK)
    };
    gpio_config(&cfg);

    gpio_set_level(PIN_SER, 0);
    gpio_set_level(PIN_SCK, 0);
    gpio_set_level(PIN_RCK, 0);

    replay_event = xEventGroupCreate();

    ESP_LOGI(TAG, "Replay iniciado (%u eventos)", SIGNAL_LEN);

    while (1) {
        xEventGroupClearBits(replay_event, REPLAY_DONE_BIT);

        start_replay();

        /* bloqueia até terminar — sem delay, sem polling */
        xEventGroupWaitBits(
            replay_event,
            REPLAY_DONE_BIT,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY
        );

        ESP_LOGI(TAG, "Replay finalizado");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
