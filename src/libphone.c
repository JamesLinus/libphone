#include "libphone.h"
#include "libphoneprivate.h"
#include <memory.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>

static phoneApplication phoneAppStruct;
phoneApplication *pApp = &phoneAppStruct;
pthread_mutex_t handleLock;

phoneHandle *pHandle(int handle) {
  assert(handle > 0 && handle <= pApp->handleArrayCapacity);
  assert(phoneGetThreadId() == pApp->mainThreadId);
  return &pApp->handleArray[handle - 1];
}

#define pHandleDebug(handle) pHandle(handle)
#define pHandle(handle) (assert(handle > 0 && \
    handle <= pApp->handleArrayCapacity), pHandleDebug(handle))

int phoneInitApplication(void) {
  memset(pApp, 0, sizeof(phoneApplication));
  pApp->mainThreadId = phoneGetThreadId();
  pthread_mutex_init(&handleLock, NULL);
  pthread_cond_init(&pApp->mainWorkQueue.cond, NULL);
  pthread_mutex_init(&pApp->mainWorkQueue.condLock, NULL);
  pthread_mutex_init(&pApp->mainWorkQueue.threadLock, NULL);
  pApp->mainWorkQueue.threadCount = 1;
  pApp->maxHandleType = PHONE_USER_DEFINED;
  pApp->displayDensity = 1.0;
  return shareInitApplication();
}

int phoneAllocHandleTypeRange(int count) {
  int startHandleType = pApp->maxHandleType + 1;
  pApp->maxHandleType += count;
  return startHandleType;
}

int phoneSetHandleType(int handle, int type) {
  pHandle(handle)->type = type;
  return 0;
}

int phoneGetHandleType(int handle) {
  return pHandle(handle)->type;
}

int phoneSetAppNotificationHandler(phoneAppNotificationHandler *handler) {
  assert(handler);
  pApp->handler = handler;
  return 0;
}

int phoneGetThreadId(void) {
  return shareGetThreadId();
}

int phoneAllocHandle(void) {
  int handle;
  assert(phoneGetThreadId() == pApp->mainThreadId);
  lockAllHandleData();
  if (!pApp->freeHandleLink) {
    int i;
    int newHandleArrayCapacity = (pApp->handleArrayCapacity + 1) * 2;
    phoneHandle *newHandleArray = (phoneHandle *)realloc(pApp->handleArray,
        newHandleArrayCapacity * sizeof(phoneHandle));
    if (!newHandleArray) {
      unlockAllHandleData();
      return 0;
    }
    for (i = pApp->handleArrayCapacity; i < newHandleArrayCapacity; ++i) {
      phoneHandle *handleData = &newHandleArray[i];
      handleData->order = i + 1;
      handleData->type = PHONE_UNKNOWN;
      handleData->next = pApp->freeHandleLink;
      pApp->freeHandleLink = handleData;
    }
    pApp->handleArray = newHandleArray;
    pApp->handleArrayCapacity = newHandleArrayCapacity;
  }
  assert(pApp->inuseHandleCount < pApp->handleArrayCapacity);
  handle = pApp->freeHandleLink->order;
  pApp->freeHandleLink->inuse = 1;
  pApp->freeHandleLink = pApp->freeHandleLink->next;
  ++pApp->inuseHandleCount;
  unlockAllHandleData();
  return handle;
}

phoneHandle *pHandleNoCheck(int handle) {
  assert(handle > 0 && handle <= pApp->handleArrayCapacity);
  return &pApp->handleArray[handle - 1];
}

int phoneFreeHandle(int handle) {
  phoneHandle *handleData;
  lockAllHandleData();
  handleData = pHandle(handle);
  assert(phoneGetThreadId() == pApp->mainThreadId);
  assert(handleData->inuse);
  handleData->inuse = 0;
  handleData->type = PHONE_UNKNOWN;
  --pApp->inuseHandleCount;
  handleData->next = pApp->freeHandleLink;
  pApp->freeHandleLink = handleData;
  unlockAllHandleData();
  return 0;
}

