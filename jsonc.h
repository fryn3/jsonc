#ifndef JSONC_H
#define JSONC_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/// Типы ошибок.
typedef enum {
    JsonSuccess,        // нет ошибок.
    JsonJustInit,       // дефолтное значение.
    JsonErrorUnknow,    // ошибка без описания, ошибка в коде.
    JsonErrorEnd,       // неожиданный конец.
    JsonErrorSyntax,    // ошибка в синтаксисе.
    JsonErrorKey,       // ошибка в ключе (ключ без кавычек и тд).
    JsonErrorValue,     // ошибка в значении.
    JsonErrorFile,      // ошибка связана с работой с файлом.
    JsonErrorPath,      // ошибка в keyPath.

    JsonErrorCount
} JsonErrorEnum;

/// Типы данных
typedef enum {
    JsonTypeNull,
    JsonTypeBool,
    JsonTypeNumber, // тут и int и double может быть.
    JsonTypeString,
    JsonTypeObject,
    JsonTypeArray,

    JsonTypeCount
} JsonTypeEnum;

/*!
 * \brief Структура элемента JSON файла.
 */
typedef struct JsonItemTypeDef {
    struct JsonItemTypeDef *parent;
    const char *key;
    size_t keyLen;
    JsonTypeEnum type;
    double number;
    const char *str;
    size_t strLen;
    struct JsonItemTypeDef *childrenList;
    size_t childrenCount;
    size_t _childrenReserve; /// Количество выделенной памяти.
} JsonItem;

/*!
 * \brief Структура для работы с файлом JSON.
 */
typedef struct {
    /// Входной текст.
    const char *jsonTextFull;
    /// Указатель на родительский элемент.
    JsonItem *parentItem;
    /// После ф-ций проверять на ошибку.
    JsonErrorEnum error;
} JsonCStruct;

/*!
 * \brief Парсит JSON строку.
 *
 * После вызова функции, можно проверить поле error на наличие ошибки.
 * \param jsonTextFull - строка JSON файла.
 * \return структуру JsonCStruct.
 */
JsonCStruct openJsonFromStr(const char *jsonTextFull);

/*!
 * \brief Парсит JSON файл.
 *
 * Не забыть, для освобождения этой структуры вызвать freeJsonCStructFull().
 * \param fileName - имя файла для чтения.
 * \return структуру JsonCStruct.
 */
JsonCStruct openJsonFromFile(const char *fileName);

JsonErrorEnum saveJsonFile(const char *fileName, JsonCStruct jStruct);
/*!
 * \brief Освобождает память parentItem и потомков.
 * \param jStruct - структура, которую возвращает openJsonFile.
 */
void freeJsonCStruct(JsonCStruct jStruct);

/*!
 * \brief Освобождает не только parentItem но и jsonTextFull.
 * \param jStruct - структура, которую возвращает openJsonFile.
 */
void freeJsonCStructFull(JsonCStruct jStruct);

/*!
 * \brief Записывает Json в файл.
 * \param file - (стандартный си) открытый файл.
 * \param item - ключ и значение JSON в том числе вложенные.
 * \return количество записанных символов.
 */
int32_t fprintJsonItem(FILE *file, const JsonItem *item);

/*!
 * \brief Записывает Json в файл.
 * \param file - (стандартный си) открытый файл.
 * \param item - ключ и значение JSON в том числе вложенные.
 * \param offset - добавление 4 * offset пробелов вначале каждой строки.
 * \return количество записанных символов.
 */
int32_t fprintJsonItemOffset(FILE *file, const JsonItem *item, uint32_t offset);

/*!
 * \brief Проверяет на ошибку и если ошибок нет, записывает в file.
 * \param file - (стандартный си) открытый файл.
 * \param jStruct - структура после вызова openJsonFile().
 * \return количество записанных символов.
 */
int32_t fprintJsonStruct(FILE *file, JsonCStruct jStruct);

// KeyPath
/*!
 * \brief Ключевое слово для поиска вложенных ключей.
 *
 * Например поиск ключа "\"pins\"[3]->\"position\"[1]->\"slot\"".
 */
const char* INTO;   // "->"

/*!
 * \brief Размер ключевого слова.
 */
const size_t INTO_SIZE; // 2

/*!
 * \brief Структура парсинга пути.
 */
typedef struct KeyItemTypeDef {
    const char *keyStr;
    size_t keyStrLen;
    struct KeyItemTypeDef *child;
    size_t index; // for array
} KeyItem;

/*!
 * \brief Парсит путь ключей.
 *
 * Пример keyPath: "\"pins\"[3]->\"position\"[1]->\"slot\"\0".
 * \param keyPath - строка ключей.
 * \param keyItem - выходная структура ключей.
 * \return true, если успешно распарсил.
 */
bool parseKeyPath(const char *keyPath, KeyItem **keyItem);

/*!
 * \brief Поиск элемента по ключу.
 * \param keyItem - инициализированный ключ.
 * \param root - элемент с которого начинается поиск.
 * \return найденный элемент.
 */
JsonItem *getItem(const KeyItem *keyItem, const JsonItem *root);

#endif  /* ndef JSONC_H */
