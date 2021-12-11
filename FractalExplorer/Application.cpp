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

    constexpr auto getColor = [](double value)->Color {
        constexpr int divider = 400;
        const int floor = static_cast<int>(value);
        double v = (floor % divider) + value - floor;
        Color colors[] = {
            Color{0,0,0,1},
            Color{0,0,1,1},
            Color{0,1,1,1},
            Color{1,0,0,1},
            Color{1,1,0,1},
            Color{1,1,1,1},
        };
        const auto size = std::size(colors);
        const double slice = static_cast<double>(divider) / size;
        for (int i = 0; i < size; ++i) {
            double start = i * slice;
            if (v <= start) {
                Color c1 = colors[i];
                Color c2 = i < size - 1 ? colors[i + 1] : colors[0];
                float f = (start - v) / slice;
                return glm::lerp(c2, c1, {f,f,f,f});
            }
        }
        return { 0,0,0,0 };
    };


    TriangleHandler triangleHandler{ [](glm::vec2 pos, double scale, int maxIter ) {
        auto a = mandelbrot::calculateSmoothEscapeTime(std::complex<float>(pos.x, pos.y), maxIter);
        //float c = a / (double)maxIter;
        return Vertex{ pos, getColor(a) };
    } };

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(m_window))
    {

        // Update mouse position
        glm::vec2 oldMousePos = m_mouseInfo.position;
        double x, y;
        glfwGetCursorPos(m_window, &x, &y);
        m_mouseInfo.position = { x,y };

        // Handle input - todo move
        if (m_mouseInfo.buttons & mouse::mouseLeft) {
            auto delta = (m_mouseInfo.position - oldMousePos);
            delta.x *= -1; 
            m_navigationInfo.realPosition += (delta * 0.001f / m_navigationInfo.realZoom);
            std::cout << "pos: " << m_navigationInfo.realPosition.x << " " << m_navigationInfo.realPosition.y << std::endl;
            std::cout << "worldPos: " << mouseWorldPos().x << " " << mouseWorldPos().y << std::endl;
        }

        // Interpolate real position and camera position
        m_navigationInfo.cameraPosition = glm::lerp(m_navigationInfo.realPosition, m_navigationInfo.cameraPosition, glm::vec2(m_navigationInfo.interpalotionValue, m_navigationInfo.interpalotionValue));
        m_navigationInfo.cameraZoom = glm::lerp(m_navigationInfo.realZoom, m_navigationInfo.cameraZoom, m_navigationInfo.interpalotionValue);

        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        // Screen bb
        auto screenSize = glm::vec2{1,1} * (1.0f / m_navigationInfo.cameraZoom);
        geom::BBox2 screenBb{ m_navigationInfo.cameraPosition - (screenSize), m_navigationInfo.cameraPosition + (screenSize) };

        triangleHandler.removeTrianglesOutsideScreen(screenBb, 10000);
        triangleHandler.generateVertices(screenBb, 100);
        const auto& indices = triangleHandler.getIndeices();
        const auto& vertices = triangleHandler.getVertices();

        glUniform4f(locationUniformId, m_navigationInfo.cameraPosition.x, m_navigationInfo.cameraPosition.y, 0.0f, 1.0f);
        glUniform1f(zoomUniformId, m_navigationInfo.cameraZoom);
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
    zoom(glm::pow(1.1f, amount));
}

void Application::onScrollDown(float amount)
{
    zoom(glm::pow(0.9f, amount));
}

void Application::zoom(float multiplier)
{
    m_navigationInfo.realZoom *= multiplier;

    // Move camera towards current cursor poosition
    auto dir = mouseWorldPos() - m_navigationInfo.realPosition;
    m_navigationInfo.realPosition += dir * (multiplier - 1);
}

void Application::toggleWireframe()
{
    static bool wireframe = false;
    wireframe = !wireframe;
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
}

glm::vec2 Application::mouseWorldPos() const
{
    int w, h;
    glfwGetWindowSize(m_window, &w, &h);

    glm::vec2 deviationFromMiddle = (glm::vec2{ m_mouseInfo.position.x / w, m_mouseInfo.position.y / h } - glm::vec2{0.5f, 0.5f})*2.0f;
    deviationFromMiddle.y *= -1;
    
    return m_navigationInfo.cameraPosition + deviationFromMiddle / m_navigationInfo.cameraZoom;
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

    if (action) {
        switch (key)
        {
            case GLFW_KEY_F6:
                App->toggleWireframe();
            default:
                break;
        }
    }
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

    if (yoffset > 0) {
        App->onScrollUp(yoffset);
    }
    if (yoffset < 0) {
        App->onScrollDown(-yoffset);
    }
}

