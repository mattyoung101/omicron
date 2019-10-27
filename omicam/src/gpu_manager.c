#include "gpu_manager.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <log/log.h>
#include <stdbool.h>
#include <interface/mmal/mmal_buffer.h>
#include "utils.h"

// Manages the OpenGL ES part of the VideoCore GPU. Receives camera frames, turns them into textures and runs a fragment shader on them.
// Source for EGL stuff: https://github.com/matusnovak/rpi-opengl-without-x/blob/master/triangle.c (Public Domain)
// OpenGL guides used:
// - https://learnopengl.com/Getting-started/Hello-Triangle
// - https://open.gl/drawing
// - https://open.gl/textures

static const EGLint configAttribs[] = {
    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8,
    EGL_RED_SIZE, 8, EGL_DEPTH_SIZE, 8,

    // Uncomment the following to enable MSAA
    // EGL_SAMPLE_BUFFERS, 1, // <-- Must be set to 1 to enable multisampling!
    // EGL_SAMPLES, 4, // <-- Number of samples

    // Uncomment the following to enable stencil buffer
    // EGL_STENCIL_SIZE, 1,

    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE
};
static const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
static EGLint pbufferAttribs[] = {
        EGL_WIDTH,
        0, // will be set dynamically
        EGL_HEIGHT,
        0, // will bet set dynamically
        EGL_NONE,
};
static const char *eglGetErrorStr(){
    switch (eglGetError()){
        case EGL_SUCCESS:
            return "The last function succeeded without error.";
        case EGL_NOT_INITIALIZED:
            return "EGL is not initialized, or could not be initialized, for the "
                   "specified EGL display connection.";
        case EGL_BAD_ACCESS:
            return "EGL cannot access a requested resource (for example a context "
                   "is bound in another thread).";
        case EGL_BAD_ALLOC:
            return "EGL failed to allocate resources for the requested operation.";
        case EGL_BAD_ATTRIBUTE:
            return "An unrecognized attribute or attribute value was passed in the "
                   "attribute list.";
        case EGL_BAD_CONTEXT:
            return "An EGLContext argument does not name a valid EGL rendering "
                   "context.";
        case EGL_BAD_CONFIG:
            return "An EGLConfig argument does not name a valid EGL frame buffer "
                   "configuration.";
        case EGL_BAD_CURRENT_SURFACE:
            return "The current surface of the calling thread is a window, pixel "
                   "buffer or pixmap that is no longer valid.";
        case EGL_BAD_DISPLAY:
            return "An EGLDisplay argument does not name a valid EGL display "
                   "connection.";
        case EGL_BAD_SURFACE:
            return "An EGLSurface argument does not name a valid surface (window, "
                   "pixel buffer or pixmap) configured for GL rendering.";
        case EGL_BAD_MATCH:
            return "Arguments are inconsistent (for example, a valid context "
                   "requires buffers not supplied by a valid surface).";
        case EGL_BAD_PARAMETER:
            return "One or more argument values are invalid.";
        case EGL_BAD_NATIVE_PIXMAP:
            return "A NativePixmapType argument does not refer to a valid native "
                   "pixmap.";
        case EGL_BAD_NATIVE_WINDOW:
            return "A NativeWindowType argument does not refer to a valid native "
                   "window.";
        case EGL_CONTEXT_LOST:
            return "A power management event has occurred. The application must "
                   "destroy all contexts and reinitialise OpenGL ES state and "
                   "objects to continue rendering.";
        default:
            break;
    }
    return "Unknown error!";
}
static float rectVertices[] = {
        -1.0f,  1.0f, 1.0f, 0.0f, 0.0f, // Top-left
        1.0f,  1.0f, 0.0f, 1.0f, 0.0f, // Top-right
        1.0f, -1.0f, 0.0f, 0.0f, 1.0f, // Bottom-right

        1.0f, -1.0f, 0.0f, 0.0f, 1.0f, // Bottom-right
        -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, // Bottom-left
        -1.0f,  1.0f, 1.0f, 0.0f, 0.0f  // Top-left
};
static EGLDisplay display;
static EGLSurface surface;
static EGLContext context;
static uint32_t vertexShader;
static uint32_t fragmentShader;
static uint32_t shaderProgram;
static GLuint texture;
static GLuint vbo; // vertex buffer object
static GLint minBallUniform, maxBallUniform, minLineUniform, maxLineUniform, minBlueUniform, maxBlueUniform, minYellowUniform, maxYellowUniform;
static GLint textureUniform;
GLfloat minBallData[3], maxBallData[3], minLineData[3], maxLineData[3], minBlueData[3], maxBlueData[3], minYellowData[3], maxYellowData[3];
static uint16_t imageWidth, imageHeight;

