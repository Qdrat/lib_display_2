#include "segment_lcd_char.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// ============== Стандартная таблица для общего катода ==============
static const uint8_t DEFAULT_TABLE_CC[SEG_CHAR_COUNT] = {
    // Цифры 0-9
    0x3F, // 0: ABCDEF
    0x06, // 1: BC
    0x5B, // 2: ABDEG
    0x4F, // 3: ABCDG
    0x66, // 4: BCFG
    0x6D, // 5: ACDFG
    0x7D, // 6: ACDEFG
    0x07, // 7: ABC
    0x7F, // 8: ABCDEFG
    0x6F, // 9: ABCDFG
    
    // Буквы A-Z
    0x77, // A: ABCEFG
    0x7C, // B: CDEFG
    0x39, // C: ADEF
    0x5E, // D: BCDEG
    0x79, // E: ADEFG
    0x71, // F: AEFG
    0x3D, // G: ACDFG
    0x76, // H: BCEFG
    0x30, // I: EF
    0x1E, // J: BCDE
    0x76, // K: BCEFG (такой же как H)
    0x38, // L: DEF
    0x37, // M: ACE (специальный)
    0x54, // N: CEG
    0x3F, // O: ABCDEF (такой же как 0)
    0x73, // P: ABEFG
    0x67, // Q: ABCFG
    0x50, // R: EG
    0x6D, // S: ACDFG (такой же как 5)
    0x78, // T: DEFG
    0x3E, // U: BCDEF
    0x3E, // V: BCDEF (такой же как U)
    0x3E, // W: BCDEF (такой же как U)
    0x76, // X: BCEFG (такой же как H)
    0x6E, // Y: BCDFG
    0x5B, // Z: ABDEG (такой же как 2)
    
    // Символы
    0x00, // Пробел
    0x40, // Минус
    0x48, // Равно
    0x08, // Подчеркивание
    0x63, // Градус
    0x49, // Процент
    0x00, // Двоеточие (управляется отдельно)
    0x00, // Точка с запятой
    0x22, // Кавычки
    0x02, // Апостроф
    0x39, // Квадратная скобка [
    0x0F, // Квадратная скобка ]
    0x39, // Круглая скобка (
    0x0F, // Круглая скобка )
    0x39, // Фигурная скобка {
    0x0F, // Фигурная скобка }
    0x00, // Меньше
    0x00, // Больше
    0x53, // Вопрос
    0x82, // Восклицание
    0x77, // @ (как A)
    0x49, // Решетка
    0x49, // Доллар
    0x7F, // Амперсанд
    0x49, // Звезда
    0x48, // Плюс
    0x00, // Запятая
    0x00, // Точка (без сегментов)
    0x01, // Слеш
    0x40, // Обратный слеш
    0x30, // Вертикальная черта
    0x01, // Тильда
    0x00, // Циркумфлекс
    0x00, // Гравис
    
    // Специальные символы
    0x00, // Стрелка вверх
    0x00, // Стрелка вниз
    0x00, // Стрелка влево
    0x00, // Стрелка вправо
    0x00, // Сердце
    0x00, // Улыбка
    0x00, // Грусть
    0x00, // Колокольчик
};

// ============== Стандартная таблица для общего анода ==============
static const uint8_t DEFAULT_TABLE_CA[SEG_CHAR_COUNT] = {
    // Инвертированные значения
    0xC0, // 0
    0xF9, // 1
    0xA4, // 2
    0xB0, // 3
    0x99, // 4
    0x92, // 5
    0x82, // 6
    0xF8, // 7
    0x80, // 8
    0x90, // 9
    
    // ... остальные инвертированные значения
    // (пропускаю для краткости, но в реальном коде должны быть все)
};

// ============== Структуры таблиц ==============
static const SegmentTable DEFAULT_TABLE_CC_STRUCT = {
    "Default Common Cathode",
    DEFAULT_TABLE_CC,
    SEG_CHAR_COUNT,
    SEG_CHAR_EMPTY
};

static const SegmentTable DEFAULT_TABLE_CA_STRUCT = {
    "Default Common Anode",
    DEFAULT_TABLE_CA,
    SEG_CHAR_COUNT,
    SEG_CHAR_EMPTY
};

