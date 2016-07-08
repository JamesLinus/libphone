/* Copyright (c) huxingyi@msn.com All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __LIBPHONE_H__
#define __LIBPHONE_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

enum phoneHandleResult {
  PHONE_DONTCARE,
  PHONE_YES,
  PHONE_NO
};

typedef struct phoneAppNotificationHandler {
  void (*showing)(void);
  void (*hiding)(void);
  void (*terminating)(void);
  int (*backClick)(void);
  void (*layoutChanging)(void);
} phoneAppNotificationHandler;

extern int phoneMain(int optionNum, const char *options[]);
int phoneAllocHandle(void);
int phoneFreeHandle(int handle);
int phoneSetHandleContext(int handle, void *context);
void *phoneGetHandleContext(int handle);
int phoneAllocHandleTypeRange(int count);
int phoneSetHandleType(int handle, int type);
int phoneGetHandleType(int handle);
#define phoneViewEventTypeMap(XX)                                                               \
  XX(PHONE_VIEW_CLICK, "click")                                                                 \
  XX(PHONE_VIEW_LONG_CLICK, "longClick")                                                        \
  XX(PHONE_VIEW_VALUE_CHANGE, "valueChange")                                                    \
  XX(PHONE_VIEW_TOUCH, "touch")                                                                 \
  XX(PHONE_VIEW_REQUEST_TABLE_CELL_CUSTOM_VIEW, "requestTableCellCustomView")                   \
  XX(PHONE_VIEW_REQUEST_TABLE_CELL_IDENTIFIER, "requestTableCellIdentifier")                    \
  XX(PHONE_VIEW_REQUEST_TABLE_SECTION_COUNT, "requestTableSectionCount")                        \
  XX(PHONE_VIEW_REQUEST_TABLE_ROW_COUNT, "requestTableRowCount")                                \
  XX(PHONE_VIEW_REQUEST_TABLE_ROW_HEIGHT, "requestTableRowHeight")                              \
  XX(PHONE_VIEW_REQUEST_TABLE_CELL_IDENTIFIER_TYPE_COUNT, "requestTableCellIdentifierTypeCount")\
  XX(PHONE_VIEW_REQUEST_TABLE_CELL_RENDER, "requestTableCellRender")                            \
  XX(PHONE_VIEW_REQUEST_TABLE_CELL_CLICK, "requestTableCellClick")                              \
  XX(PHONE_VIEW_REQUEST_TABLE_REFRESH, "requestTableRefresh")                                   \
  XX(PHONE_VIEW_REQUEST_TABLE_UPDATE_REFRESH_VIEW, "requestTableUpdateRefreshView")             \
  XX(PHONE_VIEW_REQUEST_TABLE_REFRESH_VIEW, "requestTableRefreshView")
#define XX(code, name) code,
enum phoneViewEventType {
  phoneViewEventTypeMap(XX)
};
#undef XX
typedef struct phoneViewRequestTable {
  int section;
  int row;
  char *buf;
  int bufSize;
  int renderHandle;
} phoneViewRequestTable;
const char *phoneViewEventTypeToName(int eventType);
enum phoneViewTouchType {
  PHONE_VIEW_TOUCH_BEGIN,
  PHONE_VIEW_TOUCH_END,
  PHONE_VIEW_TOUCH_MOVE,
  PHONE_VIEW_TOUCH_CANCEL
};
typedef struct phoneViewTouch {
  int touchType;
  int x;
  int y;
} phoneViewTouch;
typedef int (*phoneViewEventHandler)(int handle, int eventType,
    void *eventParam);
int phoneCreateContainerView(int parentHandle,
    phoneViewEventHandler eventHandler);
int phoneCreateTextView(int parentHandle,
    phoneViewEventHandler eventHandler);
int phoneCreateEditTextView(int parentHandle,
    phoneViewEventHandler eventHandler);
int phoneEnableViewEvent(int handle, int eventType);
enum phoneLogLevel {
  PHONE_LOG_DEBUG = 0,
  PHONE_LOG_INFO,
  PHONE_LOG_WARN,
  PHONE_LOG_ERROR,
  PHONE_LOG_FATAL
};
int phoneLog(int level, const char *tag, const char *fmt, ...);
int phoneGetThreadId(void);
int phoneSleep(unsigned int milliseconds);
int phoneCopyString(char *destBuf, int destSize, const char *src);
int phoneSetHandleTag(int handle, void *tag);
void *phoneGetHandleTag(int handle);
int phoneRemoveTimer(int handle);
int phoneRemoveWorkItem(int handle);
int phoneSetViewFrame(int handle, float x, float y, float width, float height);
int phoneSetViewBackgroundColor(int handle, unsigned int color);
int phoneSetViewFontColor(int handle, unsigned int color);
int phoneSetViewText(int handle, const char *val);
int phoneSetAppNotificationHandler(phoneAppNotificationHandler *handler);
int phoneShowView(int handle, int display);
float phoneGetViewWidth(int handle);
float phoneGetViewHeight(int handle);
typedef void (*phoneViewAnimationSetFinishHandler)(int handle);
int phoneCreateViewAnimationSet(int duration,
    phoneViewAnimationSetFinishHandler finishHandler);
int phoneAddViewAnimationToSet(int animationHandle, int setHandle);
int phoneBeginViewAnimationSet(int handle);
int phoneCreateViewTranslateAnimation(int viewHandle, float offsetX,
    float offsetY);
int phoneRemoveViewAnimationSet(int handle);
int phoneRemoveViewAnimation(int handle);
int phoneCreateViewAlphaAnimation(int viewHandle,
    float fromAlpha, float toAlpha);
int phoneBringViewToFront(int handle);
typedef void (*phoneTimerRunHandler)(int handle);
int phoneCreateTimer(unsigned int milliseconds,
    phoneTimerRunHandler runHandler);
int phoneSetMainWorkQueueThreadCount(int threadCount);
typedef void (*phoneBackgroundWorkHandler)(int itemHandle);
typedef void (*phoneAfterWorkHandler)(int itemHandle);
int phoneCreateWorkItem(phoneBackgroundWorkHandler workHandler,
    phoneAfterWorkHandler afterWorkHandler);
int phonePostToMainWorkQueue(int itemHandle);
int phoneSetViewAlpha(int handle, float alpha);
int phoneSetViewFontSize(int handle, float fontSize);
int phoneSetViewBackgroundImageResource(int handle,
    const char *imageResource);
int phoneSetViewBackgroundImagePath(int handle,
    const char *imagePath);
int phoneShowSoftInputOnView(int handle);
int phoneHideSoftInputOnView(int handle);
int phoneGetViewText(int handle, char *buf, int bufSize);
enum phoneInputType {
  PHONE_INPUT_TEXT = 0,
  PHONE_INPUT_PASSWORD,
  PHONE_INPUT_VISIBLE_PASSWORD
};
int phoneSetViewInputType(int handle, int inputType);
int phoneSetViewCornerRadius(int handle, float radius);
int phoneSetViewBorderColor(int handle, unsigned int color);
int phoneSetViewBorderWidth(int handle, float width);
int phoneIsLandscape(void);
int phoneSetStatusBarBackgroundColor(unsigned int color);
enum phoneViewAlignType {
  PHONE_VIEW_ALIGN_CENTER,
  PHONE_VIEW_ALIGN_LEFT,
  PHONE_VIEW_ALIGN_RIGHT
};
int phoneSetViewAlign(int handle, int align);
enum phoneViewVerticalAlignType {
  PHONE_VIEW_VERTICAL_ALIGN_MIDDLE,
  PHONE_VIEW_VERTICAL_ALIGN_TOP,
  PHONE_VIEW_VERTICAL_ALIGN_BOTTOM
};
int phoneSetViewVerticalAlign(int handle, int align);
int phoneCreateTableView(int parentHandle,
    phoneViewEventHandler eventHandler);
float phoneDipToPix(int dip);
#define dp(dip) phoneDipToPix(dip)
int phoneReloadTableView(int handle);
int phoneSetViewShadowColor(int handle, unsigned int color);
int phoneSetViewShadowOffset(int handle, float offsetX, float offsetY);
int phoneSetViewShadowOpacity(int handle, float opacity);
int phoneSetViewShadowRadius(int handle, float radius);
int phoneSetViewBackgroundImageRepeat(int handle, int repeat);
int phoneSetViewFontBold(int handle, int bold);
int phoneBeginTableViewRefresh(int handle);
int phoneEndTableViewRefresh(int handle);
float phoneGetTableViewRefreshHeight(int handle);
float phoneGetTableViewStableRefreshHeight(void);
int phoneRotateView(int handle, float degree);
int phoneSetEditTextViewPlaceholder(int handle, const char *text,
    unsigned int color);
int phoneSetViewEventHandler(int handle, phoneViewEventHandler eventHandler);
int phoneSetViewParent(int handle, int parentHandle);
int phoneRemoveView(int handle);
int phoneCreateOpenGLView(int parentHandle,
    phoneViewEventHandler eventHandler);
typedef void (*phoneOpenGLViewRenderHandler)(int handle);
int phoneSetOpenGLViewRenderHandler(int handle,
    phoneOpenGLViewRenderHandler renderHandler);
typedef void (*phoneThreadRunHandler)(int handle);
int phoneCreateThread(const char *threadName,
    phoneThreadRunHandler runHandler);
int phoneStartThread(int handle);
int phoneJoinThread(int handle);
int phoneRemoveThread(int handle);
FILE *phoneOpenAsset(const char *filename);
#define phoneSensorEventTypeMap(XX)                                                             \
  XX(PHONE_SENSOR_SHAKE, "shake")
#define XX(code, name) code,
enum phoneSensorEventType {
  phoneSensorEventTypeMap(XX)
};
#undef XX
typedef int (*phoneSensorEventHandler)(int handle, int eventType,
    void *eventParam);
int phoneCreateShakeSensor(phoneSensorEventHandler eventHandler);
int phoneRemoveShakeSensor(int handle);
int phoneStartSensor(int handle);
int phoneStopSensor(int handle);
int phoneIsShakeSensorSupported(void);
int phoneGetViewParent(int handle);

#if __ANDROID__
#include <jni.h>
#include <GLES2/gl2.h>

#define PHONE_ACTIVITY_CLASSNAME "com/libphone/PhoneActivity"
#define PHONE_NOTIFY_THREAD_CLASSNAME "com/libphone/PhoneNotifyThread"

/*
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
  phoneInitJava(vm);
  // Register other methods with phoneRegisterNativeMethod
  // ... ...
  return JNI_VERSION_1_6;
}*/

