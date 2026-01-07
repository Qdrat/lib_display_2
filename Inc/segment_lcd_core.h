#ifndef SEGMENT_LCD_CORE_H
#define SEGMENT_LCD_CORE_H

#include <stdint.h>
#include <stdbool.h>

// ============== Типы ошибок ==============
typedef enum
{
    LCD_CORE_OK = 0,
    LCD_CORE_ERROR_NULL,
    LCD_CORE_ERROR_NOT_INIT,
    LCD_CORE_ERROR_INVALID_DIGIT,
    LCD_CORE_ERROR_INVALID_CONFIG
} LcdCoreError;

// ============== Типы подключения ==============
typedef enum
{
    LCD_TYPE_COMMON_CATHODE = 0,
    LCD_TYPE_COMMON_ANODE = 1
} LcdType;

// ============== Callback-типы ==============
typedef void (*LcdSegmentCallback)(uint8_t segment_mask);
typedef void (*LcdDigitCallback)(uint8_t digit_index, bool state);
typedef void (*LcdBrightnessCallback)(uint8_t brightness);
typedef uint32_t (*LcdTimeCallback)(void);

// ============== Структура ядра ==============
typedef struct
{
    // Callback-функции (обязательные)
    LcdSegmentCallback set_segments;
    LcdDigitCallback set_digit;

    // Callback-функции (опциональные)
    LcdBrightnessCallback set_brightness;
    LcdTimeCallback get_time_ms;

    // Конфигурация
    uint8_t digits_count;   // Количество разрядов
    LcdType type;           // Тип подключения
    uint16_t frame_time_ms; // Время полного обновления дисплея (мс)
    uint8_t brightness;     // Яркость 0-100%

    // Внутреннее состояние
    struct
    {
        uint8_t *segment_mask;  // Маски сегментов (7 бит)
        uint8_t *dot_flags;     // Флаги точек
        uint16_t digit_time_ms; // Время горения одного разряда
        uint32_t last_update;   // Время последнего обновления
        uint8_t current_digit;  // Текущий активный разряд
        bool initialized;       // Флаг инициализации
    } state;
} LcdCore;

// ============== API ядра ==============
LcdCoreError lcd_core_init(LcdCore *core,
                           uint8_t digits_count,
                           LcdType type,
                           uint16_t frame_time_ms,
                           LcdSegmentCallback seg_cb,
                           LcdDigitCallback dig_cb);

LcdCoreError lcd_core_update(LcdCore *core);
LcdCoreError lcd_core_set_digit(LcdCore *core,
                                uint8_t digit,
                                uint8_t segment_mask,
                                bool dot);
LcdCoreError lcd_core_set_brightness(LcdCore *core, uint8_t brightness);
LcdCoreError lcd_core_clear(LcdCore *core);
uint32_t lcd_core_get_time(const LcdCore *core);

#endif // SEGMENT_LCD_CORE_H