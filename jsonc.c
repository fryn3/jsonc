#include "jsonc.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>

/*!
 * \brief Признак окончания строки.
 */
static const char END_STR = 0;

/*!
 * \brief Ключевые слова и их длина.
 */
static const char * const NULL_STR = "null";
static const size_t NULL_STR_LEN = 4;
static const char * const FALSE_STR = "false";
static const size_t FALSE_STR_LEN = 5;
static const char * const TRUE_STR = "true";
static const size_t TRUE_STR_LEN = 4;

static const size_t INIT_LEN = SIZE_MAX;

/*!
 * \brief Инициализирует JsonItem.
 * \param r - выходная структура.
 */
static void initJsonItem(JsonItem *r) {
    r->parent = NULL;
    r->key = NULL;
    r->keyLen = INIT_LEN;
    r->type = JsonTypeCount;
    r->number = 1E+37;
    r->str = NULL;
    r->strLen = INIT_LEN;
    r->childrenList = NULL;
    r->childrenCount = 0;
    r->_childrenReserve = 0;
}

/*!
 * \brief Инициализирует JsonCStruct.
 * \param r - выходная структура.
 */
static void initJsonCStruct(JsonCStruct *r) {
    r->jsonTextFull = NULL;
    r->rootItem = NULL;
    r->error = JsonJustInit;
}


/*!
 * \brief Поиск символа пропуская экранирование.
 *
 * str должен заканчиваться END_STR.
 * \param ch - символ поиска.
 * \param str - строка по которому идет поиск.
 * \return указатель на символ ch или NULL.
 */
static const char *findCh(char ch, const char *str) {
    char mask = '\\'; // символ экранирования
    for (size_t i = 0; str[i] != END_STR; ++i) {
        if (str[i] == ch) {
            return str + i;
        } else if (str[i] == mask) {
            ++i; // Пропуск следующего символа
        }
    }
    return NULL;
}

/*!
 * \brief Проверка, является ли входной символ цифрой.
 * \param ch - символ.
 * \return true, если цифра, иначе false.
 */
static inline bool isNumeric(char ch) {
    return ch >= '0' && ch <= '9';
}

/*!
 * \brief Проверка, является ли входной символ плюс или минусом.
 * \param ch - символ.
 * \return true, если является, иначе false.
 */
static inline bool isPlusMinus(char ch) {
    return ch == '-' || ch == '+';
}

/*!
 * \brief Проверка, является ли входной символ цифрой или плюс/минус.
 * \param ch - символ.
 * \return true, если является, иначе false.
 */
static inline bool isNumericPlusMinus(char ch) {
    return isNumeric(ch) || isPlusMinus(ch);
}

/*!
 * \brief Перевод строки в int32_t.
 *
 * Переводит строку до первого не цифрового символа. Первый символ может быть
 * либо '+', либо '-'. Работает только в десятичном формате.
 * str должен заканчиваться END_STR.
 * \param str - строка с числом.
 * \param number - указатель, куда будет записан ответ.
 * \return указатель на первый не цифровой символ.
 */
static const char *myAtoi(const char *str, int32_t *number) {
    *number = 0;
    size_t i = 0;
    bool isNegative = false;
    bool isMark = false;
    if (isPlusMinus(str[0])) {
        isMark = true;
        isNegative = str[0] == '-';
    }
    for (i = isMark; str[i] != END_STR; ++i) {
        if (str[i] < '0' || str[i] > '9') {
            break;
        }
        *number = *number * 10 + (str[i] - '0');
    }
    if (isNegative) {
        *number *= -1;
    }
    return str + i;
}

/*!
 * \brief Перевод строки в double.
 *
 * Переводит строку до первого символа не входящий в шаблон:
 * [+-][0-9]*[.]?[0-9]*([eE][+-]?[0-9]+)?
 * \param str - входная строка.
 * \param number - выходное число.
 * \return если ошибка то NULL, иначе ссылку на следующий символ после
 * окончания вершины.
 */