int phoneSetHandleContext(int handle, void *context) {
  pHandle(handle)->context = context;
  return 0;
}

void *phoneGetHandleContext(int handle) {
  return pHandle(handle)->context;
}

int phoneCopyString(char *destBuf, int destSize, const char *src) {
  const char *loopSrc = src;
  int destOffset = 0;
  int maxCopyLen = destSize - 1;
  while (*loopSrc && destOffset < maxCopyLen) {
    destBuf[destOffset++] = *loopSrc;
    ++loopSrc;
  }
  if (destOffset < destSize) {
    destBuf[destOffset] = '\0';
  }
  return destOffset;
}

int phoneLog(int level, const char *tag, const char *fmt, ...) {
  int n;
  char *msgBuf;
  char stackBuf[2048];
  va_list args;
  va_list argsCopy;
  va_start(args, fmt);
  va_copy(argsCopy, args);
  n = vsnprintf(0, 0, fmt, args);
  if (n + 1 > sizeof(stackBuf)) {
    msgBuf = (char *)malloc(n + 1);
  } else {
    msgBuf = stackBuf;
  }
  if (msgBuf) {
    vsnprintf(msgBuf, n + 1, fmt, argsCopy);
    shareDumpLog(level, tag, msgBuf, n);
  }
  if (msgBuf != stackBuf) {
    free(msgBuf);
  }
  va_end(argsCopy);
  va_end(args);
  return n;
}

int phoneSetHandleTag(int handle, void *tag) {
  pHandle(handle)->tag = tag;
  return 0;
}

void *phoneGetHandleTag(int handle) {
  void *tag;
  if (phoneGetThreadId() != pApp->mainThreadId) {
    lockAllHandleData();
    tag = pHandleNoCheck(handle)->tag;
    unlockAllHandleData();
  } else {
    tag = pHandle(handle)->tag;
  }
  return tag;
}

int phoneSleep(unsigned int milliseconds) {
  struct timeval val = {0, milliseconds};
  return select(0, 0, 0, 0, &val);
}

void lockAllHandleData(void) {
  pthread_mutex_lock(&handleLock);
}

void unlockAllHandleData(void) {
  pthread_mutex_unlock(&handleLock);
}

int phoneCreateWorkItem(phoneBackgroundWorkHandler workHandler,
    phoneAfterWorkHandler afterWorkHandler) {
  int handle = phoneAllocHandle();
  phoneHandle *handleData;
  phoneWorkItemContext *workItem;
  if (!handle) {
    return 0;
  }
  handleData = pHandle(handle);
  handleData->type = PHONE_WORK_ITEM;
  workItem = (phoneWorkItemContext *)calloc(1, sizeof(phoneWorkItemContext));
  if (!workItem) {
    phoneFreeHandle(handle);
    return 0;
  }
  workItem->handle = handle;
  workItem->workHandler = workHandler;
  assert(afterWorkHandler);
  workItem->afterWorkHandler = afterWorkHandler;
  handleData->canRemove = 0;
  handleData->context = workItem;
  return handle;
}

int phoneFlushMainWorkQueue(void) {
  phoneWorkItemContext *workItem;
  phoneWorkQueueContext *workQueue = &pApp->mainWorkQueue;
  for (;;) {
    pthread_mutex_lock(&workQueue->threadLock);
    workItem = workQueue->firstNeedFlushItem;
    if (workItem) {
      workQueue->firstNeedFlushItem = workItem->next;
      if (!workQueue->firstNeedFlushItem) {
        workQueue->lastNeedFlushItem = 0;
      }
    }
    pthread_mutex_unlock(&workQueue->threadLock);
    if (!workItem) {
      break;
    }
    pHandle(workItem->handle)->canRemove = 1;
    workItem->afterWorkHandler(workItem->handle);
  }
  return 0;
}

