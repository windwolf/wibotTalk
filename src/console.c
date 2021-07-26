#include "../inc/console.h"

#include <ctype.h>
#include <string.h>

#define LOG_MODULE "console"
#include "log.h"

typedef enum
{
    token_slash,
    token_dot,
    token_double_dot,
    token_lbrace,
    token_rbrace,
    token_identity,
    token_index,
    token_end,
    token_unknown,
} token_type;

typedef enum
{
    action_get,
    action_set,
    action_context_sync,
} ConsoleAction;

static inline token_type _console_next_token(const char *path, char const **token, char const **end, int32_t *size);
static inline bool _console_ctx_update(ConsoleContext *ctx, char *identity, int identitySize, int index);
static void _console_init(Console *console);

static void _console_evaluate(Console *console, void *in, void **out, ConsoleAction action);
static bool _console_path_parse(Console *console, const char *path);
static ConsoleItemEntry *_console_entry_child_find(ConsoleItemEntry *entry, const char *identity, int size);
static int _console_match_name(const ConsoleItemEntry *entry, const char *name);

bool console_item_register(Console *console, const char *containerPath, ConsoleItemEntry *entry)
{
    if (containerPath == NULL || !strcmp(containerPath, ""))
    {
        console->root = entry;
        _console_init(console);
        return true;
    }
    if (console->root == NULL)
    {
        LOG_E("root not exist");
        return false;
    }
    if (!_console_path_parse(console, containerPath))
    {
        LOG_E("container not exist");
        return false;
    }
    ConsoleItemEntry *container = console->context._parsedNodes[console->context._currentParsedNodeIndex].currentEntry;

    if (tree_child_find((Tree *)container, entry->name, (CompareFunction)&_console_match_name) != NULL)
    {
        LOG_E("%s already exist", entry->name);
        return false;
    }
    tree_child_append((Tree *)container, entry);
    return true;
};

bool console_context_change(Console *console, const char *path)
{
    if (!_console_path_parse(console, path))
    {
        return false;
    }
    _console_evaluate(console, NULL, NULL, action_context_sync);
    console->context.path[0] = '\0';
    return true;
}

bool console_value_set(Console *console, const char *path, void *value)
{
    if (!_console_path_parse(console, path))
    {
        return false;
    }
    _console_evaluate(console, value, NULL, action_set);
    return true;
};

bool console_value_get(Console *console, const char *path, void **value)
{
    if (!_console_path_parse(console, path))
    {
        return false;
    }
    _console_evaluate(console, NULL, value, action_get);
    return true;
};

char *console_context_path_get(Console *console)
{
    ConsoleContext *ctx = &console->context;
    if (ctx->path[0] != '\0')
    {
        return ctx->path;
    }
    strlcpy(ctx->path, CONSOLE_PATH_MAX_SIZE, "/");
    bool first = true;
    for (size_t i = 1; i < ctx->currentContextNodeIndex; i++)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            strlcat(ctx->path, "/", CONSOLE_PATH_MAX_SIZE);
        }
        strlcat(ctx->path, ctx->contextNodes[i].currentEntry->name, CONSOLE_PATH_MAX_SIZE);
        if (ctx->contextNodes[i].index >= 0)
        {
            char it[16];
            itoa(ctx->contextNodes[i].index, it, 10);
            strlcat(ctx->path, it, CONSOLE_PATH_MAX_SIZE);
        }
    }
    return ctx->path;
}

bool console_item_list(Console *console)
{
    return 1;
};

