#include "libphone.h"
//#include "lodepng.h"
#include "upng.h"
#include <assert.h>
#include <unistd.h>
#include <time.h>

#if __ANDROID__
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
   phoneInitJava(vm);
   return JNI_VERSION_1_6;
}
#endif

#define BACKGROUND_COLOR 0xefefef

static int backgroundView = 0;
static int openGLView = 0;
static int gameWidth = 0;
static int gameHeight = 0;

static void layout(void);

typedef struct image {
  unsigned int texId;
  unsigned int width;
  unsigned int height;
  int imageDataSize;
  unsigned char *imageData;
} image;

typedef struct {
  float position[3];
  float texCoord[2];
} vertex;

typedef struct spriteFrame {
  image *img;
  float left;
  float top;
  float width;
  float height;
} spriteFrame;

typedef struct spriteFrameSet {
  int showIndex;
  float x;
  float y;
  int frameCount;
  spriteFrame **frames;
  GLuint vertexBuffer;
} spriteFrameSet;

typedef struct gameContext {
  int inited:1;
  int initSucceed:1;
  image *cursedGraveImage;
  spriteFrameSet *cursedGraveCrashAnimation;
  spriteFrameSet *cursedGraveStillAnimation;
  int frameIntval;
  unsigned int lastTick;
  GLuint colorRenderBuffer;
  GLuint framebuffer;
  GLuint positionSlot;
  GLuint vertexShader;
  GLuint fragmentShader;
  GLuint program;
  GLuint indexBuffer;
  GLuint texCoordSlot;
  GLuint textureUniform;
} gameContext;

const GLubyte rectIndices[] = {
   0, 1, 2,
   2, 3, 0
};

static gameContext gameStruct = {0};
static gameContext *game = &gameStruct;

static void createTexForImage(image *img) {
  assert(0 == img->texId);
  if (!img->texId && img->imageData) {
    glGenTextures(1, &img->texId);
    glBindTexture(GL_TEXTURE_2D, img->texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height,
      0, GL_RGBA, GL_UNSIGNED_BYTE, img->imageData);
  }
}

static void updateSpriteFrameSetVertexs(spriteFrameSet *frameSet) {
  spriteFrame *frame = frameSet->frames[frameSet->showIndex %
      frameSet->frameCount];
  float onCanvasLeft = frameSet->x / gameWidth;
  float onCanvasTop = frameSet->y / gameHeight;
  float onCanvasWidth = frame->width / gameWidth;
  float onCanvasHeight = frame->height / gameHeight;
  float texLeft = frame->left / frame->img->width;
  float texTop = frame->top / frame->img->height;
  float texRight = (frame->left + frame->width) / frame->img->width;
  float texBottom = (frame->top + frame->height) / frame->img->height;
  vertex vertices[4] = {
    {{onCanvasLeft + onCanvasWidth, onCanvasTop + onCanvasHeight},
      {texRight, texTop}},
    {{onCanvasLeft + onCanvasWidth, onCanvasTop},
      {texRight, texBottom}},
    {{onCanvasLeft, onCanvasTop},
      {texLeft, texBottom}},
    {{onCanvasLeft, onCanvasTop + onCanvasHeight},
      {texLeft, texTop}},
  };
  if (0 == frameSet->vertexBuffer) {
    glGenBuffers(1, &frameSet->vertexBuffer);
  }
  glBindBuffer(GL_ARRAY_BUFFER, frameSet->vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
    GL_STATIC_DRAW);
}

static void setSpriteFrameSetPosition(spriteFrameSet *frameSet, float x, float y) {
  frameSet->x = x;
  frameSet->y = y;
  updateSpriteFrameSetVertexs(frameSet);
}

static void moveSpriteFrameSetToNextFrame(spriteFrameSet *frameSet) {
  frameSet->showIndex++;
  updateSpriteFrameSetVertexs(frameSet);
}

spriteFrame *createSpriteFrame(image *img,
    int left, int top, int width, int height) {
  spriteFrame *frame = (spriteFrame *)calloc(1, sizeof(spriteFrame));
  if (!frame) {
    phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "malloc failed");
    return 0;
  }
  frame->img = img;
  frame->width = width;
  frame->height = height;
  frame->left = left;
  frame->top = top;
  return frame;
}

void renderSpriteFrameSet(spriteFrameSet *frameSet) {
  spriteFrame *frame = frameSet->frames[frameSet->showIndex %
      frameSet->frameCount];
  if (frame->img &&
      !frame->img->texId) {
    createTexForImage(frame->img);
  }
  if (frame->img && frame->img->texId) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, frame->img->texId);
    glUniform1i(game->textureUniform, 0);

    glBindBuffer(GL_ARRAY_BUFFER, frameSet->vertexBuffer);

    glDrawElements(GL_TRIANGLES, sizeof(rectIndices) / sizeof(rectIndices[0]),
        GL_UNSIGNED_BYTE, 0);
  }
}

