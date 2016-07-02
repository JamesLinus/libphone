#include "libphone.h"
#include "libphoneprivate.h"
#include <assert.h>
#include <libphoneprivate.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

static JavaVM *loadedVm = 0;
static jobject *activity = 0;
static int phoneInited = 0;
static int nativeInited = 0;
static pthread_cond_t notifyMainThreadCond;
static pthread_mutex_t notifyMainThreadMutex;
static AAssetManager *assetManager = 0;

int shareInitApplication(void) {
  pthread_mutex_init(&notifyMainThreadMutex, 0);
  pthread_cond_init(&notifyMainThreadCond, 0);
  return 0;
}

JNIEnv *phoneGetJNIEnv(void) {
  JNIEnv *env;
  if ((*loadedVm)->GetEnv(loadedVm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) {
    return 0;
  }
  return env;
}

#define NOTIFY_NEED_FLUSH_MAIN_WORK_QUEUE 0x00000001

JNIEXPORT jint nativeNotifyMainThread(JNIEnv *env, jobject obj,
    jlong needNotifyMask) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "nativeNotifyMainThread called");
  if (needNotifyMask & NOTIFY_NEED_FLUSH_MAIN_WORK_QUEUE) {
    phoneFlushMainWorkQueue();
  }
  return 0;
}

JNIEXPORT jint nativeInit(JNIEnv *env, jobject obj) {
  if (!nativeInited) {
    activity = (*env)->NewGlobalRef(env, obj);
    phoneInitApplication();
    nativeInited = 1;
  }
  return 0;
}

JNIEXPORT jint nativeInitAssetManager(JNIEnv *env, jobject obj, jobject mgr) {
  assetManager = AAssetManager_fromJava(env, (*env)->NewGlobalRef(env, mgr));
  return 0;
}

JNIEXPORT jint nativeSendAppShowing(JNIEnv *env, jobject obj) {
  if (!phoneInited) {
    phoneMain(0, 0);
    phoneInited = 1;
  }
  pApp->handler->showing();
  return 0;
}

JNIEXPORT jint nativeSendAppHiding(JNIEnv *env, jobject obj) {
  pApp->handler->hiding();
  return 0;
}

JNIEXPORT jint nativeSendAppTerminating(JNIEnv *env, jobject obj) {
  pApp->handler->terminating();
  return 0;
}

JNIEXPORT jint nativeInvokeTimer(JNIEnv *env, jobject obj, jint handle) {
  assert(PHONE_TIMER == pHandle(handle)->type);
  pHandle(handle)->u.timer.runHandler(handle);
  return 0;
}

JNIEXPORT jint nativeInvokeNotifyThread(JNIEnv *env, jobject obj) {
  jobject thread = (*env)->NewGlobalRef(env, obj);
  for (;;) {
    unsigned int needFlushMask = 0;
    phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "enter nativeInvokeNotifyThread");
    if (pApp->needFlushMainWorkQueue) {
      pApp->needFlushMainWorkQueue = 0;
      needFlushMask |= NOTIFY_NEED_FLUSH_MAIN_WORK_QUEUE;
    }
    if (needFlushMask) {
      jint result;
      phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "call javaNotifyMainThread");
      phoneCallJavaReturnInt(result, env, thread,
          "javaNotifyMainThread", "(J)I",
          (jlong)needFlushMask);
    }
    pthread_mutex_lock(&notifyMainThreadMutex);
    phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "wait new notify");
    pthread_cond_wait(&notifyMainThreadCond, &notifyMainThreadMutex);
    phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "new notify is coming");
    pthread_mutex_unlock(&notifyMainThreadMutex);
  }
  return 0;
}

JNIEXPORT jint nativeAnimationFinished(JNIEnv *env, jobject obj, jint handle) {
  phoneHandle *handleData = pHandle(handle);
  phoneHandle *setHandleData;
  int setHandle;
  setHandle = handleData->u.animation.animationSetHandle;
  setHandleData = pHandle(setHandle);
  setHandleData->u.animationSet.finished++;
  if (setHandleData->u.animationSet.finished ==
      setHandleData->u.animationSet.total) {
    setHandleData->u.animationSet.finishHandler(setHandle);
  }
  return 0;
}

