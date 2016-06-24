#ifndef __LIBPHONE_H__
#define __LIBPHONE_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct phoneAppNotificationHandler {
  void (*showing)(void);
  void (*hiding)(void);
  void (*terminating)(void);
} phoneAppNotificationHandler;

extern int phoneMain(int optionNum, const char *options[]);
int phoneAllocHandle(void);
int phoneFreeHandle(int handle);
int phoneSetHandleContext(int handle, void *context);
void *phoneGetHandleContext(int handle);
int phoneAllocHandleTypeRange(int count);
int phoneSetHandleType(int handle, int type);
int phoneGetHandleType(int handle);
enum phoneViewEventResult {
  PHONE_VIEW_EVENT_DONTCARE,
  PHONE_VIEW_EVENT_YES,
  PHONE_VIEW_EVENT_NO
};
#define phoneViewEventTypeMap(XX)                                                               \
  XX(PHONE_VIEW_CLICK, "click")                                                                 \
  XX(PHONE_VIEW_LONG_CLICK, "longClick")                                                        \
  XX(PHONE_VIEW_VALUE_CHANGE, "valueChange")                                                    \
  XX(PHONE_VIEW_TOUCH, "touch")                                                                 \
  XX(PHONE_VIEW_REQUEST_TABLE_CELL_DETAIL_TEXT, "requestTableCellDetailText")                   \
  XX(PHONE_VIEW_REQUEST_TABLE_CELL_TEXT, "requestTableCellText")                                \
  XX(PHONE_VIEW_REQUEST_TABLE_CELL_SELECTION_STYLE, "requestTableCellSelectionStyle")           \
  XX(PHONE_VIEW_REQUEST_TABLE_CELL_IMAGE_RESOURCE, "requestTableCellImageResource")             \
  XX(PHONE_VIEW_REQUEST_TABLE_CELL_SEPARATOR_STYLE, "requestTableCellSeparatorStyle")           \
  XX(PHONE_VIEW_REQUEST_TABLE_CELL_ACCESSORY_VIEW, "requestTableCellAccessoryView")             \
  XX(PHONE_VIEW_REQUEST_TABLE_CELL_CUSTOM_VIEW, "requestTableCellCustomView")                   \
  XX(PHONE_VIEW_REQUEST_TABLE_CELL_IDENTIFIER, "requestTableCellIdentifier")                    \
  XX(PHONE_VIEW_REQUEST_TABLE_SECTION_COUNT, "requestTableSectionCount")                        \
  XX(PHONE_VIEW_REQUEST_TABLE_ROW_COUNT, "requestTableRowCount")                                \
  XX(PHONE_VIEW_REQUEST_TABLE_ROW_HEIGHT, "requestTableRowHeight")                              \
  XX(PHONE_VIEW_REQUEST_TABLE_SECTION_HEADER, "requestTableSectionHeader")                      \
  XX(PHONE_VIEW_REQUEST_TABLE_SECTION_FOOTER, "requestTableSectionFooter")                      \
  XX(PHONE_VIEW_REQUEST_TABLE_CELL_IDENTIFIER_TYPE_COUNT, "requestTableCellIdentifierTypeCount")\
  XX(PHONE_VIEW_REQUEST_TABLE_CELL_RENDER, "requestTableCellRender")
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
int phoneFormatString(char *destBuf, int destSize, const char *fmt, ...);
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
enum phoneTableViewStyle {
  PHONE_TABLE_VIEW_STYLE_PLAIN,
  PHONE_TABLE_VIEW_STYLE_GROUPED
};
int phoneCreateTableView(int style, int parentHandle,
    phoneViewEventHandler eventHandler);
float phoneDipToPix(int dip);
#define dp(dip) phoneDipToPix(dip)
int phoneReloadTableView(int handle);

#if __ANDROID__
#include <jni.h>

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
  jclass objClass = (*env)->GetObjectClass(env, obj); \
  jmethodID methodId = (*env)->GetMethodID(env, \
      objClass, methodName, (methodSig));
#define phoneCallJava(env, obj, methodName, methodSig, ...) do { \
  phonePrepareForCallJava(obj, methodName, methodSig); \
  (*env)->CallVoidMethod(env, obj, methodId, ##__VA_ARGS__); \
} while (0)
#define phoneCallJavaReturnBoolean(ret, env, obj, methodName, methodSig, ...) do { \
  phonePrepareForCallJava(obj, methodName, methodSig); \
  ret = (*env)->CallBooleanMethod(env, obj, methodId, ##__VA_ARGS__); \
} while (0)
#define phoneCallJavaReturnByte(ret, env, obj, methodName, methodSig, ...) do { \
  phonePrepareForCallJava(obj, methodName, methodSig); \
  ret = (*env)->CallByteMethod(env, obj, methodId, ##__VA_ARGS__); \
} while (0)
#define phoneCallJavaReturnChar(ret, env, obj, methodName, methodSig, ...) do { \
  phonePrepareForCallJava(obj, methodName, methodSig); \
  ret = (*env)->CallCharMethod(env, obj, methodId, ##__VA_ARGS__); \
} while (0)
#define phoneCallJavaReturnDouble(ret, env, obj, methodName, methodSig, ...) do { \
  phonePrepareForCallJava(obj, methodName, methodSig); \
  ret = (*env)->CallDoubleMethod(env, obj, methodId, ##__VA_ARGS__); \
} while (0)
#define phoneCallJavaReturnFloat(ret, env, obj, methodName, methodSig, ...) do { \
  phonePrepareForCallJava(obj, methodName, methodSig); \
  ret = (*env)->CallFloatMethod(env, obj, methodId, ##__VA_ARGS__); \
} while (0)
#define phoneCallJavaReturnInt(ret, env, obj, methodName, methodSig, ...) do { \
  phonePrepareForCallJava(obj, methodName, methodSig); \
  ret = (*env)->CallIntMethod(env, obj, methodId, ##__VA_ARGS__); \
} while (0)
#define phoneCallJavaReturnLong(ret, env, obj, methodName, methodSig, ...) do { \
  phonePrepareForCallJava(obj, methodName, methodSig); \
  ret = (*env)->CallLongMethod(env, obj, methodId, ##__VA_ARGS__); \
} while (0)
#define phoneCallJavaReturnObject(ret, env, obj, methodName, methodSig, ...) do { \
  phonePrepareForCallJava(obj, methodName, methodSig); \
  ret = (*env)->CallObjectMethod(env, obj, methodId, ##__VA_ARGS__); \
} while (0)
int phoneJstringToUtf8Length(jstring jstr);
int phoneJstringToUtf8(jstring jstr, char *buf, int bufSize);

#endif

#ifdef __cplusplus
}
#endif

#if __OBJC__
#ifdef __APPLE__
#import <UIKit/UIKit.h>
@interface phoneAppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@property (strong, nonatomic) UIImageView *container;
@property (strong, nonatomic) NSMutableDictionary *handleMap;
@end
#endif
#endif

#endif
