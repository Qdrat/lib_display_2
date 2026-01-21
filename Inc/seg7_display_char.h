#ifndef SEG7_DISPLAY_CHAR_H
#define SEG7_DISPLAY_CHAR_H

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
    SEG_CHAR_SPACE,      // Пробел
    SEG_CHAR_MINUS,      // '-'
    SEG_CHAR_EQUAL,      // '='
    SEG_CHAR_UNDERSCORE, // '_'
    SEG_CHAR_DEGREE,     // '°'
    SEG_CHAR_PERCENT,    // '%'
    SEG_CHAR_COLON,      // ':'
    SEG_CHAR_SEMICOLON,  // ';'
    SEG_CHAR_QUOTE,      // '"'
    SEG_CHAR_APOSTROPHE, // '''
    SEG_CHAR_LBRACKET,   // '['
    SEG_CHAR_RBRACKET,   // ']'
    SEG_CHAR_LPAREN,     // '('
    SEG_CHAR_RPAREN,     // ')'
    SEG_CHAR_LBRACE,     // '{'
    SEG_CHAR_RBRACE,     // '}'
    SEG_CHAR_LT,         // '<'
    SEG_CHAR_GT,         // '>'
    SEG_CHAR_QUESTION,   // '?'
    SEG_CHAR_EXCLAM,     // '!'
    SEG_CHAR_AT,         // '@'
    SEG_CHAR_HASH,       // '#'
    SEG_CHAR_DOLLAR,     // '$'
    SEG_CHAR_AMP,        // '&'
    SEG_CHAR_STAR,       // '*'
    SEG_CHAR_PLUS,       // '+'
    SEG_CHAR_COMMA,      // ','
    SEG_CHAR_DOT,        // '.' (только точка, без сегментов)
    SEG_CHAR_SLASH,      // '/'
    SEG_CHAR_BACKSLASH,  // '\'
    SEG_CHAR_PIPE,       // '|'
    SEG_CHAR_TILDE,      // '~'
    SEG_CHAR_CARET,      // '^'
    SEG_CHAR_GRAVE,      // '`'
    SEG_CHAR_EMPTY = 0x40,      // Пусто (все сегменты выключены)
    
	SEG_CHAR_LOX,

    SEG_CHAR_COUNT       // Общее количество символов
} SegmentChar;

// ============== Структура таблицы кодов ==============
typedef struct
{
    const char* name;            // Название таблицы
    const uint8_t* codes;        // Массив кодов
    uint16_t size;               // Размер таблицы
    uint8_t default_char;        // Код символа по умолчанию
    uint8_t dot_bit_mask;        // Битовая маска для точки
} SegmentTable;

// ============== Управление таблицами ==============

/**
 * @brief Получение стандартной таблицы (общий катод)
 */
const SegmentTable* segchar_get_default_table(void);


/**
 * @brief Получение кода сегмента для символа
 * 
 * @param character Символ
 * @param table Таблица кодов (NULL для стандартной)
 * @param invert_output Инвертировать вывод (для общего анода)
 * @return uint8_t Код сегмента (уже с инверсией, если нужно)
 */
uint8_t segchar_get_code(uint8_t character, 
                        const SegmentTable* table, 
                        bool invert_output);


/**
 * @brief Конвертация ASCII символа в SegmentChar
 */
uint8_t segchar_from_ascii(char ascii_char);

/**
 * @brief Конвертация строки в массив SegmentChar
 */
bool segchar_convert_string(const char* str, 
                           uint8_t* buffer, 
                           uint16_t buffer_size, 
                           uint16_t* converted_length);

#endif // SEG7_DISPLAY_CHAR_H
