#pragma once
// Cross-platform OpenGL header.
// Desktop: OpenGL 3.3 Core. macOS uses Apple's framework (no loader needed);
// Windows/Linux load function pointers through GLEW.
// Web (Emscripten): OpenGL ES 3.0, which the browser provides as WebGL2.
// Always include this file instead of <OpenGL/gl3.h> or <GL/gl.h>.
#if defined(__EMSCRIPTEN__)
    #include <GLES3/gl3.h>
#elif defined(__APPLE__)
    #include <OpenGL/gl3.h>
#else
    #include <GL/glew.h>
#endif
