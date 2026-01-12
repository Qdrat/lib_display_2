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
	0x00, // Пробел (SEG_CHAR_SPACE) - ВЫКЛ все сегменты
	0x40, // Минус (SEG_CHAR_MINUS)
	0x48, // Равно (SEG_CHAR_EQUAL)
	0x08, // Подчеркивание (SEG_CHAR_UNDERSCORE)
	0x63, // Градус (SEG_CHAR_DEGREE)
	0x49, // Процент (SEG_CHAR_PERCENT)
	0x00, // Двоеточие (SEG_CHAR_COLON) - управляется отдельно
	0x00, // Точка с запятой (SEG_CHAR_SEMICOLON)
	0x22, // Кавычки (SEG_CHAR_QUOTE)
	0x02, // Апостроф (SEG_CHAR_APOSTROPHE)
	0x39, // Квадратная скобка [ (SEG_CHAR_LBRACKET)
	0x0F, // Квадратная скобка ] (SEG_CHAR_RBRACKET)
	0x39, // Круглая скобка ( (SEG_CHAR_LPAREN)
	0x0F, // Круглая скобка ) (SEG_CHAR_RPAREN)
	0x39, // Фигурная скобка { (SEG_CHAR_LBRACE)
	0x0F, // Фигурная скобка } (SEG_CHAR_RBRACE)
	0x00, // Меньше (SEG_CHAR_LT)
	0x00, // Больше (SEG_CHAR_GT)
	0x53, // Вопрос (SEG_CHAR_QUESTION)
	0x82, // Восклицание (SEG_CHAR_EXCLAM)
	0x77, // @ (SEG_CHAR_AT) (как A)
	0x49, // Решетка (SEG_CHAR_HASH)
	0x49, // Доллар (SEG_CHAR_DOLLAR)
	0x7F, // Амперсанд (SEG_CHAR_AMP)
	0x49, // Звезда (SEG_CHAR_STAR)
	0x48, // Плюс (SEG_CHAR_PLUS)
	0x00, // Запятая (SEG_CHAR_COMMA)
	0x00, // Точка (SEG_CHAR_DOT) (без сегментов, только для точки в числе)
	0x01, // Слеш (SEG_CHAR_SLASH)
	0x40, // Обратный слеш (SEG_CHAR_BACKSLASH)
	0x30, // Вертикальная черта (SEG_CHAR_PIPE)
	0x01, // Тильда (SEG_CHAR_TILDE)
	0x00, // Циркумфлекс (SEG_CHAR_CARET)
	0x00, // Гравис (SEG_CHAR_GRAVE)

	// SEG_CHAR_EMPTY (индекс 64) - ПУСТОЙ символ
	0x00, // ВСЕ сегменты ВЫКЛЮЧЕНЫ

};

// ============== Стандартная таблица для общего анода ==============
static const uint8_t DEFAULT_TABLE_CA[SEG_CHAR_COUNT] = {
	// Инвертированные значения от общей катодной таблицы
	0xC0, // 0: ~0x3F
	0xF9, // 1: ~0x06
	0xA4, // 2: ~0x5B
	0xB0, // 3: ~0x4F
	0x99, // 4: ~0x66
	0x92, // 5: ~0x6D
	0x82, // 6: ~0x7D
	0xF8, // 7: ~0x07
	0x80, // 8: ~0x7F
	0x90, // 9: ~0x6F

	// Буквы A-Z (инвертированные)
	0x88, // A: ~0x77
	0x83, // B: ~0x7C
	0xC6, // C: ~0x39
	0xA1, // D: ~0x5E
	0x86, // E: ~0x79
	0x8E, // F: ~0x71
	0xC2, // G: ~0x3D
	0x89, // H: ~0x76
	0xCF, // I: ~0x30
	0xE1, // J: ~0x1E
	0x89, // K: ~0x76
	0xC7, // L: ~0x38
	0xC8, // M: ~0x37
	0xAB, // N: ~0x54
	0xC0, // O: ~0x3F
	0x8C, // P: ~0x73
	0x98, // Q: ~0x67
	0xAF, // R: ~0x50
	0x92, // S: ~0x6D
	0x87, // T: ~0x78
	0xC1, // U: ~0x3E
	0xC1, // V: ~0x3E
	0xC1, // W: ~0x3E
	0x89, // X: ~0x76
	0x91, // Y: ~0x6E
	0xA4, // Z: ~0x5B

	// Символы (инвертированные)
	0xFF, // Пробел: все сегменты ВЫКЛ (инвертированный 0x00)
	0xBF, // Минус: ~0x40
	0xB7, // Равно: ~0x48
	0xF7, // Подчеркивание: ~0x08
	0x9C, // Градус: ~0x63
	0xB6, // Процент: ~0x49
	0xFF, // Двоеточие: ~0x00
	0xFF, // Точка с запятой: ~0x00
	0xDD, // Кавычки: ~0x22
	0xFD, // Апостроф: ~0x02
	0xC6, // [: ~0x39
	0xF0, // ]: ~0x0F
	0xC6, // (: ~0x39
	0xF0, // ): ~0x0F
	0xC6, // {: ~0x39
	0xF0, // }: ~0x0F
	0xFF, // <: ~0x00
	0xFF, // >: ~0x00
	0xAC, // ?: ~0x53
	0x7D, // !: ~0x82
	0x88, // @: ~0x77
	0xB6, // #: ~0x49
	0xB6, // $: ~0x49
	0x80, // &: ~0x7F
	0xB6, // *: ~0x49
	0xB7, // +: ~0x48
	0xFF, // ,: ~0x00
	0xFF, // .: ~0x00
	0xFE, // /: ~0x01
	0xBF, // \: ~0x40
	0xCF, // |: ~0x30
	0xFE, // ~: ~0x01
	0xFF, // ^: ~0x00
	0xFF, // `: ~0x00

	// SEG_CHAR_EMPTY (индекс 64) - ПУСТОЙ символ
	0xFF, // ВСЕ сегменты ВЫКЛЮЧЕНЫ (инвертированный 0x00)
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
    	return table->codes[table->default_char];
    }
    
    return table->codes[character];
}


//SegmentChar segchar_from_ascii(char c)
//{
//    /* цифры */
//    if (c >= '0' && c <= '9')
//        return (SegmentChar)(c - '0');
//
//    /* поддерживаемые символы */
//    switch (c)
//    {
//        case '-': return SEG_CHAR_MINUS;
//        case ' ': return SEG_CHAR_EMPTY;
//        default:  return SEG_CHAR_EMPTY;  // ← КЛЮЧЕВО
//    }
//}
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
