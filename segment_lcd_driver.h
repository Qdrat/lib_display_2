#ifndef SEGMENT_LCD_DRIVER_H
#define SEGMENT_LCD_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// ============== Безопасные типы ==============
typedef enum
{
    DISPLAY_OK = 0,
    DISPLAY_ERROR_NULL_POINTER,
    DISPLAY_ERROR_INVALID_DIGIT,
    DISPLAY_ERROR_INVALID_CHAR,
    DISPLAY_ERROR_MEMORY,
    DISPLAY_ERROR_NOT_INITIALIZED,
    DISPLAY_ERROR_INVALID_CONFIG,
    DISPLAY_ERROR_BUFFER_OVERFLOW
} DisplayError;

typedef enum
{
    DISPLAY_TYPE_COMMON_CATHODE,
    DISPLAY_TYPE_COMMON_ANODE
} DisplayType;

// Тип для символов дисплея
typedef enum
{
    SEG_CHAR_0 = 0,
    SEG_CHAR_1,
    SEG_CHAR_2,
    SEG_CHAR_3,
    SEG_CHAR_4,
    SEG_CHAR_5,
    SEG_CHAR_6,
    SEG_CHAR_7,
    SEG_CHAR_8,
    SEG_CHAR_9,
    SEG_CHAR_A,
    SEG_CHAR_B,
    SEG_CHAR_C,
    SEG_CHAR_D,
    SEG_CHAR_E,
    SEG_CHAR_F,
    SEG_CHAR_G,
    SEG_CHAR_H,
    SEG_CHAR_I,
    SEG_CHAR_J,
    SEG_CHAR_K,
    SEG_CHAR_L,
    SEG_CHAR_M,
    SEG_CHAR_N,
    SEG_CHAR_O,
    SEG_CHAR_P,
    SEG_CHAR_Q,
    SEG_CHAR_R,
    SEG_CHAR_S,
    SEG_CHAR_T,
    SEG_CHAR_U,
    SEG_CHAR_V,
    SEG_CHAR_W,
    SEG_CHAR_X,
    SEG_CHAR_Y,
    SEG_CHAR_Z,
    SEG_CHAR_MINUS,      // '-'
    SEG_CHAR_UNDERSCORE, // '_'
    SEG_CHAR_DEGREE,     // '°'
    SEG_CHAR_EMPTY = 0xFF
} SegmentChar;

// Тип для состояния точки
typedef enum
{
    DOT_OFF = 0,
    DOT_ON = 1
} DotState;

// Тип для направления прокрутки
typedef enum
{
    SCROLL_RIGHT = 0,
    SCROLL_LEFT = 1
} ScrollDirection;

// Тип для яркости
typedef uint8_t BrightnessLevel; // 0-100%

// Callback-функции для управления аппаратурой
typedef void (*SegmentCallback)(uint8_t segment_mask);
typedef void (*DigitCallback)(uint8_t digit_index, bool state);
typedef void (*BrightnessCallback)(BrightnessLevel brightness);

// ============== Структуры данных ==============

// Структура для конфигурации дисплея
typedef struct
{
    SegmentCallback set_segments;      // Функция установки сегментов
    DigitCallback set_digit;           // Функция управления разрядом
    BrightnessCallback set_brightness; // Функция установки яркости (опционально)
    uint8_t digits_count;              // Количество разрядов
    DisplayType type;                  // Тип индикатора
    BrightnessLevel brightness;        // Текущая яркость (0-100%)
} DisplayConfig;

// Структура для управления временем
typedef struct
{
    uint32_t (*get_time_ms)(void); // Функция получения времени
    uint32_t internal_counter;     // Внутренний счетчик миллисекунд
} DisplayTime;

// Структура бегущей строки
typedef struct
{
    const SegmentChar *text;   // Текст для отображения
    uint16_t text_length;      // Длина текста
    int16_t current_position;  // Текущая позиция в тексте
    uint8_t display_length;    // Длина дисплея (количество разрядов)
    ScrollDirection direction; // Направление
    uint16_t scroll_delay;     // Задержка между сдвигами в мс
    uint32_t last_scroll_time; // Время последнего сдвига
    bool enabled;              // Включена ли бегущая строка
    bool loop;                 // Зациклить бесконечно
} ScrollingText;

// ============== Макросы для проверки ==============
#define DISPLAY_CHECK_INIT()                      \
    do                                            \
    {                                             \
        if (!display_initialized)                 \
            return DISPLAY_ERROR_NOT_INITIALIZED; \
    } while (0)

#define DISPLAY_CHECK_PTR(ptr)                 \
    do                                         \
    {                                          \
        if (!(ptr))                            \
            return DISPLAY_ERROR_NULL_POINTER; \
    } while (0)

#define DISPLAY_CHECK_DIGIT(digit)                   \
    do                                               \
    {                                                \
        if ((digit) >= current_config->digits_count) \
            return DISPLAY_ERROR_INVALID_DIGIT;      \
    } while (0)

// ============== Функции для работы со временем ==============
void Display_Tick(void);
DisplayError Display_DelayMs(uint32_t ms);
void Display_SetTimeCallback(uint32_t (*time_callback)(void));
uint32_t Display_GetInternalTime(void);
void Display_ResetInternalTime(void);

// ============== Основные API функции ==============
DisplayError Display_Init(DisplayConfig *config);
DisplayError Display_Update(void);
DisplayError Display_SetNumber(int32_t number);
DisplayError Display_SetFloat(float number, uint8_t decimal_places);
DisplayError Display_SetCharacters(const SegmentChar *characters);
DisplayError Display_SetCharacter(uint8_t digit, SegmentChar character);
DisplayError Display_SetDot(uint8_t digit, DotState state);
DisplayError Display_Clear(void);
DisplayError Display_SetBrightness(BrightnessLevel brightness);

// ============== Функции конфигурации ==============
DisplayError Display_CreateConfig(DisplayConfig *config,
                                  uint8_t digits_count,
                                  DisplayType type,
                                  SegmentCallback seg_cb,
                                  DigitCallback dig_cb,
                                  BrightnessCallback bright_cb);

// ============== Функции бегущей строки ==============
DisplayError Scroll_Init(ScrollingText *scroll,
                         const SegmentChar *text,
                         uint16_t length,
                         uint8_t display_len,
                         ScrollDirection direction,
                         uint16_t delay_ms,
                         bool loop);
DisplayError Scroll_Update(ScrollingText *scroll);
DisplayError Scroll_Start(ScrollingText *scroll);
DisplayError Scroll_Stop(ScrollingText *scroll);
DisplayError Scroll_Reset(ScrollingText *scroll);
bool Scroll_IsFinished(const ScrollingText *scroll);

// ============== Вспомогательные функции ==============
DisplayError String_ToSegmentCodes(const char *str,
                                   SegmentChar *buffer,
                                   uint16_t buffer_size,
                                   uint16_t *converted_length);

#endif // SEGMENT_LCD_DRIVER_H