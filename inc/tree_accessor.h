/**
 * @file tree.h
 * @author zhoujian (zhoujian.ww@gmail.com)
 * @brief Organize the tree struct, manage the context, and perform the node action.
 *  this module can be used in serial tree or gui/tree menu management.
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
    typedef bool (*TreeValueRawGetter)(const void *context, const int32_t index, void **out);
    typedef bool (*TreeValueGetter)(const void *context, const int32_t index, char *out);
    typedef bool (*TreeValueSetter)(const void *context, const int32_t index, char *in);

    typedef struct TreeAccessorItemEntry
    {
        Tree base;
        const char *name;
        TreeValueRawGetter rawGetter; // if NULL, return NULL.
        TreeValueGetter getter;       // if NULL, error when evaluate.
        TreeValueSetter setter;       // if NULL, error when evaluate.
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
        TreeAccessorItemEntry root;
        TreeAccessorContext context;
        char *_listBuf[CONSOLE_CHILDREN_MAX_SIZE];
    } TreeAccessor;

    void tree_accessor_create(TreeAccessor *tree, TreeValueRawGetter rawGetter, TreeValueGetter getter, TreeValueSetter setter);

    bool tree_accessor_item_register(TreeAccessor *tree, const char *containerPath, TreeAccessorItemEntry *entry);

    bool tree_accessor_context_change(TreeAccessor *tree, const char *path);

    char *tree_accessor_context_path_get(TreeAccessor *tree);

    bool tree_accessor_value_set(TreeAccessor *tree, const char *path, char *value);

    bool tree_accessor_value_get(TreeAccessor *tree, const char *path, char *outBuffer);

    char **tree_accessor_item_list(TreeAccessor *tree, const char *path);

#ifdef __cplusplus
}
#endif

#endif // ___TREE_ACCESSOR_H__