static void *runWorkItem(void *arg) {
  phoneWorkQueueContext *workQueue = &pApp->mainWorkQueue;
  phoneWorkItemContext *workItem;
  shareWorkQueueThreadInit();
  for (;;) {
    if (!workQueue->firstWaitingItem) {
      pthread_mutex_lock(&workQueue->condLock);
      while (!workQueue->firstWaitingItem) {
        pthread_cond_wait(&workQueue->cond, &workQueue->condLock);
      }
      pthread_mutex_unlock(&workQueue->condLock);
    }

    pthread_mutex_lock(&workQueue->threadLock);
    workItem = workQueue->firstWaitingItem;
    if (workItem) {
      workQueue->firstWaitingItem = workItem->next;
      if (!workQueue->firstWaitingItem) {
        workQueue->lastWaitingItem = 0;
      }
    }
    pthread_mutex_unlock(&workQueue->threadLock);

    if (workItem) {
      workItem->workHandler(workItem->handle);
      pthread_mutex_lock(&workQueue->threadLock);
      if (workQueue->lastNeedFlushItem) {
        workQueue->lastNeedFlushItem->next = workItem;
      } else {
        workQueue->firstNeedFlushItem = workItem;
      }
      workQueue->lastNeedFlushItem = workItem;
      workItem->next = 0;
      pthread_mutex_unlock(&workQueue->threadLock);
      shareNeedFlushMainWorkQueue();
    }
  }
  shareWorkQueueThreadUninit();
  return 0;
}

int phoneSetMainWorkQueueThreadCount(int threadCount) {
  assert(threadCount >= 1);
  pApp->mainWorkQueue.threadCount = threadCount;
  return 0;
}

int phonePostToMainWorkQueue(int itemHandle) {
  phoneHandle *itemHandleData;
  phoneWorkQueueContext *workQueue;
  phoneWorkItemContext *workItem;
  lockAllHandleData();
  itemHandleData = pHandleNoCheck(itemHandle);
  assert(PHONE_WORK_ITEM == itemHandleData->type);
  workQueue = (phoneWorkQueueContext *)&pApp->mainWorkQueue;
  workItem = (phoneWorkItemContext *)itemHandleData->context;
  unlockAllHandleData();
  if (!workItem->workHandler) {
    pthread_mutex_lock(&workQueue->threadLock);
    if (workQueue->lastNeedFlushItem) {
      workQueue->lastNeedFlushItem->next = workItem;
    } else {
      workQueue->firstNeedFlushItem = workItem;
    }
    workQueue->lastNeedFlushItem = workItem;
    workItem->next = 0;
    pthread_mutex_unlock(&workQueue->threadLock);
    shareNeedFlushMainWorkQueue();
    return 0;
  }
  pthread_mutex_lock(&workQueue->threadLock);
  if (!workQueue->threadArray) {
    int i;
    int threadCount = workQueue->threadCount;
    workQueue->threadArray = (phoneWorkQueueThread *)calloc(threadCount,
        sizeof(phoneWorkQueueThread));
    if (!workQueue->threadArray) {
      pthread_mutex_unlock(&workQueue->threadLock);
      return -1;
    }
    for (i = 0; i < threadCount; ++i) {
      phoneWorkQueueThread *threadContext =
          (phoneWorkQueueThread *)&workQueue->threadArray[i];
      phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "create new thread");
      pthread_create(&threadContext->thread, 0, runWorkItem, threadContext);
    }
  }
  if (workQueue->lastWaitingItem) {
    workQueue->lastWaitingItem->next = workItem;
  } else {
    workQueue->firstWaitingItem = workItem;
  }
  workQueue->lastWaitingItem = workItem;
  workItem->next = 0;
  pthread_mutex_unlock(&workQueue->threadLock);

  pthread_mutex_lock(&workQueue->condLock);
  pthread_cond_signal(&workQueue->cond);
  pthread_mutex_unlock(&workQueue->condLock);
  return 0;
}