JNIEXPORT jint nativeDispatchViewClickEvent(JNIEnv *env, jobject obj,
    jint handle) {
  phoneHandle *handleData = pHandle(handle);
  if (handleData->u.view.eventHandler) {
    return handleData->u.view.eventHandler(handle, PHONE_VIEW_CLICK, 0);
  }
  return PHONE_DONTCARE;
}

JNIEXPORT jint nativeDispatchViewLongClickEvent(JNIEnv *env, jobject obj,
    jint handle) {
  phoneHandle *handleData = pHandle(handle);
  if (handleData->u.view.eventHandler) {
    return handleData->u.view.eventHandler(handle, PHONE_VIEW_LONG_CLICK, 0);
  }
  return PHONE_DONTCARE;
}

JNIEXPORT jint nativeDispatchViewValueChangeEvent(JNIEnv *env, jobject obj,
    jint handle) {
  phoneHandle *handleData = pHandle(handle);
  if (handleData->u.view.eventHandler) {
    return handleData->u.view.eventHandler(handle, PHONE_VIEW_VALUE_CHANGE, 0);
  }
  return PHONE_DONTCARE;
}

static void makeTouch(phoneViewTouch *touch, int type, int x, int y) {
  memset(touch, 0, sizeof(phoneViewTouch));
  touch->touchType = type;
  touch->x = x;
  touch->y = y;
}

JNIEXPORT jint nativeDispatchViewTouchBeginEvent(JNIEnv *env, jobject obj,
    jint handle, int x, int y) {
  phoneHandle *handleData = pHandle(handle);
  if (handleData->u.view.eventHandler) {
    phoneViewTouch touch;
    makeTouch(&touch, PHONE_VIEW_TOUCH_BEGIN, x, y);
    return handleData->u.view.eventHandler(handle, PHONE_VIEW_TOUCH, &touch);
  }
  return PHONE_DONTCARE;
}

JNIEXPORT jint nativeDispatchViewTouchEndEvent(JNIEnv *env, jobject obj,
    jint handle, int x, int y) {
  phoneHandle *handleData = pHandle(handle);
  if (handleData->u.view.eventHandler) {
    phoneViewTouch touch;
    makeTouch(&touch, PHONE_VIEW_TOUCH_END, x, y);
    return handleData->u.view.eventHandler(handle, PHONE_VIEW_TOUCH, &touch);
  }
  return PHONE_DONTCARE;
}

JNIEXPORT jint nativeDispatchViewTouchMoveEvent(JNIEnv *env, jobject obj,
    jint handle, int x, int y) {
  phoneHandle *handleData = pHandle(handle);
  if (handleData->u.view.eventHandler) {
    phoneViewTouch touch;
    makeTouch(&touch, PHONE_VIEW_TOUCH_MOVE, x, y);
    return handleData->u.view.eventHandler(handle, PHONE_VIEW_TOUCH, &touch);
  }
  return PHONE_DONTCARE;
}

JNIEXPORT jint nativeDispatchViewTouchCancelEvent(JNIEnv *env, jobject obj,
    jint handle, int x, int y) {
  phoneHandle *handleData = pHandle(handle);
  if (handleData->u.view.eventHandler) {
    phoneViewTouch touch;
    makeTouch(&touch, PHONE_VIEW_TOUCH_CANCEL, x, y);
    return handleData->u.view.eventHandler(handle, PHONE_VIEW_TOUCH, &touch);
  }
  return PHONE_DONTCARE;
}

JNIEXPORT jint nativeInitDensity(JNIEnv *env, jobject obj, jfloat density) {
  pApp->displayDensity = density;
  return 0;
}

JNIEXPORT jint nativeRequestTableViewCellCustomView(JNIEnv *env, jobject obj,
    jint handle, jint section, jint row) {
  return shareRequestTableViewCellCustomView(handle, section, row);
}

JNIEXPORT jstring nativeRequestTableViewCellIdentifier(JNIEnv *env, jobject obj,
    jint handle, jint section, jint row) {
  char buf[4096];
  shareRequestTableViewCellIdentifier(handle, section, row, buf, sizeof(buf));
  return (*env)->NewStringUTF(env, buf);
}

JNIEXPORT jint nativeRequestTableViewSectionCount(JNIEnv *env, jobject obj,
    jint handle) {
  return shareRequestTableViewSectionCount(handle);
}

