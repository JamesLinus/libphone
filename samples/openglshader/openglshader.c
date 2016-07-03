#include "libphone.h"
#include <assert.h>
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
static int canvasWidth = 0;
static int canvasHeight = 0;

static void layout(void);

typedef struct {
  float position[3];
  float color[4];
} vertex;

static int inited = 0;
static GLuint colorRenderBuffer;
static GLuint framebuffer;
static GLuint positionSlot;
static GLuint vertexShader;
static GLuint fragmentShader;
static GLuint program;
static GLuint indexBuffer;
static GLuint colorSlot;
static GLuint vertexBuffer;
static GLuint positionSlot;

const vertex vertices[] = {
  {{1, -1, 0}, {1, 0, 0, 1}},
  {{1, 1, 0}, {0, 1, 0, 1}},
  {{-1, 1, 0}, {0, 0, 1, 1}},
  {{-1, -1, 0}, {0, 0, 0, 1}}
};

const GLubyte rectIndices[] = {
   0, 1, 2,
   2, 3, 0
};

#define checkOpenGLError() do {                                          \
  GLenum err;                                                            \
  err = glGetError();                                                    \
  if (GL_NO_ERROR != err) {                                              \
    phoneLog(PHONE_LOG_ERROR, __FUNCTION__, "glGetError: 0x%x %s:%u",    \
      err, __FILE__, __LINE__);                                          \
  }                                                                      \
} while (0)

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

static void init(void) {
  glGenRenderbuffers(1, &colorRenderBuffer);
  checkOpenGLError();
  glBindRenderbuffer(GL_RENDERBUFFER, colorRenderBuffer);
  checkOpenGLError();
  //glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
  //  gameWidth, gameHeight);
  //checkOpenGLError();

  glGenFramebuffers(1, &framebuffer);
  checkOpenGLError();
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  checkOpenGLError();
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      GL_RENDERBUFFER, colorRenderBuffer);
  checkOpenGLError();

  vertexShader = compileShader("vertex.glsl", GL_VERTEX_SHADER);
  fragmentShader = compileShader("fragment.glsl", GL_FRAGMENT_SHADER);
  {
    GLuint shaders[] = {vertexShader, fragmentShader};
    program = linkProgram(shaders, sizeof(shaders) / sizeof(shaders[0]));
  }

  glUseProgram(program);
  checkOpenGLError();

  positionSlot = glGetAttribLocation(program, "Position");
  colorSlot = glGetAttribLocation(program, "SourceColor");
  glEnableVertexAttribArray(positionSlot);
  checkOpenGLError();
  glEnableVertexAttribArray(colorSlot);
  checkOpenGLError();

  glGenBuffers(1, &vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glGenBuffers(1, &indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectIndices), rectIndices,
    GL_STATIC_DRAW);
}

static void renderFrame(int handle) {
  glClearColor(0.67, 0.67, 0.67, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  if (!inited) {
    init();
    inited = 1;
  }

  glVertexAttribPointer(positionSlot, 3, GL_FLOAT, GL_FALSE,
    sizeof(vertex), 0);
  glVertexAttribPointer(colorSlot, 4, GL_FLOAT, GL_FALSE,
    sizeof(vertex), (GLvoid*) (sizeof(float) * 3));
  glDrawElements(GL_TRIANGLES, sizeof(rectIndices) / sizeof(rectIndices[0]),
    GL_UNSIGNED_BYTE, 0);

#ifndef __APPLE__
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  checkOpenGLError();
#endif
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
  phoneSetOpenGLViewRenderHandler(openGLView, renderFrame);
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

  canvasWidth = (int)openGLViewWidth;
  canvasHeight = (int)openGLViewHeight;

  phoneSetViewFrame(backgroundView, 0, 0, phoneGetViewWidth(0),
    phoneGetViewHeight(0));
}