void phoneInitJava(JavaVM *vm);
JNIEnv *phoneGetJNIEnv(void);
void phoneRegisterNativeMethod(const char *className, const char *methodName,
    const char *methodSig, void *func);
#define phonePrepareForCallJava(obj, methodName, methodSig) \
  jclass objClass;                                          \
  jmethodID methodId;                                       \
  objClass = (*env)->GetObjectClass(env, obj);              \
  objClass = (*env)->NewGlobalRef(env, objClass);           \
  methodId = (*env)->GetMethodID(env,                       \
      objClass, methodName, (methodSig));
#define phoneAfterCallJava()                                \
  (*env)->DeleteGlobalRef(env, objClass)
#define phoneCallJava(env, obj, methodName, methodSig, ...) do {                    \
  phonePrepareForCallJava(obj, methodName, methodSig);                              \
  (*env)->CallVoidMethod(env, obj, methodId, ##__VA_ARGS__);                        \
  phoneAfterCallJava();                                                             \
} while (0)
#define phoneCallJavaReturnBoolean(ret, env, obj, methodName, methodSig, ...) do {  \
  phonePrepareForCallJava(obj, methodName, methodSig);                              \
  ret = (*env)->CallBooleanMethod(env, obj, methodId, ##__VA_ARGS__);               \
  phoneAfterCallJava();                                                             \
} while (0)
#define phoneCallJavaReturnByte(ret, env, obj, methodName, methodSig, ...) do {     \
  phonePrepareForCallJava(obj, methodName, methodSig);                              \
  ret = (*env)->CallByteMethod(env, obj, methodId, ##__VA_ARGS__);                  \
  phoneAfterCallJava();                                                             \
} while (0)
#define phoneCallJavaReturnChar(ret, env, obj, methodName, methodSig, ...) do {     \
  phonePrepareForCallJava(obj, methodName, methodSig);                              \
  ret = (*env)->CallCharMethod(env, obj, methodId, ##__VA_ARGS__);                  \
  phoneAfterCallJava();                                                             \
} while (0)
#define phoneCallJavaReturnDouble(ret, env, obj, methodName, methodSig, ...) do {   \
  phonePrepareForCallJava(obj, methodName, methodSig);                              \
  ret = (*env)->CallDoubleMethod(env, obj, methodId, ##__VA_ARGS__);                \
  phoneAfterCallJava();                                                             \
} while (0)
#define phoneCallJavaReturnFloat(ret, env, obj, methodName, methodSig, ...) do {    \
  phonePrepareForCallJava(obj, methodName, methodSig);                              \
  ret = (*env)->CallFloatMethod(env, obj, methodId, ##__VA_ARGS__);                 \
  phoneAfterCallJava();                                                             \
} while (0)
#define phoneCallJavaReturnInt(ret, env, obj, methodName, methodSig, ...) do {      \
  phonePrepareForCallJava(obj, methodName, methodSig);                              \
  ret = (*env)->CallIntMethod(env, obj, methodId, ##__VA_ARGS__);                   \
  phoneAfterCallJava();                                                             \
} while (0)
#define phoneCallJavaReturnLong(ret, env, obj, methodName, methodSig, ...) do {     \
  phonePrepareForCallJava(obj, methodName, methodSig);                              \
  ret = (*env)->CallLongMethod(env, obj, methodId, ##__VA_ARGS__);                  \
  phoneAfterCallJava();                                                             \
} while (0)
#define phoneCallJavaReturnObject(ret, env, obj, methodName, methodSig, ...) do {   \
  phonePrepareForCallJava(obj, methodName, methodSig);                              \
  ret = (*env)->CallObjectMethod(env, obj, methodId, ##__VA_ARGS__);                \
  phoneAfterCallJava();                                                             \
} while (0)
int phoneJstringToUtf8Length(jstring jstr);
int phoneJstringToUtf8(jstring jstr, char *buf, int bufSize);

#endif

#ifdef __cplusplus
}
#endif

#ifdef __APPLE__
#include <OpenGLES/ES2/gl.h>
#if __OBJC__
#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>
@interface phoneAppDelegate : UIResponder <UIApplicationDelegate,
  GLKViewDelegate>
@property (strong, nonatomic) UIWindow *window;
@property (strong, nonatomic) UIImageView *container;
@property (strong, nonatomic) NSMutableDictionary *handleMap;
- (void)dispatchTableViewRefresh:(UIRefreshControl *)view;
@end
#endif
#endif

#endif