JNIEXPORT jint nativeRequestTableViewRowCount(JNIEnv *env, jobject obj,
    jint handle, jint section) {
  return shareRequestTableViewRowCount(handle, section);
}

JNIEXPORT jint nativeRequestTableViewRowHeight(JNIEnv *env, jobject obj,
    jint handle, jint section, jint row) {
  return shareRequestTableViewRowHeight(handle, section, row);
}

JNIEXPORT jint nativeRequestTableViewCellIdentifierTypeCount(JNIEnv *env,
    jobject obj, jint handle) {
  return shareRequestTableViewCellIdentifierTypeCount(handle);
}

JNIEXPORT jint nativeRequestTableViewCellRender(JNIEnv *env, jobject obj,
    jint handle, jint section, jint row, jint renderHandle) {
  return shareRequestTableViewCellRender(handle, section, row, renderHandle);
}

JNIEXPORT jint nativeSendAppBackClick(JNIEnv *env, jobject obj) {
  if (pApp->handler->backClick) {
    return pApp->handler->backClick();
  }
  return PHONE_DONTCARE;
}

JNIEXPORT jint nativeRequestTableViewRefresh(JNIEnv *env, jobject obj,
    int handle) {
  return shareRequestTableViewRefresh(handle);
}

JNIEXPORT jint nativeRequestTableViewUpdateRefreshView(JNIEnv *env, jobject obj,
    int handle, int renderHandle) {
  return shareRequestTableViewUpdateRefreshView(handle, renderHandle);
}

JNIEXPORT jint nativeRequestTableViewRefreshView(JNIEnv *env, jobject obj,
    int handle) {
  return shareRequestTableViewRefreshView(handle);
}

JNIEXPORT jint nativeSendAppLayoutChanging(JNIEnv *env, jobject obj,
    int handle) {
  if (pApp->handler->layoutChanging) {
    pApp->handler->layoutChanging();
  }
  return 0;
}

JNIEXPORT jint nativeInvokeOpenGLViewRender(JNIEnv *env, jobject obj,
    int handle, jlong func) {
  ((phoneOpenGLViewRenderHandler)func)(handle);
  return 0;
}

JNIEXPORT jint nativeInvokeThread(JNIEnv *env, jobject obj, jint handle,
    jlong func) {
  ((phoneThreadRunHandler)func)(handle);
  return 0;
}