static const char *myAtof(const char *str, double *number) {
    const char *it = str;
    /// Наличие точки, или букв 'e' || 'E'.
    bool isDot = it[0] == '.';
    /// Наличие хотя бы одного числа до 'e' || 'E'.
    bool isNum = isNumeric(it[0]);
    /// Наличие букв 'e' || 'E'.
    bool isE = false;
    /// Наличие знака после букв 'e' || 'E'.
    bool isESign = false;
    /// Наличие цифр после букв 'e' || 'E'
    bool isENum = false;
    if (!isNumericPlusMinus(it[0]) && !isDot) {
        return NULL;
    }
    size_t i = 1;
    for (i = 1; it[i] != END_STR; ++i) {
        if (isNumeric(it[i])) {
            if (isE) {
                isESign = true;
                isENum = true;
            } else {
                isNum = true;
            }
            continue;
        } else if (!isDot && it[i] == '.') {
            isDot = true;
            continue;
        } else if (!isE && (it[i] == 'e' || it[i] == 'E')) {
            isDot = true;
            isE = true;
            continue;
        } else if (!isESign && isPlusMinus(it[i])) {
            isESign = true;
        }
        break;
    }
    if (isESign && !isENum) {
        // "1.1e+" -- исключаем знак и 'e'
        i -= 2;
    } else if (isE && !isENum) {
        // "1.1e" -- исключаем 'e'.
        --i;
    }
    if (isDot && !isNum) {
        // "." -- если введена только точка до 'e' || 'E'.
        return NULL;
    }
    char * const doubleStr = malloc((i + 1) * sizeof(char));
    if (doubleStr == NULL) { return NULL; }
    memcpy(doubleStr, it, i);
    doubleStr[i] = 0;
    *number = atof(doubleStr);
    free(doubleStr);
    return it + i;
}

/*!
 * \brief Сравнение строк.
 *
 * Сравнение идет до n или до конца строки если окажется раньше.
 * \param str1
 * \param str2
 * \param n - максимальная длина сравнений.
 * \return true, если различий нет, иначе false.
 */
static bool myStrcmp(const char *str1, const char *str2, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (str1[i] == END_STR
                    || str2[i] == END_STR
                    || str1[i] != str2[i]) {
            return str1[i] == str2[i];
        }
    }
    return true;
}


/*!
 * \brief Парсит строку.
 * \param str - начинается с '"'.
 * \return второй не экранированный символ '"'.
 */
static const char *parseString(const char *str) {
    const char *it = str;
    if (it[0] != '"') {
        return NULL;
    }
    if (!(it = findCh('"', it + 1))) {
        return NULL;
    }
    return it;
}

/*!
 * \brief Пропуск пробельных символов и комментариев.
 * \param jsonText - указатель, с какого символа начинать поиск.
 * \return NULL - если не найдено, иначе указатель на символ.
 */
static const char *firstChar(const char *jsonText) {
    for (size_t i = 0; jsonText[i] != END_STR; ++i) {
        if (jsonText[i] == ' '
                || jsonText[i] == '\n'
                || jsonText[i] == '\t'
                || jsonText[i] == '\r') {
            continue;
        }
        // Обработка комментариев в Json файле
        if (jsonText[i] == '/'
                && jsonText[i + 1] != END_STR
                && jsonText [i + 1] == '/') {
            bool endLine = false;
            for (i = i + 2; jsonText[i] != END_STR; ++i) {
                if (jsonText[i] == '\n') {
                    endLine = true;
                    break;
                }
            }
            if (endLine) {
                continue;
            } else {
                return NULL;
            }
        }
        return jsonText + i;
    }
    return NULL;
}

// вспомогательный макрос для функции parseValue.
#define IF_TO_ERROR(x, err) \
    do { \
        if (x) { \
            printf("%d", __LINE__); \
            (*ppStruct)->error = err; \
            return NULL; \
        } \
    } while (false)

/*!
 * \brief Парсит json, следит за синтаксисом.
 *
 * Вызывается внутри openJsonFile().
 * \param json - массив данных json файла
 * \param ppCurrent - текущая вершина.
 * \param ppStruct - структура
 * \return если ошибка то NULL, иначе ссылку на следующий символ после
 * окончания вершины.
 */
