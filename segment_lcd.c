#include "segment_lcd.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============== Внутренние функции ==============

/**
 * @brief Обновление буфера ядра из символьного буфера
 */
static LcdError update_core_buffer(LcdDisplay* display)
{
    if (!display || !display->initialized)
    {
        return LCD_ERROR_NOT_INITIALIZED;
    }
    
    bool is_common_cathode = (display->core.type == LCD_TYPE_COMMON_CATHODE);
    
    for (uint8_t i = 0; i < display->core.digits_count; i++)
    {
        // Получаем код сегмента для символа
        uint8_t segment_code = segchar_get_code(
            (SegmentChar)display->char_buffer[i],
            display->table,
            is_common_cathode
        );
        
        // Устанавливаем в ядро
        LcdCoreError err = lcd_core_set_segment_mask(
            &display->core,
            i,
            segment_code
        );
        
        if (err != LCD_CORE_OK)
        {
            return LCD_ERROR_NULL_POINTER;
        }
        
        // Устанавливаем точку
        lcd_core_set_dot(
            &display->core,
            i,
            display->dot_buffer[i] ? true : false
        );
    }
    
    return LCD_OK;
}

/**
 * @brief Конвертация числа в строку символов
 */
static LcdError int_to_chars(int32_t number, SegmentChar* buffer, 
                            uint8_t buffer_size, uint8_t* length, 
                            bool leading_zeros)
{
    if (!buffer || buffer_size == 0)
    {
        return LCD_ERROR_NULL_POINTER;
    }
    
    bool negative = (number < 0);
    uint32_t abs_value = negative ? (uint32_t)(-number) : (uint32_t)number;
    
    // Преобразуем в строку цифр
    uint8_t pos = buffer_size - 1;
    uint8_t digit_count = 0;
    
    do
    {
        buffer[pos--] = (SegmentChar)(abs_value % 10);
        abs_value /= 10;
        digit_count++;
    } while (abs_value > 0 && pos < buffer_size);
    
    // Заполняем ведущими нулями или пробелами
    uint8_t start_pos = buffer_size - digit_count;
    
    if (leading_zeros)
    {
        for (uint8_t i = 0; i < start_pos; i++)
        {
            buffer[i] = SEG_CHAR_0;
        }
    }
    else
    {
        for (uint8_t i = 0; i < start_pos; i++)
        {
            buffer[i] = SEG_CHAR_EMPTY;
        }
    }
    
    // Добавляем знак минус
    if (negative && start_pos > 0)
    {
        buffer[start_pos - 1] = SEG_CHAR_MINUS;
    }
    
    if (length)
    {
        *length = buffer_size;
    }
    
    return LCD_OK;
}

// ============== Публичные функции ==============

LcdError lcd_init(LcdDisplay* display,
                  uint8_t digits_count,
                  LcdType type,
                  uint16_t refresh_rate_hz,
                  LcdSegmentCallback seg_cb,
                  LcdDigitCallback dig_cb,
                  LcdBrightnessCallback bright_cb,
                  LcdTimeCallback time_cb,
                  const SegmentTable* custom_table)
{
    if (!display || !seg_cb || !dig_cb)
    {
        return LCD_ERROR_NULL_POINTER;
    }
    
    // Инициализация ядра
    display->core.set_segments = seg_cb;
    display->core.set_digit = dig_cb;
    display->core.set_brightness = bright_cb;
    display->core.get_time_ms = time_cb;
    display->core.digits_count = digits_count;
    display->core.type = type;
    display->core.refresh_rate_hz = refresh_rate_hz;
    display->core.custom_segment_table = NULL; // Используется через таблицу символов
    
    LcdCoreError core_err = lcd_core_init(&display->core);
    if (core_err != LCD_CORE_OK)
    {
        return LCD_ERROR_NULL_POINTER;
    }
    
    // Установка таблицы символов
    if (custom_table)
    {
        display->table = custom_table;
    }
    else
    {
        display->table = (type == LCD_TYPE_COMMON_CATHODE) ?
                        segchar_get_default_table_cc() :
                        segchar_get_default_table_ca();
    }
    
    // Выделение памяти для буферов
    display->buffer_size = digits_count;
    display->char_buffer = (uint8_t*)malloc(digits_count);
    display->dot_buffer = (uint8_t*)malloc(digits_count);
    display->temp_buffer = (uint8_t*)malloc(digits_count);
    
    if (!display->char_buffer || !display->dot_buffer || !display->temp_buffer)
    {
        free(display->char_buffer);
        free(display->dot_buffer);
        free(display->temp_buffer);
        return LCD_ERROR_MEMORY;
    }
    
    // Инициализация буферов
    memset(display->char_buffer, SEG_CHAR_EMPTY, digits_count);
    memset(display->dot_buffer, 0, digits_count);
    memset(display->temp_buffer, 0, digits_count);
    
    display->initialized = true;
    
    return LCD_OK;
}