/** Checks for an OpenGL error and logs if one occurred **/
static void check_gl_error(void){
    GLenum error = glGetError();
    if (error != GL_NO_ERROR){
        log_warn("An OpenGL error occurred: 0x%X (consult gl2.h:185 for error codes)", error);
    }
}

/** Sets all the uniforms to the values specified in the associated uniform array **/
static void update_uniforms(void) {
    UPDATE_UNIFORM(minBallUniform, minBallData)
    UPDATE_UNIFORM(maxBallUniform, maxBallData)
    UPDATE_UNIFORM(minLineUniform, minLineData)
    UPDATE_UNIFORM(maxLineUniform, maxLineData)
    UPDATE_UNIFORM(minBlueUniform, minBlueData)
    UPDATE_UNIFORM(maxBlueUniform, maxBlueData)
    UPDATE_UNIFORM(minYellowUniform, minYellowData)
    UPDATE_UNIFORM(maxYellowUniform, maxYellowData)
    log_trace("Updated uniforms successfully");
}

/** Reads a shader from disk into a string **/
static const GLchar *read_shader(char *path){
    // source for reading file: https://stackoverflow.com/a/174552/5007892
    size_t length = 0;
    FILE *file = fopen(path, "r");
    if (file == NULL){
        log_error("Failed to open shader file: %s", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    length = ftell(file);
    char *buf = calloc(length + 1, sizeof(char));
    fseek(file, 0, SEEK_SET);
    fread(buf, 1, length, file);
    fclose(file);
    return buf;
}

/** Checks for shader compile errors and logs them if they occurred **/
static bool check_shader_compilation(uint32_t shader, char *name){
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    glGetShaderInfoLog(shader, 512, NULL, infoLog);

    if (!success){
        log_error("Error compiling %s shader! Log:", name);
        log_error("%s", infoLog);
        return false;
    }
    return true;
}

void gpu_manager_init(uint16_t width, uint16_t height) {
    pbufferAttribs[1] = width;
    pbufferAttribs[3] = height;
    imageWidth = width;
    imageHeight = height;
    int major, minor;
    EGLint numConfigs;
    EGLConfig config;
    log_debug("Initialising GPU pipeline...");

    // initialise and configure display
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        log_error("Cannot get EGL display: %s", eglGetErrorStr());
        return;
    }
    if (!eglInitialize(display, &major, &minor)) {
        log_error("Failed to initialise EGL: %s", eglGetErrorStr());
        eglTerminate(display);
        return;
    }
    if (!eglChooseConfig(display, configAttribs, &config, 1, &numConfigs)) {
        log_error("Failed to choose EGL config: %s", eglGetErrorStr());
        eglTerminate(display);
        return;
    }
    log_trace("Successfully initialised EGL");

    // create and bind surface
    surface = eglCreatePbufferSurface(display, config, pbufferAttribs);
    if (surface == EGL_NO_SURFACE) {
        log_error("Failed to create EGL pbuffer surface: %s", eglGetErrorStr());
        eglTerminate(display);
        return;
    }
    eglBindAPI(EGL_OPENGL_API);

    // create context
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        eglDestroySurface(display, surface);
        eglTerminate(display);
        return;
    }
    eglMakeCurrent(display, surface, surface, context);
    log_debug("Successfully acquired OpenGL context from EGL");

    // print version info
    const GLubyte *version = glGetString(GL_VERSION);
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const char *eglVendor = eglQueryString(display, EGL_VENDOR);
    const char *eglVersion = eglQueryString(display, EGL_VERSION);
    log_debug("GL: %s on %s %s", version, vendor, renderer);
    log_debug("EGL: %s provided by %s", eglVersion, eglVendor);

    // check the OpenGL context works
    glViewport(0, 0, width, height);
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    check_gl_error();

    // apparently (according to the triangle.c source) if you update EGL on the Pi it install a "fake version"
    if (width != viewport[2] || height != viewport[3]){
        log_warn("OpenGL context integrity check failed (glViewport/glGetIntegerv seems to be broken), check EGL driver");
    } else {
        log_trace("OpenGL context seems to be working correctly");
    }

    // first things first, deal with vertex stuff first (create VBOs, etc)
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);
    check_gl_error();
    log_trace("Vertex data initialised successfully");

    // now create the vertex and fragment shaders
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    shaderProgram = glCreateProgram();
    check_gl_error();

    // read from disk
    const GLchar *vertexSource = read_shader("../omicam.vert");
    const GLchar *fragmentSource = read_shader("../omicam.frag");
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    check_gl_error();

    // compile
    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);
    check_shader_compilation(vertexShader, "Vertex");
    check_shader_compilation(fragmentShader, "Fragment");
    check_gl_error();

    // attach to shader program and link (this does the actual compilation)
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    check_gl_error();
    glLinkProgram(shaderProgram);
    check_gl_error();

    // handle link errors and use program if successful
    int linkSuccess = 0;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkSuccess);
    if(!linkSuccess) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        log_error("Shader program link failed. Log:");
        log_error("%s", infoLog);
    } else {
        log_trace("Shader built successfully");
        glUseProgram(shaderProgram);
    }
    check_gl_error();

    // uniforms are variables in the GLSL shader accessible by the CPU, in this case we keep the ball thresholds in there
    // so they can be modified by a simple INI and also at runtime
    // we use macros because there's a shitload of code to error check each one of these
    GET_UNIFORM_LOCATION(minBallUniform, "minBall")
    GET_UNIFORM_LOCATION(maxBallUniform, "maxBall")
    GET_UNIFORM_LOCATION(minLineUniform, "minLine")
    GET_UNIFORM_LOCATION(maxLineUniform, "maxLine")
    GET_UNIFORM_LOCATION(minBlueUniform, "minBlue")
    GET_UNIFORM_LOCATION(maxBlueUniform, "maxBlue")
    GET_UNIFORM_LOCATION(minYellowUniform, "minYellow")
    GET_UNIFORM_LOCATION(maxYellowUniform, "maxYellow")
    textureUniform = glGetUniformLocation(shaderProgram, "u_texture");
    check_gl_error();
    update_uniforms();

    // setup attributes
    GLint posAttrib = glGetAttribLocation(shaderProgram, "a_position");
    glVertexAttribPointer(posAttrib, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(posAttrib);
    check_gl_error();
    log_trace("Shader values set successfully");

    // now that we've got our vertices and the shaders to deal with them, it's time to create the texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    check_gl_error();

    log_debug("GPU pipeline initialised successfully");
    free((GLchar*) vertexSource);
    free((GLchar*) fragmentSource);
}

