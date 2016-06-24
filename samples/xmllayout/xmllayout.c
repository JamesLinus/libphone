#include "libphone.h"
#include "layout.h"

#define BACKGROUND_COLOR 0xefefef

static int backgroundView = 0;
static int tableView = 0;
static board *page = 0;

static void appShowing(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "app showing");
}

static void appHiding(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "app hiding");
}

static void appTerminating(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "app terminating");
}

int phoneMain(int argc, const char *argv[]) {
  static phoneAppNotificationHandler handler = {
    appShowing,
    appHiding,
    appTerminating
  };
  phoneSetAppNotificationHandler(&handler);

  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "display size: %f x %f",
    phoneGetViewWidth(0), phoneGetViewHeight(0));

  page = boardCreate();

  return 0;
}