LcdError lcd_deinit(LcdDisplay* display)
{
    if (!display)
    {
        return LCD_ERROR_NULL_POINTER;
    }
    
    if (display->initialized)
    {
        // Освобождение памяти
        free(display->char_buffer);
        free(display->dot_buffer);
        free(display->temp_buffer);
        
        // Освобождение памяти ядра
        free(display->core._internal.segment_buffer);
        free(display->core._internal.dot_buffer);
        
        display->initialized = false;
    }
    
    return LCD_OK;
}

LcdError lcd_update(LcdDisplay* display)
{
    if (!display || !display->initialized)
    {
        return LCD_ERROR_NOT_INITIALIZED;
    }
    
    // Обновление ядра
    return (lcd_core_update(&display->core) == LCD_CORE_OK) ? 
           LCD_OK : LCD_ERROR_NULL_POINTER;
}

LcdError lcd_show_int(LcdDisplay* display, int32_t number, 
                      uint8_t decimal_position, bool leading_zeros)
{
    if (!display || !display->initialized)
    {
        return LCD_ERROR_NOT_INITIALIZED;
    }
    
    // Конвертируем число в символы
    SegmentChar temp_buffer[16]; // Максимум 16 разрядов
    uint8_t length = 0;
    
    LcdError err = int_to_chars(number, temp_buffer, 
                               display->buffer_size, &length, 
                               leading_zeros);
    
    if (err != LCD_OK)
    {
        return err;
    }
    
    // Копируем в буфер дисплея
    memcpy(display->char_buffer, temp_buffer, display->buffer_size);
    
    // Устанавливаем точку, если нужно
    if (decimal_position > 0 && decimal_position < display->buffer_size)
    {
        memset(display->dot_buffer, 0, display->buffer_size);
        display->dot_buffer[decimal_position] = 1;
    }
    else
    {
        memset(display->dot_buffer, 0, display->buffer_size);
    }
    
    // Обновляем ядро
    return update_core_buffer(display);
}

LcdError lcd_show_float(LcdDisplay* display, float number, 
                        uint8_t decimal_places)
{
    if (!display || !display->initialized)
    {
        return LCD_ERROR_NOT_INITIALIZED;
    }
    
    if (decimal_places >= display->buffer_size)
    {
        decimal_places = display->buffer_size - 1;
    }
    
    // Проверяем диапазон
    float max_value = powf(10.0f, (float)(display->buffer_size - decimal_places - 1)) - 1.0f;
    float min_value = -max_value;
    
    if (number > max_value) number = max_value;
    if (number < min_value) number = min_value;
    
    // Конвертируем в целое с учетом десятичных знаков
    int32_t scaled = (int32_t)(number * powf(10.0f, (float)decimal_places));
    
    // Отображаем как целое с точкой
    return lcd_show_int(display, scaled, 
                       display->buffer_size - decimal_places, 
                       false);
}

LcdError lcd_show_string(LcdDisplay* display, const SegmentChar* chars, 
                         uint8_t length)
{
    if (!display || !display->initialized || !chars)
    {
        return LCD_ERROR_NULL_POINTER;
    }
    
    if (length > display->buffer_size)
    {
        length = display->buffer_size;
    }
    
    // Копируем символы
    memset(display->char_buffer, SEG_CHAR_EMPTY, display->buffer_size);
    memcpy(display->char_buffer, chars, length);
    
    // Очищаем точки
    memset(display->dot_buffer, 0, display->buffer_size);
    
    // Обновляем ядро
    return update_core_buffer(display);
}

LcdError lcd_show_ascii(LcdDisplay* display, const char* str)
{
    if (!display || !display->initialized || !str)
    {
        return LCD_ERROR_NULL_POINTER;
    }
    
    // Конвертируем строку
    SegmentChar temp_buffer[16];
    uint16_t converted_length = 0;
    
    if (!segchar_convert_string(str, temp_buffer, 
                               display->buffer_size, 
                               &converted_length))
    {
        return LCD_ERROR_INVALID_CHAR;
    }
    
    // Отображаем
    return lcd_show_string(display, temp_buffer, converted_length);
}

LcdError lcd_set_char(LcdDisplay* display, uint8_t position, 
                      SegmentChar character)
{
    if (!display || !display->initialized)
    {
        return LCD_ERROR_NOT_INITIALIZED;
    }
    
    if (position >= display->buffer_size)
    {
        return LCD_ERROR_INVALID_PARAM;
    }
    
    display->char_buffer[position] = (uint8_t)character;
    
    // Обновляем ядро
    return update_core_buffer(display);
}

LcdError lcd_set_dot(LcdDisplay* display, uint8_t position, DotState state)
{
    if (!display || !display->initialized)
    {
        return LCD_ERROR_NOT_INITIALIZED;
    }
    
    if (position >= display->buffer_size)
    {
        return LCD_ERROR_INVALID_PARAM;
    }
    
    display->dot_buffer[position] = (state == DOT_ON) ? 1 : 0;
    
    // Обновляем ядро
    LcdCoreError err = lcd_core_set_dot(&display->core, position, 
                                        state == DOT_ON);
    
    return (err == LCD_CORE_OK) ? LCD_OK : LCD_ERROR_NULL_POINTER;
}

