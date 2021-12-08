#pragma once


inline void glClearError() {
	while (glGetError() != GL_NO_ERROR);
}

inline void glCheckError() {
	while (GLenum error = glGetError() != GL_NO_ERROR) {
		std::cout << "[Error|OpenGL] " << error << std::endl;
		__debugbreak();
	}
}

template <typename Function>
void glv(Function f) {
	glClearError();
	f();
	glCheckError();
}

template <typename Function>
auto gl(Function f) {
	glClearError();
	auto r = f();
	glCheckError();
	return r;
}