static const char *parseValue(const char *json, JsonItem **ppCurrent, JsonCStruct **ppStruct) {
    const bool IS_ROOT = !(*ppStruct);
    if (IS_ROOT) {
        (*ppStruct) = malloc(sizeof(JsonCStruct));
        if (*ppStruct == NULL) { return NULL; }
        initJsonCStruct(*ppStruct);
        (*ppStruct)->jsonTextFull = json;
        *ppCurrent = malloc(sizeof(JsonItem));
        if (*ppCurrent == NULL) { return NULL; }
        initJsonItem(*ppCurrent);
        (*ppStruct)->rootItem = *ppCurrent;
        (*ppStruct)->error = JsonSuccess;
    }
    JsonItem *pCurrent = *ppCurrent;
    const char *it1 = json;
    const char *it2 = NULL;
    it1 = firstChar(json);
    if (!it1) {
        return NULL;
    } else if (strcmp(it1, NULL_STR) == 0) {
        pCurrent->type = JsonTypeNull;
        it1 += NULL_STR_LEN;
    } else if (strcmp(it1, FALSE_STR) == 0) {
        pCurrent->type = JsonTypeBool;
        pCurrent->number = 0;
        it1 += FALSE_STR_LEN;
    } else if (strcmp(it1, TRUE_STR) == 0) {
        pCurrent->type = JsonTypeBool;
        pCurrent->number = 1;
        it1 += TRUE_STR_LEN;
    } else if (isNumericPlusMinus(it1[0])) {
        pCurrent->type = JsonTypeNumber;
        it1 = myAtof(it1, &pCurrent->number);
    } else if (it1[0] == '"') {
        pCurrent->type = JsonTypeString;
        it2 = parseString(it1);
        IF_TO_ERROR(!it2, JsonErrorValue);
        pCurrent->str = ++it1;
        pCurrent->strLen = it2 - it1;
        it1 = it2 + 1;
    } else if (it1[0] == '[') {
        pCurrent->type = JsonTypeArray;
        it2 = firstChar(it1 + 1);
        if (it2[0] != ']') {
            while (it1[0] != ']') {
                JsonItem *jNew = addChild(pCurrent);
                it1 = parseValue(it1 + 1, &jNew, ppStruct);
                if (!it1) {
                    // parseValue сам заполняет jCurrent->error.
                    return NULL;
                }
                it1 = firstChar(it1);
                IF_TO_ERROR(!it1, JsonErrorSyntax);
                IF_TO_ERROR(it1[0] != ',' && it1[0] != ']', JsonErrorSyntax);
            }
        } else {
            it1 = it2;
        }
        ++it1;
    } else if (it1[0] == '{') {
        pCurrent->type = JsonTypeObject;
        it2 = firstChar(it1 + 1);
        if (it2[0] != '}') {    // Проверка на пустоту.
            while (it1[0] != '}') {
                it1 = firstChar(it1 + 1);
                IF_TO_ERROR(!it1, JsonErrorEnd);
                IF_TO_ERROR(it1[0] != '"', JsonErrorSyntax);
                it2 = parseString(it1);
                IF_TO_ERROR(!it2, JsonErrorKey);
                JsonItem *jNew = addChild(pCurrent);
                jNew->key = ++it1;
                jNew->keyLen = it2 - it1;
                it1 = firstChar(it2 + 1);
                IF_TO_ERROR(!it1, JsonErrorEnd);
                IF_TO_ERROR(it1[0] != ':', JsonErrorSyntax);
                it1 = parseValue(it1 + 1, &jNew, ppStruct);
                if (!it1) {
                    // parseValue сам заполняет jCurrent->error.
                    return NULL;
                }
                it1 = firstChar(it1);
                IF_TO_ERROR(!it1, JsonErrorSyntax);
                IF_TO_ERROR(it1[0] != ',' && it1[0] != '}', JsonErrorSyntax);
            }
        } else {
            it1 = it2;
        }
        ++it1;
    }
    return it1;
}