void phoneInitJava(JavaVM *vm) {
  JNIEnv *env;
  jclass objClass;
  JNINativeMethod nativeMethods[] = {
    {"nativeSendAppShowing", "()I", nativeSendAppShowing},
    {"nativeSendAppHiding", "()I", nativeSendAppHiding},
    {"nativeSendAppTerminating", "()I", nativeSendAppTerminating},
    {"nativeSendAppBackClick", "()I", nativeSendAppBackClick},
    {"nativeInvokeTimer", "(I)I", nativeInvokeTimer},
    {"nativeNotifyMainThread", "(J)I", nativeNotifyMainThread},
    {"nativeAnimationFinished", "(I)I", nativeAnimationFinished},
    {"nativeDispatchViewClickEvent", "(I)I", nativeDispatchViewClickEvent},
    {"nativeDispatchViewLongClickEvent", "(I)I", nativeDispatchViewLongClickEvent},
    {"nativeDispatchViewValueChangeEvent", "(I)I", nativeDispatchViewValueChangeEvent},
    {"nativeDispatchViewTouchBeginEvent", "(III)I", nativeDispatchViewTouchBeginEvent},
    {"nativeDispatchViewTouchEndEvent", "(III)I", nativeDispatchViewTouchEndEvent},
    {"nativeDispatchViewTouchMoveEvent", "(III)I", nativeDispatchViewTouchMoveEvent},
    {"nativeDispatchViewTouchCancelEvent", "(III)I", nativeDispatchViewTouchCancelEvent},
    {"nativeInitDensity", "(F)I", nativeInitDensity},
    {"nativeInit", "()I", nativeInit},
    {"nativeInitAssetManager", "(Landroid/content/res/AssetManager;)I", nativeInitAssetManager},
    {"nativeRequestTableViewCellCustomView", "(III)I", nativeRequestTableViewCellCustomView},
    {"nativeRequestTableViewCellIdentifier", "(III)Ljava/lang/String;", nativeRequestTableViewCellIdentifier},
    {"nativeRequestTableViewSectionCount", "(I)I", nativeRequestTableViewSectionCount},
    {"nativeRequestTableViewRowCount", "(II)I", nativeRequestTableViewRowCount},
    {"nativeRequestTableViewRowHeight", "(III)I", nativeRequestTableViewRowHeight},
    {"nativeRequestTableViewCellIdentifierTypeCount", "(I)I", nativeRequestTableViewCellIdentifierTypeCount},
    {"nativeRequestTableViewCellRender", "(IIII)I", nativeRequestTableViewCellRender},
    {"nativeRequestTableViewRefresh", "(I)I", nativeRequestTableViewRefresh},
    {"nativeRequestTableViewUpdateRefreshView", "(II)I", nativeRequestTableViewUpdateRefreshView},
    {"nativeRequestTableViewRefreshView", "(I)I", nativeRequestTableViewRefreshView},
    {"nativeSendAppLayoutChanging", "()I", nativeSendAppLayoutChanging},
    {"nativeInvokeOpenGLViewRender", "(IJ)I", nativeInvokeOpenGLViewRender},
    {"nativeInvokeThread", "(IJ)I", nativeInvokeThread},
  };
  JNINativeMethod nativeNotifyThreadMethods[] = {
    {"nativeInvokeNotifyThread", "()I", nativeInvokeNotifyThread}
  };
  loadedVm = vm;
  env = phoneGetJNIEnv();
  objClass = (*env)->FindClass(env, PHONE_ACTIVITY_CLASSNAME);
  (*env)->RegisterNatives(env, objClass, nativeMethods,
      sizeof(nativeMethods) / sizeof(nativeMethods[0]));
  objClass = (*env)->FindClass(env,
      PHONE_NOTIFY_THREAD_CLASSNAME);
  (*env)->RegisterNatives(env, objClass, nativeNotifyThreadMethods,
      sizeof(nativeNotifyThreadMethods) / sizeof(nativeNotifyThreadMethods[0]));
}

void phoneRegisterNativeMethod(const char *className, const char *methodName,
    const char *methodSig, void *func) {
  JNINativeMethod nativeMethod = {methodName, methodSig, func};
  JNIEnv *env = phoneGetJNIEnv();
  jclass objClass = (*env)->FindClass(env, className);
  (*env)->RegisterNatives(env, objClass, &nativeMethod, 1);
}

int phoneJstringToUtf8Length(jstring jstr) {
  JNIEnv *env = phoneGetJNIEnv();
  return (*env)->GetStringUTFLength(env, jstr);
}

int phoneJstringToUtf8(jstring jstr, char *buf, int bufSize) {
  JNIEnv *env = phoneGetJNIEnv();
  const char *src;
  int copyLen;
  jboolean isCopy = JNI_TRUE;
  src = (*env)->GetStringUTFChars(env, jstr, &isCopy);
  copyLen = phoneCopyString(buf, bufSize, src);
  if (JNI_TRUE == isCopy) {
    (*env)->ReleaseStringUTFChars(env, jstr, src);
  }
  return copyLen;
}

int shareRemoveTimer(int handle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneHandle *handleData = pHandle(handle);
  phoneCallJavaReturnInt(result, env, activity,
    "javaRemoveTimer", "(I)I",
    (jint)handle);
  return result;
}

int shareCreateTimer(int handle, unsigned int milliseconds) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaCreateTimer", "(IJ)I",
    (jint)handle, (jlong)milliseconds);
  return result;
}

int shareCreateContainerView(int handle, int parentHandle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaCreateContainerView", "(II)I",
    (jint)handle, (jint)parentHandle);
  return result;
}

int shareSetViewFrame(int handle, float x, float y, float width,
    float height) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewFrame", "(IFFFF)I",
    (jint)handle, (jfloat)x, (jfloat)y, (jfloat)width, (jfloat)height);
  return result;
}

int shareSetViewBackgroundColor(int handle, unsigned int color) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewBackgroundColor", "(II)I",
    (jint)handle, (jint)color);
  return result;
}

