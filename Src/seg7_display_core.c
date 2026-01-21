#include "seg7_display_core.h"
#include <stdlib.h>
#include <string.h>

// ============== Внутренние функции ==============

/**
 * @brief Получение текущего времени
 */
static uint32_t get_current_time(const LcdCoreConfig* config)
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
    if (refresh_rate_hz == 0) refresh_rate_hz = 60; // По умолчанию 60 Гц
    
    // Время кадра = 1 / частота (в микросекундах)
    uint32_t frame_time_us = 1000000UL / refresh_rate_hz;
    
    // Время на разряд = время кадра / количество разрядов
    return (uint16_t)(frame_time_us / digits_count);
}

/**
 * @brief Подготовка кода сегментов с точкой
 */
static uint8_t prepare_segment_mask(const LcdCoreConfig* config, uint8_t segment_mask, bool dot_on)
{
    uint8_t result = segment_mask;
    
    // Добавляем точку если нужно
    if (dot_on && config->dot_bit_mask)
    {
        result |= config->dot_bit_mask;
    }
    else
    {
        result &= ~config->dot_bit_mask;
    }
    
    // Инвертируем если нужно
    if (config->invert_output)
    {
        result = ~result;
    }
    
    return result;
}

/**
 * @brief Определение режима управления
 */
static LcdControlMode determine_control_mode(LcdCoreConfig* config)
{
    if (config->control_mode == LCD_MODE_AUTO)
    {
        // Автовыбор: если задан set_indicator, используем совместный режим
        if (config->set_indicator != NULL)
        {
            return LCD_MODE_COMBINED;
        }
        // Иначе проверяем обязательные callback-и для раздельного режима
        else if (config->set_segments != NULL && config->set_digit != NULL)
        {
            return LCD_MODE_SEPARATE;
        }
        else
        {
            return LCD_MODE_SEPARATE; // По умолчанию
        }
    }

    return config->control_mode;
}

/**
 * @brief Аппаратное обновление разряда
 */
static void update_hardware_digit_separate(LcdCoreConfig* config)
{
    uint8_t digit = config->_internal.current_digit;
    
    // 1. Выключаем текущий разряд
    bool off_state = config->invert_output; // Для инвертированного вывода логика обратная
    config->set_digit(digit, off_state, config->ctx);
    
    // 2. Подготавливаем маску сегментов с точкой
    uint8_t segment_mask = prepare_segment_mask(
        config,
        config->_internal.segment_buffer[digit],
        config->_internal.dot_buffer[digit] ? true : false
    );
    
    // 3. Устанавливаем сегменты
    config->set_segments(segment_mask, config->ctx);
    
    // 4. Включаем текущий разряд
    config->set_digit(digit, !off_state, config->ctx);
    
    // 5. Переходим к следующему разряду
    config->_internal.current_digit = 
        (digit + 1) % config->digits_count;
}

/**
 * @brief Аппаратное обновление разряда (совместный режим)
 */
static void update_hardware_digit_combined(LcdCoreConfig* config)
{
    uint8_t digit = config->_internal.current_digit;

    // Подготавливаем маску сегментов с точкой
    uint8_t segment_mask = prepare_segment_mask(
        config,
        config->_internal.segment_buffer[digit],
        config->_internal.dot_buffer[digit] ? true : false
    );

    // Устанавливаем сегменты и разряд одновременно
    config->set_indicator(segment_mask, digit, config->ctx);

    // Переходим к следующему разряду
    config->_internal.current_digit =
        (digit + 1) % config->digits_count;
}

// ============== Публичные функции ==============

