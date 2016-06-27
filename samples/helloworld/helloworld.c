#include "libphone.h"

#define BACKGROUND_COLOR 0xefefef
#define FONT_COLOR 0xffffff
#define FONT_BACKGROUND_COLOR 0x1faece

static int backgroundView = 0;
static int textBackgroundView = 0;
static int textView = 0;

static int animationSet = 0;
static int delayedTask = 0;

static void appShowing(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "app showing");
}

static void appHiding(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "app hiding");
}

static void appTerminating(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "app terminating");
}

static void onAnimationSetFinish(int handle) {
  phoneRemoveViewAnimationSet(handle);
  animationSet = 0;
}

static void doCpuIntensiveWorkInBackgroundThread(int handle) {
  void *tag = phoneGetHandleTag(handle);
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "get user defined data:%d", (int)tag);
}

static void disposeAtUiThread(int handle) {
  animationSet = phoneCreateViewAnimationSet(500, onAnimationSetFinish);
  phoneAddViewAnimationToSet(
      phoneCreateViewTranslateAnimation(textBackgroundView, 0, -50),
      animationSet);
  phoneBeginViewAnimationSet(animationSet);

  phoneRemoveWorkItem(handle);
}

static int onTextBackgroundViewEvent(int handle, int eventType, void *param) {
    phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "onTextBackgroundViewEvent:%s",
             phoneViewEventTypeToName(eventType));
    return PHONE_DONTCARE;
}

int phoneMain(int argc, const char *argv[]) {
  static phoneAppNotificationHandler handler = {
    appShowing,
    appHiding,
    appTerminating
  };
  phoneSetAppNotificationHandler(&handler);

  phoneSetStatusBarBackgroundColor(BACKGROUND_COLOR);
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "screenSize: %dx%d",
    phoneGetViewWidth(0), phoneGetViewHeight(0));
  backgroundView = phoneCreateContainerView(0, 0);
  phoneSetViewFrame(backgroundView, 0, 0, phoneGetViewWidth(0),
    phoneGetViewHeight(0));
  phoneSetViewBackgroundColor(backgroundView, BACKGROUND_COLOR);

  textBackgroundView = phoneCreateContainerView(backgroundView,
    onTextBackgroundViewEvent);
  phoneSetViewFrame(textBackgroundView, phoneDipToPix(20), phoneGetViewHeight(0) / 2 - phoneDipToPix(31),
    phoneGetViewWidth(0) - phoneDipToPix(40), phoneDipToPix(31));
  phoneSetViewBackgroundColor(textBackgroundView, FONT_BACKGROUND_COLOR);
  phoneSetViewCornerRadius(textBackgroundView, phoneDipToPix(3));
  //phoneSetViewBorderColor(textBackgroundView, 0xff0000);
  //phoneSetViewBorderWidth(textBackgroundView, phoneDipToPix(1));
  phoneEnableViewEvent(textBackgroundView, PHONE_VIEW_TOUCH);

  phoneSetViewShadowColor(textBackgroundView, 0x00ffff);
  phoneSetViewShadowOffset(textBackgroundView, dp(0), dp(0));
  phoneSetViewShadowRadius(textBackgroundView, dp(20));
  phoneSetViewShadowOpacity(textBackgroundView, 1);

  textView = phoneCreateTextView(textBackgroundView, 0);
  phoneSetViewText(textView, "Hello World");
  phoneSetViewFontSize(textView, phoneDipToPix(14));
  phoneSetViewFrame(textView, 0, 0, phoneGetViewWidth(0) - phoneDipToPix(40), phoneDipToPix(31));
  phoneSetViewFontColor(textView, FONT_COLOR);

  /*
  delayedTask = phoneCreateWorkItem(doCpuIntensiveWorkInBackgroundThread,
    disposeAtUiThread);
  phoneSetHandleTag(delayedTask, (void *)123456);
  phonePostToMainWorkQueue(delayedTask);*/

  return 0;
}
