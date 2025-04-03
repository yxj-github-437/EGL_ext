#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <iostream>

// 声明EGL相关变量
EGLDisplay display;
EGLContext context;

// PBO相关变量
GLuint pbo;
GLuint fbo;
GLuint texture;

GLuint shaderProgram;
int width = 512, height = 512;

// 着色器源码
const char* vertexShaderSource = R"(
    #version 300 es
    layout(location = 0) in vec4 aPos;
    void main() {
        gl_Position = aPos;
    }
)";

const char* fragmentShaderSource = R"(
    #version 300 es
    precision mediump float;
    out vec4 FragColor;
    void main() {
        FragColor = vec4(1.0, 0.8, 0.5, 1.0);
    }
)";

// 编译单个着色器的函数
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader Compilation Error: " << infoLog << std::endl;
    }
    return shader;
}

// 创建着色器程序
GLuint createShaderProgram() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Program Linking Error: " << infoLog << std::endl;
    }
    return program;
}

// 初始化EGL环境
bool initializeEGL() {
    // 获取EGL显示设备
    display = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
    if (display == EGL_NO_DISPLAY) {
        std::cerr << "Unable to get EGL display" << std::endl;
        return false;
    }

    // 初始化EGL
    EGLint major = 0, minor = 0;
    if (!eglInitialize(display, &major, &minor)) {
        std::cerr << "Unable to initialize EGL" << std::endl;
        return false;
    }
    std::cout << "EGL version: " << major << "." << minor << std::endl;

    // 配置EGL属性（选择一个支持OpenGL ES 3.0的配置）
    EGLConfig config;
    EGLint numConfigs;
    const EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
    if (!eglChooseConfig(display, attribs, &config, 1, &numConfigs) || numConfigs == 0) {
        std::cerr << "Unable to choose EGL config" << std::endl;
        return false;
    }

    // 创建OpenGL ES上下文
    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3, // OpenGL ES 3.0
        EGL_NONE
    };
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        std::cerr << "Unable to create EGL context" << std::endl;
        return false;
    }

    // 将上下文和表面关联
    if (!eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context)) {
        std::cerr << "Unable to make EGL context current" << std::endl;
        return false;
    }

    return true;
}

// 初始化OpenGL ES
void initializeOpenGL() {
    // 设置视口
    glViewport(0, 0, width, height); // 设置为512x512的视口大小

    // 创建纹理
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    // 创建FBO
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    // 检查FBO状态
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer is not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 创建PBO
    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
    glBufferData(GL_PIXEL_PACK_BUFFER, 512 * 512 * 4, nullptr, GL_STATIC_READ); // 512x512 RGBA
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

// 渲染到FBO并读取像素数据
void renderToFBO() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 渲染操作（这里假设已经有渲染代码）
    glUseProgram(shaderProgram); // 使用着色器程序

    // 定义一个正方形的顶点数据
    GLfloat vertices[] = {
        -1.0f,  1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4); // 绘制正方形

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

// 从PBO读取像素数据并打印
void readPixelsWithPBO() {
    // 读取像素数据到PBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
    glReadPixels(0, 0, 512, 512, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // 读取像素数据
    GLubyte* ptr = (GLubyte*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, width * height * 4, GL_MAP_READ_BIT);

    if (ptr) {
        // 打印部分像素数据（例如，打印前10个像素的RGBA值）
        std::cout << "First 10 pixels (RGBA values):" << std::endl;
        for (int i = 0; i < 10; ++i) {
            int index = i * 4; // 每个像素占用4个字节（RGBA）
            std::cout << "("
                      << (int)ptr[index] << ", "
                      << (int)ptr[index + 1] << ", "
                      << (int)ptr[index + 2] << ", "
                      << (int)ptr[index + 3] << ")" << std::endl;
        }

        // 解除映射
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    } else {
        std::cerr << "Failed to map PBO" << std::endl;
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// 主函数
int main() {
    // 初始化EGL环境
    if (!initializeEGL()) {
        return -1;
    }

    // 初始化OpenGL ES
    initializeOpenGL();

    // 创建着色器程序
    shaderProgram = createShaderProgram();


    // 渲染到FBO并读取像素数据
    renderToFBO();

    // 读取像素数据并打印
    readPixelsWithPBO();

    // 清理资源
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texture);
    glDeleteBuffers(1, &pbo);

    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglTerminate(display);
    return 0;
}