int phoneRemoveTimer(int handle) {
  assert(PHONE_TIMER == pHandle(handle)->type);
  return shareRemoveTimer(handle);
}

int phoneRemoveWorkItem(int handle) {
  phoneHandle *handleData = pHandle(handle);
  assert(PHONE_WORK_ITEM == handleData->type);
  assert(handleData->canRemove);
  handleData->canRemove = 0;
  free(handleData->context);
  handleData->context = 0;
  return 0;
}

int phoneCreateTimer(unsigned int milliseconds,
    phoneTimerRunHandler runHandler) {
  int handle = phoneAllocHandle();
  phoneHandle *handleData;
  if (!handle) {
    return 0;
  }
  handleData = pHandle(handle);
  handleData->type = PHONE_TIMER;
  handleData->u.timer.runHandler = runHandler;
  if (0 != shareCreateTimer(handle, milliseconds)) {
    phoneFreeHandle(handle);
    return 0;
  }
  return handle;
}

int phoneCreateContainerView(int parentHandle,
    phoneViewEventHandler eventHandler) {
  int handle = phoneAllocHandle();
  phoneHandle *handleData;
  if (!handle) {
    return 0;
  }
  handleData = pHandle(handle);
  handleData->type = PHONE_CONTAINER_VIEW;
  handleData->u.view.eventHandler = eventHandler;
  if (0 != shareCreateContainerView(handle, parentHandle)) {
    phoneFreeHandle(handle);
    return 0;
  }
  return handle;
}

int phoneSetViewFrame(int handle, float x, float y, float width, float height) {
  if (0 == handle) {
    return -1;
  }
  return shareSetViewFrame(handle, x, y, width, height);
}

int phoneSetViewBackgroundColor(int handle, unsigned int color) {
  assert(0 == handle || PHONE_CONTAINER_VIEW == pHandle(handle)->type);
  return shareSetViewBackgroundColor(handle, color);
}

int phoneCreateTextView(int parentHandle,
    phoneViewEventHandler eventHandler) {
  int handle = phoneAllocHandle();
  phoneHandle *handleData;
  if (!handle) {
    return 0;
  }
  handleData = pHandle(handle);
  handleData->type = PHONE_TEXT_VIEW;
  handleData->u.view.eventHandler = eventHandler;
  if (0 != shareCreateTextView(handle, parentHandle)) {
    phoneFreeHandle(handle);
    return 0;
  }
  shareSetViewAlign(handle, PHONE_VIEW_ALIGN_CENTER);
  shareSetViewAlign(handle, PHONE_VIEW_VERTICAL_ALIGN_MIDDLE);
  return handle;
}

int phoneSetViewText(int handle, const char *val) {
  return shareSetViewText(handle, val);
}

int phoneSetViewFontColor(int handle, unsigned int color) {
  return shareSetViewFontColor(handle, color);
}

int phoneShowView(int handle, int display) {
  return shareShowView(handle, display);
}

float phoneGetViewWidth(int handle) {
  return shareGetViewWidth(handle);
}

float phoneGetViewHeight(int handle) {
  return shareGetViewHeight(handle);
}

int phoneCreateViewAnimationSet(int duration,
    phoneViewAnimationSetFinishHandler finishHandler) {
  int handle = phoneAllocHandle();
  phoneHandle *handleData;
  if (!handle) {
    return 0;
  }
  handleData = pHandle(handle);
  handleData->next = 0;
  handleData->type = PHONE_VIEW_ANIMATION_SET;
  handleData->u.animationSet.duration = duration;
  handleData->u.animationSet.finishHandler = finishHandler;
  handleData->u.animationSet.total = 0;
  handleData->u.animationSet.finished = 0;
  if (0 != shareCreateViewAnimationSet(handle)) {
    phoneFreeHandle(handle);
    return 0;
  }
  return handle;
}

