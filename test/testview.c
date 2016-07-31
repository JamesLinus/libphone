#include "test.h"

void testContainerViewChangeParent(testItem *item) {
  int handle = phoneCreateContainerView(0, 0);
  int newParent = phoneCreateContainerView(0, 0);
  phoneSetViewParent(handle, newParent);
  if (newParent == phoneGetViewParent(handle)) {
    testSucceed(item);
  } else {
    testFail(item, "should be %d but %d", newParent,
      phoneGetViewParent(handle));
  }
}

void testRemoveView(testItem *item) {
  int handle = phoneCreateContainerView(0, 0);
  phoneRemoveView(handle);
  testSucceed(item);
}

typedef struct testToggleStatusBarContext {
  testItem *item;
  int heightWithStatusBarPresented;
  int heightWithStatusBarHidden;
  int firstDelay;
  int secondDelay;
} testToggleStatusBarContext;

static void checkHeightWithStatusBarPresented(int handle) {
  void *tag = phoneGetHandleTag(handle);
  testToggleStatusBarContext *ctx = (testToggleStatusBarContext *)tag;
  int currentHeight = (int)phoneGetViewHeight(0);
  phoneRemoveTimer(handle);
  if (ctx->heightWithStatusBarPresented == currentHeight) {
    testSucceed(ctx->item);
    free(ctx);
    return;
  }
  testFail(ctx->item,
    "heightWithStatusBarPresented %d but currentHeight %d",
    ctx->heightWithStatusBarPresented,
    currentHeight);
  free(ctx);
}

static void checkHeightWithStatusBarHidden(int handle) {
  void *tag = phoneGetHandleTag(handle);
  testToggleStatusBarContext *ctx = (testToggleStatusBarContext *)tag;
  ctx->heightWithStatusBarHidden = (int)phoneGetViewHeight(0);
  phoneRemoveTimer(handle);
  if (ctx->heightWithStatusBarHidden > ctx->heightWithStatusBarPresented &&
      (ctx->heightWithStatusBarHidden -
        ctx->heightWithStatusBarPresented) < dp(50)) {
    phoneShowStatusBar(1);
    ctx->secondDelay = phoneCreateTimer(1000, checkHeightWithStatusBarPresented);
    phoneSetHandleTag(ctx->secondDelay, ctx);
    return;
  }
  testFail(ctx->item,
    "heightWithStatusBarPresented %d but heightWithStatusBarHidden %d",
    ctx->heightWithStatusBarPresented,
    ctx->heightWithStatusBarHidden);
  free(ctx);
}

void testToggleStatusBar(testItem *item) {
  testToggleStatusBarContext *ctx = (testToggleStatusBarContext *)calloc(1,
    sizeof(testToggleStatusBarContext));
  ctx->item = item;
  ctx->heightWithStatusBarPresented = (int)phoneGetViewHeight(0);
  phoneShowStatusBar(0);
  ctx->firstDelay = phoneCreateTimer(1000, checkHeightWithStatusBarHidden);
  phoneSetHandleTag(ctx->firstDelay, ctx);
}

typedef struct testToggleOrientationContext {
  testItem *item;
  int delayTask;
} testToggleOrientationContext;

static void checkIfOrientationIsLandscape(int handle) {
  void *tag = phoneGetHandleTag(handle);
  testToggleOrientationContext *ctx = (testToggleOrientationContext *)tag;
  phoneRemoveTimer(ctx->delayTask);
  if (phoneIsLandscape()) {
    testSucceed(ctx->item);
    free(ctx);
    return;
  }
  testFail(ctx->item,
    "want landscape but portrait presented");
  free(ctx);
}

static void checkIfOrientationIsPortrait(int handle) {
  void *tag = phoneGetHandleTag(handle);
  testToggleOrientationContext *ctx = (testToggleOrientationContext *)tag;
  phoneRemoveTimer(ctx->delayTask);
  if (!phoneIsLandscape()) {
    phoneForceOrientation(PHONE_ORIENTATION_SETTING_LANDSCAPE);
    ctx->delayTask = phoneCreateTimer(1000, checkIfOrientationIsLandscape);
    phoneSetHandleTag(ctx->delayTask, ctx);
    return;
  }
  testFail(ctx->item,
    "want portrait but landscape presented");
  free(ctx);
}

void testToggleOrientation(testItem *item) {
  testToggleOrientationContext *ctx = (testToggleOrientationContext *)calloc(1,
    sizeof(testToggleOrientationContext));
  ctx->item = item;
  phoneForceOrientation(PHONE_ORIENTATION_SETTING_PORTRAIT);
  ctx->delayTask = phoneCreateTimer(1000, checkIfOrientationIsPortrait);
  phoneSetHandleTag(ctx->delayTask, ctx);
}
