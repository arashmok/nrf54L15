/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>
#include <hw_id.h>

LOG_MODULE_REGISTER(nrf54l15_hello, LOG_LEVEL_INF);

static struct k_timer blink_timer;
static atomic_t fast_mode;

static void blink_timer_handler(struct k_timer *timer)
{
    ARG_UNUSED(timer);
    static bool on;
    on = !on;
    dk_set_led(DK_LED1, on);
}

static void button_handler(uint32_t button_state, uint32_t has_changed)
{
    if (!(has_changed & DK_BTN1_MSK)) {
        return;
    }

    /* Toggle mode when BTN1 is pressed */
    if (button_state & DK_BTN1_MSK) {
        bool now_fast = !atomic_get(&fast_mode);
        atomic_set(&fast_mode, now_fast);

        int slow_ms = CONFIG_NRF54L15_HELLO_SLOW_MS;
        int fast_ms = CONFIG_NRF54L15_HELLO_FAST_MS;
        int period_ms = now_fast ? fast_ms : slow_ms;

        LOG_INF("Button 1 -> %s blink (%d ms)", now_fast ? "fast" : "slow", period_ms);
        k_timer_start(&blink_timer, K_MSEC(period_ms), K_MSEC(period_ms));
    }
}

int main(void)
{
    int ret;

    ret = dk_leds_init();
    if (ret) {
        LOG_ERR("dk_leds_init failed: %d", ret);
        return ret;
    }

    ret = dk_buttons_init(button_handler);
    if (ret) {
        LOG_ERR("dk_buttons_init failed: %d", ret);
        return ret;
    }

    atomic_set(&fast_mode, false);
    k_timer_init(&blink_timer, blink_timer_handler, NULL);

    /* Print device ID (from HW ID library) */
    char dev_id[HW_ID_LEN];
    if (hw_id_get(dev_id, sizeof(dev_id)) == 0) {
        LOG_INF("Device ID: %s", dev_id);
    } else {
        LOG_WRN("Device ID unavailable");
    }

    int period_ms = CONFIG_NRF54L15_HELLO_SLOW_MS;
    LOG_INF("nRF54L15 Hello started. Start slow blink %d ms", period_ms);
    LOG_INF("Press Button 1 to toggle blink speed.");

    k_timer_start(&blink_timer, K_MSEC(period_ms), K_MSEC(period_ms));

    for (;;) {
        k_sleep(K_SECONDS(10));
    }
}
