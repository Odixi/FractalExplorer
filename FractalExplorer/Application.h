#pragma once


namespace mouse {
	constexpr int mouseLeft = 0x1;
	constexpr int mouseRight = 0x2;
}

class Application
{
public: 
	Application();
	~Application();

	void run();

	void onScrollUp(float amount);
	void onScrollDown(float amount);

private:
	void initialize();

	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	void zoom(float multiplier);
	void toggleWireframe();

	glm::vec2 mouseWorldPos() const;

	GLFWwindow* m_window = nullptr;

	struct NavigationInfo {
		glm::vec2 cameraPosition = {0,0};
		glm::vec2 realPosition = { 0,0 };
		float cameraZoom = 1;
		float realZoom = 1;
		float interpalotionValue = 0.95f;
	} m_navigationInfo;

	struct MouseInfo {
		glm::vec2 position;
		int buttons;
	} m_mouseInfo;



};

