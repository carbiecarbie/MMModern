#include "platform/sdl/SdlCrtFilter.h"
#include <SDL.h>
#include <SDL_opengl.h>

#include <stdexcept>
#include <string_view>

namespace mmodern {
namespace {

template<class T> T entry(const char *name) {
    auto proc = reinterpret_cast<T>(SDL_GL_GetProcAddress(name));
    if (!proc) throw std::runtime_error(std::string("OpenGL is missing ") + name);
    return proc;
}

// Adapted from zfast_crt_standard by Greg Hogan (SoltanGris42), copyright 2017.
// GPL-2.0-or-later. libretro/glsl-shaders, a35069864b4bf30f8c8e740d57206543fb7e745e,
// crt/shaders/zfast_crt.glsl. MMModern supplies the output rectangle, compensates
// for the VGA line doubling, and selects two brightness/scanline/mask profiles.
constexpr char vertex[] = R"(#version 120
varying vec2 uv;
void main() {
    gl_Position = gl_Vertex;
    uv = vec2(gl_Vertex.x * 0.5 + 0.5, 0.5 - gl_Vertex.y * 0.5);
})";
constexpr char fragment[] = R"(#version 120
uniform sampler2D frame;
uniform vec2 sourceSize;
uniform vec2 textureExtent;
uniform float outputHeight;
uniform float phosphor;
varying vec2 uv;
void main() {
    vec2 source = uv * sourceSize;
    vec2 center = floor(source) + 0.5;
    vec2 f = source - center;
    vec2 sampleAt = center + 4.0 * f*f*f;
    sampleAt.x = mix(sampleAt.x, source.x, mix(0.18, 0.40, phosphor));
    vec3 colour = texture2D(frame, clamp(sampleAt, vec2(0.5), sourceSize - 0.5)
                                   * textureExtent / sourceSize).rgb;

    // Put the beam centre on a physical pixel centre. At Deck resolution
    // exactly two output rows sample each VGA line; a symmetric phase
    // would give those two samples identical brightness.
    float y = fract(source.y * 2.0 - sourceSize.y / outputHeight + 0.5) - 0.5;
    float yy = y*y;
    float low = 1.0 - mix(2.6, 4.8, phosphor) * (yy - 2.05*yy*yy);
    float high = 1.0 - mix(3.8, 7.0, phosphor) * (yy*yy - 2.8*yy*yy*yy);
    // Fade fine lines when the window has too few physical rows to resolve them.
    float resolved = clamp(outputHeight / (sourceSize.y * 2.0) - 1.0, 0.0, 1.0);
    float beam = mix(1.0, mix(low, high, dot(colour, vec3(0.2667))), resolved);
    float dark = mix(0.055, 0.30, phosphor);
    float mask = 1.0 - dark * mod(floor(gl_FragCoord.x), 2.0);
    gl_FragColor = vec4(colour * beam * mask * (1.0 + dark * 0.45 + 0.04 * resolved), 1.0);
})";

} // namespace

struct SdlCrtFilter::Program {
    PFNGLCREATESHADERPROC createShader = entry<PFNGLCREATESHADERPROC>("glCreateShader");
    PFNGLSHADERSOURCEPROC shaderSource = entry<PFNGLSHADERSOURCEPROC>("glShaderSource");
    PFNGLCOMPILESHADERPROC compileShader = entry<PFNGLCOMPILESHADERPROC>("glCompileShader");
    PFNGLGETSHADERIVPROC getShader = entry<PFNGLGETSHADERIVPROC>("glGetShaderiv");
    PFNGLGETSHADERINFOLOGPROC shaderLog = entry<PFNGLGETSHADERINFOLOGPROC>("glGetShaderInfoLog");
    PFNGLDELETESHADERPROC deleteShader = entry<PFNGLDELETESHADERPROC>("glDeleteShader");
    PFNGLCREATEPROGRAMPROC createProgram = entry<PFNGLCREATEPROGRAMPROC>("glCreateProgram");
    PFNGLATTACHSHADERPROC attach = entry<PFNGLATTACHSHADERPROC>("glAttachShader");
    PFNGLLINKPROGRAMPROC link = entry<PFNGLLINKPROGRAMPROC>("glLinkProgram");
    PFNGLGETPROGRAMIVPROC getProgram = entry<PFNGLGETPROGRAMIVPROC>("glGetProgramiv");
    PFNGLGETPROGRAMINFOLOGPROC programLog = entry<PFNGLGETPROGRAMINFOLOGPROC>("glGetProgramInfoLog");
    PFNGLDELETEPROGRAMPROC deleteProgram = entry<PFNGLDELETEPROGRAMPROC>("glDeleteProgram");
    PFNGLUSEPROGRAMPROC use = entry<PFNGLUSEPROGRAMPROC>("glUseProgram");
    PFNGLGETUNIFORMLOCATIONPROC location = entry<PFNGLGETUNIFORMLOCATIONPROC>("glGetUniformLocation");
    PFNGLUNIFORM1FPROC set1 = entry<PFNGLUNIFORM1FPROC>("glUniform1f");
    PFNGLUNIFORM2FPROC set2 = entry<PFNGLUNIFORM2FPROC>("glUniform2f");
    decltype(&glGetIntegerv) getInt = entry<decltype(getInt)>("glGetIntegerv");
    decltype(&glPushAttrib) push = entry<decltype(push)>("glPushAttrib");
    decltype(&glPopAttrib) pop = entry<decltype(pop)>("glPopAttrib");
    decltype(&glViewport) viewport = entry<decltype(viewport)>("glViewport");
    decltype(&glDisable) disable = entry<decltype(disable)>("glDisable");
    decltype(&glRectf) rectangle = entry<decltype(rectangle)>("glRectf");