static void freeJsonItemChild(JsonItem *item) {
    if (!item) {
        return;
    }
    for (size_t i = 0; i < item->childrenCount; ++i) {
        freeJsonItemChild(item->childrenList + i);
    }
    free(item->childrenList);
}

static void freeJsonItemChildFull(JsonItem *item) {
    if (!item) {
        return;
    }
    for (size_t i = 0; i < item->childrenCount; ++i) {
        freeJsonItemChild(item);
    }
    free((void*)item->key);
    free((void*)item->str);
    free(item->childrenList);
}

JsonCStruct openJsonFromStr(const char *jsonTextFull) {
    JsonItem *jCurrent = NULL;
    JsonCStruct *jStruct = NULL;
    parseValue(jsonTextFull, &jCurrent, &jStruct);
    if (!jStruct) {
        JsonCStruct r;
        initJsonCStruct(&r);
        return r;
    }
    JsonCStruct r = *jStruct;
    free(jStruct);
    return r;
}

JsonCStruct openJsonFromFile(const char *fileName) {
    JsonCStruct r;
    initJsonCStruct(&r);
    FILE *ptrFile = fopen(fileName, "r");
    if (ptrFile == NULL) {
        r.error = JsonErrorFile;
        return r;
    }
    fseek(ptrFile, 0, SEEK_END);
    long lSize = ftell(ptrFile) + 1;    // +1 для нулевого символа
    rewind(ptrFile);

    char *buffer = (char*)malloc(sizeof(char) * lSize);
    if (buffer == NULL) {
        r.error = JsonErrorFile;
        return r;
    }

    size_t result = fread(buffer, 1, lSize, ptrFile);
    fclose(ptrFile);
    buffer[result] = 0; // добавления нулевого символа
    return openJsonFromStr(buffer);
}

int32_t saveJsonCStruct(const char *fileName, JsonCStruct jStruct) {
    FILE *ptrFile = fopen(fileName, "w");
    if (ptrFile == NULL) {
        return -1;
    }
    int32_t res = fprintJsonItem(ptrFile, jStruct.rootItem);
    fclose(ptrFile);

    return res;
}

void freeJsonCStruct(JsonCStruct jStruct) {
    freeJsonParent(jStruct.rootItem);
}

void freeJsonCStructFull(JsonCStruct jStruct) {
    freeJsonCStruct(jStruct);
    free((void*)jStruct.jsonTextFull);
}

JsonItem *createJsonParent(void) {
    JsonItem *jIt = (JsonItem*)malloc(sizeof(JsonItem));
    initJsonItem(jIt);
    jIt->type = JsonTypeObject;
    return jIt;
}

bool freeJsonParent(JsonItem *item) {
    if (!item || item->parent) {
        return false;
    }
    freeJsonItemChild(item->childrenList);
    free(item);
    return true;
}

bool freeJsonItemFull(JsonItem *item) {
    if (!item || item->parent) {
        return false;
    }
    freeJsonItemChildFull(item->childrenList);
    free((void*)item->key);
    free((void*)item->str);
    free(item);
    return true;
}

JsonItem *findChildKey(const JsonItem *root, const char *key) {
    return findChildKeyLen(root, key, strlen(key));
}

JsonItem *findChildKeyLen(const JsonItem *root, const char *key, size_t keyLen) {
    if (!root || root->type != JsonTypeObject) {
        return NULL;
    }
    for (size_t i = 0; i < root->childrenCount; ++i) {
        if (root->childrenList[i].keyLen == keyLen &&
                myStrcmp(key, root->childrenList[i].key, keyLen)) {
            return root->childrenList + i;
        }
    }
    return NULL;
}

JsonItem *findChildIndex(JsonItem *root, size_t index) {
    if (index == INIT_LEN
            || root->type != JsonTypeArray
            || root->childrenCount <= index) {
        return NULL;
    }
    return root->childrenList + index;
}

size_t indexOfChild(JsonItem* pChild) {
    JsonItem *parent = pChild->parent;
    if (!parent
            || (parent->type != JsonTypeArray
                && parent->type != JsonTypeObject)) {
        return SIZE_MAX;
    }
    if (pChild < parent->childrenList
            || pChild > parent->childrenList + parent->childrenCount) {
        return SIZE_MAX;
    }
    return parent->childrenList - pChild;
}

