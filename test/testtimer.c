#include "test.h"
#include <time.h>

typedef struct testTimerWillTriggerInOneSecondsContext {
  testItem *item;
  int startTime;
} testTimerWillTriggerInOneSecondsContext;

static void timerTriggeredInOneSeconds(int handle) {
  int timeOffset;
  testTimerWillTriggerInOneSecondsContext *ctx;
  ctx = (testTimerWillTriggerInOneSecondsContext *)phoneGetHandleTag(handle);
  timeOffset = abs(time(0) - ctx->startTime);
  if (timeOffset <= 2) {
    testSucceed(ctx->item);
  } else {
    testFail(ctx->item, "time offset %s is too large", timeOffset);
  }
  phoneRemoveTimer(handle);
  free(ctx);
}

void testTimerWillTriggerInOneSeconds(testItem *item) {
  int handle;
  testTimerWillTriggerInOneSecondsContext *ctx;
  ctx = calloc(1, sizeof(testTimerWillTriggerInOneSecondsContext));
  ctx->item = item;
  ctx->startTime = time(0);
  handle = phoneCreateTimer(1000, timerTriggeredInOneSeconds);
  phoneSetHandleTag(handle, ctx);
}