int shareCreateTextView(int handle, int parentHandle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaCreateTextView", "(II)I",
    (jint)handle, (jint)parentHandle);
  return result;
}

int shareSetViewText(int handle, const char *val) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewText", "(ILjava/lang/String;)I",
    (jint)handle, (*env)->NewStringUTF(env, val));
  return result;
}

int shareSetViewFontColor(int handle, unsigned int color) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewFontColor", "(II)I",
    (jint)handle, (jint)color);
  return result;
}

int shareShowView(int handle, int display) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaShowView", "(II)I",
    (jint)handle, (jint)display);
  return result;
}

float shareGetViewWidth(int handle) {
  jfloat result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnFloat(result, env, activity,
    "javaGetViewWidth", "(I)F",
    (jint)handle);
  return result;
}

float shareGetViewHeight(int handle) {
  jfloat result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnFloat(result, env, activity,
    "javaGetViewHeight", "(I)F",
    (jint)handle);
  return result;
}

static int phoneToAndroidLogLevel(int level) {
  switch (level) {
    case PHONE_LOG_DEBUG:
      return ANDROID_LOG_DEBUG;
    case PHONE_LOG_INFO:
      return ANDROID_LOG_INFO;
    case PHONE_LOG_WARN:
      return ANDROID_LOG_WARN;
    case PHONE_LOG_ERROR:
      return ANDROID_LOG_ERROR;
    case PHONE_LOG_FATAL:
      return ANDROID_LOG_FATAL;
    default: {
      if (level < PHONE_LOG_DEBUG) {
        return ANDROID_LOG_DEBUG;
      } else {
        return ANDROID_LOG_FATAL;
      }
    }
  }
}

int shareDumpLog(int level, const char *tag, const char *log, int len) {
  __android_log_write(phoneToAndroidLogLevel(level), tag, log);
  return 0;
}

int shareNeedFlushMainWorkQueue(void) {
  pApp->needFlushMainWorkQueue = 1;
  pthread_mutex_lock(&notifyMainThreadMutex);
  pthread_cond_signal(&notifyMainThreadCond);
  pthread_mutex_unlock(&notifyMainThreadMutex);
  return 0;
}

int shareCreateViewAnimationSet(int handle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaCreateViewAnimationSet", "(I)I",
    (jint)handle);
  return result;
}

int shareAddViewAnimationToSet(int animationHandle,
    int setHandle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaAddViewAnimationToSet", "(II)I",
    (jint)animationHandle, (jint)setHandle);
  return result;
}

int shareRemoveViewAnimationSet(int handle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaRemoveViewAnimationSet", "(I)I",
    (jint)handle);
  return result;
}

int shareRemoveViewAnimation(int handle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaRemoveViewAnimation", "(I)I",
    (jint)handle);
  return result;
}

int shareCreateViewTranslateAnimation(int handle, int viewHandle,
    float offsetX, float offsetY) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaCreateViewTranslateAnimation", "(IIFF)I",
    (jint)handle, (jint)viewHandle,
    (jfloat)offsetX, (jfloat)offsetY);
  return result;
}

int shareBeginAnimationSet(int handle, int duration) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaBeginAnimationSet", "(II)I",
    (jint)handle, (jint)duration);
  return result;
}

int shareCreateViewAlphaAnimation(int handle, int viewHandle,
    float fromAlpha, float toAlpha) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaCreateViewAlphaAnimation", "(IIFF)I",
    (jint)handle, (jint)viewHandle, (jfloat)fromAlpha, (jfloat)toAlpha);
  return result;
}

int shareBringViewToFront(int handle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaBringViewToFront", "(I)I",
    (jint)handle);
  return result;
}

int shareSetViewAlpha(int handle, float alpha) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewAlpha", "(IF)I",
    (jint)handle, (jfloat)alpha);
  return result;
}

int shareSetViewFontSize(int handle, float fontSize) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewFontSize", "(IF)I",
    (jint)handle, (jfloat)fontSize);
  return result;
}

int shareSetViewBackgroundImageResource(int handle,
    const char *imageResource) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewBackgroundImageResource", "(ILjava/lang/String;)I",
    (jint)handle, (*env)->NewStringUTF(env, imageResource));
  return result;
}