LcdError lcd_clear(LcdDisplay* display)
{
    if (!display || !display->initialized)
    {
        return LCD_ERROR_NOT_INITIALIZED;
    }
    
    // Очищаем буферы
    memset(display->char_buffer, SEG_CHAR_EMPTY, display->buffer_size);
    memset(display->dot_buffer, 0, display->buffer_size);
    
    // Очищаем ядро
    return (lcd_core_clear(&display->core) == LCD_CORE_OK) ? 
           LCD_OK : LCD_ERROR_NULL_POINTER;
}

LcdError lcd_set_brightness(LcdDisplay* display, uint8_t brightness)
{
    if (!display || !display->initialized)
    {
        return LCD_ERROR_NOT_INITIALIZED;
    }
    
    return (lcd_core_set_brightness(&display->core, brightness) == LCD_CORE_OK) ?
           LCD_OK : LCD_ERROR_NULL_POINTER;
}

// ============== Функции бегущей строки ==============

LcdError lcd_scroll_init(LcdScrollingText* scroll, LcdDisplay* display,
                         const char* text, ScrollDirection direction,
                         uint16_t delay_ms, bool loop)
{
    if (!scroll || !display || !text)
    {
        return LCD_ERROR_NULL_POINTER;
    }
    
    // Сохраняем ссылку на дисплей
    scroll->display = display;
    scroll->text = text;
    scroll->direction = direction;
    scroll->delay_ms = delay_ms;
    scroll->loop = loop;
    scroll->enabled = false;
    scroll->position = 0;
    scroll->auto_convert = true;
    
    // Конвертируем текст
    scroll->text_length = strlen(text);
    scroll->converted_text = (SegmentChar*)malloc(scroll->text_length);
    
    if (!scroll->converted_text)
    {
        return LCD_ERROR_MEMORY;
    }
    
    // Конвертируем ASCII в SegmentChar
    uint16_t converted_length = 0;
    segchar_convert_string(text, scroll->converted_text, 
                          scroll->text_length, &converted_length);
    
    scroll->text_length = converted_length;
    
    return LCD_OK;
}

LcdError lcd_scroll_update(LcdScrollingText* scroll)
{
    if (!scroll || !scroll->display || !scroll->enabled)
    {
        return LCD_ERROR_NULL_POINTER;
    }
    
    // Проверяем время
    uint32_t current_time = lcd_core_get_time(&scroll->display->core);
    
    if (current_time - scroll->last_update < scroll->delay_ms)
    {
        return LCD_OK;
    }
    
    scroll->last_update = current_time;
    
    // Создаем буфер для отображения
    SegmentChar display_buffer[16];
    uint8_t display_len = scroll->display->buffer_size;
    
    memset(display_buffer, SEG_CHAR_EMPTY, sizeof(display_buffer));
    
    // Заполняем буфер в зависимости от направления
    if (scroll->direction == SCROLL_LEFT)
    {
        for (uint8_t i = 0; i < display_len; i++)
        {
            int16_t text_index = (int16_t)scroll->position + i;
            
            if (text_index >= 0 && text_index < scroll->text_length)
            {
                display_buffer[i] = scroll->converted_text[text_index];
            }
        }
        
        scroll->position++;
        
        // Проверяем завершение
        if (!scroll->loop && scroll->position >= scroll->text_length + display_len)
        {
            scroll->enabled = false;
        }
        else if (scroll->loop && scroll->position >= scroll->text_length + display_len)
        {
            scroll->position = 0;
        }
    }
    else if (scroll->direction == SCROLL_RIGHT)
    {
        for (uint8_t i = 0; i < display_len; i++)
        {
            int16_t text_index = (int16_t)scroll->text_length - 1 - scroll->position + i;
            
            if (text_index >= 0 && text_index < scroll->text_length)
            {
                display_buffer[i] = scroll->converted_text[text_index];
            }
        }
        
        scroll->position++;
        
        // Проверяем завершение
        if (!scroll->loop && scroll->position >= scroll->text_length + display_len)
        {
            scroll->enabled = false;
        }
        else if (scroll->loop && scroll->position >= scroll->text_length + display_len)
        {
            scroll->position = 0;
        }
    }
    
    // Отображаем
    return lcd_show_string(scroll->display, display_buffer, display_len);
}

LcdError lcd_scroll_start(LcdScrollingText* scroll)
{
    if (!scroll)
    {
        return LCD_ERROR_NULL_POINTER;
    }
    
    scroll->enabled = true;
    scroll->last_update = lcd_core_get_time(&scroll->display->core);
    
    return LCD_OK;
}

LcdError lcd_scroll_stop(LcdScrollingText* scroll)
{
    if (!scroll)
    {
        return LCD_ERROR_NULL_POINTER;
    }
    
    scroll->enabled = false;
    
    return LCD_OK;
}