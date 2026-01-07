#ifndef SEGMENT_LCD_H
#define SEGMENT_LCD_H

#include "segment_lcd_core.h"
#include "segment_lcd_char.h"

// ============== Типы ошибок (высокий уровень) ==============
typedef enum
{
    LCD_OK = 0,
    LCD_ERROR_NULL_POINTER,
    LCD_ERROR_NOT_INITIALIZED,
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
    SCROLL_RIGHT = 1,
    SCROLL_UP = 2,
    SCROLL_DOWN = 3
} ScrollDirection;

typedef enum
{
    ANIMATION_NONE = 0,
    ANIMATION_BLINK,
    ANIMATION_FADE_IN,
    ANIMATION_FADE_OUT,
    ANIMATION_SLIDE_LEFT,
    ANIMATION_SLIDE_RIGHT
} AnimationType;

// ============== Структура дисплея ==============
typedef struct
{
    LcdCoreConfig core;           // Ядро
    const SegmentTable* table;    // Таблица символов
    uint8_t* char_buffer;         // Буфер символов
    uint8_t* dot_buffer;          // Буфер точек
    uint8_t* temp_buffer;         // Временный буфер для анимаций
    uint16_t buffer_size;         // Размер буферов
    bool initialized;             // Флаг инициализации
} LcdDisplay;

// ============== Структура бегущей строки ==============
typedef struct
{
    LcdDisplay* display;          // Ссылка на дисплей
    const char* text;             // Текст
    SegmentChar* converted_text;  // Конвертированный текст
    uint16_t text_length;         // Длина текста
    uint16_t position;            // Текущая позиция
    ScrollDirection direction;    // Направление
    uint16_t delay_ms;            // Задержка между сдвигами
    uint32_t last_update;         // Время последнего обновления
    bool enabled;                 // Включена
    bool loop;                    // Зациклить
    bool auto_convert;            // Автоконвертация текста
} LcdScrollingText;

// ============== Структура анимации ==============
typedef struct
{
    LcdDisplay* display;          // Ссылка на дисплей
    AnimationType type;           // Тип анимации
    uint16_t duration_ms;         // Длительность
    uint16_t frame_delay_ms;      // Задержка между кадрами
    uint32_t start_time;          // Время начала
    bool active;                  // Активна
    void* user_data;              // Пользовательские данные
} LcdAnimation;

// ============== Основные функции ==============

/**
 * @brief Инициализация дисплея
 */
LcdError lcd_init(LcdDisplay* display,
                  uint8_t digits_count,
                  LcdType type,
                  uint16_t refresh_rate_hz,
                  LcdSegmentCallback seg_cb,
                  LcdDigitCallback dig_cb,
                  LcdBrightnessCallback bright_cb,
                  LcdTimeCallback time_cb,
                  const SegmentTable* custom_table);

/**
 * @brief Освобождение ресурсов дисплея
 */
LcdError lcd_deinit(LcdDisplay* display);

/**
 * @brief Обновление дисплея (должен вызываться периодически)
 */
LcdError lcd_update(LcdDisplay* display);

// ============== Функции отображения ==============

/**
 * @brief Отображение целого числа
 */
LcdError lcd_show_int(LcdDisplay* display, int32_t number, 
                      uint8_t decimal_position, bool leading_zeros);

/**
 * @brief Отображение дробного числа
 */
LcdError lcd_show_float(LcdDisplay* display, float number, 
                        uint8_t decimal_places);

/**
 * @brief Отображение шестнадцатеричного числа
 */
LcdError lcd_show_hex(LcdDisplay* display, uint32_t number, 
                      bool uppercase);

/**
 * @brief Отображение строки символов
 */
LcdError lcd_show_string(LcdDisplay* display, const SegmentChar* chars, 
                         uint8_t length);

/**
 * @brief Отображение ASCII строки
 */
LcdError lcd_show_ascii(LcdDisplay* display, const char* str);

/**
 * @brief Установка символа в позицию
 */
LcdError lcd_set_char(LcdDisplay* display, uint8_t position, 
                      SegmentChar character);

/**
 * @brief Установка точки в позиции
 */
LcdError lcd_set_dot(LcdDisplay* display, uint8_t position, DotState state);

/**
 * @brief Очистка дисплея
 */
LcdError lcd_clear(LcdDisplay* display);

/**
 * @brief Установка яркости
 */
LcdError lcd_set_brightness(LcdDisplay* display, uint8_t brightness);

// ============== Функции бегущей строки ==============

/**
 * @brief Инициализация бегущей строки
 */
LcdError lcd_scroll_init(LcdScrollingText* scroll, LcdDisplay* display,
                         const char* text, ScrollDirection direction,
                         uint16_t delay_ms, bool loop);

/**
 * @brief Обновление бегущей строки
 */
LcdError lcd_scroll_update(LcdScrollingText* scroll);

/**
 * @brief Запуск бегущей строки
 */
LcdError lcd_scroll_start(LcdScrollingText* scroll);

/**
 * @brief Остановка бегущей строки
 */
LcdError lcd_scroll_stop(LcdScrollingText* scroll);

/**
 * @brief Сброс бегущей строки
 */
LcdError lcd_scroll_reset(LcdScrollingText* scroll);

// ============== Функции анимации ==============

/**
 * @brief Запуск анимации
 */
LcdError lcd_animate(LcdDisplay* display, AnimationType type, 
                     uint16_t duration_ms);

/**
 * @brief Обновление анимации
 */
LcdError lcd_animation_update(LcdDisplay* display);

// ============== Вспомогательные функции ==============

/**
 * @brief Создание пользовательского символа
 */
LcdError lcd_create_custom_char(LcdDisplay* display, uint8_t index, 
                                uint8_t segment_mask);

/**
 * @brief Сохранение текущего состояния дисплея
 */
LcdError lcd_save_state(LcdDisplay* display, uint8_t* buffer, 
                        uint16_t buffer_size);

/**
 * @brief Восстановление состояния дисплея
 */
LcdError lcd_restore_state(LcdDisplay* display, const uint8_t* buffer, 
                           uint16_t buffer_size);

#endif // SEGMENT_LCD_H