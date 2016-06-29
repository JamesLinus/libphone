#include "libphone.h"
#include <assert.h>

#if __ANDROID__
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
   phoneInitJava(vm);
   return JNI_VERSION_1_6;
}
#endif

#define ANIMATION_DURATION 200

#define MAX_STACK_DEPTH  10
static int viewStack[MAX_STACK_DEPTH];
static int top = -1;
static int isMoving = 0;

static void push(int view);
static int pop(void);

static void layout(void);

#define PAGE_NUM 5
typedef struct pageContext {
  int index;
  int backgroundView;
  int pushButton;
  int pushText;
  int popButton;
  int popText;
} pageContext;
static pageContext *allPages[PAGE_NUM];

static int onPushButtonEvent(int handle, int eventType, void *param) {
  int pageIndex = (int)phoneGetHandleTag(handle);
  pageContext *page = allPages[pageIndex];
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__,
    "[%d] onPushButtonEvent eventType:%s", pageIndex, phoneViewEventTypeToName(eventType));
  switch (eventType) {
    case PHONE_VIEW_TOUCH: {
      phoneViewTouch *touch = (phoneViewTouch *)param;
      if (PHONE_VIEW_TOUCH_BEGIN == touch->touchType ||
          PHONE_VIEW_TOUCH_MOVE == touch->touchType) {
        phoneSetViewBackgroundColor(page->pushButton, 0x0e873e);
      } else {
        phoneSetViewBackgroundColor(page->pushButton, 0x21c064);
      }
    } break;
    case PHONE_VIEW_CLICK: {
      if (!isMoving) {
        if (pageIndex + 1 < PAGE_NUM) {
          push(allPages[pageIndex + 1]->backgroundView);
        }
      }
    } break;
  }
  return PHONE_DONTCARE;
}

static int onPopButtonEvent(int handle, int eventType, void *param) {
  int pageIndex = (int)phoneGetHandleTag(handle);
  pageContext *page = allPages[pageIndex];
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__,
    "[%d] onPopButtonEvent eventType:%s", pageIndex, phoneViewEventTypeToName(eventType));
  switch (eventType) {
    case PHONE_VIEW_TOUCH: {
      phoneViewTouch *touch = (phoneViewTouch *)param;
      if (PHONE_VIEW_TOUCH_BEGIN == touch->touchType ||
          PHONE_VIEW_TOUCH_MOVE == touch->touchType) {
        phoneSetViewBackgroundColor(page->popButton, 0x0e873e);
      } else {
        phoneSetViewBackgroundColor(page->popButton, 0x21c064);
      }
    } break;
    case PHONE_VIEW_CLICK: {
      if (!isMoving) {
        pop();
      }
    } break;
  }
  return PHONE_DONTCARE;
}

#define BUTTON_WIDTH ((phoneGetViewWidth(0) - dp(20)) / 2)
#define BUTTON_HEIGHT dp(35)
#define BUTTON_CORNEL_RADIUS dp(7)

static unsigned int chooseColor(int index) {
  static const int colors[] = {
    0xaaaaaa, 0xb4eff7, 0x292d33, 0xfbfae6, 0xacbbd2, 0xf8f7ed
  };
  return colors[index % (sizeof(colors) / sizeof(colors[0]))];
}

static void layoutPage(pageContext *page) {
  float offsetY = dp(20) + (BUTTON_HEIGHT + dp(5)) * 2 * page->index;
  phoneSetViewFrame(page->backgroundView, 0, 0,
    phoneGetViewWidth(0), phoneGetViewHeight(0));

  phoneSetViewFrame(page->pushButton,
    (phoneGetViewWidth(0) - BUTTON_WIDTH) / 2,
    offsetY,
    BUTTON_WIDTH,
    BUTTON_HEIGHT);
  phoneSetViewFrame(page->pushText,
    0, 0, BUTTON_WIDTH, BUTTON_HEIGHT);

  phoneSetViewFrame(page->popButton,
    (phoneGetViewWidth(0) - BUTTON_WIDTH) / 2,
    offsetY + BUTTON_HEIGHT + dp(5),
    BUTTON_WIDTH,
    BUTTON_HEIGHT);
  phoneSetViewFrame(page->popText,
    0, 0, BUTTON_WIDTH, BUTTON_HEIGHT);
}