int phoneAddViewAnimationToSet(int animationHandle, int setHandle) {
  phoneHandle *setHandleData = pHandle(setHandle);
  phoneHandle *aniHandleData = pHandle(animationHandle);
  assert(PHONE_VIEW_ANIMATION_SET == pHandle(setHandle)->type);
  assert(pHandle(animationHandle)->canRemove);
  pHandle(animationHandle)->canRemove = 0;
  setHandleData->u.animationSet.total++;
  aniHandleData->next = setHandleData->next;
  setHandleData->next = (void *)animationHandle;
  aniHandleData->u.animation.animationSetHandle = setHandle;
  return shareAddViewAnimationToSet(animationHandle, setHandle);
}

int phoneBeginViewAnimationSet(int handle) {
  assert(PHONE_VIEW_ANIMATION_SET == pHandle(handle)->type);
  return shareBeginAnimationSet(handle,
      pHandle(handle)->u.animationSet.duration);
}

int phoneCreateViewTranslateAnimation(int viewHandle, float offsetX,
    float offsetY) {
  int handle = phoneAllocHandle();
  phoneHandle *handleData;
  if (!handle) {
    return 0;
  }
  handleData = pHandle(handle);
  handleData->type = PHONE_VIEW_TRANSLATE_ANIMATION;
  handleData->canRemove = 1;
  handleData->next = 0;
  handleData->u.animation.viewHandle = viewHandle;
  handleData->u.animation.u.translate.offsetX = offsetX;
  handleData->u.animation.u.translate.offsetY = offsetY;
  if (0 != shareCreateViewTranslateAnimation(handle, viewHandle,
      offsetX, offsetY)) {
    phoneFreeHandle(handle);
    return 0;
  }
  return handle;
}

int phoneRemoveViewAnimationSet(int handle) {
  int loopHandle = (int)pHandle(handle)->next;
  assert(PHONE_VIEW_ANIMATION_SET == pHandle(handle)->type);
  while (loopHandle) {
    int needRemoveHandle = loopHandle;
    phoneHandle *needRemoveHandleData = pHandle(needRemoveHandle);
    loopHandle = (int)needRemoveHandleData->next;
    needRemoveHandleData->canRemove = 1;
    phoneRemoveViewAnimation(needRemoveHandle);
  }
  return shareRemoveViewAnimationSet(handle);
}

int phoneRemoveViewAnimation(int handle) {
  assert(pHandle(handle)->canRemove);
  return shareRemoveViewAnimation(handle);
}

int phoneCreateViewAlphaAnimation(int viewHandle,
    float fromAlpha, float toAlpha) {
  int handle = phoneAllocHandle();
  phoneHandle *handleData;
  if (!handle) {
    return 0;
  }
  handleData = pHandle(handle);
  handleData->type = PHONE_VIEW_ALPHA_ANIMATION;
  handleData->canRemove = 1;
  handleData->next = 0;
  handleData->u.animation.viewHandle = viewHandle;
  handleData->u.animation.u.alpha.fromAlpha = fromAlpha;
  handleData->u.animation.u.alpha.toAlpha = toAlpha;
  if (0 != shareCreateViewAlphaAnimation(handle, viewHandle,
      fromAlpha, toAlpha)) {
    phoneFreeHandle(handle);
    return 0;
  }
  return handle;
}

int phoneBringViewToFront(int handle) {
  return shareBringViewToFront(handle);
}

int phoneSetViewAlpha(int handle, float alpha) {
  return shareSetViewAlpha(handle, alpha);
}

int phoneSetViewFontSize(int handle, float fontSize) {
  return shareSetViewFontSize(handle, fontSize);
}

int phoneSetViewBackgroundImageResource(int handle,
    const char *imageResource) {
  assert(PHONE_CONTAINER_VIEW == pHandle(handle)->type);
  return shareSetViewBackgroundImageResource(handle, imageResource);
}

int phoneSetViewBackgroundImagePath(int handle,
    const char *imagePath) {
  assert(PHONE_CONTAINER_VIEW == pHandle(handle)->type);
  return shareSetViewBackgroundImagePath(handle, imagePath);
}

