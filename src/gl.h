#pragma once
// Cross-platform OpenGL 3.3 Core Profile header.
// On macOS: use Apple's built-in OpenGL framework (no loader needed).
// On Windows/Linux: use GLEW to load function pointers at runtime.
// Always include this file instead of <OpenGL/gl3.h> or <GL/gl.h>.
#ifdef __APPLE__
    #include <OpenGL/gl3.h>
#else
    #include <GL/glew.h>
#endif