static pageContext *createPage(int index, unsigned int backgroundColor) {
  pageContext *page = (pageContext *)calloc(1, sizeof(pageContext));
  page->index = index;

  page->backgroundView = phoneCreateContainerView(0, 0);

  page->pushButton = phoneCreateContainerView(page->backgroundView,
      onPushButtonEvent);
  phoneEnableViewEvent(page->pushButton, PHONE_VIEW_TOUCH);
  phoneEnableViewEvent(page->pushButton, PHONE_VIEW_CLICK);
  page->pushText = phoneCreateTextView(page->pushButton, 0);
  phoneSetViewText(page->pushText, "PUSH");
  phoneSetViewFontColor(page->pushText, 0xffffff);
  phoneSetViewBackgroundColor(page->pushButton, 0x21c064);
  phoneSetViewCornerRadius(page->pushButton, BUTTON_CORNEL_RADIUS);
  phoneSetHandleTag(page->pushButton, index);

  page->popButton = phoneCreateContainerView(page->backgroundView,
      onPopButtonEvent);
  phoneEnableViewEvent(page->popButton, PHONE_VIEW_TOUCH);
  phoneEnableViewEvent(page->popButton, PHONE_VIEW_CLICK);
  page->popText = phoneCreateTextView(page->popButton, 0);
  phoneSetViewText(page->popText, "POP");
  phoneSetViewFontColor(page->popText, 0xffffff);
  phoneSetViewBackgroundColor(page->popButton, 0x21c064);
  phoneSetViewCornerRadius(page->popButton, BUTTON_CORNEL_RADIUS);
  phoneSetHandleTag(page->popButton, index);

  phoneSetViewBackgroundColor(page->backgroundView, backgroundColor);
  phoneShowView(page->backgroundView, 0);
  phoneSetHandleTag(page->backgroundView, index);
  layoutPage(page);
  return page;
}

static void onAnimationSetFinish(int handle) {
  int gone = (int)phoneGetHandleTag(handle);
  phoneRemoveViewAnimationSet(handle);
  isMoving = 0;
}

static void applyPushPageAnimation(int gone, int come) {
  int set = phoneCreateViewAnimationSet(ANIMATION_DURATION,
      onAnimationSetFinish);
  int pageWidth = phoneGetViewWidth(0);
  int pageHeight = phoneGetViewHeight(0);
  int moveOut;
  int moveIn;
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__,
    "applyPushPageAnimation gone:%d come:%d", phoneGetHandleTag(gone),
    phoneGetHandleTag(come));
  phoneShowView(gone, 1);
  phoneShowView(come, 1);
  phoneSetViewFrame(gone,
      0, 0, pageWidth, pageHeight);
  phoneSetViewFrame(come,
      -pageWidth, 0, pageWidth, pageHeight);
  moveOut = phoneCreateViewTranslateAnimation(gone,
      pageWidth, 0);
  moveIn = phoneCreateViewTranslateAnimation(come,
      pageWidth, 0);
  phoneAddViewAnimationToSet(moveOut, set);
  phoneAddViewAnimationToSet(moveIn, set);
  phoneSetHandleTag(set, gone);
  isMoving = 1;
  phoneBeginViewAnimationSet(set);
}

static void applyPopPageAnimation(int gone, int come) {
  int set = phoneCreateViewAnimationSet(ANIMATION_DURATION,
      onAnimationSetFinish);
  int pageWidth = phoneGetViewWidth(0);
  int pageHeight = phoneGetViewHeight(0);
  int moveOut;
  int moveIn;
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__,
    "applyPopPageAnimation gone:%d come:%d", phoneGetHandleTag(gone),
    phoneGetHandleTag(come));
  phoneShowView(gone, 1);
  phoneShowView(come, 1);
  phoneSetViewFrame(gone,
      0, 0, pageWidth, pageHeight);
  phoneSetViewFrame(come,
      pageWidth, 0, pageWidth, pageHeight);
  moveOut = phoneCreateViewTranslateAnimation(gone,
      -pageWidth, 0);
  moveIn = phoneCreateViewTranslateAnimation(come,
      -pageWidth, 0);
  phoneAddViewAnimationToSet(moveOut, set);
  phoneAddViewAnimationToSet(moveIn, set);
  phoneSetHandleTag(set, gone);
  isMoving = 1;
  phoneBeginViewAnimationSet(set);
}

static void push(int view) {
  assert(top + 1 < MAX_STACK_DEPTH);
  viewStack[++top] = view;
  if (top - 1 >= 0) {
    applyPushPageAnimation(viewStack[top - 1], viewStack[top]);
  } else {
    phoneShowView(viewStack[top], 1);
  }
}

static int pop(void) {
  if (top - 1 >= 0) {
    --top;
    applyPopPageAnimation(viewStack[top + 1], viewStack[top]);
    return viewStack[top];
  }
  return 0;
}

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
  if (pop()) {
    return PHONE_YES;
  }
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

  {
    int i;
    for (i = 0; i < PAGE_NUM; ++i) {
      allPages[i] = createPage(i, chooseColor(i));
    }
  }
  push(allPages[0]->backgroundView);

  return 0;
}

static void layout(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "display size: %f x %f",
    phoneGetViewWidth(0), phoneGetViewHeight(0));
  {
    int i;
    for (i = 0; i < PAGE_NUM; ++i) {
      layoutPage(allPages[i]);
    }
  }
}
