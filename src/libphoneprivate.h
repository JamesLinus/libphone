#ifndef __LIBPHONEPRIVATE_H__
#define __LIBPHONEPRIVATE_H__
#include "libphone.h"
#include <pthread.h>

enum phoneHandleType {
  PHONE_UNKNOWN = 0,
  PHONE_TIMER = 1,
  PHONE_WORK_ITEM = 2,
  PHONE_CONTAINER_VIEW = 3,
  PHONE_TEXT_VIEW = 4,
  PHONE_VIEW_ANIMATION_SET = 5,
  PHONE_VIEW_TRANSLATE_ANIMATION = 6,
  PHONE_VIEW_ALPHA_ANIMATION = 7,
  PHONE_EDIT_TEXT_VIEW = 8,
  PHONE_TABLE_VIEW = 9,
  PHONE_USER_DEFINED = 10000
};

typedef struct phoneWorkQueueThread {
  pthread_t thread;
} phoneWorkQueueThread;

typedef struct phoneWorkItemContext {
  int handle;
  phoneBackgroundWorkHandler workHandler;
  phoneAfterWorkHandler afterWorkHandler;
  struct phoneWorkItemContext *next;
} phoneWorkItemContext;

typedef struct phoneWorkQueueContext {
  int threadCount;
  pthread_mutex_t condLock;
  pthread_mutex_t threadLock;
  pthread_cond_t cond;
  phoneWorkItemContext *firstWaitingItem;
  phoneWorkItemContext *lastWaitingItem;
  phoneWorkItemContext *firstNeedFlushItem;
  phoneWorkItemContext *lastNeedFlushItem;
  phoneWorkQueueThread *threadArray;
} phoneWorkQueueContext;

#define PHONE_MAX_VIEW_STACK_DEPTH 10

typedef struct phoneHandle {
  int order;
  int type;
  void *tag;
  void *context;
  struct phoneHandle *next;
  int inuse:1;
  int canRemove:1;
  union {
    struct {
      phoneViewEventHandler eventHandler;
    } view;
    struct {
      phoneTimerRunHandler runHandler;
    } timer;
    struct {
      int viewHandle;
      int animationSetHandle;
      union {
        struct {
          float offsetX;
          float offsetY;
        } translate;
        struct {
          float fromAlpha;
          float toAlpha;
        } alpha;
      } u;
    } animation;
    struct {
      int duration;
      phoneViewAnimationSetFinishHandler finishHandler;
      int total;
      int finished;
    } animationSet;
  } u;
} phoneHandle;

typedef struct phoneApplication {
  phoneAppNotificationHandler *handler;
  phoneHandle *freeHandleLink;
  phoneHandle *handleArray;
  int handleArrayCapacity;
  int inuseHandleCount;
  int mainThreadId;
  int mainWorkQueueThreadCount;
  phoneWorkQueueContext mainWorkQueue;
  volatile int needFlushMainWorkQueue;
  int maxHandleType;
  float displayDensity;
} phoneApplication;

extern phoneApplication *pApp;

int phoneInitApplication(void);
phoneHandle *pHandle(int handle);
phoneHandle *pHandleNoCheck(int handle);
void lockAllHandleData(void);
void unlockAllHandleData(void);
int phoneFlushMainWorkQueue(void);
int shareDumpLog(int level, const char *tag, const char *log, int len);
int shareNeedFlushMainWorkQueue(void);
int shareInitApplication(void);
int shareCreateTimer(int handle, unsigned int milliseconds);
int shareRemoveTimer(int handle);
int shareCreateContainerView(int handle, int parentHandle);
int shareSetViewBackgroundColor(int handle, unsigned int color);
int shareSetViewFrame(int handle, float x, float y, float width, float height);
int shareCreateTextView(int handle, int parentHandle);
int shareSetViewText(int handle, const char *val);
int shareSetViewFontColor(int handle, unsigned int color);
int shareShowView(int handle, int display);
float shareGetViewWidth(int handle);
float shareGetViewHeight(int handle);
int shareCreateViewAnimationSet(int handle);
int shareAddViewAnimationToSet(int animationHandle, int setHandle);
int shareRemoveViewAnimationSet(int handle);
int shareRemoveViewAnimation(int handle);
int shareCreateViewTranslateAnimation(int handle, int viewHandle,
    float offsetX, float offsetY);
int shareBeginAnimationSet(int handle, int duration);
int shareCreateViewAlphaAnimation(int handle, int viewHandle,
    float fromAlpha, float toAlpha);
int shareBringViewToFront(int handle);
int shareSetViewAlpha(int handle, float alpha);
int shareSetViewFontSize(int handle, float fontSize);
int shareSetViewBackgroundImageResource(int handle,
    const char *imageResource);
int shareSetViewBackgroundImagePath(int handle,
    const char *imagePath);
int shareCreateEditTextView(int handle, int parentHandle);
int shareShowSoftInputOnView(int handle);
int shareHideSoftInputOnView(int handle);
int shareGetViewText(int handle, char *buf, int bufSize);
int shareSetViewInputTypeAsVisiblePassword(int handle);
int shareSetViewInputTypeAsPassword(int handle);
int shareSetViewInputTypeAsText(int handle);
int shareEnableViewClickEvent(int handle);
int shareEnableViewLongClickEvent(int handle);
int shareEnableViewValueChangeEvent(int handle);
int shareEnableViewTouchEvent(int handle);
int shareGetThreadId(void);
int shareSetViewCornerRadius(int handle, float radius);
int shareSetViewBorderColor(int handle, unsigned int color);
int shareSetViewBorderWidth(int handle, float width);
int shareIsLandscape(void);
int shareSetStatusBarBackgroundColor(unsigned int color);
int shareSetViewAlign(int handle, int align);
int shareSetViewVerticalAlign(int handle, int align);
int shareRequestTableViewCellDetailText(int handle, int section, int row,
    char *buf, int bufSize);
int shareRequestTableViewCellText(int handle, int section, int row,
    char *buf, int bufSize);
int shareRequestTableViewCellSelectionStyle(int handle, int section, int row);
int shareRequestTableViewCellImageResource(int handle, int section, int row,
    char *buf, int bufSize);
int shareRequestTableViewCellSeparatorStyle(int handle);
int shareRequestTableViewCellAccessoryView(int handle, int section, int row);
int shareRequestTableViewCellCustomView(int handle, int section, int row);
int shareRequestTableViewCellIdentifier(int handle, int section, int row,
    char *buf, int bufSize);
int shareRequestTableViewSectionCount(int handle);
int shareRequestTableViewRowCount(int handle, int section);
int shareRequestTableViewRowHeight(int handle, int section, int row);
int shareRequestTableViewSectionHeader(int handle, int section,
    char *buf, int bufSize);
int shareRequestTableViewSectionFooter(int handle, int section,
    char *buf, int bufSize);
int shareRequestTableViewCellIdentifierTypeCount(int handle);
int shareCreateTableView(int style, int handle, int parentHandle);
int shareReloadTableView(int handle);
int shareRequestTableViewCellRender(int handle, int section, int row,
    int renderHandle);
int shareRequestTableViewCellClick(int handle, int section, int row,
    int renderHandle);
int shareSetViewShadowColor(int handle, unsigned int color);
int shareSetViewShadowOffset(int handle, float offsetX, float offsetY);
int shareSetViewShadowOpacity(int handle, float opacity);
int shareSetViewShadowRadius(int handle, float radius);
int shareSetViewBackgroundImageRepeat(int handle, int repeat);

#endif