static void _console_evaluate(Console *console, void *in, void **out, ConsoleAction action)
{
    ConsoleContext *ctx = &console->context;
    int32_t cIdx = ctx->_currentParsedNodeIndex;
    ConsolePathNode *pNode;
    if (action == action_context_sync)
    {
        for (int32_t i = ctx->_parsedNodeBackTrace + 1; i <= cIdx; i++)
        {
            pNode = &ctx->_parsedNodes[i];
            ConsolePathContextNode *cNode = &ctx->contextNodes[i];
            ConsolePathContextNode *cNodeParent = &ctx->contextNodes[i - 1];

            cNode->currentEntry = pNode->currentEntry;
            cNode->index = pNode->index;
            cNodeParent->currentEntry->accessor(cNodeParent->parentData, cNodeParent->index, NULL, &cNode->parentData, false);
        }
        ctx->currentContextNodeIndex = ctx->_currentParsedNodeIndex;
    }
    else
    {
        void *parentData;
        ConsolePathContextNode *parentNode = &ctx->contextNodes[ctx->_parsedNodeBackTrace];
        parentNode->currentEntry->accessor(parentNode->parentData, parentNode->index, NULL, &parentData, false);

        for (int32_t i = ctx->_parsedNodeBackTrace + 1; i <= cIdx - 1; i++)
        {
            void *pData;
            pNode = &ctx->_parsedNodes[i];
            pNode->currentEntry->accessor(parentData, pNode->index, NULL, &pData, false);
            parentData = pData;
        }
        if (action == action_get)
        {
            void *pData;
            pNode = &ctx->_parsedNodes[cIdx];
            pNode->currentEntry->accessor(parentData, pNode->index, NULL, &pData, false);
            *out = pData;
        }
        else
        {
            pNode = &ctx->_parsedNodes[cIdx];
            pNode->currentEntry->accessor(parentData, pNode->index, in, NULL, true);
        }
    }
}

/*            ┌--------------------------------------------------┐
              |      ┌------------ .. ----------------------┐    |
              |      |------------ . -----------------------┐    |
   ┌----------┼-----┐|           ┌--------------------------┐    |
   |          |     ↓|           |                          ↓    ↓
--(00)-> / --(10)-( 20 )-> id --(30)-> [ --> index --> ] --(40)-(50)-> end
                    ↑                                       |
                    └--------------- / ---------------------┘
*/
static bool _console_path_parse(Console *console, const char *path)
{
    if (path == NULL || !strcmp(path, ""))
    {
        return false;
    }
    char *token;
    int32_t tokenSize = -1;
    token_type t;
    int32_t index;
    char *id;
    int32_t idSize;
    ConsoleContext *ctx = &console->context;
#define next_token() t = _console_next_token(path, &token, &path, &tokenSize)
    next_token();
n00:
    if (t == token_slash)
    {
        ctx->_parsedNodes[0].currentEntry = console->root;
        ctx->_parsedNodes[0].index = -1;
        ctx->_currentParsedNodeIndex = 0;
        ctx->_parsedNodeBackTrace = 0;
        id = "/";
        idSize = 1;
        next_token();
        //goto n10;
    }
    else
    {
        for (int32_t i = 0; i < ctx->currentContextNodeIndex; i++)
        {
            ctx->_parsedNodes[i].currentEntry = ctx->contextNodes[i].currentEntry;
            ctx->_parsedNodes[i].index = ctx->contextNodes[i].index;
        }
        ctx->_currentParsedNodeIndex = ctx->currentContextNodeIndex;
        ctx->_parsedNodeBackTrace = ctx->currentContextNodeIndex;
        goto n20;
    }
n10:
    if (t != token_end)
    {
        //goto n20;
    }
    else
    {
        goto n50;
    }
n20:
    if (t == token_identity)
    {
        id = token;
        idSize = tokenSize;
        next_token();
        // goto n30;
    }
    else if (t == token_dot)
    {
        id = ".";
        idSize = 1;
        next_token();
        goto n40;
    }
    else if (t == token_double_dot)
    {
        id = "..";
        idSize = 2;
        next_token();
        goto n40;
    }
    else
    {
        goto error;
    }
n30:
    if (t == token_lbrace)
    {
        next_token();
        if (t != token_index)
        {
            goto error;
        }
        index = atoi(token);
        next_token();

        if (t != token_rbrace)
        {
            goto error;
        }
        next_token();
    }
    else
    {
        index = -1;
    }
n40:
    if (t == token_slash)
    {
        if (!_console_ctx_update(ctx, id, idSize, index))
        {
            return false;
        }
        next_token();

        goto n20;
    }
n50:
    if (t == token_end)
    {
        if (!_console_ctx_update(ctx, id, idSize, index))
        {
            return false;
        }
        return true;
    }
    else
    {
        goto error;
    }

error:
    LOG_E("syntax error");
    return false;
};

