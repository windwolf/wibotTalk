#include "../inc/tree_accessor_test.h"
#include "minunit.h"
#include <stdlib.h>
#include "../inc/tree_accessor.h"
#include <string.h>

static void test_access_nil(const void *context, const int32_t index, void *in, void **out, bool isSet);
static void test_accessor_cfg(const void *context, const int32_t index, void *in, void **out, bool isSet);
static void test_accessor_cfg_value(const void *context, const int32_t index, void *in, void **out, bool isSet);
static TreeAccessor cons;
static TreeAccessorItemEntry root = {.name = "/", .accessor = test_access_nil};
static TreeAccessorItemEntry aa = {.name = "aa", .accessor = test_access_nil};
static TreeAccessorItemEntry bbs = {.name = "bbs", .accessor = test_accessor_cfg};
static TreeAccessorItemEntry bf1 = {.name = "bf1", .accessor = test_accessor_cfg_value};
static TreeAccessorItemEntry aa_a1 = {.name = "a1", .accessor = test_access_nil};
static TreeAccessorItemEntry aa_a2 = {.name = "a2", .accessor = test_access_nil};
static TreeAccessorItemEntry aa_a1_f1 = {.name = "f1", .accessor = test_access_nil};
static TreeAccessorItemEntry aa_a1_f2 = {.name = "f2", .accessor = test_access_nil};

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

void tree_accessor_test()
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
    rst = tree_accessor_item_register(&cons, "", &root);
    MU_ASSERT(rst);
    rst = tree_accessor_item_register(&cons, "/", &aa);
    MU_ASSERT(rst);
    rst = tree_accessor_item_register(&cons, "/", &bbs);
    MU_ASSERT(rst);
    rst = tree_accessor_item_register(&cons, "/bbs", &bf1);
    MU_ASSERT(rst);
    rst = tree_accessor_item_register(&cons, "/aa", &aa_a1);
    MU_ASSERT(rst);
    rst = tree_accessor_item_register(&cons, "/cc", &aa_a1);
    MU_ASSERT(!rst);
    rst = tree_accessor_item_register(&cons, "/aa", &aa_a2);
    MU_ASSERT(rst);
    rst = tree_accessor_item_register(&cons, "/aa/a1", &aa_a1_f1);
    MU_ASSERT(rst);
    rst = tree_accessor_item_register(&cons, "/aa/a1", &aa_a1_f2);
    MU_ASSERT(rst);

    TreeAccessorItemEntry *_root = cons.root;
    MU_ASSERT(!strcmp(_root->name, "/"));
    TreeAccessorItemEntry *_aa = (TreeAccessorItemEntry *)_root->base.child;
    MU_ASSERT(!strcmp(_aa->name, "aa"));
    TreeAccessorItemEntry *_bbs = (TreeAccessorItemEntry *)_aa->base.next;
    MU_ASSERT(!strcmp(_bbs->name, "bbs"));
    TreeAccessorItemEntry *_bf1 = (TreeAccessorItemEntry *)_bbs->base.child;
    MU_ASSERT(!strcmp(_bf1->name, "bf1"));
    TreeAccessorItemEntry *_aa_a1 = (TreeAccessorItemEntry *)_aa->base.child;
    MU_ASSERT(!strcmp(_aa_a1->name, "a1"));
    TreeAccessorItemEntry *_aa_a2 = (TreeAccessorItemEntry *)_aa_a1->base.next;
    MU_ASSERT(!strcmp(_aa_a2->name, "a2"));
    TreeAccessorItemEntry *_aa_a1_f1 = (TreeAccessorItemEntry *)_aa_a1->base.child;
    MU_ASSERT(!strcmp(_aa_a1_f1->name, "f1"));
    TreeAccessorItemEntry *_aa_a1_f2 = (TreeAccessorItemEntry *)_aa_a1_f1->base.next;
    MU_ASSERT(!strcmp(_aa_a1_f2->name, "f2"));
};

