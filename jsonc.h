#ifndef JSONC_H
#define JSONC_H


#if defined(__cplusplus) || defined(__cplusplus__)
extern "C" {
#endif

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
    /// Указатель на корневой элемент.
    JsonItem *rootItem;
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

/*!
 * \brief Сохраняет в JSON формате.
 * \param fileName - имя файла.
 * \param jStruct - структура JSON.
 * \return отрицательное число при ошибке, иначе количество записанных
 * символов.
 */
int32_t saveJsonCStruct(const char *fileName, JsonCStruct jStruct);

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
 * \brief Создает инициализированный JsonItem.
 *
 * Не забыть вызвать freeJsonItem для освобождении памяти.
 * \return JsonItem.
 */
JsonItem *createItem(void);

/*!
 * \brief Рекурсивно освобождает память.
 * \param item - объект удаления.
 */
void freeJsonItem(JsonItem *item);

/*!
 * \brief Рекурсивно освобождает память так же у полей key & str.
 * \param item - объект удаления.
 */
void freeJsonItemFull(JsonItem *item);

/*!
 * \brief Добавляет вложенный элемент с инициализированным родителем.
 *
 * Текущий элемент должен быть массивом или объектом.
 * \param pCurrent - элемент родитель.
 * \return указатель на вложенный элемент.
 */
JsonItem *addChild(JsonItem *pCurrent);

/*!
 * \brief Добавляет вложенный элемент с указанным типом.
 * \param pCurrent - элемент родитель.
 * \param type - тип элемента.
 * \return указатель на вложенный элемент.
 */
JsonItem *addChildType(JsonItem *pCurrent, JsonTypeEnum type);

/*!
 * \brief Добавляет вложенный элемент.
 *
 * Если key не заканчивается нулевым символом, обязательно нужно
 * использовать addChildKeyLenType().
 * \param pCurrent - элемент родитель.
 * \param key - строка с нулевым символом в конце.
 * \param type - тип элемента.
 * \return указатель на вложенный элемент.
 */
JsonItem *addChildKeyType(JsonItem *pCurrent, const char *key, JsonTypeEnum type);
JsonItem *addChildKeyLenType(JsonItem *pCurrent, const char *key, size_t keyLen, JsonTypeEnum type);


/*!
 * \brief Добавляет вложенный элемент типа JsonTypeBool.
 * \param pCurrent - элемент родитель.
 * \param boolValue - значение элемента.
 * \return указатель на вложенный элемент.
 */
JsonItem *addChildBool(JsonItem *pCurrent, bool boolValue);
JsonItem *addChildKeyBool(JsonItem *pCurrent, const char *key, bool boolValue);
JsonItem *addChildKeyLenBool(JsonItem *pCurrent, const char *key, size_t keyLen, bool boolValue);

/*!
 * \brief Добавляет вложенный элемент типа JsonTypeNumber.
 * \param pCurrent - элемент родитель.
 * \param number - значение элемента.
 * \return указатель на вложенный элемент.
 */
JsonItem *addChildNumber(JsonItem *pCurrent, double number);
JsonItem *addChildKeyNumber(JsonItem *pCurrent, const char *key, double number);
JsonItem *addChildKeyLenNumber(JsonItem *pCurrent, const char *key, size_t keyLen, double number);

/*!
 * \brief Добавляет вложенный элемент типа JsonTypeString.
 * \param pCurrent - элемент родитель.
 * \param str - значение элемента.
 * \return указатель на вложенный элемент.
 */
JsonItem *addChildStr(JsonItem *pCurrent, const char *str);
JsonItem *addChildStrLen(JsonItem *pCurrent, const char *str, size_t strLen);
JsonItem *addChildKeyStr(JsonItem *pCurrent, const char *key, const char *str);
JsonItem *addChildKeyLenStr(JsonItem *pCurrent, const char *key, size_t keyLen, const char *str);
JsonItem *addChildKeyLenStrLen(JsonItem *pCurrent, const char *key, size_t keyLen, const char *str, size_t strLen);

/*!
 * \brief Находит дочерний элемент по ключу.
 * \param root - элемент поиска.
 * \param key - ключ поиска.
 * \param keyLen - максимальная длина ключа.
 * \return NULL если не находит, иначе дочерний элемент.
 */
JsonItem *findChildStr(const JsonItem *root, const char *key, size_t keyLen);

/*!
 * \brief Находит дочерний элемент по индексу.
 *
 * Важно, root должен иметь тип массив.
 * \param root - элемент поиска.
 * \param index - индекс дочернего элемента.
 * \return NULL если root не массив или index > root.childrenCount, иначе
 * дочерний элемент.
 */
JsonItem *findChildIndex(JsonItem *root, size_t index);

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

/*!
 * \brief Сохраняет в JSON формате.
 *
 * Для корректной записи, у root не должно быть родителя.
 * \param filename - имя файла.
 * \param root - корневой элемент.
 * \return отрицательное число при ошибке, иначе количество записанных
 * символов.
 */
int32_t saveJsonItem(const char *fileName, const JsonItem* root);

// KeyPath

/*!
 * \brief Ключевое слово для поиска вложенных ключей.
 *
 * Например поиск ключа "\"pins\"[3]->\"position\"[1]->\"slot\"".
 */
extern const char* const INTO;   // "->"

/*!
 * \brief Размер ключевого слова.
 */
extern const size_t INTO_SIZE; // 2

/*!
 * \brief Структура парсинга пути.
 *
 * Ссылается на строку. Важно, чтобы строка не была удалена.
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
 * \brief Освобождает память keyItem и потомков.
 * \param keyItem - структура начиная с которого все удалится.
 */
void freeKeyItem(KeyItem *keyItem);

/*!
 * \brief Поиск элемента по KeyItem.
 * \param keyItem - инициализированный ключ.
 * \param root - элемент, с которого начинается поиск.
 * \return найденный элемент.
 */
JsonItem *getItem(const KeyItem *keyItem, const JsonItem *root);

/*!
 * \brief Поиск элемента по строке.
 *
 * Пример keyPath: "\"pins\"[3]->\"position\"[1]->\"slot\"\0".
 * \param keyPath - строка поиска.
 * \param root - элемент, с которого начинается поиск.
 * \return найденный элемент.
 */
JsonItem *getItemStr(const char *keyPath, const JsonItem *root);

#if defined(__cplusplus) || defined(__cplusplus__)
}
#endif

#endif  /* ndef JSONC_H */
