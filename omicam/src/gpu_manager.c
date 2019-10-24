#include "gpu_manager.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <log/log.h>

// Manages the OpenGL ES part of the VideoCore GPU. Receives camera frames, turns them into textures and runs a fragment shader on them.
// Source for EGL stuff: https://github.com/matusnovak/rpi-opengl-without-x/blob/master/triangle.c (Public Domain)

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

static const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2,
                                        EGL_NONE};

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

static EGLDisplay display;
static EGLSurface surface;
static EGLContext context;

void gpu_manager_init(uint16_t width, uint16_t height) {
    pbufferAttribs[1] = width;
    pbufferAttribs[3] = height;
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
    log_debug("Successfully initialised EGL");

    // create and bind surface
    surface = eglCreatePbufferSurface(display, config, pbufferAttribs);
    if (surface == EGL_NO_SURFACE) {
        log_error("Failed to create EGL pbuffer surface: %s", eglGetErrorStr());
        eglTerminate(display);
        return;
    }
    eglBindAPI(EGL_OPENGL_API);
    log_debug("Successfully created and bound EGL surface");

    // create context
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        eglDestroySurface(display, surface);
        eglTerminate(display);
        return;
    }

    eglMakeCurrent(display, surface, surface, context);
    log_info("GPU initialised successfully");
}

void gpu_manager_post(uint8_t *frameBuffer){
    // turn framebuffer into opengl texture
    // upload to GPU and invoke fragment shader
    // download resulting texture from GPU back into a new buffer (will need to return this) with glReadPixels
}

void gpu_manager_dispose(void){
    log_trace("Disposing GPU manager");
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);
}