bool reserveChildCount(JsonItem *pCurrent, size_t childrenReserve) {
    if (!pCurrent
            || (pCurrent->type != JsonTypeArray
                && pCurrent->type != JsonTypeObject)) {
        return false;
    }
    childrenReserve = childrenReserve >= pCurrent->childrenCount ?
                                childrenReserve : pCurrent->childrenCount;
    if (childrenReserve == pCurrent->_childrenReserve) {
        return true;
    }
    /// \todo необходимо проверить работу realloc().
    void *newList = realloc(pCurrent->childrenList,
                                     childrenReserve * sizeof(JsonItem));
    if (!newList) {
        return false;
    }
    pCurrent->childrenList = newList;
    for (size_t i = 0; i < pCurrent->childrenCount; ++i) {
        for (size_t j = 0; j < pCurrent->childrenList[i].childrenCount; ++j) {
            pCurrent->childrenList[i].childrenList[j].parent
                                                = pCurrent->childrenList + i;
        }
    }
    pCurrent->_childrenReserve = childrenReserve;
    return true;
}

bool removeChild(JsonItem *pChild) {
    size_t ind = indexOfChild(pChild);
    if (ind == SIZE_MAX) {
        return false;
    }
    freeJsonItemChild(pChild);
    JsonItem *parent = pChild->parent;
    for (size_t i = ind; i < parent->childrenCount - 1; ++i) {
        parent->childrenList[i] = parent->childrenList[i + 1];
    }
    --parent->childrenCount;
    initJsonItem(parent->childrenList + parent->childrenCount);
    return true;
}

JsonItem *addChild(JsonItem *pCurrent) {
    if (pCurrent->type != JsonTypeArray
            && pCurrent->type != JsonTypeObject) {
        return NULL;
    }
    if (pCurrent->_childrenReserve == pCurrent->childrenCount) {
        if (pCurrent->_childrenReserve == 0) {
            reserveChildCount(pCurrent, 1);
        } else {
            reserveChildCount(pCurrent, pCurrent->childrenCount * 2);
        }
    }
    JsonItem *child = &pCurrent->childrenList[pCurrent->childrenCount];
    initJsonItem(child);
    ++(pCurrent->childrenCount);
    child->parent = pCurrent;
    return child;
}

JsonItem *addChildType(JsonItem *pCurrent, JsonTypeEnum type) {
    JsonItem *r = addChild(pCurrent);
    r->type = type;
    return r;
}

JsonItem *addChildKeyType(JsonItem *pCurrent, const char *key, JsonTypeEnum type) {
    JsonItem *r = addChild(pCurrent);
    r->type = type;
    r->key = key;
    r->keyLen = strlen(key);
    return r;
}

JsonItem *addChildKeyLenType(JsonItem *pCurrent, const char *key, size_t keyLen, JsonTypeEnum type) {
    JsonItem *r = addChild(pCurrent);
    r->type = type;
    r->key = key;
    r->keyLen = keyLen;
    return r;
}

JsonItem *addChildBool(JsonItem *pCurrent, bool boolValue) {
    JsonItem *r = addChild(pCurrent);
    r->type = JsonTypeBool;
    r->number = boolValue;
    return r;
}

JsonItem *addChildKeyBool(JsonItem *pCurrent, const char *key, bool boolValue) {
    JsonItem *r = addChild(pCurrent);
    r->type = JsonTypeBool;
    r->key = key;
    r->keyLen = strlen(key);
    r->number = boolValue;
    return r;
}

JsonItem *addChildKeyLenBool(JsonItem *pCurrent, const char *key, size_t keyLen, bool boolValue) {
    JsonItem *r = addChild(pCurrent);
    r->type = JsonTypeBool;
    r->key = key;
    r->keyLen = keyLen;
    r->number = boolValue;
    return r;
}

