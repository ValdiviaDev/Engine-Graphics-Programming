#ifndef DEFERREDRENDERER_H
#define DEFERREDRENDERER_H

#include "renderer.h"
#include "gl.h"

class ShaderProgram;
class FramebufferObject;

class DeferredRenderer : public Renderer
{
public:

    DeferredRenderer();
    ~DeferredRenderer() override;

    void initialize() override;
    void finalize() override;

    void resize(int width, int height) override;
    void render(Camera *camera) override;

private:

    void passMeshes(Camera *camera);
    void passLights(const QMatrix4x4 &viewMatrix);
    bool passGrid(Camera *camera);
    void passBackground(Camera *camera);
    void passOutline();
    void passBlit();

    void initializeBloom(int width, int height);
    void passBloom();
    void passBlitBrightPixel(FramebufferObject *current_fbo, const QVector2D &viewportSize, GLenum colorAttachment, GLuint inputTexture, GLuint inputLod, float threshhold);
    void passBlur(FramebufferObject *pfbo, const QVector2D &viewportSize, GLenum colorAttachment, GLuint inputTexture, GLuint inputLod, const QVector2D &direction);
    void passBloom2(FramebufferObject *current_fbo, GLenum colorAttachment, GLuint inputTexture, int maxLod);

    // Shaders
    ShaderProgram *deferredProgram = nullptr;
    ShaderProgram *lightProgram = nullptr;
    ShaderProgram *gridProgram = nullptr;
    ShaderProgram *backgroundProgram = nullptr;
    ShaderProgram *outlineProgram = nullptr;
    ShaderProgram *blitProgram;
    ShaderProgram *blitBrightestPixelProgram = nullptr;
    ShaderProgram *blur = nullptr;
    ShaderProgram *bloomProgram = nullptr;

    GLuint fboColor = 0;
    GLuint albedoColor = 0;
    GLuint positionColor = 0;
    GLuint normalColor = 0;
    GLuint fboDepth = 0;
    GLuint selectionColor = 0;

    GLuint rtBright; //Brightest pixel and vertical blur
    GLuint rtBloomH; //Horizontal Blur

    FramebufferObject *fbo = nullptr;

    FramebufferObject *fboBloom1 = nullptr;
    FramebufferObject *fboBloom2 = nullptr;
    FramebufferObject *fboBloom3 = nullptr;
    FramebufferObject *fboBloom4 = nullptr;
    FramebufferObject *fboBloom5 = nullptr;

    int viewportWidth = 0;
    int viewportHeight = 0;
};

#endif // DEFERREDRENDERER_H
