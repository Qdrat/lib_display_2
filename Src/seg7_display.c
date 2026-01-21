#include "seg7_display.h"
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
    
    const SegmentTable* table = display->table ? display->table : segchar_get_default_table();
    bool invert_output = display->core.invert_output;
    
    for (uint8_t i = 0; i < display->core.digits_count; i++)
    {
        // Получаем код сегмента для символа
        uint8_t segment_code = segchar_get_code(
            display->char_buffer[i],
            table,
            invert_output
        );
        
        // Устанавливаем в ядро (без точки)
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
static LcdError int_to_chars(int32_t number, uint8_t* buffer, 
                             uint8_t buffer_size, uint8_t* length,
                             bool leading_zeros)
{
    if (!buffer || buffer_size == 0)
    {
        return LCD_ERROR_NULL_POINTER;
    }

    // Инициализируем буфер пустыми символами
    for (uint8_t i = 0; i < buffer_size; ++i)
    {
        buffer[i] = SEG_CHAR_EMPTY;
    }

    bool negative = (number < 0);
    uint32_t abs_value = negative ? (uint32_t)(-number) : (uint32_t)number;

    // Преобразуем в строку цифр
    uint8_t pos = buffer_size - 1;
    uint8_t digit_count = 0;

    // ОСОБЫЙ СЛУЧАЙ: число 0
    if (abs_value == 0)
    {
        buffer[pos] = SEG_CHAR_0;
        digit_count = 1;
        pos--;
    }
    else
    {
        while (abs_value > 0 && pos < buffer_size) // pos должен быть >= 0
        {
            buffer[pos--] = (uint8_t)(abs_value % 10);
            abs_value /= 10;
            digit_count++;
        }
    }

    // Заполняем ведущими нулями или пробелами
    uint8_t start_pos = buffer_size - digit_count;

    if (leading_zeros)
    {
        for (uint8_t i = 0; i < start_pos; i++)
        {
            buffer[i] = SEG_CHAR_0;
        }
    }

    // Добавляем знак минус, если есть место
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

/**
 * @brief Обработка анимации
 */
static void process_animation(LcdDisplay* display)
{
    if (!display->animation.active) return;
    
    uint32_t current_time = lcd_core_get_time(&display->core);
    
    // Проверяем, прошло ли достаточно времени
    if (current_time - display->animation.last_update < display->animation.delay_ms)
    {
        return;
    }
    
    display->animation.last_update = current_time;
    
    // Обработка в зависимости от типа анимации
    switch (display->animation.type)
    {
        case ANIMATION_SCROLL_LEFT:
            // Копируем символы и точки из буфера анимации
            for (uint8_t i = 0; i < display->buffer_size; i++)
            {
                uint16_t src_idx = display->animation.position + i;
                
                if (src_idx < display->animation.anim_buffer_len)
                {
                    display->char_buffer[i] = display->animation.animation_buffer[src_idx];
                    display->dot_buffer[i] = display->animation.animation_dots[src_idx];
                }
                else
                {
                    display->char_buffer[i] = SEG_CHAR_EMPTY;
                    display->dot_buffer[i] = 0;
                }
            }
            
            display->animation.position++;
            
            // Проверка завершения
            if (display->animation.position >= display->animation.anim_buffer_len)
            {
                if (display->animation.loop)
                {
                    display->animation.position = 0;
                }
                else
                {
                    display->animation.active = false;
                    if (display->animation.params.on_end)
                    {
                        display->animation.params.on_end(display->animation.params.user_data);
                    }
                }
            }
            break;
            
        case ANIMATION_BLINK:
            // Мигание: чередование видимости/невидимости
            if ((current_time - display->animation.start_time) / display->animation.delay_ms % 2 == 0)
            {
                // Показываем оригинальные символы
                memcpy(display->char_buffer, display->animation.original_chars, display->buffer_size);
                memcpy(display->dot_buffer, display->animation.original_dots, display->buffer_size);
            }
            else
            {
                // Скрываем (заполняем пустыми символами)
                memset(display->char_buffer, SEG_CHAR_EMPTY, display->buffer_size);
                memset(display->dot_buffer, 0, display->buffer_size);
            }
            break;
            
        default:
            break;
    }
    
    // Обновляем дисплей
    update_core_buffer(display);
}

// ============== Вспомогательная функция для обработки точек ==============
static void process_dots_in_buffer(const uint8_t* input_buffer, uint8_t input_len,
                                         uint8_t* output_chars, uint8_t* output_dots,
                                         uint8_t* output_len, uint8_t max_output_len)
{
    uint8_t out_pos = 0;
    uint8_t i = 0;

    while (i < input_len && out_pos < max_output_len)
    {
        if (input_buffer[i] == SEG_CHAR_DOT)
        {
            // Нашли точку
            // Проверяем, есть ли перед ней символ (не точка)
            if (i > 0 && input_buffer[i - 1] != SEG_CHAR_DOT)
            {
                // Точка после символа
                // Проверяем, сколько точек после этого символа
                uint8_t dot_count = 0;
                while (i + dot_count < input_len && input_buffer[i + dot_count] == SEG_CHAR_DOT)
                {
                    dot_count++;
                }

                if (dot_count == 1)
                {
                    // Одна точка - добавляем к предыдущему символу
                    if (out_pos > 0)
                    {
                        output_dots[out_pos - 1] = 1;
                    }
                }
                else
                {
                    // Многоточие: первая точка к символу, остальные отдельно
                    if (out_pos > 0)
                    {
                        output_dots[out_pos - 1] = 1;
                    }

                    // Добавляем остальные точки как отдельные разряды
                    for (uint8_t j = 1; j < dot_count && out_pos < max_output_len; j++)
                    {
                        output_chars[out_pos] = SEG_CHAR_EMPTY;
                        output_dots[out_pos] = 1;
                        out_pos++;
                    }
                }

                i += dot_count;
                continue;
            }
            else
            {
                // Точка в начале или после другой точки
                // Считаем все точки подряд
                uint8_t dot_count = 0;
                while (i + dot_count < input_len && input_buffer[i + dot_count] == SEG_CHAR_DOT)
                {
                    dot_count++;
                }

                // Создаем отдельные разряды для всех точек
                for (uint8_t j = 0; j < dot_count && out_pos < max_output_len; j++)
                {
                    output_chars[out_pos] = SEG_CHAR_EMPTY;
                    output_dots[out_pos] = 1;
                    out_pos++;
                }

                i += dot_count;
                continue;
            }
        }
        else
        {
            // Обычный символ
            output_chars[out_pos] = input_buffer[i];
            output_dots[out_pos] = 0;
            out_pos++;
            i++;
        }
    }

    *output_len = out_pos;
}

// ============== Публичные функции ==============

LcdError lcd_init_ex(LcdDisplay* display,
                    uint8_t digits_count,
                    uint16_t refresh_rate_hz,
                    bool invert_output,
                    LcdSegmentCallback seg_cb,
                    LcdDigitCallback dig_cb,
                    LcdIndicatorCallback ind_cb,
                    LcdBrightnessCallback bright_cb,
                    LcdTimeCallback time_cb,
                    const SegmentTable* custom_table,
                    void* ctx,
                    LcdControlMode control_mode)
{
    if (!display || (!seg_cb && !dig_cb && !ind_cb))
    {
        return LCD_ERROR_NULL_POINTER;
    }
    
    // Инициализация ядра
    display->core.set_segments = seg_cb;
    display->core.set_digit = dig_cb;
    display->core.set_indicator = ind_cb;
    display->core.set_brightness = bright_cb;
    display->core.get_time_ms = time_cb;
    display->core.ctx = ctx;
    display->core.control_mode = control_mode;
    display->core.digits_count = digits_count;
    display->core.refresh_rate_hz = refresh_rate_hz;
    display->core.invert_output = invert_output;

    // Установка бита для точки из таблицы
    const SegmentTable* table = custom_table ? custom_table : segchar_get_default_table();
    display->core.dot_bit_mask = table->dot_bit_mask;
    
    LcdCoreError core_err = lcd_core_init(&display->core);
    if (core_err != LCD_CORE_OK)
    {
        if (core_err == LCD_CORE_ERROR_MEMORY)
            return LCD_ERROR_MEMORY;
        return LCD_ERROR_NULL_POINTER;
    }
    
    // Установка таблицы символов
    display->table = table;
    display->buffer_size = digits_count;
    
    // Инициализация буферов
    memset(display->char_buffer, SEG_CHAR_EMPTY, sizeof(display->char_buffer));
    memset(display->dot_buffer, 0, sizeof(display->dot_buffer));
    
    // Инициализация анимации
    memset(&display->animation, 0, sizeof(display->animation));
    
    display->initialized = true;
    
    return LCD_OK;
}

LcdError lcd_init(LcdDisplay* display,
                  uint8_t digits_count,
                  uint16_t refresh_rate_hz,
                  bool invert_output,
                  LcdSegmentCallback seg_cb,
                  LcdDigitCallback dig_cb,
                  LcdBrightnessCallback bright_cb,
                  LcdTimeCallback time_cb,
                  const SegmentTable* custom_table,
                  void* ctx)
{
	// Вызываем расширенную версию с AUTO режимом и без индикатора
	return lcd_init_ex(display,
					   digits_count,
					   refresh_rate_hz,
					   invert_output,
					   seg_cb,
					   dig_cb,
					   NULL,  // индикатор не задан
					   bright_cb,
					   time_cb,
					   custom_table,
					   ctx,
					   LCD_MODE_AUTO);
}

LcdError lcd_deinit(LcdDisplay* display)
{
    if (!display)
    {
        return LCD_ERROR_NULL_POINTER;
    }
    
    if (display->initialized)
    {
        // Останавливаем анимацию
        lcd_stop_animation(display);
        
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
    
    // Обновление анимации (если активна)
    if (display->animation.active)
    {
        process_animation(display);
    }

    // Обновление ядра (динамическая индикация)
    LcdCoreError core_err = lcd_core_update(&display->core);
    return (core_err == LCD_CORE_OK) ? LCD_OK : LCD_ERROR_NULL_POINTER;
}

LcdError lcd_show_num(LcdDisplay* display, int32_t number, 
                      uint8_t decimal_position, bool leading_zeros)
{
    if (!display || !display->initialized)
    {
        return LCD_ERROR_NOT_INITIALIZED;
    }

    // Останавливаем текущую анимацию
    lcd_stop_animation(display);
    
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
    
    // очистка точек
    memset(display->dot_buffer, 0, display->buffer_size);

    // decimal_position — номер digit (1 = левый)
    if (decimal_position >= 1 && decimal_position <= display->buffer_size)
    {
        display->dot_buffer[decimal_position - 1] = 1;
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

    // Для отладки: проверим входные данные
    // number = 23.5f, decimal_places = 1, buffer_size = 4

    // Масштабируем число
    float scale = 1.0f;
    for (uint8_t i = 0; i < decimal_places; i++) {
        scale *= 10.0f;
    }

    // Округляем правильно
    int32_t scaled;
    if (number >= 0) {
        scaled = (int32_t)(number * scale + 0.5f);
    } else {
        scaled = (int32_t)(number * scale - 0.5f);
    }

    // Отладочный вывод
    // Для 23.5: scaled должен быть 235

    // Проверяем диапазон
    // Максимальное значение для 4 разрядов с 1 десятичным знаком: 999.9
    // scaled_max = 9999
    int32_t max_scaled = 1;
    for (uint8_t i = 0; i < display->buffer_size; i++) {
        max_scaled *= 10;
    }
    max_scaled = max_scaled - 1; // 9999 для 4 разрядов

    int32_t min_scaled = -max_scaled;

    if (scaled > max_scaled) scaled = max_scaled;
    if (scaled < min_scaled) scaled = min_scaled;


    return lcd_show_num(display, scaled,
                       display->buffer_size - decimal_places,
                       false);
}

LcdError lcd_show_string(LcdDisplay* display, const uint8_t* chars, 
                         uint8_t length, const AnimationParams* animation_params)
{
    if (!display || !display->initialized || !chars)
    {
        return LCD_ERROR_NULL_POINTER;
    }
    
    if (length == 0)
    {
        return LCD_OK;
    }
    
    // Проверяем, активна ли анимация
    if (display->animation.active)
    {
        // Обрабатываем входные данные для сравнения
        uint8_t processed_chars[64];
        uint8_t processed_dots[64] = {0};
        uint8_t processed_len = 0;

        process_dots_in_buffer(chars, length, processed_chars, processed_dots,
                              &processed_len, sizeof(processed_chars));

        // Сравниваем с текущей анимацией
        if (animation_params &&
            display->animation.type == animation_params->type &&
            display->animation.loop == animation_params->loop &&
            display->animation.delay_ms == animation_params->delay_ms &&
            processed_len == display->animation.anim_buffer_len &&
            memcmp(processed_chars, display->animation.animation_buffer, processed_len) == 0 &&
            memcmp(processed_dots, display->animation.animation_dots, processed_len) == 0)
        {
            return LCD_OK;
        }

        // Если другая анимация - можно остановить или вернуть ошибку
        return LCD_ERROR_ANIMATION_IN_PROGRESS;
    }
    
    if (animation_params)
    {
        // Настройка анимации
        display->animation.type = animation_params->type;
        display->animation.active = true;
        display->animation.loop = animation_params->loop;
        display->animation.delay_ms = animation_params->delay_ms;
        display->animation.start_time = lcd_core_get_time(&display->core);
        display->animation.last_update = display->animation.start_time;
        display->animation.position = 0;
        display->animation.params = *animation_params;
        
        // Сохраняем оригинальные символы и точки
        memcpy(display->animation.original_chars, display->char_buffer, display->buffer_size);
        memcpy(display->animation.original_dots, display->dot_buffer, display->buffer_size);
        
        // Обрабатываем входной массив для анимации
        uint8_t processed_len = 0;
        
        process_dots_in_buffer(chars, length,
                              display->animation.animation_buffer,
                              display->animation.animation_dots,
                              &processed_len,
                              sizeof(display->animation.animation_buffer));

        display->animation.anim_buffer_len = processed_len;

        // Callback начала
        if (animation_params->on_start)
        {
            animation_params->on_start(animation_params->user_data);
        }

        // Сразу обновляем первый кадр
        process_animation(display);
    }
    else
    {
        // Статическое отображение
        // Обрабатываем точки в буфере
        uint8_t processed_len = 0;

        process_dots_in_buffer(chars, length,
                              display->char_buffer,
                              display->dot_buffer,
                              &processed_len,
                              display->buffer_size);

        // Очищаем оставшиеся разряды
        for (uint8_t i = processed_len; i < display->buffer_size; i++)
        {
            display->char_buffer[i] = SEG_CHAR_EMPTY;
            display->dot_buffer[i] = 0;
        }

        // Обновляем ядро
        update_core_buffer(display);
    }

    return LCD_OK;
}

LcdError lcd_show_ascii(LcdDisplay* display, const char* str, 
                        const AnimationParams* animation_params)
{
    if (!display || !display->initialized || !str)
    {
        return LCD_ERROR_NULL_POINTER;
    }
    
    // Конвертируем строку в символы
    uint8_t buffer[64];
    uint8_t out_pos = 0;
    
    // Обрабатываем строку символ за символом
    while (*str && out_pos < sizeof(buffer))
    {
        if (*str == '.')
        {
            // Добавляем SEG_CHAR_DOT для каждой точки
            buffer[out_pos] = SEG_CHAR_DOT;
            out_pos++;
        }
        else
        {
            // Конвертируем символ
            buffer[out_pos] = segchar_from_ascii(*str);
            out_pos++;
        }
        str++;
    }
    
    // Отображаем строку с обработкой точек
    return lcd_show_string(display, buffer, out_pos, animation_params);
}

LcdError lcd_set_char(LcdDisplay* display, uint8_t position, 
                      uint8_t character)
{
    if (!display || !display->initialized)
    {
        return LCD_ERROR_NOT_INITIALIZED;
    }
    
    if (position >= display->buffer_size)
    {
        return LCD_ERROR_INVALID_PARAM;
    }
    
    // Останавливаем анимацию
    lcd_stop_animation(display);

    // Устанавливаем символ
    display->char_buffer[position] = character;

    // Получаем код сегмента для обновленного символа
    const SegmentTable* table = display->table ? display->table : segchar_get_default_table();
    bool invert_output = display->core.invert_output;
    
    uint8_t segment_code = segchar_get_code(character, table, invert_output);

    // Обновляем только измененный разряд в ядре
    LcdCoreError err = lcd_core_set_segment_mask(&display->core, position, segment_code);
    if (err != LCD_CORE_OK)
    {
    	return LCD_ERROR_NULL_POINTER;
    }

    // Обновляем точку для этого разряда
    lcd_core_set_dot(&display->core, position, display->dot_buffer[position] ? true : false);

    return LCD_OK;
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
    
    // Останавливаем анимацию
    lcd_stop_animation(display);
    
    display->dot_buffer[position] = (state == DOT_ON) ? 1 : 0;
    
    // Обновляем ядро
    return update_core_buffer(display);
}

LcdError lcd_clear(LcdDisplay* display)
{
    if (!display || !display->initialized)
    {
        return LCD_ERROR_NOT_INITIALIZED;
    }
    
    // Очищаем буферы
    for (uint8_t i = 0; i < display->buffer_size; i++)
    {
    	display->char_buffer[i] = SEG_CHAR_SPACE;  // Используем пробел вместо EMPTY
    	display->dot_buffer[i] = 0;
    }

    // Обновляем ядро
    update_core_buffer(display);
    
    return LCD_OK;
}


LcdError lcd_set_brightness(LcdDisplay* display, uint8_t brightness)
{
    if (!display || !display->initialized)
    {
        return LCD_ERROR_NOT_INITIALIZED;
    }
    
    LcdCoreError core_err = lcd_core_set_brightness(&display->core, brightness);
    return (core_err == LCD_CORE_OK) ? LCD_OK : LCD_ERROR_NULL_POINTER;
}

bool lcd_is_animation_active(LcdDisplay* display)
{
    if (!display) return false;
    return display->animation.active;
}

LcdError lcd_stop_animation(LcdDisplay* display)
{
    if (!display || !display->initialized)
    {
        return LCD_ERROR_NOT_INITIALIZED;
    }
    
    if (display->animation.active)
    {
        // Восстанавливаем оригинальные символы
        for (uint8_t i = 0; i < display->buffer_size; i++)
        {
            display->char_buffer[i] = display->animation.original_chars[i];
        }

        // Обновляем ядро
        update_core_buffer(display);
        
        // Callback завершения
        if (display->animation.params.on_end)
        {
            display->animation.params.on_end(display->animation.params.user_data);
        }
        
        // Сбрасываем состояние анимации
        display->animation.active = false;
    }
    
    return LCD_OK;
}