    GLuint id = 0;
    GLint size = -1, extent = -1, height = -1, profile = -1;
    SDL_GLContext context = SDL_GL_GetCurrentContext();
    ~Program() {
        if (id && context == SDL_GL_GetCurrentContext()) deleteProgram(id);
    }

    void addShader(GLenum type, const char *source) {
        GLuint shader = createShader(type);
        shaderSource(shader, 1, &source, nullptr);
        compileShader(shader);
        GLint compiled = 0;
        getShader(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            char message[512]{};
            shaderLog(shader, sizeof(message), nullptr, message);
            deleteShader(shader);
            throw std::runtime_error(std::string("CRT shader compilation failed: ") + message);
        }
        attach(id, shader);
        deleteShader(shader);
    }
    void initialize() {
        id = createProgram();
        addShader(GL_VERTEX_SHADER, vertex);
        addShader(GL_FRAGMENT_SHADER, fragment);
        link(id);
        GLint linked = 0;
        getProgram(id, GL_LINK_STATUS, &linked);
        if (!linked) {
            char message[512]{};
            programLog(id, sizeof(message), nullptr, message);
            throw std::runtime_error(std::string("CRT shader link failed: ") + message);
        }
        size = location(id, "sourceSize");
        extent = location(id, "textureExtent");
        height = location(id, "outputHeight");
        profile = location(id, "phosphor");
    }
};

SdlCrtFilter::SdlCrtFilter() = default;
SdlCrtFilter::~SdlCrtFilter() = default;

void SdlCrtFilter::reset() {
    _program.reset();
    _error.clear();
}

bool SdlCrtFilter::draw(SDL_Renderer *renderer, SDL_Texture *frame, int width, int height,
        PictureRect picture, int outputHeight, PictureFilter filter) {
    if (!_error.empty()) return false;
    if (!_program) {
        SDL_RendererInfo info{};
        if (SDL_GetRendererInfo(renderer, &info) || std::string_view(info.name) != "opengl") {
            _error = "Desktop OpenGL is unavailable";
            return false;
        }
    }

    float tw = 0, th = 0;
    if (SDL_RenderFlush(renderer) || SDL_GL_BindTexture(frame, &tw, &th)) {
        _error = SDL_GetError();
        return false;
    }
    // Binding activates this renderer's context, including on return from
    // a nested game window. Create and delete programs only in their owner.
    struct Unbind {
        SDL_Texture *texture;
        ~Unbind() { SDL_GL_UnbindTexture(texture); }
    } unbind{frame};
    if (tw > 1 || th > 1) {
        _error = "The OpenGL driver requires rectangle textures";
        return false;
    }
    if (!_program || _program->context != SDL_GL_GetCurrentContext()) {
        try {
            _program = std::make_unique<Program>();
            _program->initialize();
        } catch (const std::exception &error) {
            _error = error.what();
            _program.reset();
            return false;
        }
    }
    auto &gl = *_program;
    GLint previous = 0;
    gl.getInt(GL_CURRENT_PROGRAM, &previous);
    gl.push(GL_VIEWPORT_BIT | GL_ENABLE_BIT);
    gl.viewport(picture.x, outputHeight - picture.y - picture.height, picture.width, picture.height);
    gl.disable(GL_SCISSOR_TEST);
    gl.disable(GL_BLEND);
    gl.use(gl.id);
    gl.set2(gl.size, float(width), float(height));
    gl.set2(gl.extent, tw, th);
    gl.set1(gl.height, float(picture.height));
    gl.set1(gl.profile, filter == PictureFilter::CrtPhosphor ? 1.0f : 0.0f);
    gl.rectangle(-1, -1, 1, 1);
    gl.use(GLuint(previous));
    gl.pop();
    return true;
}

} // namespace mmodern