static void cd_test1()
{
    bool rst;
    MU_ASSERT(cons.context.currentContextNodeIndex == 0);
    MU_ASSERT(!strcmp(tree_accessor_context_path_get(&cons), "/"));

    rst = tree_accessor_context_change(&cons, "/aa");
    MU_ASSERT(rst);
    MU_ASSERT(cons.context.currentContextNodeIndex == 1);
    MU_ASSERT(!strcmp(tree_accessor_context_path_get(&cons), "/aa"));

    rst = tree_accessor_context_change(&cons, "..");
    MU_ASSERT(rst);
    MU_ASSERT(cons.context.currentContextNodeIndex == 0);
    MU_ASSERT(!strcmp(tree_accessor_context_path_get(&cons), "/"));

    rst = tree_accessor_context_change(&cons, "/aa/a1");
    MU_ASSERT(rst);
    MU_ASSERT(cons.context.currentContextNodeIndex == 2);
    MU_ASSERT(!strcmp(tree_accessor_context_path_get(&cons), "/aa/a1"));

    rst = tree_accessor_context_change(&cons, ".");
    MU_ASSERT(rst);
    MU_ASSERT(cons.context.currentContextNodeIndex == 2);
    MU_ASSERT(!strcmp(tree_accessor_context_path_get(&cons), "/aa/a1"));

    rst = tree_accessor_context_change(&cons, "f1");
    MU_ASSERT(rst);
    MU_ASSERT(cons.context.currentContextNodeIndex == 3);
    MU_ASSERT(!strcmp(tree_accessor_context_path_get(&cons), "/aa/a1/f1"));

    rst = tree_accessor_context_change(&cons, "../../a2");
    MU_ASSERT(rst);
    MU_ASSERT(cons.context.currentContextNodeIndex == 2);
    MU_ASSERT(!strcmp(tree_accessor_context_path_get(&cons), "/aa/a2"));

    rst = tree_accessor_context_change(&cons, "../a1/f2");
    MU_ASSERT(rst);
    MU_ASSERT(cons.context.currentContextNodeIndex == 3);
    MU_ASSERT(!strcmp(tree_accessor_context_path_get(&cons), "/aa/a1/f2"));

    rst = tree_accessor_context_change(&cons, "/");
    MU_ASSERT(rst);
    MU_ASSERT(cons.context.currentContextNodeIndex == 0);
    MU_ASSERT(!strcmp(tree_accessor_context_path_get(&cons), "/"));
};

static void set_test1()
{
    bool rst;
    void *value;
    rst = tree_accessor_value_set(&cons, "/aa/a1/f1", &value);
    MU_ASSERT(rst);
    rst = tree_accessor_value_set(&cons, "/aa/a1", &value);
    MU_ASSERT(rst);
    rst = tree_accessor_value_set(&cons, "/aa", &value);
    MU_ASSERT(rst);
    rst = tree_accessor_value_set(&cons, "/", &value);
    MU_ASSERT(rst);
    rst = tree_accessor_value_set(&cons, "/cc", &value);
    MU_ASSERT(!rst);
    rst = tree_accessor_value_set(&cons, "/aa/cc", &value);
    MU_ASSERT(!rst);
    rst = tree_accessor_value_set(&cons, "/aa/a1/cc", &value);
    MU_ASSERT(!rst);
    rst = tree_accessor_value_set(&cons, "/cc/a1/f1/cc", &value);
    MU_ASSERT(!rst);
    rst = tree_accessor_value_set(&cons, "/cc/cc", &value);
    MU_ASSERT(!rst);
};
static void get_test1()
{
    bool rst;
    void *value;
    rst = tree_accessor_value_get(&cons, "/aa/a1/f1", &value);
    MU_ASSERT(rst);
    rst = tree_accessor_value_get(&cons, "/aa/a1", &value);
    MU_ASSERT(rst);
    rst = tree_accessor_value_get(&cons, "/aa", &value);
    MU_ASSERT(rst);
    rst = tree_accessor_value_get(&cons, "/", &value);
    MU_ASSERT(rst);
    rst = tree_accessor_value_get(&cons, "/cc", &value);
    MU_ASSERT(!rst);
    rst = tree_accessor_value_get(&cons, "/aa/cc", &value);
    MU_ASSERT(!rst);
    rst = tree_accessor_value_get(&cons, "/aa/a1/cc", &value);
    MU_ASSERT(!rst);
    rst = tree_accessor_value_get(&cons, "/cc/a1/f1/cc", &value);
    MU_ASSERT(!rst);
    rst = tree_accessor_value_get(&cons, "/cc/cc", &value);
    MU_ASSERT(!rst);
};