int phoneCreateEditTextView(int parentHandle,
    phoneViewEventHandler eventHandler) {
  int handle = phoneAllocHandle();
  phoneHandle *handleData;
  if (!handle) {
    return 0;
  }
  handleData = pHandle(handle);
  handleData->type = PHONE_EDIT_TEXT_VIEW;
  handleData->u.view.eventHandler = eventHandler;
  if (0 != shareCreateEditTextView(handle, parentHandle)) {
    phoneFreeHandle(handle);
    return 0;
  }
  phoneEnableViewEvent(handle, PHONE_VIEW_VALUE_CHANGE);
  return handle;
}

int phoneShowSoftInputOnView(int handle) {
  assert(PHONE_EDIT_TEXT_VIEW == pHandle(handle)->type);
  return shareShowSoftInputOnView(handle);
}

int phoneHideSoftInputOnView(int handle) {
  assert(PHONE_EDIT_TEXT_VIEW == pHandle(handle)->type);
  return shareHideSoftInputOnView(handle);
}

int phoneGetViewText(int handle, char *buf, int bufSize) {
  return shareGetViewText(handle, buf, bufSize);
}

int phoneSetViewInputType(int handle, int inputType) {
  int result = 0;
  switch (inputType) {
    case PHONE_INPUT_TEXT:
      result = shareSetViewInputTypeAsText(handle);
      break;
    case PHONE_INPUT_PASSWORD:
      result = shareSetViewInputTypeAsPassword(handle);
      break;
    case PHONE_INPUT_VISIBLE_PASSWORD:
      result = shareSetViewInputTypeAsVisiblePassword(handle);
      break;
  }
  return result;
}

int phoneEnableViewEvent(int handle, int eventType) {
  int result = 0;
  switch (eventType) {
    case PHONE_VIEW_CLICK:
      result = shareEnableViewClickEvent(handle);
      break;
    case PHONE_VIEW_LONG_CLICK:
      result = shareEnableViewLongClickEvent(handle);
      break;
    case PHONE_VIEW_VALUE_CHANGE:
      result = shareEnableViewValueChangeEvent(handle);
      break;
    case PHONE_VIEW_TOUCH:
      result = shareEnableViewTouchEvent(handle);
      break;
  }
  return result;
}

int phoneSetViewCornerRadius(int handle, float radius) {
  return shareSetViewCornerRadius(handle, radius);
}

int phoneSetViewBorderColor(int handle, unsigned int color) {
  return shareSetViewBorderColor(handle, color);
}

int phoneSetViewBorderWidth(int handle, float width) {
  return shareSetViewBorderWidth(handle, width);
}

int phoneIsLandscape(void) {
  return shareIsLandscape();
}

const char *phoneViewEventTypeToName(int eventType) {
# define XX(code, name) case code: return name;
  switch (eventType) {
    phoneViewEventTypeMap(XX)
    default:
      return "";
  }
# undef XX
}

int phoneSetStatusBarBackgroundColor(unsigned int color) {
  return shareSetStatusBarBackgroundColor(color);
}

int phoneSetViewAlign(int handle, int align) {
  return shareSetViewAlign(handle, align);
}

int phoneSetViewVerticalAlign(int handle, int align) {
  return shareSetViewVerticalAlign(handle, align);
}

int phoneCreateTableView(int parentHandle,
    phoneViewEventHandler eventHandler) {
  int handle = phoneAllocHandle();
  phoneHandle *handleData;
  if (!handle) {
    return 0;
  }
  handleData = pHandle(handle);
  handleData->type = PHONE_TABLE_VIEW;
  handleData->u.view.eventHandler = eventHandler;
  if (0 != shareCreateTableView(handle, parentHandle)) {
    phoneFreeHandle(handle);
    return 0;
  }
  return handle;
}

