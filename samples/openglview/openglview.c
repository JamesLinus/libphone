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

#define BUTTON_HEIGHT dp(31)

static int backgroundView = 0;
static int textBackgroundView = 0;
static int textView = 0;
static int openGLView = 0;

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

static int onTextBackgroundViewEvent(int handle, int eventType, void *param) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "onTextBackgroundViewEvent:%s",
           phoneViewEventTypeToName(eventType));
  return PHONE_DONTCARE;
}

static void renderGameFrame(int handle) {
  glClearColor(((FONT_BACKGROUND_COLOR & 0xff0000) >> 16) / 256.0,
    ((FONT_BACKGROUND_COLOR & 0x00ff00) >> 8) / 256.0,
    (FONT_BACKGROUND_COLOR & 0x0000ff) / 256.0,
    0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "renderThreadId: %d",
    phoneGetThreadId());
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
  backgroundView = phoneCreateContainerView(0, 0);
  phoneSetViewBackgroundColor(backgroundView, BACKGROUND_COLOR);

  textBackgroundView = phoneCreateContainerView(backgroundView,
    onTextBackgroundViewEvent);
  phoneSetViewBackgroundColor(textBackgroundView, FONT_BACKGROUND_COLOR);
  phoneEnableViewEvent(textBackgroundView, PHONE_VIEW_TOUCH);

  phoneSetViewShadowColor(textBackgroundView, 0x00ffff);
  phoneSetViewShadowOffset(textBackgroundView, dp(0), dp(0));
  phoneSetViewShadowRadius(textBackgroundView, dp(20));
  phoneSetViewShadowOpacity(textBackgroundView, 1);

  textView = phoneCreateTextView(textBackgroundView, 0);
  phoneSetViewText(textView, "Start Game");
  phoneSetViewFontSize(textView, phoneDipToPix(14));
  phoneSetViewFontColor(textView, FONT_COLOR);

  openGLView = phoneCreateOpenGLView(0, 0);
  phoneSetOpenGLViewRenderHandler(openGLView, renderGameFrame);
  layout();

  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "mainThreadId: %d",
    phoneGetThreadId());

  return 0;
}

static void layout(void) {
  float openGLViewWidth;
  float openGLViewHeight;
  float margin;
  float openGLViewTop;
  float padding = dp(10);

  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "screenSize: %dx%d",
    phoneGetViewWidth(0), phoneGetViewHeight(0));

  if (phoneGetViewWidth(0) < phoneGetViewHeight(0)) {
    margin = dp(20);
    openGLViewWidth = phoneGetViewWidth(0) - margin * 2;
    openGLViewHeight = openGLViewWidth * 320 / 480;
  } else {
    margin = dp(10);
    openGLViewHeight = phoneGetViewHeight(0) - padding -
      BUTTON_HEIGHT - margin * 2;
    openGLViewWidth = openGLViewHeight * 480 / 320;
  }
  openGLViewTop = (phoneGetViewHeight(0) -
    openGLViewHeight - padding - BUTTON_HEIGHT) / 2;
  phoneSetViewFrame(openGLView, margin, openGLViewTop,
    openGLViewWidth, openGLViewHeight);

  phoneSetViewFrame(textView, 0, 0, openGLViewWidth, BUTTON_HEIGHT);

  phoneSetViewFrame(textBackgroundView, margin,
    openGLViewTop + openGLViewHeight + padding,
    openGLViewWidth, BUTTON_HEIGHT);

  phoneSetViewFrame(backgroundView, 0, 0, phoneGetViewWidth(0),
    phoneGetViewHeight(0));
}
