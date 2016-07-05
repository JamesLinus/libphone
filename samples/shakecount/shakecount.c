#include "libphone.h"

#if __ANDROID__
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
   phoneInitJava(vm);
   return JNI_VERSION_1_6;
}
#endif

#define BACKGROUND_COLOR 0xefefef
#define FONT_COLOR 0xffffff
#define FONT_BACKGROUND_COLOR 0x1faece

static int backgroundView = 0;
static int textBackgroundView = 0;
static int textView = 0;

static int shakeSensor = 0;
static int shakeCount = 0;

static void layout(void);

static void appShowing(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "app showing");
}

static void appHiding(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "app hiding");
}

static void appTerminating(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "app terminating");
}

static int appBackClick(void) {
  return PHONE_DONTCARE;
}

static void appLayoutChanging(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "app layoutChanging");
  layout();
}

static void addShakeCount(void) {
  char buf[100];
  ++shakeCount;
  snprintf(buf, sizeof(buf), "Shake Count: %d", shakeCount);
  phoneSetViewText(textView, buf);
}

static int shakeHandler(int handle, int eventType, void *eventParam) {
  if (PHONE_SENSOR_SHAKE == eventType) {
    addShakeCount();
    return PHONE_YES;
  }
  return PHONE_DONTCARE;
}

int phoneMain(int argc, const char *argv[]) {
  static phoneAppNotificationHandler handler = {
    appShowing,
    appHiding,
    appTerminating,
    appBackClick,
    appLayoutChanging,
  };
  phoneSetAppNotificationHandler(&handler);

  phoneSetStatusBarBackgroundColor(BACKGROUND_COLOR);
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "screenSize: %dx%d",
    phoneGetViewWidth(0), phoneGetViewHeight(0));
  backgroundView = phoneCreateContainerView(0, 0);
  phoneSetViewBackgroundColor(backgroundView, BACKGROUND_COLOR);

  textBackgroundView = phoneCreateContainerView(backgroundView, 0);
  phoneSetViewFrame(textBackgroundView, dp(20), phoneGetViewHeight(0) / 2 - dp(31),
    phoneGetViewWidth(0) - dp(40), dp(31));
  phoneSetViewBackgroundColor(textBackgroundView, FONT_BACKGROUND_COLOR);
  phoneSetViewCornerRadius(textBackgroundView, dp(3));

  textView = phoneCreateTextView(textBackgroundView, 0);
  phoneSetViewText(textView, "Shake Count: 0");
  phoneSetViewFontSize(textView, dp(14));
  phoneSetViewFontColor(textView, FONT_COLOR);

  shakeSensor = phoneCreateShakeSensor(shakeHandler);
  phoneStartSensor(shakeSensor);

  layout();
  return 0;
}

static void layout(void) {
  phoneSetViewFrame(backgroundView, 0, 0, phoneGetViewWidth(0),
    phoneGetViewHeight(0));
  phoneSetViewFrame(textView, 0, 0, phoneGetViewWidth(0) - dp(40), dp(31));
}