int shareRequestTableViewCellCustomView(int handle, int section, int row) {
  phoneHandle *handleData = pHandle(handle);
  phoneViewRequestTable request = {section, row};
  return handleData->u.view.eventHandler(handle,
      PHONE_VIEW_REQUEST_TABLE_CELL_CUSTOM_VIEW,
      &request);
}

int shareRequestTableViewCellIdentifier(int handle, int section, int row,
    char *buf, int bufSize) {
  phoneHandle *handleData = pHandle(handle);
  phoneViewRequestTable request = {section, row, buf, bufSize};
  buf[0] = '\0';
  return handleData->u.view.eventHandler(handle,
      PHONE_VIEW_REQUEST_TABLE_CELL_IDENTIFIER,
      &request);
}

int shareRequestTableViewSectionCount(int handle) {
  phoneHandle *handleData = pHandle(handle);
  return handleData->u.view.eventHandler(handle,
      PHONE_VIEW_REQUEST_TABLE_SECTION_COUNT, 0);
}

int shareRequestTableViewRowCount(int handle, int section) {
  phoneHandle *handleData = pHandle(handle);
  phoneViewRequestTable request = {section};
  return handleData->u.view.eventHandler(handle,
      PHONE_VIEW_REQUEST_TABLE_ROW_COUNT,
      &request);
}

int shareRequestTableViewRowHeight(int handle, int section, int row) {
  phoneHandle *handleData = pHandle(handle);
  phoneViewRequestTable request = {section, row};
  return handleData->u.view.eventHandler(handle,
      PHONE_VIEW_REQUEST_TABLE_ROW_HEIGHT,
      &request);
}

int shareRequestTableViewCellIdentifierTypeCount(int handle) {
  phoneHandle *handleData = pHandle(handle);
  return handleData->u.view.eventHandler(handle,
      PHONE_VIEW_REQUEST_TABLE_CELL_IDENTIFIER_TYPE_COUNT, 0);
}

float phoneDipToPix(int dip) {
  return pApp->displayDensity * dip;
}

int phoneReloadTableView(int handle) {
  assert(PHONE_TABLE_VIEW == pHandle(handle)->type);
  return shareReloadTableView(handle);
}

int shareRequestTableViewCellRender(int handle, int section, int row,
    int renderHandle) {
  phoneHandle *handleData = pHandle(handle);
  phoneViewRequestTable request = {section, row, 0, 0, renderHandle};
  return handleData->u.view.eventHandler(handle,
      PHONE_VIEW_REQUEST_TABLE_CELL_RENDER,
      &request);
}

int shareRequestTableViewCellClick(int handle, int section, int row,
    int renderHandle) {
  phoneHandle *handleData = pHandle(handle);
  phoneViewRequestTable request = {section, row, 0, 0, renderHandle};
  return handleData->u.view.eventHandler(handle,
      PHONE_VIEW_REQUEST_TABLE_CELL_CLICK,
      &request);
}

int phoneSetViewShadowColor(int handle, unsigned int color) {
  return shareSetViewShadowColor(handle, color);
}

int phoneSetViewShadowOffset(int handle, float offsetX, float offsetY) {
  return shareSetViewShadowOffset(handle, offsetX, offsetY);
}

int phoneSetViewShadowOpacity(int handle, float opacity) {
  return shareSetViewShadowOpacity(handle, opacity);
}

int phoneSetViewShadowRadius(int handle, float radius) {
  return shareSetViewShadowRadius(handle, radius);
}

int phoneSetViewBackgroundImageRepeat(int handle, int repeat) {
  return shareSetViewBackgroundImageRepeat(handle, repeat);
}

int phoneSetViewFontBold(int handle, int bold) {
  return shareSetViewFontBold(handle, bold);
}

int phoneBeginTableViewRefresh(int handle) {
  return shareBeginTableViewRefresh(handle);
}

int phoneEndTableViewRefresh(int handle) {
  return shareEndTableViewRefresh(handle);
}

int shareRequestTableViewRefresh(int handle) {
  phoneHandle *handleData = pHandle(handle);
  return handleData->u.view.eventHandler(handle,
      PHONE_VIEW_REQUEST_TABLE_REFRESH, 0);
}