spriteFrameSet *createSpriteFrameSet(image *img, int srcLeft, int srcTop,
    int srcWidth, int srcHeight, int frameWidth, int frameHeight) {
  spriteFrameSet *frameSet;
  int i;
  assert(frameWidth == srcWidth || frameHeight == srcHeight);
  frameSet = (spriteFrameSet *)calloc(1, sizeof(spriteFrameSet));
  if (!frameSet) {
    return 0;
  }
  if (frameWidth == srcWidth) {
    frameSet->frameCount = srcHeight / frameHeight;
    frameSet->frames = calloc(frameSet->frameCount, sizeof(spriteFrame *));
    for (i = 0; i < frameSet->frameCount; ++i) {
      frameSet->frames[i] = createSpriteFrame(img,
          0, frameHeight * i, frameWidth, frameHeight);
    }
  } else {
    frameSet->frameCount = srcWidth / frameWidth;
    frameSet->frames = calloc(frameSet->frameCount, sizeof(spriteFrame *));
    for (i = 0; i < frameSet->frameCount; ++i) {
      frameSet->frames[i] = createSpriteFrame(img,
          frameWidth * i, 0, frameWidth, frameHeight);
    }
  }
  setSpriteFrameSetPosition(frameSet, 0, 0);
  return frameSet;
}

image *createImageFromAsset(const char *assetName) {
  FILE *fp;
  int fileSize;
  unsigned char *fileContent;
  int err;
  image *img = (image *)calloc(1, sizeof(img));
  if (!img) {
    return 0;
  }
  fp = phoneOpenAsset(assetName);
  if (!fp) {
    phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "phoneOpenAsset %s failed",
      assetName);
    free(img);
    return 0;
  }
  fseek(fp, 0, SEEK_END);
  fileSize = ftell(fp);
  rewind(fp);
  fileContent = malloc(fileSize);
  if (!fileContent) {
    phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "malloc %d failed",
      fileSize);
    fclose(fp);
    free(img);
    return 0;
  }
  if (fileSize != fread(fileContent, 1, fileSize, fp)) {
    phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "fread %s failed",
      assetName);
    fclose(fp);
    free(img);
    return 0;
  }
  fclose(fp);
  {
    upng_t *upng = upng_new_from_bytes(fileContent, fileSize);
    if (upng) {
      upng_decode(upng);
    }
    img->width = upng_get_width(upng);
    img->height = upng_get_height(upng);
    assert(upng_get_size(upng) == img->width * img->height * 4);
    img->imageData = malloc(upng_get_size(upng));
    memcpy(img->imageData, upng_get_buffer(upng), upng_get_size(upng));
    upng_free(upng);
  }
  /*
  err = lodepng_decode32(&img->imageData, &img->width, &img->height,
      fileContent, fileSize);
  if (err) {
    phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "lodepng_decode32 %s failed, error:%s",
      assetName, lodepng_error_text(err));
    free(img);
    return 0;
  }*/

  return img;
}

static unsigned int getTick(void) {
  /*
  struct timespec ts;
  unsigned int theTick = 0U;
  clock_gettime(CLOCK_REALTIME, &ts);
  theTick = ts.tv_nsec / 1000000;
  theTick += ts.tv_sec * 1000;
  return theTick;*/
  return (unsigned int)(1000 * clock() / CLOCKS_PER_SEC);
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
  return PHONE_DONTCARE;
}

static void appLayoutChanging(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "app layoutChanging");
  layout();
}

static char *createStringFromAsset(const char *assetName, int *len) {
  int fileSize;
  char *fileContent;
  FILE *fp = phoneOpenAsset(assetName);
  if (!fp) {
    return 0;
  }
  fseek(fp, 0, SEEK_END);
  fileSize = ftell(fp);
  rewind(fp);
  fileContent = shareMalloc(fileSize + 1);
  if (!fileContent) {
    fclose(fp);
    return 0;
  }
  fread(fileContent, 1, fileSize, fp);
  fileContent[fileSize] = '\0';
  if (len) {
    *len = fileSize;
  }
  fclose(fp);
  return fileContent;
}

