#include "segment_lcd.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============== Основные функции ==============
LcdError lcd_init(LcdDisplay *display,
                  uint8_t digits_count,
                  LcdType type,
                  uint16_t frame_time_ms,
                  LcdSegmentCallback seg_cb,
                  LcdDigitCallback dig_cb,
                  LcdBrightnessCallback bright_cb,
                  LcdTimeCallback time_cb)
{
    if (!display)
    {
        return LCD_ERROR_NULL;
    }

    // Инициализация ядра
    LcdCoreError core_err = lcd_core_init(&display->core,
                                          digits_count,
                                          type,
                                          frame_time_ms,
                                          seg_cb,
                                          dig_cb);

    if (core_err != LCD_CORE_OK)
    {
        return LCD_ERROR_NULL;
    }

    // Устанавливаем опциональные callback
    display->core.set_brightness = bright_cb;
    display->core.get_time_ms = time_cb;

    // Устанавливаем яркость по умолчанию
    if (bright_cb)
    {
        bright_cb(100);
    }

    display->initialized = true;
    return LCD_OK;
}

LcdError lcd_deinit(LcdDisplay *display)
{
    if (!display)
    {
        return LCD_ERROR_NULL;
    }

    if (display->initialized)
    {
        // Очищаем память ядра
        free(display->core.state.segment_mask);
        free(display->core.state.dot_flags);

        display->initialized = false;
    }

    return LCD_OK;
}

LcdError lcd_update(LcdDisplay *display)
{
    if (!display || !display->initialized)
    {
        return LCD_ERROR_NOT_INIT;
    }

    return (lcd_core_update(&display->core) == LCD_CORE_OK) ? LCD_OK : LCD_ERROR_NULL;
}

// ============== Функции отображения ==============
LcdError lcd_show_int(LcdDisplay *display, int32_t number)
{
    if (!display || !display->initialized)
    {
        return LCD_ERROR_NOT_INIT;
    }

    uint8_t digits = display->core.digits_count;
    bool is_negative = (number < 0);
    uint32_t abs_value = is_negative ? (uint32_t)(-number) : (uint32_t)number;

    // Преобразуем число в цифры
    for (int8_t i = digits - 1; i >= 0; i--)
    {
        if (abs_value > 0 || i == digits - 1)
        {
            // Устанавливаем цифру
            uint8_t digit_value = abs_value % 10;
            uint8_t segment_mask = segchar_get_code(
                (SegmentChar)digit_value,
                display->core.type == LCD_TYPE_COMMON_CATHODE);

            lcd_core_set_digit(&display->core, i, segment_mask, false);
            abs_value /= 10;
        }
        else
        {
            // Очищаем разряд
            uint8_t segment_mask = segchar_get_code(
                SEG_CHAR_EMPTY,
                display->core.type == LCD_TYPE_COMMON_CATHODE);

            lcd_core_set_digit(&display->core, i, segment_mask, false);
        }
    }

    // Добавляем знак минус для отрицательных чисел
    if (is_negative)
    {
        // Ищем первую цифру слева
        for (uint8_t i = 0; i < digits; i++)
        {
            uint8_t segment_mask = segchar_get_code(
                SEG_CHAR_MINUS,
                display->core.type == LCD_TYPE_COMMON_CATHODE);

            lcd_core_set_digit(&display->core, i, segment_mask, false);
            break;
        }
    }

    return LCD_OK;
}

LcdError lcd_show_string(LcdDisplay *display, const char *str)
{
    if (!display || !display->initialized || !str)
    {
        return LCD_ERROR_NULL;
    }

    // Подготавливаем буферы
    SegmentChar char_buffer[16];
    uint8_t dot_buffer[16];
    uint16_t converted_length;

    // Конвертируем строку
    if (!segchar_convert_string(str, char_buffer, dot_buffer,
                                display->core.digits_count,
                                &converted_length))
    {
        return LCD_ERROR_INVALID_CHAR;
    }

    // Устанавливаем символы и точки
    for (uint8_t i = 0; i < display->core.digits_count; i++)
    {
        if (i < converted_length)
        {
            uint8_t segment_mask = segchar_get_code(
                char_buffer[i],
                display->core.type == LCD_TYPE_COMMON_CATHODE);

            lcd_core_set_digit(&display->core, i, segment_mask, dot_buffer[i]);
        }
        else
        {
            // Очищаем оставшиеся разряды
            uint8_t segment_mask = segchar_get_code(
                SEG_CHAR_EMPTY,
                display->core.type == LCD_TYPE_COMMON_CATHODE);

            lcd_core_set_digit(&display->core, i, segment_mask, false);
        }
    }

    return LCD_OK;
}

LcdError lcd_set_char(LcdDisplay *display, uint8_t position, SegmentChar character)
{
    if (!display || !display->initialized)
    {
        return LCD_ERROR_NOT_INIT;
    }

    if (position >= display->core.digits_count)
    {
        return LCD_ERROR_INVALID_PARAM;
    }

    uint8_t segment_mask = segchar_get_code(
        character,
        display->core.type == LCD_TYPE_COMMON_CATHODE);

    return (lcd_core_set_digit(&display->core, position, segment_mask, false) == LCD_CORE_OK) ? LCD_OK : LCD_ERROR_NULL;
}

LcdError lcd_set_dot(LcdDisplay *display, uint8_t position, DotState state)
{
    if (!display || !display->initialized)
    {
        return LCD_ERROR_NOT_INIT;
    }

    if (position >= display->core.digits_count)
    {
        return LCD_ERROR_INVALID_PARAM;
    }

    // Получаем текущую маску сегментов
    uint8_t segment_mask = display->core.state.segment_mask[position];

    return (lcd_core_set_digit(&display->core, position, segment_mask, state == DOT_ON) == LCD_CORE_OK) ? LCD_OK : LCD_ERROR_NULL;
}

LcdError lcd_clear(LcdDisplay *display)
{
    if (!display || !display->initialized)
    {
        return LCD_ERROR_NOT_INIT;
    }

    return (lcd_core_clear(&display->core) == LCD_CORE_OK) ? LCD_OK : LCD_ERROR_NULL;
}

LcdError lcd_set_brightness(LcdDisplay *display, uint8_t brightness)
{
    if (!display || !display->initialized)
    {
        return LCD_ERROR_NOT_INIT;
    }

    return (lcd_core_set_brightness(&display->core, brightness) == LCD_CORE_OK) ? LCD_OK : LCD_ERROR_NULL;
}