int shareRequestTableViewUpdateRefreshView(int handle, int renderHandle) {
  phoneHandle *handleData = pHandle(handle);
  phoneViewRequestTable request = {0, 0, 0, 0, renderHandle};
  return handleData->u.view.eventHandler(handle,
      PHONE_VIEW_REQUEST_TABLE_UPDATE_REFRESH_VIEW,
      &request);
}

int shareRequestTableViewRefreshView(int handle) {
  phoneHandle *handleData = pHandle(handle);
  return handleData->u.view.eventHandler(handle,
      PHONE_VIEW_REQUEST_TABLE_REFRESH_VIEW, 0);
}

float phoneGetTableViewRefreshHeight(int handle) {
  return shareGetTableViewRefreshHeight(handle);
}

float phoneGetTableViewStableRefreshHeight(void) {
  return dp(60);
}

int phoneRotateView(int handle, float degree) {
  return shareRotateView(handle, degree);
}

int phoneSetEditTextViewPlaceholder(int handle, const char *text,
    unsigned int color) {
  return shareSetEditTextViewPlaceholder(handle, text, color);
}

int phoneSetViewEventHandler(int handle, phoneViewEventHandler eventHandler) {
  phoneHandle *handleData = pHandle(handle);
  handleData->u.view.eventHandler = eventHandler;
  return 0;
}

int phoneSetViewParent(int handle, int parentHandle) {
  return shareSetViewParent(handle, parentHandle);
}

int phoneRemoveView(int handle) {
  int result = shareRemoveView(handle);
  phoneFreeHandle(handle);
  return result;
}

int phoneCreateOpenGLView(int parentHandle,
    phoneViewEventHandler eventHandler) {
  int handle = phoneAllocHandle();
  phoneHandle *handleData;
  if (!handle) {
    return 0;
  }
  handleData = pHandle(handle);
  handleData->type = PHONE_OPENGL_VIEW;
  handleData->u.view.eventHandler = eventHandler;
  if (0 != shareCreateOpenGLView(handle, parentHandle)) {
    phoneFreeHandle(handle);
    return 0;
  }
  return handle;
}

int phoneSetOpenGLViewRenderHandler(int handle,
    phoneOpenGLViewRenderHandler renderHandler) {
  phoneHandle *handleData = pHandle(handle);
  assert(PHONE_OPENGL_VIEW == handleData->type);
  handleData->u.view.u.opengl.renderHandler = renderHandler;
  return shareBeginOpenGLViewRender(handle, renderHandler);
}

int phoneCreateThread(const char *threadName,
    phoneThreadRunHandler runHandler) {
  int handle = phoneAllocHandle();
  phoneHandle *handleData;
  if (!handle) {
    return 0;
  }
  handleData = pHandle(handle);
  handleData->type = PHONE_THREAD;
  handleData->u.thread.runHandler = runHandler;
  if (0 != shareCreateThread(handle, threadName)) {
    phoneFreeHandle(handle);
    return 0;
  }
  return handle;
}

int phoneStartThread(int handle) {
  phoneHandle *handleData = pHandle(handle);
  assert(PHONE_THREAD == handleData->type);
  return shareStartThread(handle);
}

int phoneJoinThread(int handle) {
  phoneHandle *handleData = pHandle(handle);
  assert(PHONE_THREAD == handleData->type);
  return shareJoinThread(handle);
}

int phoneRemoveThread(int handle) {
  phoneHandle *handleData = pHandle(handle);
  assert(PHONE_THREAD == handleData->type);
  shareRemoveThread(handle);
  phoneFreeHandle(handle);
  return 0;
}

FILE *phoneOpenAsset(const char *filename) {
  return shareOpenAsset(filename);
}

void *shareMalloc(int size) {
  return malloc(size);
}

void *shareCalloc(int count, int size) {
  return calloc(count, size);
}