static GLuint compileShader(const char *shaderName, GLenum shaderType) {
  GLint compileResult;
  int fileLen = 0;
  GLuint shaderHandle;
  char *fileContent = createStringFromAsset(shaderName, &fileLen);
  if (!fileContent) {
    phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "load shader %s failed",
      shaderName);
    return 0;
  }
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__,
    "\n\n-------- %s --------\n%s\n----------------------------------\n",
    shaderName, fileContent);
  shaderHandle = glCreateShader(shaderType);
  glShaderSource(shaderHandle, 1, &fileContent, &fileLen);
  glCompileShader(shaderHandle);
  free(fileContent);
  glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileResult);
  if (GL_FALSE == compileResult) {
    GLchar messages[1024];
    glGetShaderInfoLog(shaderHandle, sizeof(messages), 0, &messages[0]);
    phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "%s: %s", shaderName, messages);
    return 0;
  }
  return shaderHandle;
}

static GLint linkProgram(GLuint *shaders, int shaderCount) {
  GLint linkResult;
  GLuint program = glCreateProgram();
  int i;
  for (i = 0; i < shaderCount; ++i) {
    glAttachShader(program, shaders[i]);
  }
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &linkResult);
  if (GL_FALSE == linkResult) {
    GLchar messages[1024];
    glGetShaderInfoLog(program, sizeof(messages), 0, &messages[0]);
    phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "%s", messages);
    return 0;
  }
  return program;
}

static int gameInit(int fps) {
  game->frameIntval = 1000 / fps;

  game->cursedGraveImage = createImageFromAsset("cursed_grave_textrue.png");
  game->cursedGraveCrashAnimation = createSpriteFrameSet(game->cursedGraveImage,
      128, 0, 1024 - 128, 128, 128, 128);
  game->cursedGraveStillAnimation = createSpriteFrameSet(game->cursedGraveImage,
      0, 0, 128, 128, 128, 128);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);

  glGenRenderbuffers(1, &game->colorRenderBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, game->colorRenderBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
    gameWidth, gameHeight);

  glGenFramebuffers(1, &game->framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, game->framebuffer);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      GL_RENDERBUFFER, game->colorRenderBuffer);

  game->vertexShader = compileShader("vertex.glsl", GL_VERTEX_SHADER);
  if (0 == game->vertexShader) {
    return -1;
  }

  game->fragmentShader = compileShader("fragment.glsl", GL_FRAGMENT_SHADER);
  if (0 == game->fragmentShader) {
    return -1;
  }
  {
    GLuint shaders[] = {game->vertexShader, game->fragmentShader};
    game->program = linkProgram(shaders, sizeof(shaders) / sizeof(shaders[0]));
    if (0 == game->program) {
      return -1;
    }
  }

  glUseProgram(game->program);

  game->positionSlot = glGetAttribLocation(game->program, "Position");
  game->texCoordSlot = glGetAttribLocation(game->program, "TexCoordIn");
  game->textureUniform = glGetUniformLocation(game->program, "Texture");
  glEnableVertexAttribArray(game->texCoordSlot);
  glEnableVertexAttribArray(game->positionSlot);

  glGenBuffers(1, &game->indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, game->indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectIndices), rectIndices,
    GL_STATIC_DRAW);

  game->initSucceed = 1;

  return 0;
}

static int isGameNeedMoveToNextFrame(void) {
  long current = getTick();
  int needMove = current < game->lastTick ||
      game->lastTick + game->frameIntval < current;
  if (needMove) {
    game->lastTick = current;
  }
  return needMove;
}

static void renderGameFrame(int handle) {
  if (!game->inited) {
    gameInit(60);
    game->inited = 1;
  }

  if (!game->initSucceed) {
    return;
  }

  glClearColor(0.67, 0.67, 0.67, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  glVertexAttribPointer(game->positionSlot, 3, GL_FLOAT, GL_FALSE,
      sizeof(vertex), 0);
  glVertexAttribPointer(game->texCoordSlot, 2, GL_FLOAT, GL_FALSE,
      sizeof(vertex), (GLvoid*)(sizeof(float) * 3));

  if (isGameNeedMoveToNextFrame()) {
    if (game->cursedGraveCrashAnimation) {
      moveSpriteFrameSetToNextFrame(game->cursedGraveCrashAnimation);
    }
  }

  if (game->cursedGraveCrashAnimation) {
    renderSpriteFrameSet(game->cursedGraveCrashAnimation);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

  openGLView = phoneCreateOpenGLView(backgroundView, 0);
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
    openGLViewHeight = phoneGetViewHeight(0) - margin * 2;
    openGLViewWidth = openGLViewHeight * 480 / 320;
  }
  openGLViewTop = (phoneGetViewHeight(0) -
    openGLViewHeight) / 2;
  phoneSetViewFrame(openGLView, margin, openGLViewTop,
    openGLViewWidth, openGLViewHeight);

  gameWidth = (int)openGLViewWidth;
  gameHeight = (int)openGLViewHeight;

  phoneSetViewFrame(backgroundView, 0, 0, phoneGetViewWidth(0),
    phoneGetViewHeight(0));
}
