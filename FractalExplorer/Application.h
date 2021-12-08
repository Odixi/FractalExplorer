#pragma once


class Application
{
public: 
	Application();
	~Application();

	void run();

private:
	void initialize();

	GLFWwindow* m_window = nullptr;
};