JsonItem *addChildNumber(JsonItem *pCurrent, double number) {
    JsonItem *r = addChild(pCurrent);
    if (!r) {
        return r;
    }
    r->type = JsonTypeNumber;
    r->number = number;
    return r;
}

JsonItem *addChildKeyNumber(JsonItem *pCurrent, const char *key, double number) {
    JsonItem *r = addChild(pCurrent);
    r->type = JsonTypeNumber;
    r->key = key;
    r->keyLen = strlen(key);
    r->number = number;
    return r;
}

JsonItem *addChildKeyLenNumber(JsonItem *pCurrent, const char *key, size_t keyLen, double number) {
    JsonItem *r = addChild(pCurrent);
    r->type = JsonTypeNumber;
    r->key = key;
    r->keyLen = keyLen;
    r->number = number;
    return r;
}

JsonItem *addChildStr(JsonItem *pCurrent, const char *str) {
    JsonItem *r = addChild(pCurrent);
    r->type = JsonTypeString;
    r->str = str;
    r->strLen = strlen(str);
    return r;
}

JsonItem *addChildStrLen(JsonItem *pCurrent, const char *str, size_t strLen) {
    JsonItem *r = addChild(pCurrent);
    r->type = JsonTypeString;
    r->str = str;
    r->strLen = strLen;
    return r;
}

JsonItem *addChildKeyStr(JsonItem *pCurrent, const char *key, const char *str) {
    JsonItem *r = addChild(pCurrent);
    r->type = JsonTypeString;
    r->key = key;
    r->keyLen = strlen(key);
    r->str = str;
    r->strLen = strlen(str);
    return r;
}

JsonItem *addChildKeyStrLen(JsonItem *pCurrent, const char *key, const char *str, size_t strLen) {
    JsonItem *r = addChild(pCurrent);
    r->type = JsonTypeString;
    r->key = key;
    r->keyLen = strlen(key);
    r->str = str;
    r->strLen = strLen;
    return r;
}

JsonItem *addChildKeyLenStr(JsonItem *pCurrent, const char *key, size_t keyLen, const char *str) {
    JsonItem *r = addChild(pCurrent);
    r->type = JsonTypeString;
    r->key = key;
    r->keyLen = keyLen;
    r->str = str;
    r->strLen = strlen(str);
    return r;
}

JsonItem *addChildKeyLenStrLen(JsonItem *pCurrent, const char *key, size_t keyLen, const char *str, size_t strLen) {
    JsonItem *r = addChild(pCurrent);
    r->type = JsonTypeString;
    r->key = key;
    r->keyLen = keyLen;
    r->str = str;
    r->strLen = strLen;
    return r;
}

int32_t fprintJsonItem(FILE *file, const JsonItem *item) {
    return fprintJsonItemOffset(file, item, 0);
}

int32_t fprintJsonItemOffset(FILE *file, const JsonItem *item, uint32_t offset) {
    const char* offsetStr = "    ";
    int32_t printSize = 0;
    for (uint32_t i = 0; i < offset; ++i) {
        printSize += fprintf(file, "%s", offsetStr);
    }
    if (item->parent && item->parent->type != JsonTypeArray) {
        printSize += fprintf(file, "\"%*.*s\": ", (int32_t)item->keyLen, (int32_t)item->keyLen, item->key);
    }
    switch (item->type) {
    case JsonTypeNull:
        printSize += fprintf(file, "%s", NULL_STR);
        break;
    case JsonTypeBool:
        printSize += fprintf(file, "%s", item->number ? TRUE_STR : FALSE_STR);
        break;
    case JsonTypeNumber:
        printSize += fprintf(file, "%g", item->number);
        break;
    case JsonTypeString:
        printSize += fprintf(file, "\"%*.*s\"", (int32_t)item->strLen, (int32_t)item->strLen, item->str);
        break;
    case JsonTypeObject:
    case JsonTypeArray:
        printSize += fprintf(file, item->type == JsonTypeObject ? "{\n" : "[\n");
        for (uint32_t i = 0; i < item->childrenCount; ++i) {
            printSize += fprintJsonItemOffset(file, item->childrenList + i, offset + 1);
            if (i + 1 != item->childrenCount) {
                printSize += fprintf(file, ",\n");
            }
        }
        printSize += fprintf(file, "\n");
        for (uint32_t i = 0; i < offset; ++i) {
            printSize += fprintf(file, "%s", offsetStr);
        }
        printSize += fprintf(file, item->type == JsonTypeObject ? "}" : "]");
        break;
    default:
        return INT32_MIN;
    }
    return printSize;
}