static ConsoleItemEntry *_console_entry_child_find(ConsoleItemEntry *entry, const char *identity, int size)
{
    const ConsoleItemEntry *child = entry->base.child;
    while (child != NULL)
    {
        if (!strncmp(child->name, identity, size))
        {
            return child;
        }
        child = child->base.next;
    };
    return NULL;
};

static inline bool _console_ctx_update(ConsoleContext *ctx, char *identity, int identitySize, int index)
{
    if (!strncmp(identity, "/", identitySize))
    {
        return true;
    }
    if (!strncmp(identity, "..", identitySize) && identitySize == 2)
    {
        if (ctx->_currentParsedNodeIndex == 0)
        {
            LOG_E("beyond the root");
            return false;
        }
        ctx->_currentParsedNodeIndex--;
        ctx->_parsedNodeBackTrace = min(ctx->_parsedNodeBackTrace, ctx->_currentParsedNodeIndex);
        return true;
    }
    else if (!strncmp(identity, ".", identitySize) && identitySize == 1)
    {
        return true;
    }
    else
    {
        int32_t idx = ctx->_currentParsedNodeIndex;
        if (idx >= CONSOLE_NODE_MAX_DEPTH - 1)
        {
            LOG_E("path reach the max depth");
            return false;
        }

        ConsolePathNode *parentNode = &ctx->_parsedNodes[idx];
        ConsoleItemEntry *curEntry = _console_entry_child_find(parentNode->currentEntry, identity, identitySize);
        if (curEntry == NULL)
        {
            LOG_E("identity not exist");
            return false;
        }
        idx++;
        ctx->_parsedNodes[idx].currentEntry = curEntry;
        ctx->_parsedNodes[idx].index = index;
        ctx->_currentParsedNodeIndex = idx;
        return true;
    }
};

static inline token_type _console_next_token(const char *path, char const **token, char const **end, int32_t *size)
{
#define next()       \
    do               \
    {                \
        c = *path++; \
    } while (0)
    char c;
    next();
    while (isblank(c))
    {
        next();
    }
    switch (c)
    {
    case '/':
        *token = path - 1;
        *size = 1;
        *end = path;
        return token_slash;
    case '.':
        *token = path - 1;
        next();
        if (c == '.')
        {
            *size = 2;
            *end = path;
            return token_double_dot;
        }
        else
        {
            path -= 2;
            next();
            *size = 1;
            *end = path;
            return token_dot;
        }

    case '[':
        *token = path - 1;
        *size = 1;
        *end = path;
        return token_lbrace;
    case ']':
        *token = path - 1;
        *size = 1;
        *end = path;
        return token_rbrace;
    case '\0':
        *token = path - 1;
        *size = 1;
        *end = path;
        return token_end;
    default:
        if (isdigit(c))
        {
            *token = path - 1;
            next();
            do
            {
                if (isdigit(c))
                {
                    next();
                }
                else
                {
                    *size = path - 1 - *token;
                    *end = path - 1;
                    return token_index;
                }

            } while (1);
        }
        else if (isalpha(c) || c == '_')
        {
            *token = path - 1;
            next();
            do
            {
                if (isalnum(c) || c == '_')
                {
                    next();
                }
                else
                {
                    *size = path - 1 - *token;
                    *end = path - 1;
                    return token_identity;
                }

            } while (1);
        }
        else
        {
            return token_unknown;
        }
        break;
    }
}

static void _console_init(Console *console)
{
    ConsoleItemEntry *root = console->root;
    root->name = "/";
    console->context.contextNodes[0].currentEntry = root;
    console->context.contextNodes[0].index = -1;
    console->context.contextNodes[0].parentData = NULL;
    console->context.currentContextNodeIndex = 0;

    console->context._parsedNodes[0].currentEntry = root;
    console->context._parsedNodes[0].index = -1;
    console->context._currentParsedNodeIndex = 0;
    console->context._parsedNodeBackTrace = 0;

    console->context.path[0] = '/';
    console->context.path[1] = "\0";
};

static int _console_match_name(const ConsoleItemEntry *entry, const char *name)
{
    return strcmp(entry->name, name);
};