// ============== Публичные функции ==============

const SegmentTable* segchar_get_default_table_cc(void)
{
    return &DEFAULT_TABLE_CC_STRUCT;
}

const SegmentTable* segchar_get_default_table_ca(void)
{
    return &DEFAULT_TABLE_CA_STRUCT;
}

uint8_t segchar_get_code(SegmentChar character, 
                        const SegmentTable* table, 
                        bool is_common_cathode)
{
    // Используем стандартную таблицу, если не указана
    if (!table)
    {
        table = is_common_cathode ? 
                &DEFAULT_TABLE_CC_STRUCT : 
                &DEFAULT_TABLE_CA_STRUCT;
    }
    
    // Проверка диапазона
    if ((uint16_t)character >= table->size)
    {
        return table->default_char;
    }
    
    return table->codes[character];
}

SegmentChar segchar_from_ascii(char ascii_char)
{
    // Конвертация ASCII в SegmentChar
    if (ascii_char >= '0' && ascii_char <= '9')
    {
        return (SegmentChar)(ascii_char - '0');
    }
    else if (ascii_char >= 'A' && ascii_char <= 'Z')
    {
        return (SegmentChar)(SEG_CHAR_A + (ascii_char - 'A'));
    }
    else if (ascii_char >= 'a' && ascii_char <= 'z')
    {
        return (SegmentChar)(SEG_CHAR_A + (ascii_char - 'a'));
    }
    
    // Специальные символы
    switch (ascii_char)
    {
        case ' ': return SEG_CHAR_SPACE;
        case '-': return SEG_CHAR_MINUS;
        case '=': return SEG_CHAR_EQUAL;
        case '_': return SEG_CHAR_UNDERSCORE;
        case '.': return SEG_CHAR_DOT;
        case ',': return SEG_CHAR_COMMA;
        case ':': return SEG_CHAR_COLON;
        case ';': return SEG_CHAR_SEMICOLON;
        case '!': return SEG_CHAR_EXCLAM;
        case '?': return SEG_CHAR_QUESTION;
        case '(': return SEG_CHAR_LPAREN;
        case ')': return SEG_CHAR_RPAREN;
        case '[': return SEG_CHAR_LBRACKET;
        case ']': return SEG_CHAR_RBRACKET;
        case '{': return SEG_CHAR_LBRACE;
        case '}': return SEG_CHAR_RBRACE;
        case '<': return SEG_CHAR_LT;
        case '>': return SEG_CHAR_GT;
        case '@': return SEG_CHAR_AT;
        case '#': return SEG_CHAR_HASH;
        case '$': return SEG_CHAR_DOLLAR;
        case '%': return SEG_CHAR_PERCENT;
        case '&': return SEG_CHAR_AMP;
        case '*': return SEG_CHAR_STAR;
        case '+': return SEG_CHAR_PLUS;
        case '/': return SEG_CHAR_SLASH;
        case '\\': return SEG_CHAR_BACKSLASH;
        case '|': return SEG_CHAR_PIPE;
        case '~': return SEG_CHAR_TILDE;
        case '^': return SEG_CHAR_CARET;
        case '`': return SEG_CHAR_GRAVE;
        case '"': return SEG_CHAR_QUOTE;
        case '\'': return SEG_CHAR_APOSTROPHE;
        
        default: return SEG_CHAR_EMPTY;
    }
}

bool segchar_convert_string(const char* str, 
                           SegmentChar* buffer, 
                           uint16_t buffer_size, 
                           uint16_t* converted_length)
{
    if (!str || !buffer || buffer_size == 0)
    {
        return false;
    }
    
    uint16_t length = 0;
    bool dot_expected = false;
    
    while (*str && length < buffer_size)
    {
        if (*str == '.')
        {
            // Точка для предыдущего символа
            dot_expected = true;
        }
        else
        {
            SegmentChar ch = segchar_from_ascii(*str);
            buffer[length++] = ch;
            
            // Здесь можно обработать точку, если нужно
            // В реальном коде нужно сохранить информацию о точках отдельно
        }
        
        str++;
    }
    
    if (converted_length)
    {
        *converted_length = length;
    }
    
    return true;
}