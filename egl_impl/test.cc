#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdio.h>

int main()
{
    // 初始化EGL
    EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint major = 0, minor = 0;
    eglInitialize(eglDisplay, &major, &minor);
    printf("egl version %d.%d\n", major, minor);

    // 配置EGL
    EGLConfig config;
    EGLint numConfigs;
    EGLint configAttribs[] = {EGL_SURFACE_TYPE,
                              EGL_PBUFFER_BIT,
                              EGL_RENDERABLE_TYPE,
                              EGL_OPENGL_ES2_BIT,
                              EGL_RED_SIZE,
                              8,
                              EGL_GREEN_SIZE,
                              8,
                              EGL_BLUE_SIZE,
                              8,
                              EGL_NONE};
    eglChooseConfig(eglDisplay, configAttribs, &config, 1, &numConfigs);

    // 创建EGL Pbuffer表面
    EGLint attribList[] = {EGL_WIDTH, 512, EGL_HEIGHT, 512, EGL_NONE};
    EGLSurface eglSurface =
        eglCreatePbufferSurface(eglDisplay, config, attribList);
    printf("get pbuffer surface %p\n", eglSurface);

    // 创建EGL上下文
    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext eglContext =
        eglCreateContext(eglDisplay, config, EGL_NO_CONTEXT, contextAttribs);
    printf("get egl context %p\n", eglContext);

    // 关联上下文
    eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);

    // 渲染操作
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    // 交换缓冲区
    eglSwapBuffers(eglDisplay, eglSurface);

    uint8_t pixels[4] = {};
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    printf("[0]=0x%02x [1]=0x%02x [2]=0x%02x [3]=0x%02x\n", pixels[0],
           pixels[1], pixels[2], pixels[3]);

    // 销毁资源
    eglDestroySurface(eglDisplay, eglSurface);
    eglDestroyContext(eglDisplay, eglContext);
    eglTerminate(eglDisplay);
}
