#include "../inc/console_test.h"
#include "minunit.h"
#include <stdlib.h>
#include "../inc/console.h"
#include <string.h>

static void test_access_nil(const void *context, const int32_t index, void *in, void **out, bool isSet);
static void test_accessor_cfg(const void *context, const int32_t index, void *in, void **out, bool isSet);
static void test_accessor_cfg_value(const void *context, const int32_t index, void *in, void **out, bool isSet);
static Console cons;
static ConsoleItemEntry root = {.name = "/", .type = CONSOLE_ITEM_CONTAINER, .accessor = test_access_nil};
static ConsoleItemEntry aa = {.name = "aa", .type = CONSOLE_ITEM_CONTAINER, .accessor = test_access_nil};
static ConsoleItemEntry bbs = {.name = "bbs", .type = CONSOLE_ITEM_CONTAINER, .accessor = test_accessor_cfg};
static ConsoleItemEntry bf1 = {.name = "bf1", .type = CONSOLE_ITEM_CONTAINER, .accessor = test_accessor_cfg_value};
static ConsoleItemEntry aa_a1 = {.name = "a1", .type = CONSOLE_ITEM_CONTAINER, .accessor = test_access_nil};
static ConsoleItemEntry aa_a2 = {.name = "a2", .type = CONSOLE_ITEM_CONTAINER, .accessor = test_access_nil};
static ConsoleItemEntry aa_a1_f1 = {.name = "f1", .type = CONSOLE_ITEM_CONTAINER, .accessor = test_access_nil};
static ConsoleItemEntry aa_a1_f2 = {.name = "f2", .type = CONSOLE_ITEM_CONTAINER, .accessor = test_access_nil};

static void register_test1();

static void get_test1();
static void get_test2();
static void get_test3();

static void set_test1();
static void set_test2();
static void set_test3();

static void cd_test1();
static void cd_test2();
static void cd_test3();

static void li_test1();
static void li_test2();
static void li_test3();

typedef struct
{
    int f1;
} test_data;

static test_data test_cfg_data[2];

static void test_access_nil(const void *context, const int32_t index, void *in, void **out, bool isSet)
{
    return;
}

static void test_accessor_cfg(const void *context, const int32_t index, void *in, void **out, bool isSet)
{
    *out = &test_cfg_data[index];
    return;
}

static void test_accessor_cfg_value(const void *context, const int32_t index, void *in, void **out, bool isSet)
{
    if (isSet)
    {
        test_data *d = (test_data *)context;
        d->f1 = *((int *)in);
    }
    else
    {
        test_data *d = (test_data *)context;
        *out = &d->f1;
    }

    return;
}

void console_test()
{
    register_test1();

    cd_test1();
    set_test1();
    get_test1();
    li_test1();

    cd_test2();
    set_test2();
    get_test2();

    cd_test3();
    set_test3();
    get_test3();
};

static void register_test1()
{
    bool rst;
    rst = console_item_register(&cons, "", &root);
    assert(rst);
    rst = console_item_register(&cons, "/", &aa);
    assert(rst);
    rst = console_item_register(&cons, "/", &bbs);
    assert(rst);
    rst = console_item_register(&cons, "/bbs", &bf1);
    assert(rst);
    rst = console_item_register(&cons, "/aa", &aa_a1);
    assert(rst);
    rst = console_item_register(&cons, "/cc", &aa_a1);
    assert(!rst);
    rst = console_item_register(&cons, "/aa", &aa_a2);
    assert(rst);
    rst = console_item_register(&cons, "/aa/a1", &aa_a1_f1);
    assert(rst);
    rst = console_item_register(&cons, "/aa/a1", &aa_a1_f2);
    assert(rst);

    ConsoleItemEntry *_root = cons.root;
    assert(!strcmp(_root->name, "/"));
    ConsoleItemEntry *_aa = (ConsoleItemEntry *)_root->base.child;
    assert(!strcmp(_aa->name, "aa"));
    ConsoleItemEntry *_bbs = (ConsoleItemEntry *)_aa->base.next;
    assert(!strcmp(_bbs->name, "bbs"));
    ConsoleItemEntry *_bf1 = (ConsoleItemEntry *)_bbs->base.child;
    assert(!strcmp(_bf1->name, "bf1"));
    ConsoleItemEntry *_aa_a1 = (ConsoleItemEntry *)_aa->base.child;
    assert(!strcmp(_aa_a1->name, "a1"));
    ConsoleItemEntry *_aa_a2 = (ConsoleItemEntry *)_aa_a1->base.next;
    assert(!strcmp(_aa_a2->name, "a2"));
    ConsoleItemEntry *_aa_a1_f1 = (ConsoleItemEntry *)_aa_a1->base.child;
    assert(!strcmp(_aa_a1_f1->name, "f1"));
    ConsoleItemEntry *_aa_a1_f2 = (ConsoleItemEntry *)_aa_a1_f1->base.next;
    assert(!strcmp(_aa_a1_f2->name, "f2"));
};

