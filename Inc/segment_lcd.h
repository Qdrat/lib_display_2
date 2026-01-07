#ifndef SEGMENT_LCD_H
#define SEGMENT_LCD_H

#include "segment_lcd_core.h"
#include "segment_lcd_char.h"

// ============== Типы ошибок ==============
typedef enum
{
    LCD_OK = 0,
    LCD_ERROR_NULL,
    LCD_ERROR_NOT_INIT,
    LCD_ERROR_INVALID_CHAR,
    LCD_ERROR_BUFFER_OVERFLOW,
    LCD_ERROR_INVALID_PARAM
} LcdError;

// ============== Типы данных ==============
typedef enum
{
    DOT_OFF = 0,
    DOT_ON = 1
} DotState;

typedef enum
{
    SCROLL_LEFT = 0,
    SCROLL_RIGHT = 1
} ScrollDirection;

// ============== Структура дисплея ==============
typedef struct
{
    LcdCore core;     // Ядро дисплея
    bool initialized; // Флаг инициализации
} LcdDisplay;

// ============== Структура бегущей строки ==============
typedef struct
{
    LcdDisplay *display;         // Ссылка на дисплей
    const char *text;            // Текст
    SegmentChar *converted_text; // Конвертированный текст
    uint16_t text_length;        // Длина текста
    uint16_t position;           // Текущая позиция
    ScrollDirection direction;   // Направление
    uint16_t delay_ms;           // Задержка между сдвигами
    uint32_t last_update;        // Время последнего обновления
    bool enabled;                // Включена
    bool loop;                   // Зациклить
} LcdScrollingText;

// ============== Основные функции ==============
LcdError lcd_init(LcdDisplay *display,
                  uint8_t digits_count,
                  LcdType type,
                  uint16_t frame_time_ms,
                  LcdSegmentCallback seg_cb,
                  LcdDigitCallback dig_cb,
                  LcdBrightnessCallback bright_cb,
                  LcdTimeCallback time_cb);

LcdError lcd_deinit(LcdDisplay *display);
LcdError lcd_update(LcdDisplay *display);

// ============== Функции отображения ==============
LcdError lcd_show_int(LcdDisplay *display, int32_t number);
LcdError lcd_show_float(LcdDisplay *display, float number, uint8_t decimal_places);
LcdError lcd_show_string(LcdDisplay *display, const char *str);
LcdError lcd_set_char(LcdDisplay *display, uint8_t position, SegmentChar character);
LcdError lcd_set_dot(LcdDisplay *display, uint8_t position, DotState state);
LcdError lcd_clear(LcdDisplay *display);
LcdError lcd_set_brightness(LcdDisplay *display, uint8_t brightness);

// ============== Функции бегущей строки ==============
LcdError lcd_scroll_init(LcdScrollingText *scroll,
                         LcdDisplay *display,
                         const char *text,
                         ScrollDirection direction,
                         uint16_t delay_ms,
                         bool loop);

LcdError lcd_scroll_update(LcdScrollingText *scroll);
LcdError lcd_scroll_start(LcdScrollingText *scroll);
LcdError lcd_scroll_stop(LcdScrollingText *scroll);

#endif // SEGMENT_LCD_H