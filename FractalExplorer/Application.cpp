#include "pch.h"

#include "Application.h"
#include "Shader.h"
#include "utils.h"
#include "TriangleHandler.h"
#include "Mandelbrot.h"

namespace {

    Application* App = nullptr;
}

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
    if (App != nullptr) {
        throw;
    }
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
    auto locationUniformId = glGetUniformLocation(program->getId(), "camera");
    auto zoomUniformId = glGetUniformLocation(program->getId(), "zoom");


    TriangleHandler triangleHandler{ [](glm::vec2 pos, double scale, int maxIter ) {
        auto a = mandelbrot::calculateSmoothEscapeTime(std::complex<float>(pos.x, pos.y), maxIter);
        float c = a / (double)maxIter;
        return Vertex{ pos, Color{c,c,c,1} };
    } };

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(m_window))
    {

        // Update mouse position
        glm::vec2 oldMousePos = m_mouseInfo.position;
        double x, y;
        glfwGetCursorPos(m_window, &x, &y);
        m_mouseInfo.position = { -x,y };

        // Handle input - todo move
        if (m_mouseInfo.buttons & mouse::mouseLeft) {
            auto delta = m_mouseInfo.position - oldMousePos;
            m_navigationInfo.position += (delta * 0.001f / m_navigationInfo.zoom);
            std::cout << "pos: " << m_navigationInfo.position.x << " " << m_navigationInfo.position.y << std::endl;
        }

        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        // Screen bb
        auto screenSize = glm::vec2{1,1} * (1.0f / m_navigationInfo.zoom);
        geom::BBox2 screenBb{ m_navigationInfo.position - (screenSize), m_navigationInfo.position + (screenSize) };

        triangleHandler.removeTrianglesOutsideScreen(screenBb, 1000);
        triangleHandler.generateVertices(screenBb, 1000);
        const auto& indices = triangleHandler.getIndeices();
        const auto& vertices = triangleHandler.getVertices();

        glUniform4f(locationUniformId, m_navigationInfo.position.x, m_navigationInfo.position.y, 0.0f, 1.0f);
        glUniform1f(zoomUniformId, m_navigationInfo.zoom);
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

void Application::onScrollUp(float amount)
{
    m_navigationInfo.zoom *= (glm::pow(1.1f, amount));
}

void Application::onScrollDown(float amount)
{
    m_navigationInfo.zoom *= (glm::pow(0.9f, amount));
}

void Application::initialize()
{
    App = this;
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    /* Create a windowed mode window and its OpenGL context */
    m_window = glfwCreateWindow(1280, 1280, "Fractal Explorer", NULL, NULL);
    if (!m_window) {
        std::cout << "GLFW failed to create window" << std::endl;
        return;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(m_window);

    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetScrollCallback(m_window, scrollCallback);

    double x, y;
    glfwGetCursorPos(m_window, &x, &y);
    m_mouseInfo.position = { x,y };

    /*Init glew*/
    if (glewInit() != GLEW_OK) {
        std::cout << "GLEW init failed" << std::endl;
        return;
    }

    std::cout << glGetString(GL_VERSION) << std::endl;

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(glMessageCallback, nullptr);

}

void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    std::cout << "Key event: " << key
        << " scancode: " << scancode << " action: " << action << " mods: " << mods << std::endl;
}

void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    std::cout << "Mouse event: " << button
         << " action: " << action << " mods: " << mods << std::endl;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == 0) {
            App->m_mouseInfo.buttons &= ~mouse::mouseLeft;
        }
        else {
            App->m_mouseInfo.buttons |= mouse::mouseLeft;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_1) {
        if (action == 0) {
            App->m_mouseInfo.buttons &= ~mouse::mouseRight;
        }
        else {
            App->m_mouseInfo.buttons |= mouse::mouseRight;
        }
    }
}

void Application::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    std::cout << "Scroll event "
        << " xoffset: " << xoffset << " yoffset: " << yoffset << std::endl;

    if (yoffset > 0) {
        App->onScrollUp(yoffset);
    }
    if (yoffset < 0) {
        App->onScrollDown(-yoffset);
    }
}
