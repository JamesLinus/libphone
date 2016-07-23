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
