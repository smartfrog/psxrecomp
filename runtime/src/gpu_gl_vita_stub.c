/* gpu_gl_vita_stub.c — link-time stubs for the ~28 desktop-GL 1.x entry
 * points gpu_gl_renderer.c calls directly (everything newer is loaded through
 * SDL_GL_GetProcAddress). Vita has no OpenGL: SDL_GL_CreateContext cannot
 * succeed there, so gl_renderer_init_context always fails and falls back to
 * the SDL_Renderer software present path before any of these can be reached.
 * Each stub aborts loudly instead of silently returning, so a future GL-on-
 * Vita attempt (vgl) surfaces immediately instead of corrupting state.
 * Only linked on Vita (runtime.cmake appends it under if(VITA)). */
#include "psx_sdl.h"
#if !defined(PSX_SDL3)
#include <SDL_opengl.h>
#endif

#include <stdlib.h>

static void psx_gl_vita_unreachable(const char *name)
{
    /* No GL context can exist on Vita; reaching here is a programming error. */
    (void)fprintf(stderr, "psxrecomp: GL function %s called on Vita (no GL)\n",
                  name);
    abort();
}

#define PSX_GL_VITA_STUB(ret, name, args)                        \
    ret name args                                                \
    {                                                            \
        psx_gl_vita_unreachable(#name);                          \
    }

PSX_GL_VITA_STUB(void, glClear, (GLbitfield mask))
PSX_GL_VITA_STUB(void, glClearColor, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha))
PSX_GL_VITA_STUB(void, glClearStencil, (GLint s))
PSX_GL_VITA_STUB(void, glColorMask, (GLboolean r, GLboolean g, GLboolean b, GLboolean a))
PSX_GL_VITA_STUB(void, glViewport, (GLint x, GLint y, GLsizei width, GLsizei height))
PSX_GL_VITA_STUB(void, glEnable, (GLenum cap))
PSX_GL_VITA_STUB(void, glDisable, (GLenum cap))
PSX_GL_VITA_STUB(void, glScissor, (GLint x, GLint y, GLsizei width, GLsizei height))
PSX_GL_VITA_STUB(void, glLineWidth, (GLfloat width))
PSX_GL_VITA_STUB(void, glPixelStorei, (GLenum pname, GLint param))
PSX_GL_VITA_STUB(void, glReadBuffer, (GLenum mode))
PSX_GL_VITA_STUB(void, glReadPixels, (GLint x, GLint y, GLsizei width, GLsizei height,
                                      GLenum format, GLenum type, void *data))
PSX_GL_VITA_STUB(void, glGenTextures, (GLsizei n, GLuint *textures))
PSX_GL_VITA_STUB(void, glDeleteTextures, (GLsizei n, const GLuint *textures))
PSX_GL_VITA_STUB(void, glBindTexture, (GLenum target, GLuint texture))
PSX_GL_VITA_STUB(void, glTexParameteri, (GLenum target, GLenum pname, GLint param))
PSX_GL_VITA_STUB(void, glTexImage2D, (GLenum target, GLint level, GLint internalformat,
                                      GLsizei width, GLsizei height, GLint border,
                                      GLenum format, GLenum type, const void *pixels))
PSX_GL_VITA_STUB(void, glTexSubImage2D, (GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                         GLsizei width, GLsizei height, GLenum format,
                                         GLenum type, const void *pixels))
PSX_GL_VITA_STUB(void, glCopyTexSubImage2D, (GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                             GLint x, GLint y, GLsizei width, GLsizei height))
PSX_GL_VITA_STUB(void, glDrawArrays, (GLenum mode, GLint first, GLsizei count))
PSX_GL_VITA_STUB(void, glStencilFunc, (GLenum func, GLint ref, GLuint mask))
PSX_GL_VITA_STUB(void, glStencilMask, (GLuint mask))
PSX_GL_VITA_STUB(void, glStencilOp, (GLenum fail, GLenum zfail, GLenum zpass))
PSX_GL_VITA_STUB(void, glGetIntegerv, (GLenum pname, GLint *data))
PSX_GL_VITA_STUB(const GLubyte *, glGetString, (GLenum name))
PSX_GL_VITA_STUB(GLenum, glGetError, (void))
PSX_GL_VITA_STUB(void, glFlush, (void))
PSX_GL_VITA_STUB(void, glFinish, (void))