int32_t fprintJsonStruct(FILE *file, JsonCStruct jStruct) {
    if (jStruct.error != JsonSuccess) {
        printf("error = %d\n", jStruct.error);
        return 0;
    }
    int32_t s = fprintJsonItem(file, jStruct.rootItem);
    s += fprintf(file, "\n");
    return s;
}

int32_t saveJsonItem(const char *fileName, const JsonItem* root) {
    FILE *ptrFile = fopen(fileName, "w");
    if (ptrFile == NULL) {
        return -1;
    }
    int32_t res = fprintJsonItem(ptrFile, root);

    fclose(ptrFile);

    return res;
}
// KeyPath

const char* const INTO = "->";
const size_t INTO_SIZE = 2;

/*!
 * \brief Инициализирует KeyItem.
 * \param r - выходная структура.
 */
static void initKeyItem(KeyItem *r) {
    r->keyStr = NULL;
    r->keyStrLen = INIT_LEN;
    r->child = NULL;
    r->index = INIT_LEN;
}

/*!
 * \brief Проверка на последовательность INTO.
 * \param str - строка, где происходит проверка.
 * \return NULL - если не совпало, иначе на первый символ после INTO.
 */
static const char* checkInto(const char *str) {
    for (size_t i = 0; i < INTO_SIZE; ++i) {
        if (str[i] != INTO[i]) {
            return NULL;
        }
    }
    return str + INTO_SIZE;
}

bool parseKeyPath(const char *keyPath, KeyItem **keyItem) {
    KeyItem *root = malloc(sizeof(KeyItem));
    if (root == NULL) { return false; }
    initKeyItem(root);
    const char *it = keyPath;
    if (!(it = parseString(keyPath))) {
        free(root);
        return false;
    }
    root->keyStr = keyPath + 1;
    root->keyStrLen = it - root->keyStr;
    ++it;
    if (it[0] == END_STR) {
        *keyItem = root;
        return true;
    }
    if (it[0] == '[') {
        int32_t t;
        it = myAtoi(it + 1, &t);
        root->index = t;
        if (it[0] != ']') {
            free(root);
            return false;
        }
        ++it;
        if (it[0] == END_STR) {
            *keyItem = root;
            return true;
        }
    }
    it = checkInto(it);
    if (!it) {
        free(root);
        return false;
    }
    if (!parseKeyPath(it, &root->child)) {
        free(root);
        return false;
    }
    *keyItem = root;
    return true;
}

void freeKeyItem(KeyItem *keyItem) {
    if (!keyItem) {
        return;
    }
    if (keyItem->child) {
        freeKeyItem(keyItem->child);
    }
    free(keyItem);
}

JsonItem *getItem(const KeyItem *keyItem, const JsonItem *root) {
    if (!keyItem || !root) {
        printf("bad param");
        return NULL;
    }
    JsonItem *child = NULL;
    child = findChildKeyLen(root, keyItem->keyStr, keyItem->keyStrLen);
    if (!child) {
        printf("could not find key");
        return NULL;
    }
    if (keyItem->index != INIT_LEN) {
        child = findChildIndex(child, keyItem->index);
        if (!child) {
            printf("could not find index");
            return NULL;
        }
    }
    if (keyItem->child) {
        child = getItem(keyItem->child, child);
    }
    return child;
}


JsonItem *getItemStr(const char *keyPath, const JsonItem *root) {
    KeyItem *keyItem = NULL;
    bool err = parseKeyPath(keyPath, &keyItem);
    if (err || !keyItem) {
        return NULL;
    }
    JsonItem *resultItem = getItem(keyItem, root);
    freeKeyItem(keyItem);
    return resultItem;
}