int shareSetViewBackgroundImagePath(int handle,
    const char *imagePath) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewBackgroundImagePath", "(ILjava/lang/String;)I",
    (jint)handle, (*env)->NewStringUTF(env, imagePath));
  return result;
}

int shareCreateEditTextView(int handle, int parentHandle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaCreateEditTextView", "(II)I",
    (jint)handle, (jint)parentHandle);
  return result;
}

int shareShowSoftInputOnView(int handle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaShowSoftInputOnView", "(I)I",
    (jint)handle);
  return result;
}

int shareHideSoftInputOnView(int handle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaHideSoftInputOnView", "(I)I",
    (jint)handle);
  return result;
}

int shareGetViewText(int handle, char *buf, int bufSize) {
  jstring result = 0;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnObject(result, env, activity,
    "javaGetViewText", "(I)Ljava/lang/String;",
    (jint)handle);
  return phoneJstringToUtf8(result, buf, bufSize);
}

int shareSetViewInputTypeAsVisiblePassword(int handle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewInputTypeAsVisiblePassword", "(I)I",
    (jint)handle);
  return result;
}

int shareSetViewInputTypeAsPassword(int handle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewInputTypeAsPassword", "(I)I",
    (jint)handle);
  return result;
}

int shareSetViewInputTypeAsText(int handle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewInputTypeAsText", "(I)I",
    (jint)handle);
  return result;
}

int shareEnableViewClickEvent(int handle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaEnableViewClickEvent", "(I)I",
    (jint)handle);
  return result;
}

int shareEnableViewLongClickEvent(int handle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaEnableViewLongClickEvent", "(I)I",
    (jint)handle);
  return result;
}

int shareEnableViewValueChangeEvent(int handle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaEnableViewValueChangeEvent", "(I)I",
    (jint)handle);
  return result;
}

int shareEnableViewTouchEvent(int handle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaEnableViewTouchEvent", "(I)I",
    (jint)handle);
  return result;
}

int shareGetThreadId(void) {
  return (int)gettid();
}

int shareIsLandscape(void) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaIsLandscape", "()I");
  return result;
}

int shareSetStatusBarBackgroundColor(unsigned int color) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetStatusBarBackgroundColor", "(I)I", (jint)color);
  return result;
}

int shareSetViewAlign(int handle, int align) {
  jint result = 0;
  JNIEnv *env = phoneGetJNIEnv();
  switch (align) {
    case PHONE_VIEW_ALIGN_CENTER:
      phoneCallJavaReturnInt(result, env, activity,
        "javaSetViewAlignCenter", "(I)I",
        (jint)handle);
      break;
    case PHONE_VIEW_ALIGN_LEFT:
      phoneCallJavaReturnInt(result, env, activity,
        "javaSetViewAlignLeft", "(I)I",
        (jint)handle);
      break;
    case PHONE_VIEW_ALIGN_RIGHT:
      phoneCallJavaReturnInt(result, env, activity,
        "javaSetViewAlignRight", "(I)I",
        (jint)handle);
      break;
  }
  return result;
}

int shareSetViewVerticalAlign(int handle, int align) {
  // TODO:
  return 0;
}

int shareSetViewCornerRadius(int handle, float radius) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewCornerRadius", "(IF)I",
    (jint)handle, (jfloat)radius);
  return result;
}

int shareSetViewBorderColor(int handle, unsigned int color) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewBorderColor", "(II)I",
    (jint)handle, (jint)color);
  return result;
}

int shareSetViewBorderWidth(int handle, float width) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewBorderWidth", "(IF)I",
    (jint)handle, (jfloat)width);
  return result;
}

int shareCreateTableView(int handle, int parentHandle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  jint isGrouped = 0;
  phoneCallJavaReturnInt(result, env, activity,
    "javaCreateTableView", "(III)I",
    (jint)isGrouped, (jint)handle, (jint)parentHandle);
  return result;
}

int shareReloadTableView(int handle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaReloadTableView", "(I)I",
    (jint)handle);
  return result;
}

int shareSetViewShadowColor(int handle, unsigned int color) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewShadowColor", "(II)I",
    (jint)handle, (jint)color);
  return result;
}

int shareSetViewShadowOffset(int handle, float offsetX, float offsetY) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewShadowOffset", "(IFF)I",
    (jint)handle, (jfloat)offsetX, (jfloat)offsetY);
  return result;
}

