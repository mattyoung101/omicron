#include "gpu_manager.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <log/log.h>
#include <stdbool.h>
#include <interface/mmal/mmal_buffer.h>
#include "utils.h"
#include <SDL2/SDL.h>

// Manages the OpenGL ES part of the VideoCore GPU. Receives camera frames, turns them into textures and runs a fragment shader on them.
// Source for EGL stuff: https://github.com/matusnovak/rpi-opengl-without-x/blob/master/triangle.c (Public Domain)
// OpenGL guides used:
// - https://learnopengl.com/Getting-started/Hello-Triangle
// - https://open.gl/drawing
// - https://open.gl/textures
// - https://learnopengl.com/Getting-started/Textures

static uint16_t imageWidth, imageHeight;
static uint32_t vertexShader;
static uint32_t fragmentShader;
static uint32_t shaderProgram;
static GLint minBallUniform, maxBallUniform, minLineUniform, maxLineUniform, minBlueUniform, maxBlueUniform, minYellowUniform, maxYellowUniform;
static GLint textureUniform;
GLfloat minBallData[3], maxBallData[3], minLineData[3], maxLineData[3], minBlueData[3], maxBlueData[3], minYellowData[3], maxYellowData[3];

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *renderTex = NULL, *frameTex = NULL, *testBmp = NULL;

/** Checks for an OpenGL error and logs if one occurred **/
static void check_gl_error(void){
    GLenum error = glGetError();
    if (error != GL_NO_ERROR){
        log_warn("An OpenGL error occurred: %s (0x%X)", glErrorStr(error), error);
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

static int32_t ticks = 0;
uint8_t *gpu_manager_post(MMAL_BUFFER_HEADER_T *buf){
    uint8_t *outBuf = malloc(imageWidth * imageHeight * 3);
//    uint8_t number = (uint8_t) (rand() % (255 + 1 - 0) - 0); // NOLINT

    SDL_RenderClear(renderer);
    // "pitch" is supposed to be the number of bytes in a single row in the image, however, that causes a heap buffer
    // overflow so instead we use height and somehow that works??? honestly don't even know. I assume it's the way
    // MMAL stores the buffer or something
    // FIXME check if SDL_LockTexture and UnlockTexture is faster - note that are WRITE-ONLY!!!
    SDL_UpdateTexture(frameTex, NULL, buf->data, imageHeight * 3);
    SDL_RenderCopy(renderer, frameTex, NULL, NULL);
    SDL_RenderPresent(renderer);
    SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGB888, outBuf, imageHeight * 3);

    check_gl_error();
    return outBuf;
}

void gpu_manager_test(void){
    puts("running GPU manager test");
    SDL_SetRenderTarget(renderer, frameTex);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, testBmp, NULL, NULL);
    SDL_RenderPresent(renderer);

    puts("saving to disk");
    // note byte order is little endian
    SDL_Surface *surf = SDL_CreateRGBSurface(0, imageWidth, imageHeight, 24, 0, 0, 0, 0);
    if (surf == NULL){
        log_error("SDL_CreateRGBSurface failed: %s", SDL_GetError());
        return;
    }
    SDL_RenderReadPixels(renderer, NULL, surf->format->format, surf->pixels, surf->pitch);
    SDL_SaveBMP(surf, "frame.bmp");
    SDL_FreeSurface(surf);
}

void gpu_manager_init(uint16_t width, uint16_t height) {
    imageWidth = width;
    imageHeight = height;
    log_debug("Initialising GPU pipleine...");

    if (SDL_Init(SDL_INIT_VIDEO) != 0){
        log_error("Failed to initialise SDL2: %s", SDL_GetError());
        return;
    }

    window = SDL_CreateWindow("Omicam", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, 0);
    if (window == NULL){
        log_error("Failed to create SDL window: %s", SDL_GetError());
        return;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    if (renderer == NULL){
        log_error("Failed to create SDL renderer: %s", SDL_GetError());
        return;
    }

    renderTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, width, height);
    frameTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (frameTex == NULL || renderTex == NULL){
        log_error("Failed to create textures: %s", SDL_GetError());
        return;
    }

    SDL_Surface *testBmpSurf = SDL_LoadBMP("../test.bmp");
    if (testBmpSurf == NULL){
        log_error("Failed to load test bitmap: %s", SDL_GetError());
        return;
    }
    testBmp = SDL_CreateTextureFromSurface(renderer, testBmpSurf);
    if (testBmp == NULL){
        log_error("Failed to create test bitmap texture: %s", SDL_GetError());
        return;
    }
    SDL_FreeSurface(testBmpSurf);

    log_debug("SDL window and renderer created successfully");
}

void gpu_manager_dispose(void){
    log_trace("Disposing GPU manager");
    SDL_DestroyTexture(renderTex);
    SDL_DestroyTexture(frameTex);
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(testBmp);
    SDL_Quit();
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

