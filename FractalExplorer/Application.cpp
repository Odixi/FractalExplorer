#include "pch.h"

#include "Application.h"
#include "Shader.h"
#include "utils.h"
#include "TriangleHandler.h"
#include "Mandelbrot.h"

static void GLAPIENTRY glMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {

    if (type == GL_DEBUG_TYPE_ERROR) {
    std::cout << "[OpenGL message] source: " << source
        << " type: " << type << " id: " << id << " severity: " << severity << std::endl
        << "message: " << message << std::endl;
        __debugbreak();
    }
}

Application::Application()
{
    if (!glfwInit()) {
        std::cout << "GLFW init failed" << std::endl;
        throw;
    }
}

Application::~Application()
{
   glfwTerminate();
}


void Application::run()
{
    initialize();

    auto vertexShader = Shader::compileShader(utils::readTextFile("vertex.shader"), ShaderType::Vertex).value();
    auto fragmentShader = Shader::compileShader(utils::readTextFile("fragment.shader"), ShaderType::Fragment).value();

    auto program = ShaderProgram::createAndLink({
        &vertexShader,
        &fragmentShader
        });

    program->bind();

    // Just use this one for now
    uint32_t vertexArrayId;
    glGenVertexArrays(1, &vertexArrayId);
    glBindVertexArray(vertexArrayId);



    uint32_t indexBufferId;
    glGenBuffers(1, &indexBufferId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferId);


    uint32_t vertexBufferId;
    glGenBuffers(1, &vertexBufferId);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);


    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    // Uniform stuff
    auto location = glGetUniformLocation(program->getId(), "u_color");
    float v = 0;
    bool p = true;

    TriangleHandler triangleHandler{ [](glm::vec2 pos, double scale, int maxIter ) {
        auto a = mandelbrot::calculateSmoothEscapeTime(std::complex<float>(pos.x, pos.y), maxIter);
        float c = a / (double)maxIter;
        return Vertex{ pos, Color{c,c,c,1} };
    } };

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(m_window))
    {
        /* Render here */

        glClear(GL_COLOR_BUFFER_BIT);

        triangleHandler.generateVertices(100);
        const auto& indices = triangleHandler.getIndeices();
        const auto& vertices = triangleHandler.getVertices();

        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_DYNAMIC_DRAW);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);

        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

        /* Swap front and back buffers */
        glfwSwapBuffers(m_window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vertexArrayId);
}

void Application::initialize()
{
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    /* Create a windowed mode window and its OpenGL context */
    m_window = glfwCreateWindow(640, 480, "Fractal Explorer", NULL, NULL);
    if (!m_window) {
        std::cout << "GLFW failed to create window" << std::endl;
        return;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(m_window);

    /*Init glew*/
    if (glewInit() != GLEW_OK) {
        std::cout << "GLEW init failed" << std::endl;
        return;
    }

    std::cout << glGetString(GL_VERSION) << std::endl;

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(glMessageCallback, nullptr);

}