int shareSetViewShadowOpacity(int handle, float opacity) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewShadowOpacity", "(IF)I",
    (jint)handle, (jfloat)opacity);
  return result;
}

int shareSetViewShadowRadius(int handle, float radius) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewShadowRadius", "(IF)I",
    (jint)handle, (jfloat)radius);
  return result;
}

int shareSetViewBackgroundImageRepeat(int handle, int repeat) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewBackgroundImageRepeat", "(II)I",
    (jint)handle, (jint)repeat);
  return result;
}

int shareSetViewFontBold(int handle, int bold) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewFontBold", "(II)I",
    (jint)handle, (jint)bold);
  return result;
}

int shareBeginTableViewRefresh(int handle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaBeginTableViewRefresh", "(I)I",
    (jint)handle);
  return result;
}

int shareEndTableViewRefresh(int handle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaEndTableViewRefresh", "(I)I",
    (jint)handle);
  return result;
}

float shareGetTableViewRefreshHeight(int handle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnFloat(result, env, activity,
    "javaGetTableViewRefreshHeight", "(I)F",
    (jint)handle);
  return result;
}

int shareRotateView(int handle, float degree) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaRotateView", "(IF)I",
    (jint)handle, (jfloat)degree);
  return result;
}

int shareSetEditTextViewPlaceholder(int handle, const char *text,
    unsigned int color) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetEditTextViewPlaceholder", "(ILjava/lang/String;I)I",
    (jint)handle, (*env)->NewStringUTF(env, text), (jint)color);
  return result;
}

int shareSetViewParent(int handle, int parentHandle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaSetViewParent", "(II)I",
    (jint)handle, (jint)parentHandle);
  return result;
}

int shareRemoveView(int handle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaRemoveView", "(I)I",
    (jint)handle);
  return result;
}

int shareCreateOpenGLView(int handle, int parentHandle) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaCreateOpenGLView", "(II)I",
    (jint)handle, (jint)parentHandle);
  return result;
}

int shareBeginOpenGLViewRender(int handle,
    phoneOpenGLViewRenderHandler renderHandler) {
  jint result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaBeginOpenGLViewRender", "(IJ)I",
    (jint)handle, (jlong)renderHandler);
  return result;
}

int shareCreateThread(int handle, const char *threadName) {
  int result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaCreateThread", "(ILjava/lang/String;)I",
    (jint)handle, (*env)->NewStringUTF(env, threadName));
  return handle;
}

int shareStartThread(int handle) {
  int result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneHandle *handleData = pHandle(handle);
  phoneCallJavaReturnInt(result, env, activity,
    "javaStartThread", "(IJJ)I",
    (jint)handle, (jlong)handleData->u.thread.runHandler);
  return 0;
}

int shareJoinThread(int handle) {
  int result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaJoinThread", "(I)I",
    (jint)handle);
  return 0;
}

int shareRemoveThread(int handle) {
  int result;
  JNIEnv *env = phoneGetJNIEnv();
  phoneCallJavaReturnInt(result, env, activity,
    "javaRemoveThread", "(I)I",
    (jint)handle);
  return 0;
}

int shareWorkQueueThreadInit(void) {
  JNIEnv *env;
  (*loadedVm)->AttachCurrentThread(loadedVm, &env, 0);
  return 0;
}

int shareWorkQueueThreadUninit(void) {
  (*loadedVm)->DetachCurrentThread(loadedVm);
  return 0;
}

static int assetRead(void *cookie, char *buf, int size) {
  return AAsset_read((AAsset *)cookie, buf, size);
}

static int assetWrite(void *cookie, const char *buf, int size) {
  return 0;
}

static fpos_t assetSeek(void *cookie, fpos_t offset, int whence) {
  return AAsset_seek((AAsset *)cookie, offset, whence);
}

static int assetClose(void *cookie) {
  AAsset_close((AAsset *)cookie);
  return 0;
}

FILE *shareOpenAsset(const char *filename) {
  AAsset *asset = AAssetManager_open(assetManager, filename, 0);
  if (!asset) {
    return 0;
  }
  return funopen(asset, assetRead, assetWrite, assetSeek, assetClose);
}