void gpu_manager_post(MMAL_BUFFER_HEADER_T *buf){
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // bind texture with glTexImage2D here
    // TODO we still need to bind the texture before this and also somehow set a_texCoord ??
    glDrawArrays(GL_TRIANGLES, 0, 6);

    check_gl_error();
}

void gpu_manager_dispose(void){
    log_trace("Disposing GPU manager");
    glDetachShader(shaderProgram, vertexShader);
    glDetachShader(shaderProgram, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteProgram(shaderProgram);
    glDeleteTextures(1, &texture);
    glDeleteBuffers(1, &vbo);
    check_gl_error();

    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);
}

void gpu_manager_parse_thresh(char *threshStr, GLfloat *uniformArray){
    char *token;
    char *threshOrig = strdup(threshStr);
    uint8_t i = 0;
    token = strtok(threshStr, ",");

    while (token != NULL){
        char *invalid = NULL;
        float number = strtof(token, &invalid);

        if (number > 255){
            log_error("Invalid threshold string \"%s\": token %s > 255 (not in RGB colour range)", threshOrig, token);
        } else if (strlen(invalid) != 0){
            log_error("Invalid threshold string \"%s\": invalid token: \"%s\"", threshOrig, invalid);
        } else {
            uniformArray[i++] = number;
            if (i > 3){
                log_error("Too many values for key: %s (max: 3)", threshOrig);
                return;
            }
        }
        token = strtok(NULL, ",");
    }
    // log_trace("Successfully parsed threshold key: %s", threshOrig);
    free(threshOrig);
}

