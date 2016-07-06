#ifndef __TEST_H__
#define __TEST_H__
#include "libphone.h"
#include <assert.h>

#define TEST_TAG "TEST[libphone]"

typedef struct testContext testContext;
typedef struct testItem testItem;

typedef int (*testItemHandler)(testItem *item);

void testSetItemTag(testItem *item, void *tag);
void *testGetItemTag(testItem *item);
void testSetItemAppNotificationHandler(testItem *item,
    phoneAppNotificationHandler *handler);
void testAddItem(const char *testName, testItemHandler testHandler);
void testSucceed(testItem *item);
void testFail(testItem *item, const char *fmt, ...);
#define testAssert(x) assert(x)

#endif