LcdCoreError lcd_core_init(LcdCoreConfig* config)
{
    // Проверка указателей
    if (!config)
    {
        return LCD_CORE_ERROR_NULL_POINTER;
    }
    
    // Определение режима управления
    LcdControlMode mode = determine_control_mode(config);

    // Проверка необходимых callback-ов в зависимости от режима
    if (mode == LCD_MODE_SEPARATE)
    {
        if (!config->set_segments || !config->set_digit)
        {
            return LCD_CORE_ERROR_NULL_POINTER;
        }
    }
    else if (mode == LCD_MODE_COMBINED)
    {
        if (!config->set_indicator)
        {
            return LCD_CORE_ERROR_NULL_POINTER;
        }
    }

    // Проверка конфигурации
    if (config->digits_count == 0 || config->digits_count > 16)
    {
        return LCD_CORE_ERROR_INVALID_CONFIG;
    }
    
    // Установка значений по умолчанию
    if (config->dot_bit_mask == 0)
    {
        config->dot_bit_mask = 0x80; // По умолчанию бит 7 для точки
    }

    // Выделение памяти для буферов
    config->_internal.segment_buffer = 
        (uint8_t*)malloc(config->digits_count);
    
    config->_internal.dot_buffer = 
        (uint8_t*)malloc(config->digits_count);
    
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
    memset(config->_internal.segment_buffer, 0, config->digits_count);
    memset(config->_internal.dot_buffer, 0, config->digits_count);
    
    // Инициализация переменных
    config->_internal.current_digit = 0;
    config->_internal.last_update_time = 0;
    config->_internal.brightness = 100;
    config->_internal.initialized = true;
    
    // Установка яркости
    if (config->set_brightness)
    {
        config->set_brightness(config->_internal.brightness, config->ctx);
    }
    
    return LCD_CORE_OK;
}

LcdCoreError lcd_core_update(LcdCoreConfig* config)
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
    	// Выбираем режим обновления
    	LcdControlMode mode = determine_control_mode(config);

    	if (mode == LCD_MODE_SEPARATE)
    	{
    		update_hardware_digit_separate(config);
    	}
    	else if (mode == LCD_MODE_COMBINED)
    	{
    		update_hardware_digit_combined(config);
    	}

    	config->_internal.last_update_time = current_time;
    }
    
    return LCD_CORE_OK;
}

LcdCoreError lcd_core_set_segment_mask(LcdCoreConfig* config, 
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

LcdCoreError lcd_core_set_dot(LcdCoreConfig* config, 
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

LcdCoreError lcd_core_clear(LcdCoreConfig* config)
{
    if (!config || !config->_internal.initialized)
    {
        return LCD_CORE_ERROR_NOT_INITIALIZED;
    }
    
    // Очищаем буферы
    memset(config->_internal.segment_buffer, 0, config->digits_count);
    memset(config->_internal.dot_buffer, 0, config->digits_count);
    
    // Определяем режим управления
    LcdControlMode mode = determine_control_mode(config);

    if (mode == LCD_MODE_SEPARATE)
    {
    	// Выключаем все разряды аппаратно
    	bool off_state = config->invert_output;
    	for (uint8_t i = 0; i < config->digits_count; i++)
    	{
    		config->set_digit(i, off_state, config->ctx);
    	}

    	// Выключаем сегменты
    	uint8_t clear_segments = config->invert_output ? 0xFF : 0x00;
    	config->set_segments(clear_segments, config->ctx);
    }
    else if (mode == LCD_MODE_COMBINED)
    {
    	// Для совместного режима: отправляем пустые сегменты для каждого разряда
    	for (uint8_t i = 0; i < config->digits_count; i++)
    	{
    		uint8_t clear_segments = config->invert_output ? 0xFF : 0x00;
    		config->set_indicator(clear_segments, i, config->ctx);
    	}
    }
    
    return LCD_CORE_OK;
}

LcdCoreError lcd_core_set_brightness(LcdCoreConfig* config, uint8_t brightness)
{
    if (!config || !config->_internal.initialized)
    {
        return LCD_CORE_ERROR_NOT_INITIALIZED;
    }
    
    // Проверка диапазона
    if (brightness > 100) brightness = 100;
    
    config->_internal.brightness = brightness;
    
    // Аппаратная установка яркости
    if (config->set_brightness)
    {
        config->set_brightness(brightness, config->ctx);
    }
    
    return LCD_CORE_OK;
}

uint32_t lcd_core_get_time(const LcdCoreConfig* config)
{
    if (!config) return 0;
    return get_current_time(config);
}
