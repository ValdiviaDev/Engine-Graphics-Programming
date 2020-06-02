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
    void passBlit();

    // Shaders
    ShaderProgram *deferredProgram = nullptr;
    ShaderProgram *lightProgram = nullptr;
    ShaderProgram *gridProgram = nullptr;
    ShaderProgram *blitProgram;

    GLuint fboColor = 0;
    GLuint albedoColor = 0;
    GLuint positionColor = 0;
    GLuint normalColor = 0;
    GLuint fboDepth = 0;
    FramebufferObject *fbo = nullptr;
};

#endif // DEFERREDRENDERER_H
