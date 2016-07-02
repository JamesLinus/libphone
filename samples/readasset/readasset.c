#include "libphone.h"

#if __ANDROID__
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
   phoneInitJava(vm);
   return JNI_VERSION_1_6;
}
#endif

#define BACKGROUND_COLOR 0xefefef
#define FONT_COLOR 0x333333

static int backgroundView = 0;
static int textView = 0;

static int loadResourceWork = 0;

static void appShowing(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "app showing");
}

static void appHiding(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "app hiding");
}

static void appTerminating(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "app terminating");
}

typedef struct loadDemoAssetContext {
  char resourceName[768];
  char *fileContent;
  int fileSize;
} loadDemoAssetContext;

static void loadResource(loadDemoAssetContext *ctx) {
  FILE *fp = phoneOpenAsset(ctx->resourceName);
  if (fp) {
    fseek(fp, 0, SEEK_END);
    ctx->fileSize = ftell(fp);
    rewind(fp);
    ctx->fileContent = malloc(ctx->fileSize + 1);
    fread(ctx->fileContent, 1, ctx->fileSize, fp);
    ctx->fileContent[ctx->fileSize] = '\0';
    fclose(fp);
  }
}

static void doCpuIntensiveWorkInBackgroundThread(int handle) {
  loadDemoAssetContext *ctx = (loadDemoAssetContext *)phoneGetHandleTag(handle);
  loadResource(ctx);
}

static void disposeAtUiThread(int handle) {
  loadDemoAssetContext *ctx = (loadDemoAssetContext *)phoneGetHandleTag(handle);
  if (ctx->fileContent) {
    phoneSetViewText(textView, ctx->fileContent);
  } else {
    phoneSetViewText(textView, "Not found");
  }
  free(ctx->fileContent);
  free(ctx);
  phoneRemoveWorkItem(handle);
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

  textView = phoneCreateTextView(backgroundView, 0);
  phoneSetViewFontSize(textView, dp(14));
  phoneSetViewFrame(textView, 0, 0, phoneGetViewWidth(0),
    phoneGetViewHeight(0));
  phoneSetViewFontColor(textView, FONT_COLOR);

  phoneSetViewText(textView, "Loading..");

  {
    loadDemoAssetContext *ctx = (loadDemoAssetContext *)calloc(1,
        sizeof(loadDemoAssetContext));
    strcpy(ctx->resourceName, "demoasset.txt");
    loadResourceWork = phoneCreateWorkItem(doCpuIntensiveWorkInBackgroundThread,
      disposeAtUiThread);
    phoneSetHandleTag(loadResourceWork, ctx);
    phonePostToMainWorkQueue(loadResourceWork);
  }

  return 0;
}
