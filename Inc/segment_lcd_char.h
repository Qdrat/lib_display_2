#ifndef SEGMENT_LCD_CHAR_H
#define SEGMENT_LCD_CHAR_H

#include <stdint.h>
#include <stdbool.h>

// ============== Перечисление символов ==============
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
    SEG_CHAR_SPACE,
    SEG_CHAR_MINUS,
    SEG_CHAR_EQUAL,
    SEG_CHAR_UNDERSCORE,
    SEG_CHAR_DEGREE,
    SEG_CHAR_PERCENT,
    SEG_CHAR_COLON,
    SEG_CHAR_EXCLAM,
    SEG_CHAR_QUESTION,
    SEG_CHAR_QUOTE,
    SEG_CHAR_APOSTROPHE,
    SEG_CHAR_LPAREN,
    SEG_CHAR_RPAREN,
    SEG_CHAR_LBRACKET,
    SEG_CHAR_RBRACKET,
    SEG_CHAR_PLUS,
    SEG_CHAR_STAR,
    SEG_CHAR_SLASH,
    SEG_CHAR_BACKSLASH,
    SEG_CHAR_PIPE,
    SEG_CHAR_TILDE,
    SEG_CHAR_CARET,
    SEG_CHAR_EMPTY,

    // Специальные символы
    SEG_CHAR_ARROW_UP,
    SEG_CHAR_ARROW_DOWN,
    SEG_CHAR_ARROW_LEFT,
    SEG_CHAR_ARROW_RIGHT,
    SEG_CHAR_HEART,
    SEG_CHAR_SMILE,
    SEG_CHAR_SAD,
    SEG_CHAR_BELL,

    SEG_CHAR_COUNT
} SegmentChar;

// ============== Управление таблицами ==============

/**
 * @brief Получение кода сегмента для символа
 * @note Для общего анода код инвертируется автоматически
 */
uint8_t segchar_get_code(SegmentChar character, bool is_common_cathode);

/**
 * @brief Конвертация ASCII символа в SegmentChar
 */
SegmentChar segchar_from_ascii(char ascii_char);

/**
 * @brief Конвертация строки с поддержкой точек
 */
bool segchar_convert_string(const char *str,
                            SegmentChar *char_buffer,
                            uint8_t *dot_buffer,
                            uint16_t buffer_size,
                            uint16_t *converted_length);

#endif // SEGMENT_LCD_CHAR_H