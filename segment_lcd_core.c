#include "segment_lcd_core.h"
#include <stdlib.h>
#include <string.h>

// ============== Внутренние функции ==============
static uint32_t get_current_time(const LcdCore *core)
{
    if (core->get_time_ms)
    {
        return core->get_time_ms();
    }

    static uint32_t dummy_time = 0;
    return dummy_time++;
}

static void update_digit(LcdCore *core)
{
    uint8_t digit = core->state.current_digit;

    // 1. Выключаем текущий разряд
    bool off_state = (core->type == LCD_TYPE_COMMON_CATHODE);
    core->set_digit(digit, off_state);

    // 2. Формируем маску сегментов с точкой
    uint8_t segment_mask = core->state.segment_mask[digit] & 0x7F; // 7 бит

    // Добавляем точку (бит 7)
    if (core->state.dot_flags[digit])
    {
        segment_mask |= 0x80;
    }

    // 3. Инвертируем для общего анода
    if (core->type == LCD_TYPE_COMMON_ANODE)
    {
        segment_mask = ~segment_mask;
    }

    // 4. Устанавливаем сегменты
    core->set_segments(segment_mask);

    // 5. Включаем текущий разряд
    core->set_digit(digit, !off_state);

    // 6. Переходим к следующему разряду
    core->state.current_digit = (digit + 1) % core->digits_count;
}

// ============== Публичные функции ==============
LcdCoreError lcd_core_init(LcdCore *core,
                           uint8_t digits_count,
                           LcdType type,
                           uint16_t frame_time_ms,
                           LcdSegmentCallback seg_cb,
                           LcdDigitCallback dig_cb)
{
    if (!core || !seg_cb || !dig_cb)
    {
        return LCD_CORE_ERROR_NULL;
    }

    if (digits_count == 0 || frame_time_ms == 0)
    {
        return LCD_CORE_ERROR_INVALID_CONFIG;
    }

    // Выделяем память для буферов
    core->state.segment_mask = (uint8_t *)malloc(digits_count);
    core->state.dot_flags = (uint8_t *)malloc(digits_count);

    if (!core->state.segment_mask || !core->state.dot_flags)
    {
        free(core->state.segment_mask);
        free(core->state.dot_flags);
        return LCD_CORE_ERROR_NULL;
    }

    // Вычисляем время на разряд
    core->state.digit_time_ms = frame_time_ms / digits_count;
    if (core->state.digit_time_ms == 0)
    {
        core->state.digit_time_ms = 1;
    }

    // Инициализация
    core->set_segments = seg_cb;
    core->set_digit = dig_cb;
    core->set_brightness = NULL;
    core->get_time_ms = NULL;
    core->digits_count = digits_count;
    core->type = type;
    core->frame_time_ms = frame_time_ms;
    core->brightness = 100;

    // Инициализация состояния
    uint8_t clear_value = (type == LCD_TYPE_COMMON_CATHODE) ? 0x00 : 0xFF;
    memset(core->state.segment_mask, clear_value, digits_count);
    memset(core->state.dot_flags, 0, digits_count);
    core->state.last_update = 0;
    core->state.current_digit = 0;
    core->state.initialized = true;

    return LCD_CORE_OK;
}

LcdCoreError lcd_core_update(LcdCore *core)
{
    if (!core || !core->state.initialized)
    {
        return LCD_CORE_ERROR_NOT_INIT;
    }

    uint32_t current_time = get_current_time(core);

    // Проверяем, пришло ли время обновить разряд
    if (current_time - core->state.last_update >= core->state.digit_time_ms)
    {
        update_digit(core);
        core->state.last_update = current_time;
    }

    return LCD_CORE_OK;
}

LcdCoreError lcd_core_set_digit(LcdCore *core,
                                uint8_t digit,
                                uint8_t segment_mask,
                                bool dot)
{
    if (!core || !core->state.initialized)
    {
        return LCD_CORE_ERROR_NOT_INIT;
    }

    if (digit >= core->digits_count)
    {
        return LCD_CORE_ERROR_INVALID_DIGIT;
    }

    core->state.segment_mask[digit] = segment_mask & 0x7F;
    core->state.dot_flags[digit] = dot ? 1 : 0;

    return LCD_CORE_OK;
}

LcdCoreError lcd_core_set_brightness(LcdCore *core, uint8_t brightness)
{
    if (!core || !core->state.initialized)
    {
        return LCD_CORE_ERROR_NOT_INIT;
    }

    if (brightness > 100)
    {
        brightness = 100;
    }

    core->brightness = brightness;

    if (core->set_brightness)
    {
        core->set_brightness(brightness);
    }

    return LCD_CORE_OK;
}

LcdCoreError lcd_core_clear(LcdCore *core)
{
    if (!core || !core->state.initialized)
    {
        return LCD_CORE_ERROR_NOT_INIT;
    }

    // Очищаем буферы
    uint8_t clear_value = (core->type == LCD_TYPE_COMMON_CATHODE) ? 0x00 : 0xFF;
    memset(core->state.segment_mask, clear_value, core->digits_count);
    memset(core->state.dot_flags, 0, core->digits_count);

    // Выключаем все разряды аппаратно
    bool off_state = (core->type == LCD_TYPE_COMMON_CATHODE);
    for (uint8_t i = 0; i < core->digits_count; i++)
    {
        core->set_digit(i, off_state);
    }

    // Выключаем все сегменты
    core->set_segments(clear_value);

    return LCD_CORE_OK;
}

uint32_t lcd_core_get_time(const LcdCore *core)
{
    if (!core)
    {
        return 0;
    }

    return get_current_time(core);
}