static void cd_test1()
{
    bool rst;
    assert(cons.context.currentContextNodeIndex == 0);
    assert(!strcmp(console_context_path_get(&cons), "/"));

    rst = console_context_change(&cons, "/aa");
    assert(rst);
    assert(cons.context.currentContextNodeIndex == 1);
    assert(!strcmp(console_context_path_get(&cons), "/aa"));

    rst = console_context_change(&cons, "..");
    assert(rst);
    assert(cons.context.currentContextNodeIndex == 0);
    assert(!strcmp(console_context_path_get(&cons), "/"));

    rst = console_context_change(&cons, "/aa/a1");
    assert(rst);
    assert(cons.context.currentContextNodeIndex == 2);
    assert(!strcmp(console_context_path_get(&cons), "/aa/a1"));

    rst = console_context_change(&cons, ".");
    assert(rst);
    assert(cons.context.currentContextNodeIndex == 2);
    assert(!strcmp(console_context_path_get(&cons), "/aa/a1"));

    rst = console_context_change(&cons, "f1");
    assert(rst);
    assert(cons.context.currentContextNodeIndex == 3);
    assert(!strcmp(console_context_path_get(&cons), "/aa/a1/f1"));

    rst = console_context_change(&cons, "../../a2");
    assert(rst);
    assert(cons.context.currentContextNodeIndex == 2);
    assert(!strcmp(console_context_path_get(&cons), "/aa/a2"));

    rst = console_context_change(&cons, "../a1/f2");
    assert(rst);
    assert(cons.context.currentContextNodeIndex == 3);
    assert(!strcmp(console_context_path_get(&cons), "/aa/a1/f2"));

    rst = console_context_change(&cons, "/");
    assert(rst);
    assert(cons.context.currentContextNodeIndex == 0);
    assert(!strcmp(console_context_path_get(&cons), "/"));
};

static void set_test1()
{
    bool rst;
    void *value;
    rst = console_value_set(&cons, "/aa/a1/f1", &value);
    assert(rst);
    rst = console_value_set(&cons, "/aa/a1", &value);
    assert(rst);
    rst = console_value_set(&cons, "/aa", &value);
    assert(rst);
    rst = console_value_set(&cons, "/", &value);
    assert(rst);
    rst = console_value_set(&cons, "/cc", &value);
    assert(!rst);
    rst = console_value_set(&cons, "/aa/cc", &value);
    assert(!rst);
    rst = console_value_set(&cons, "/aa/a1/cc", &value);
    assert(!rst);
    rst = console_value_set(&cons, "/cc/a1/f1/cc", &value);
    assert(!rst);
    rst = console_value_set(&cons, "/cc/cc", &value);
    assert(!rst);
};
static void get_test1()
{
    bool rst;
    void *value;
    rst = console_value_get(&cons, "/aa/a1/f1", &value);
    assert(rst);
    rst = console_value_get(&cons, "/aa/a1", &value);
    assert(rst);
    rst = console_value_get(&cons, "/aa", &value);
    assert(rst);
    rst = console_value_get(&cons, "/", &value);
    assert(rst);
    rst = console_value_get(&cons, "/cc", &value);
    assert(!rst);
    rst = console_value_get(&cons, "/aa/cc", &value);
    assert(!rst);
    rst = console_value_get(&cons, "/aa/a1/cc", &value);
    assert(!rst);
    rst = console_value_get(&cons, "/cc/a1/f1/cc", &value);
    assert(!rst);
    rst = console_value_get(&cons, "/cc/cc", &value);
    assert(!rst);
};

static void li_test1()
{
    bool rst;
    char **li;
    rst = console_context_change(&cons, "/");
    li = console_item_list(&cons, ".");
    assert(li != NULL);
    assert(!strcmp(li[0], "aa"));
    assert(!strcmp(li[1], "bbs"));
    assert(li[2] == NULL);

    li = console_item_list(&cons, "aa");
    assert(li != NULL);
    assert(!strcmp(li[0], "a1"));
    assert(!strcmp(li[1], "a2"));
    assert(li[2] == NULL);

    li = console_item_list(&cons, "aa/a1");
    assert(li != NULL);
    assert(!strcmp(li[0], "f1"));
    assert(!strcmp(li[1], "f2"));
    assert(li[2] == NULL);

    rst = console_context_change(&cons, "/aa");
    li = console_item_list(&cons, ".");
    assert(li != NULL);
    assert(!strcmp(li[0], "a1"));
    assert(!strcmp(li[1], "a2"));
    assert(li[2] == NULL);

    li = console_item_list(&cons, "/");
    assert(li != NULL);
    assert(!strcmp(li[0], "aa"));
    assert(!strcmp(li[1], "bbs"));
    assert(li[2] == NULL);

    li = console_item_list(&cons, "a1");
    assert(li != NULL);
    assert(!strcmp(li[0], "f1"));
    assert(!strcmp(li[1], "f2"));
    assert(li[2] == NULL);
};

static void cd_test2()
{
    bool rst;
    rst = console_context_change(&cons, "/");
    assert(rst);
    assert(cons.context.currentContextNodeIndex == 0);
    assert(!strcmp(console_context_path_get(&cons), "/"));
};
static void set_test2()
{

    bool rst;
    int value = 1000;
    rst = console_value_set(&cons, "/bbs[0]/bf1", &value);
    assert(rst);
    assert(test_cfg_data[0].f1 == 1000);

    value = 2000;
    rst = console_value_set(&cons, "/bbs[1]/bf1", &value);
    assert(rst);
    assert(test_cfg_data[1].f1 == 2000);
};
static void get_test2()
{
    bool rst;
    int *value;
    rst = console_value_get(&cons, "/bbs[0]/bf1", &value);
    assert(rst);
    assert(*value == 1000);

    rst = console_value_get(&cons, "/bbs[1]/bf1", &value);
    assert(rst);
    assert(*value == 2000);
};

static void cd_test3()
{
    bool rst;
    rst = console_context_change(&cons, "/bbs[0]");
    assert(rst);
    assert(cons.context.currentContextNodeIndex == 1);
    assert(!strcmp(console_context_path_get(&cons), "/bbs[0]"));
};
static void set_test3()
{
    bool rst;
    int value = 3000;
    rst = console_value_set(&cons, "bf1", &value);
    assert(rst);
    assert(test_cfg_data[0].f1 == 3000);
};
static void get_test3()
{
    bool rst;
    int *value;
    rst = console_value_get(&cons, "bf1", &value);
    assert(rst);
    assert(*value == 3000);
};
