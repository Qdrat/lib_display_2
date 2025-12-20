#include "segment_lcd_core.h"
#include <stdlib.h>
#include <string.h>

// ============== Внутренние функции ==============

/**
 * @brief Получение текущего времени
 */
static uint32_t get_current_time(const LcdCoreConfig *config)
{
    if (config->get_time_ms)
    {
        return config->get_time_ms();
    }

    // Возвращаем системное время (заглушка)
    static uint32_t dummy_time = 0;
    return dummy_time++;
}

/**
 * @brief Вычисление времени горения разряда
 */
static uint16_t calculate_digit_time_us(uint16_t refresh_rate_hz, uint8_t digits_count)
{
    if (refresh_rate_hz == 0)
        refresh_rate_hz = 60; // По умолчанию 60 Гц

    // Время кадра = 1 / частота (в микросекундах)
    uint32_t frame_time_us = 1000000UL / refresh_rate_hz;

    // Время на разряд = время кадра / количество разрядов
    return (uint16_t)(frame_time_us / digits_count);
}

/**
 * @brief Аппаратное обновление разряда
 */
static void update_hardware_digit(LcdCoreConfig *config)
{
    uint8_t digit = config->_internal.current_digit;

    // 1. Выключаем текущий разряд
    bool off_state = (config->type == LCD_TYPE_COMMON_CATHODE);
    config->set_digit(digit, off_state);

    // 2. Подготавливаем маску сегментов
    uint8_t segment_mask = config->_internal.segment_buffer[digit];

    // 3. Добавляем точку
    if (config->_internal.dot_buffer[digit])
    {
        // Для общего катода: точка = бит 7 = 1
        // Для общего анода: точка = бит 7 = 0
        if (config->type == LCD_TYPE_COMMON_CATHODE)
        {
            segment_mask |= 0x80;
        }
        else
        {
            segment_mask &= ~0x80;
        }
    }
    else
    {
        if (config->type == LCD_TYPE_COMMON_CATHODE)
        {
            segment_mask &= ~0x80;
        }
        else
        {
            segment_mask |= 0x80;
        }
    }

    // 4. Устанавливаем сегменты
    config->set_segments(segment_mask);

    // 5. Включаем текущий разряд
    config->set_digit(digit, !off_state);

    // 6. Переходим к следующему разряду
    config->_internal.current_digit =
        (digit + 1) % config->digits_count;
}

// ============== Публичные функции ==============

LcdCoreError lcd_core_init(LcdCoreConfig *config)
{
    // Проверка указателей
    if (!config || !config->set_segments || !config->set_digit)
    {
        return LCD_CORE_ERROR_NULL_POINTER;
    }

    // Проверка конфигурации
    if (config->digits_count == 0)
    {
        return LCD_CORE_ERROR_INVALID_CONFIG;
    }

    // Выделение памяти для буферов
    config->_internal.segment_buffer =
        (uint8_t *)malloc(config->digits_count);

    config->_internal.dot_buffer =
        (uint8_t *)malloc(config->digits_count);

    if (!config->_internal.segment_buffer ||
        !config->_internal.dot_buffer)
    {
        free(config->_internal.segment_buffer);
        free(config->_internal.dot_buffer);
        return LCD_CORE_ERROR_MEMORY;
    }

    // Вычисление времени горения разряда
    config->_internal.digit_time_us =
        calculate_digit_time_us(config->refresh_rate_hz,
                                config->digits_count);

    // Инициализация буферов
    memset(config->_internal.segment_buffer,
           (config->type == LCD_TYPE_COMMON_CATHODE) ? 0x00 : 0xFF,
           config->digits_count);

    memset(config->_internal.dot_buffer, 0, config->digits_count);

    // Инициализация переменных
    config->_internal.current_digit = 0;
    config->_internal.last_update_time = 0;
    config->_internal.brightness = 100;
    config->_initialized = true;

    // Установка яркости
    if (config->set_brightness)
    {
        config->set_brightness(config->_internal.brightness);
    }

    return LCD_CORE_OK;
}

LcdCoreError lcd_core_update(LcdCoreConfig *config)
{
    if (!config || !config->_internal.initialized)
    {
        return LCD_CORE_ERROR_NOT_INITIALIZED;
    }

    uint32_t current_time = get_current_time(config);
    uint32_t elapsed_us = (current_time - config->_internal.last_update_time) * 1000;

    // Проверяем, прошло ли достаточно времени
    if (elapsed_us >= config->_internal.digit_time_us)
    {
        update_hardware_digit(config);
        config->_internal.last_update_time = current_time;
    }

    return LCD_CORE_OK;
}

LcdCoreError lcd_core_set_segment_mask(LcdCoreConfig *config,
                                       uint8_t digit,
                                       uint8_t segment_mask)
{
    if (!config || !config->_internal.initialized)
    {
        return LCD_CORE_ERROR_NOT_INITIALIZED;
    }

    if (digit >= config->digits_count)
    {
        return LCD_CORE_ERROR_INVALID_DIGIT;
    }

    config->_internal.segment_buffer[digit] = segment_mask;
    return LCD_CORE_OK;
}

LcdCoreError lcd_core_set_dot(LcdCoreConfig *config,
                              uint8_t digit,
                              bool dot_on)
{
    if (!config || !config->_internal.initialized)
    {
        return LCD_CORE_ERROR_NOT_INITIALIZED;
    }

    if (digit >= config->digits_count)
    {
        return LCD_CORE_ERROR_INVALID_DIGIT;
    }

    config->_internal.dot_buffer[digit] = dot_on ? 1 : 0;
    return LCD_CORE_OK;
}

LcdCoreError lcd_core_clear(LcdCoreConfig *config)
{
    if (!config || !config->_internal.initialized)
    {
        return LCD_CORE_ERROR_NOT_INITIALIZED;
    }

    // Очищаем буферы
    uint8_t clear_value = (config->type == LCD_TYPE_COMMON_CATHODE) ? 0x00 : 0xFF;

    memset(config->_internal.segment_buffer, clear_value, config->digits_count);
    memset(config->_internal.dot_buffer, 0, config->digits_count);

    // Выключаем все разряды аппаратно
    bool off_state = (config->type == LCD_TYPE_COMMON_CATHODE);
    for (uint8_t i = 0; i < config->digits_count; i++)
    {
        config->set_digit(i, off_state);
    }

    // Выключаем сегменты
    config->set_segments(clear_value);

    return LCD_CORE_OK;
}

LcdCoreError lcd_core_set_brightness(LcdCoreConfig *config, uint8_t brightness)
{
    if (!config || !config->_internal.initialized)
    {
        return LCD_CORE_ERROR_NOT_INITIALIZED;
    }

    // Проверка диапазона
    if (brightness > 100)
        brightness = 100;

    config->_internal.brightness = brightness;

    // Аппаратная установка яркости
    if (config->set_brightness)
    {
        config->set_brightness(brightness);
    }

    return LCD_CORE_OK;
}

uint32_t lcd_core_get_time(const LcdCoreConfig *config)
{
    if (!config)
        return 0;
    return get_current_time(config);
}