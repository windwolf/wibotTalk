#ifndef ___CONSOLE_H__
#define ___CONSOLE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"
#include "shared.h"
#include "algorithm/inc/basic.h"
#include "stdbool.h"

#define CONSOLE_NODE_MAX_DEPTH 16
#define CONSOLE_PATH_MAX_SIZE 256

    typedef enum
    {
        CONSOLE_ITEM_TYPE_INT,
        CONSOLE_ITEM_TYPE_FLOAT,
        CONSOLE_ITEM_TYPE_STRING,
        CONSOLE_ITEM_CONTAINER,
    } CONSOLE_ITEM_TYPE;

    typedef enum
    {
        CONSOLE_COMMAND_TYPE_CD,
        CONSOLE_COMMAND_TYPE_LIST,
        CONSOLE_COMMAND_TYPE_SET,
        CONSOLE_COMMAND_TYPE_GET,

    } CONSOLE_COMMAND_TYPE;

    typedef bool (*ConsoleItemAccessor)(const void *context, const int32_t index, void *in, void **out, bool isSet);

    typedef struct ConsoleItemEntry
    {
        Tree base;
        const char *name;
        CONSOLE_ITEM_TYPE type;
        ConsoleItemAccessor accessor;
    } ConsoleItemEntry;

    typedef struct ConsolePathNode
    {
        ConsoleItemEntry *currentEntry;
        int32_t index;
    } ConsolePathNode;

    typedef struct ConsolePathContextNode
    {
        ConsoleItemEntry *currentEntry;
        int32_t index;
        void *parentData;
    } ConsolePathContextNode;

    typedef struct ConsoleContext
    {
        char path[CONSOLE_PATH_MAX_SIZE];
        ConsolePathContextNode contextNodes[CONSOLE_NODE_MAX_DEPTH];
        int32_t currentContextNodeIndex;
        ConsolePathNode _parsedNodes[CONSOLE_NODE_MAX_DEPTH];
        int32_t _currentParsedNodeIndex;
        int32_t _parsedNodeBackTrace;
    } ConsoleContext;

    typedef struct CommandLine
    {
        const char *context;
        ConsoleItemEntry *entry;

    } ConsoleItemPath;

    typedef struct Console
    {
        ConsoleItemEntry *root;
        ConsoleContext context;

    } Console;

    typedef struct ConsoleCommand
    {
        CONSOLE_COMMAND_TYPE type;
        char *path;
        char *value;
    } ConsoleCommand;

    bool console_item_register(Console *console, const char *containerPath, ConsoleItemEntry *entry);

    bool console_context_change(Console *console, const char *path);

    char *console_context_path_get(Console *console);

    bool console_value_set(Console *console, const char *path, void *value);

    bool console_value_get(Console *console, const char *path, void **value);

    bool console_item_list(Console *console);

#ifdef __cplusplus
}
#endif

#endif // ___CONSOLE_H__