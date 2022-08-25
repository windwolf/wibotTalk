// /**
//  * @file tree.h
//  * @author zhoujian (zhoujian.ww@gmail.com)
//  * @brief Organize the tree struct, manage the context, and perform the node
//  * action. this module can be used in serial tree or gui/tree menu management.
//  * @version 0.1
//  * @date 2021-07-27
//  *
//  * @copyright Copyright (c) 2021
//  *
//  */

// #ifndef ___WWTALK_TREE_ACCESSOR_HPP__
// #define ___WWTALK_TREE_ACCESSOR_HPP__

// #include "base.hpp"
// #include "tree.hpp"

// namespace ww::comm {
// #define CONSOLE_NODE_MAX_DEPTH 16
// #define CONSOLE_CHILDREN_MAX_SIZE 16
// #define CONSOLE_PATH_MAX_SIZE 256
// typedef bool (*TreeValueRawGetter)(const void *context, const int32_t index,
//                                    void **out);
// typedef bool (*TreeValueGetter)(const void *context, const int32_t index,
//                                 char *out);
// typedef bool (*TreeValueSetter)(const void *context, const int32_t index,
//                                 char *in);

// struct TreeAccessorItemEntry : public Tree {
//     const char *name;
//     TreeValueRawGetter rawGetter; // if NULL, return NULL.
//     TreeValueGetter getter;       // if NULL, error when evaluate.
//     TreeValueSetter setter;       // if NULL, error when evaluate.
// };

// struct TreeAccessorPathNode {
//     TreeAccessorItemEntry *currentEntry;
//     int32_t index;
// };

// struct TreeAccessorPathContextNode {
//     TreeAccessorItemEntry *currentEntry;
//     int32_t index;
//     void *parentData;
// };

// struct TreeAccessorContext {
//     char path[CONSOLE_PATH_MAX_SIZE];
//     TreeAccessorPathContextNode contextNodes[CONSOLE_NODE_MAX_DEPTH];
//     int32_t currentContextNodeIndex;
//     TreeAccessorPathNode _parsedNodes[CONSOLE_NODE_MAX_DEPTH];
//     int32_t _currentParsedNodeIndex;
//     int32_t _parsedNodeBackTrace;
// };

// struct TreeItemPath {
//     const char *context;
//     TreeAccessorItemEntry *entry;
// };

// class TreeAccessor {
//   public:
//     TreeAccessor(TreeValueRawGetter rawGetter, TreeValueGetter getter,
//                  TreeValueSetter setter);

//     bool item_register(const char *containerPath, TreeAccessorItemEntry &entry);

//     bool context_change(const char *path);

//     char context_path_get();

//     bool value_set(const char *path, char *value);

//     bool value_get(const char *path, char *outBuffer);

//     TreeAccessorItemEntry root;
//     TreeAccessorContext context;
//     const char *_listBuf[CONSOLE_CHILDREN_MAX_SIZE];
// };

// const char **tree_accessor_item_list(TreeAccessor *tree, const char *path);
// } // namespace ww::comm

// #endif // ___WWTALK_TREE_ACCESSOR_HPP__