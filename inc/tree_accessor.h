/**
 * @file console.h
 * @author zhoujian (zhoujian.ww@gmail.com)
 * @brief Organize the tree struct, manage the context, and perform the node action.
 *  this module can be used in serial console or gui/console menu management.
 * @version 0.1
 * @date 2021-07-27
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef ___TREE_ACCESSOR_H__
#define ___TREE_ACCESSOR_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"
#include "shared.h"
#include "algorithm/inc/basic.h"
#include "stdbool.h"

#define CONSOLE_NODE_MAX_DEPTH 16
#define CONSOLE_CHILDREN_MAX_SIZE 16
#define CONSOLE_PATH_MAX_SIZE 256

    typedef bool (*TreeValueAccessor)(const void *context, const int32_t index, void *in, void **out, bool isSet);

    typedef struct TreeAccessorItemEntry
    {
        Tree base;
        const char *name;
        TreeValueAccessor accessor;
    } TreeAccessorItemEntry;

    typedef struct TreeAccessorPathNode
    {
        TreeAccessorItemEntry *currentEntry;
        int32_t index;
    } TreeAccessorPathNode;

    typedef struct TreeAccessorPathContextNode
    {
        TreeAccessorItemEntry *currentEntry;
        int32_t index;
        void *parentData;
    } TreeAccessorPathContextNode;

    typedef struct TreeAccessorContext
    {
        char path[CONSOLE_PATH_MAX_SIZE];
        TreeAccessorPathContextNode contextNodes[CONSOLE_NODE_MAX_DEPTH];
        int32_t currentContextNodeIndex;
        TreeAccessorPathNode _parsedNodes[CONSOLE_NODE_MAX_DEPTH];
        int32_t _currentParsedNodeIndex;
        int32_t _parsedNodeBackTrace;
    } TreeAccessorContext;

    typedef struct TreeItemPath
    {
        const char *context;
        TreeAccessorItemEntry *entry;

    } TreeItemPath;

    typedef struct TreeAccessor
    {
        TreeAccessorItemEntry *root;
        TreeAccessorContext context;
        char *_listBuf[CONSOLE_CHILDREN_MAX_SIZE];
    } TreeAccessor;

    bool tree_accessor_item_register(TreeAccessor *console, const char *containerPath, TreeAccessorItemEntry *entry);

    bool tree_accessor_context_change(TreeAccessor *console, const char *path);

    char *tree_accessor_context_path_get(TreeAccessor *console);

    bool tree_accessor_value_set(TreeAccessor *console, const char *path, void *value);

    bool tree_accessor_value_get(TreeAccessor *console, const char *path, void **value);

    char **tree_accessor_item_list(TreeAccessor *console, const char *path);

#ifdef __cplusplus
}
#endif

#endif // ___TREE_ACCESSOR_H__