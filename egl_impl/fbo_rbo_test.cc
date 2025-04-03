#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <iostream>

EGLDisplay display;
EGLContext context;
GLuint fbo, texture, rbo;

GLuint shaderProgram;
GLint width = 800, height = 600;


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
    glViewport(0, 0, width, height);
}


// 创建并设置 FBO
void createFBO()
{
    // 创建颜色纹理
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // 将纹理附加到 FBO
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           texture, 0);
    // 检查 FBO 完整性
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Framebuffer not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind FBO

    // 创建并绑定 RBO (用于深度测试)
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

// 使用 FBO 渲染场景
void renderScene()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo); // 绑定 FBO 进行离屏渲染

    // 在这里执行绘制操作
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

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // 完成渲染后解绑 FBO
}

// 读取 FBO 中的像素数据
void readPixelsFromFBO()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo); // 绑定 FBO

    glPixelStorei(GL_PACK_ROW_LENGTH, width);
    // 使用 glReadPixels 读取像素数据
    GLubyte* pixels = new GLubyte[width * height * 4]; // 存储 RGBA 数据
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // 处理读取到的像素数据 (例如打印部分数据)
    // 打印部分像素数据（例如，打印前10个像素的RGBA值）
    std::cout << "First 10 pixels (RGBA values):" << std::endl;
    for (int i = 0; i < 10; ++i) {
        int index = i * 4; // 每个像素占用4个字节（RGBA）
        std::cout << "("
                  << (int)pixels[index] << ", "
                  << (int)pixels[index + 1] << ", "
                  << (int)pixels[index + 2] << ", "
                  << (int)pixels[index + 3] << ")" << std::endl;
    }

    delete[] pixels;
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // 完成读取后解绑 FBO
}

int main()
{
    // 初始化EGL环境
    if (!initializeEGL())
    {
        return -1;
    }

    // 初始化OpenGL ES
    initializeOpenGL();

    // 创建着色器程序
    shaderProgram = createShaderProgram();

    createFBO();
    renderScene();
    readPixelsFromFBO();

    // 清理资源
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texture);
    glDeleteRenderbuffers(1, &rbo);

    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglTerminate(display);
    return 0;
}
