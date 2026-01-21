#ifndef SEG7_DISPLAY_H
#define SEG7_DISPLAY_H

#include "seg7_display_core.h"
#include "seg7_display_char.h"

// ============== Типы ошибок (высокий уровень) ==============
typedef enum
{
    LCD_OK = 0,
    LCD_ERROR_NULL_POINTER,
    LCD_ERROR_NOT_INITIALIZED,
    LCD_ERROR_INVALID_CHAR,
    LCD_ERROR_BUFFER_OVERFLOW,
    LCD_ERROR_INVALID_PARAM,
	LCD_ERROR_MEMORY,
	LCD_ERROR_ANIMATION_IN_PROGRESS
} LcdError;

// ============== Типы данных ==============
typedef enum
{
    DOT_OFF = 0,
    DOT_ON = 1
} DotState;


typedef enum
{
    ANIMATION_NONE = 0,
    ANIMATION_SCROLL_LEFT = 1,
    ANIMATION_SROLL_RIGHT = 2,
    ANIMATION_BLINK = 3
} AnimationType;

// ============== Параметры анимации ==============
typedef struct
{
    AnimationType type;          // Тип анимации
    uint16_t delay_ms;           // Задержка между кадрами (для скролла)
    uint16_t blink_interval_ms;  // Интервал для мигания
    bool loop;                   // Зациклить анимацию
    void (*on_start)(void*);     // Callback при старте
    void (*on_frame)(void*, uint16_t);  // Callback на каждом кадре
    void (*on_end)(void*);       // Callback при завершении
    void* user_data;             // Пользовательские данные для callback
} AnimationParams;

// ============== Структура дисплея ==============
typedef struct
{
    LcdCoreConfig core;                 // Ядро дисплея
    const SegmentTable* table;          // Таблица символов
    uint8_t char_buffer[16];            // Буфер символов (статический)
    uint8_t dot_buffer[16];             // Буфер точек (статический)
    uint16_t buffer_size;               // Размер буферов
    bool initialized;                   // Флаг инициализации
    
    // Состояние анимации
    struct {
        AnimationType type;             // Текущий тип анимации
        bool active;                    // Активна ли анимация
        bool loop;                      // Зациклена ли анимация
        uint32_t start_time;            // Время начала анимации
        uint32_t last_update;           // Время последнего обновления
        uint16_t delay_ms;              // Задержка между кадрами
        uint16_t position;              // Позиция для скролла
        uint8_t animation_buffer[64];   // Буфер для анимации
        uint8_t animation_dots[64];     // Буфер для точек анимации (новое поле!)
        uint16_t anim_buffer_len;       // Длина буфера анимации
        uint8_t original_chars[16];     // Оригинальные символы
        uint8_t original_dots[16]; // Добавляем для хранения точек
        AnimationParams params;         // Параметры анимации
    } animation;    
} LcdDisplay;


// ============== Основные функции ==============

/**
 * @brief Инициализация дисплея (расширенная версия с контекстом)
 */
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
                    LcdControlMode control_mode);

/**
 * @brief Инициализация дисплея
 */
LcdError lcd_init(LcdDisplay* display,
                  uint8_t digits_count,
                  uint16_t refresh_rate_hz,
                  bool invert_output,
                  LcdSegmentCallback seg_cb,
                  LcdDigitCallback dig_cb,
                  LcdBrightnessCallback bright_cb,
                  LcdTimeCallback time_cb,
                  const SegmentTable* custom_table,
				  void* ctx);

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
 * @brief Отображение числа
 */
LcdError lcd_show_num(LcdDisplay* display, int32_t number, 
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
 * @param animation_params Параметры анимации (NULL для статического отображения)
 */
LcdError lcd_show_string(LcdDisplay* display, const uint8_t* chars, 
                         uint8_t length, const AnimationParams* animation_params);

/**
 * @brief Отображение ASCII строки
 * @param animation_params Параметры анимации (NULL для статического отображения)
 */
LcdError lcd_show_ascii(LcdDisplay* display, const char* str, 
                        const AnimationParams* animation_params);

/**
 * @brief Установка символа в позицию
 */
LcdError lcd_set_char(LcdDisplay* display, uint8_t position, 
                      uint8_t character);

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

// ============== Функции анимации ==============

/**
 * @brief Проверка активности анимации
 */
bool lcd_is_animation_active(LcdDisplay* display);

/**
 * @brief Остановка текущей анимации
 */
LcdError lcd_stop_animation(LcdDisplay* display);

#endif // SEG7_DISPLAY_H
