#include "libphone.h"
#include "layout.h"

#if __ANDROID__
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
   phoneInitJava(vm);
   return JNI_VERSION_1_6;
}
#endif

static signInPage *page = 0;

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
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "app orientationChanging");
  layout();
}

int phoneMain(int argc, const char *argv[]) {
  static phoneAppNotificationHandler handler = {
    appShowing,
    appHiding,
    appTerminating,
    appBackClick,
    appLayoutChanging
  };
  phoneSetAppNotificationHandler(&handler);

  page = signInPageCreate(0);

  return 0;
}

static void layout(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "display size: %f x %f",
    phoneGetViewWidth(0), phoneGetViewHeight(0));
  signInPageLayout(page);
}