static void li_test1()
{
    bool rst;
    char **li;
    rst = tree_accessor_context_change(&cons, "/");
    li = tree_accessor_item_list(&cons, ".");
    MU_ASSERT(li != NULL);
    MU_ASSERT(!strcmp(li[0], "aa"));
    MU_ASSERT(!strcmp(li[1], "bbs"));
    MU_ASSERT(li[2] == NULL);

    li = tree_accessor_item_list(&cons, "aa");
    MU_ASSERT(li != NULL);
    MU_ASSERT(!strcmp(li[0], "a1"));
    MU_ASSERT(!strcmp(li[1], "a2"));
    MU_ASSERT(li[2] == NULL);

    li = tree_accessor_item_list(&cons, "aa/a1");
    MU_ASSERT(li != NULL);
    MU_ASSERT(!strcmp(li[0], "f1"));
    MU_ASSERT(!strcmp(li[1], "f2"));
    MU_ASSERT(li[2] == NULL);

    rst = tree_accessor_context_change(&cons, "/aa");
    li = tree_accessor_item_list(&cons, ".");
    MU_ASSERT(li != NULL);
    MU_ASSERT(!strcmp(li[0], "a1"));
    MU_ASSERT(!strcmp(li[1], "a2"));
    MU_ASSERT(li[2] == NULL);

    li = tree_accessor_item_list(&cons, "/");
    MU_ASSERT(li != NULL);
    MU_ASSERT(!strcmp(li[0], "aa"));
    MU_ASSERT(!strcmp(li[1], "bbs"));
    MU_ASSERT(li[2] == NULL);

    li = tree_accessor_item_list(&cons, "a1");
    MU_ASSERT(li != NULL);
    MU_ASSERT(!strcmp(li[0], "f1"));
    MU_ASSERT(!strcmp(li[1], "f2"));
    MU_ASSERT(li[2] == NULL);
};

static void cd_test2()
{
    bool rst;
    rst = tree_accessor_context_change(&cons, "/");
    MU_ASSERT(rst);
    MU_ASSERT(cons.context.currentContextNodeIndex == 0);
    MU_ASSERT(!strcmp(tree_accessor_context_path_get(&cons), "/"));
};
static void set_test2()
{

    bool rst;
    int value = 1000;
    rst = tree_accessor_value_set(&cons, "/bbs[0]/bf1", &value);
    MU_ASSERT(rst);
    MU_ASSERT(test_cfg_data[0].f1 == 1000);

    value = 2000;
    rst = tree_accessor_value_set(&cons, "/bbs[1]/bf1", &value);
    MU_ASSERT(rst);
    MU_ASSERT(test_cfg_data[1].f1 == 2000);
};
static void get_test2()
{
    bool rst;
    int *value;
    rst = tree_accessor_value_get(&cons, "/bbs[0]/bf1", &value);
    MU_ASSERT(rst);
    MU_ASSERT(*value == 1000);

    rst = tree_accessor_value_get(&cons, "/bbs[1]/bf1", &value);
    MU_ASSERT(rst);
    MU_ASSERT(*value == 2000);
};

static void cd_test3()
{
    bool rst;
    rst = tree_accessor_context_change(&cons, "/bbs[0]");
    MU_ASSERT(rst);
    MU_ASSERT(cons.context.currentContextNodeIndex == 1);
    MU_ASSERT(!strcmp(tree_accessor_context_path_get(&cons), "/bbs[0]"));
};

static void set_test3()
{
    bool rst;
    int value = 3000;
    rst = tree_accessor_value_set(&cons, "bf1", &value);
    MU_ASSERT(rst);
    MU_ASSERT(test_cfg_data[0].f1 == 3000);
};

static void get_test3()
{
    bool rst;
    int *value;
    rst = tree_accessor_value_get(&cons, "bf1", &value);
    MU_ASSERT(rst);
    MU_ASSERT(*value == 3000);
};
