#include "test.h"
#include <sys/time.h>

typedef struct testGetThreadIdContext {
  testItem *item;
  int mainThreadId;
  int workThreadId;
  int work;
} testGetThreadIdContext;

static void runGetThreadIdInBackgroundThread(int handle) {
  testGetThreadIdContext *ctx =
    (testGetThreadIdContext *)phoneGetHandleTag(handle);
  ctx->workThreadId = phoneGetThreadId();
}

static void afterRunGetThreadId(int handle) {
  testGetThreadIdContext *ctx =
    (testGetThreadIdContext *)phoneGetHandleTag(handle);
  phoneRemoveWorkItem(ctx->work);
  if (ctx->workThreadId == ctx->mainThreadId) {
    testFail(ctx->item,
      "work thread id equal to main thread id(%d)",
      ctx->mainThreadId);
    free(ctx);
    return;
  }
  if (0 == ctx->workThreadId) {
    testFail(ctx->item,
      "phoneGetThreadId return 0");
    free(ctx);
    return;
  }
  if (ctx->mainThreadId != phoneGetThreadId()) {
    testFail(ctx->item,
      "get different thread id in main thread");
    free(ctx);
    return;
  }
  testSucceed(ctx->item);
  free(ctx);
}

void testGetThreadId(testItem *item) {
  testGetThreadIdContext *ctx = (testGetThreadIdContext *)calloc(1,
    sizeof(testGetThreadIdContext));
  ctx->item = item;
  ctx->mainThreadId = phoneGetThreadId();
  if (0 == ctx->mainThreadId) {
    testFail(ctx->item,
      "phoneGetThreadId return 0");
    free(ctx);
    return;
  }
  ctx->work = phoneCreateWorkItem(runGetThreadIdInBackgroundThread,
    afterRunGetThreadId);
  phoneSetHandleTag(ctx->work, ctx);
  phonePostToMainWorkQueue(ctx->work);
}

typedef struct testSleepContext {
  testItem *item;
  int work;
  unsigned long long beforeSleep;
  unsigned long long afterSleep;
} testSleepContext;

static unsigned long long now(void) {
  struct timeval time;
  unsigned long long millisecs;
  gettimeofday(&time, 0);
  millisecs = ((unsigned long long)time.tv_sec * 1000) + (time.tv_usec / 1000);
  return millisecs;
}

static void runSleepInBackgroundThread(int handle) {
  testSleepContext *ctx =
    (testSleepContext *)phoneGetHandleTag(handle);
  ctx->beforeSleep = now();
  phoneSleep(1895);
  ctx->afterSleep = now();
}

static void afterRunSleep(int handle) {
  testSleepContext *ctx =
    (testSleepContext *)phoneGetHandleTag(handle);
  phoneRemoveWorkItem(ctx->work);
  if (ctx->afterSleep - ctx->beforeSleep > 2500 ||
      ctx->afterSleep - ctx->beforeSleep < 1500) {
    testFail(ctx->item,
      "want sleep 1895ms, but %dms",
      ctx->afterSleep - ctx->beforeSleep);
    free(ctx);
    return;
  }
  testSucceed(ctx->item);
  free(ctx);
}

void testSleep(testItem *item) {
  testSleepContext *ctx = (testSleepContext *)calloc(1,
    sizeof(testSleepContext));
  ctx->item = item;
  ctx->work = phoneCreateWorkItem(runSleepInBackgroundThread,
    afterRunSleep);
  phoneSetHandleTag(ctx->work, ctx);
  phonePostToMainWorkQueue(ctx->work);
}

void testCopyString(testItem *item) {
  char buf[2];
  int len = phoneCopyString(buf, sizeof(buf), "hello");
  if (1 != len || 'h' != buf[0] || '\0' != buf[1]) {
    testFail(item,
      "copy string failed");
    return;
  }
  testSucceed(item);
}
