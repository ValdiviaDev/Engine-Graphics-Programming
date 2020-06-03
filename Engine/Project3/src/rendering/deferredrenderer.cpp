#include "Deferredrenderer.h"
#include "miscsettings.h"
#include "ecs/scene.h"
#include "ecs/camera.h"
#include "resources/material.h"
#include "resources/mesh.h"
#include "resources/texture.h"
#include "resources/shaderprogram.h"
#include "resources/resourcemanager.h"
#include "framebufferobject.h"
#include "gl.h"
#include "globals.h"
#include <QVector>
#include <QVector3D>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <random>

#define MIPMAP_BASE_LEVEL 0
#define MIPMAP_MAX_LEVEL 4

void DeferredRenderer::passLights(const QMatrix4x4 &viewMatrix)
{
    QVector<int> lightType;
    QVector<QVector3D> lightPosition;
    QVector<QVector3D> lightDirection;
    QVector<QVector3D> lightColor;

    QOpenGLShaderProgram &program = lightProgram->program;

    GLenum draw_buffers = GL_COLOR_ATTACHMENT3;
    gl->glDrawBuffer(draw_buffers);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);

    program.bind();

    program.setUniformValue(program.uniformLocation("gPosition"), 1);
    gl->glActiveTexture(GL_TEXTURE1);
    gl->glBindTexture(GL_TEXTURE_2D, positionColor);
    program.setUniformValue(program.uniformLocation("gNormal"), 2);
    gl->glActiveTexture(GL_TEXTURE2);
    gl->glBindTexture(GL_TEXTURE_2D, normalColor);
    program.setUniformValue(program.uniformLocation("gAlbedoSpec"), 0);
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, albedoColor);
    program.setUniformValue(program.uniformLocation("ssao"), 3);
    gl->glActiveTexture(GL_TEXTURE3);
    gl->glBindTexture(GL_TEXTURE_2D, ssaoBlurColor);

    int lights_count = 0;
   for (auto entity : scene->entities)
   {
       if (entity->active && entity->lightSource != nullptr)
       {
           auto light = entity->lightSource;
           //lightType.push_back(int(light->type));
           if(light->type == LightSource::Type::Point){
               program.setUniformValue("lightType", 1);
           }
           else{
               program.setUniformValue("lightType", 0);
           }
           //program.setUniformValue("lightType", int(light->type));
           //lightPosition.push_back(QVector3D(viewMatrix * entity->transform->matrix() * QVector4D(0.0, 0.0, 0.0, 1.0)));
           program.setUniformValue("lightPosition", QVector3D(viewMatrix * entity->transform->matrix() * QVector4D(0.0, 0.0, 0.0, 1.0)));
           //lightDirection.push_back(QVector3D(viewMatrix * entity->transform->matrix() * QVector4D(0.0, 1.0, 0.0, 0.0)));
           program.setUniformValue("lightDirection", QVector3D(viewMatrix * entity->transform->matrix() * QVector4D(0.0, 1.0, 0.0, 0.0)));
           QVector3D color(light->color.redF(), light->color.greenF(), light->color.blueF());
           program.setUniformValue("lightColor", color * light->intensity);
           //program.setUniformValue("lightIntensity", light->intensity);
           //lightColor.push_back(color * light->intensity);
           resourceManager->quad->submeshes[0]->draw();
           lights_count++;
       }
   }
   if(lights_count == 0){
        resourceManager->quad->submeshes[0]->draw();
   }

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    program.release();

}

DeferredRenderer::DeferredRenderer() :
    fboColor(QOpenGLTexture::Target2D),
    albedoColor(QOpenGLTexture::Target2D),
    positionColor(QOpenGLTexture::Target2D),
    normalColor(QOpenGLTexture::Target2D),
    fboDepth(QOpenGLTexture::Target2D)
{
    fbo = nullptr;

    // List of textures
    addTexture("Final render");
    addTexture("White");
    addTexture("Black");
    addTexture("Position texture");
    addTexture("Normal texture");
    addTexture("Albedo");
}

DeferredRenderer::~DeferredRenderer()
{
    delete fbo;
    delete fboBloom1;
    delete fboBloom2;
    delete fboBloom3;
    delete fboBloom4;
    delete fboBloom5;
    delete ssaoFBO;
    delete ssaoBlurFBO;
}

void DeferredRenderer::initialize()
{
    OpenGLErrorGuard guard("DeferredRenderer::initialize()");

    // Create programs

    deferredProgram = resourceManager->createShaderProgram();
    deferredProgram->name = "Deferred shading";
    deferredProgram->vertexShaderFilename = "res/shaders/deferred_shading.vert";
    deferredProgram->fragmentShaderFilename = "res/shaders/deferred_shading.frag";
    deferredProgram->includeForSerialization = false;

    lightProgram = resourceManager->createShaderProgram();
    lightProgram->name = "Light shading";
    lightProgram->vertexShaderFilename = "res/shaders/light_shading.vert";
    lightProgram->fragmentShaderFilename = "res/shaders/light_shading.frag";
    lightProgram->includeForSerialization = false;

    gridProgram = resourceManager->createShaderProgram();
    gridProgram->name = "Grid shading";
    gridProgram->vertexShaderFilename = "res/shaders/grid_shading.vert";
    gridProgram->fragmentShaderFilename = "res/shaders/grid_shading.frag";
    gridProgram->includeForSerialization = false;

    backgroundProgram = resourceManager->createShaderProgram();
    backgroundProgram->name = "Background shading";
    backgroundProgram->vertexShaderFilename = "res/shaders/background_shading.vert";
    backgroundProgram->fragmentShaderFilename = "res/shaders/background_shading.frag";
    backgroundProgram->includeForSerialization = false;

    outlineProgram = resourceManager->createShaderProgram();
    outlineProgram->name = "Outline shading";
    outlineProgram->vertexShaderFilename = "res/shaders/outline_shading.vert";
    outlineProgram->fragmentShaderFilename = "res/shaders/outline_shading.frag";
    outlineProgram->includeForSerialization = false;


    blitBrightestPixelProgram = resourceManager->createShaderProgram();
    blitBrightestPixelProgram->name = "Blit Bright Pixel shading";
    blitBrightestPixelProgram->vertexShaderFilename = "res/shaders/blit_bright_pixel_shading.vert";
    blitBrightestPixelProgram->fragmentShaderFilename = "res/shaders/blit_bright_pixel_shading.frag";
    blitBrightestPixelProgram->includeForSerialization = false;

    blur = resourceManager->createShaderProgram();
    blur->name = "Blur shading";
    blur->vertexShaderFilename = "res/shaders/blur_shading.vert";
    blur->fragmentShaderFilename = "res/shaders/blur_shading.frag";
    blur->includeForSerialization = false;

    bloomProgram = resourceManager->createShaderProgram();
    bloomProgram->name = "Bloom shading";
    bloomProgram->vertexShaderFilename = "res/shaders/bloom_shading.vert";
    bloomProgram->fragmentShaderFilename = "res/shaders/bloom_shading.frag";
    bloomProgram->includeForSerialization = false;

    ssaoProgram = resourceManager->createShaderProgram();
    ssaoProgram->name = "SSAO shading";
    ssaoProgram->vertexShaderFilename = "res/shaders/ssao_shading.vert";
    ssaoProgram->fragmentShaderFilename = "res/shaders/ssao_shading.frag";
    ssaoProgram->includeForSerialization = false;

    ssaoBlurProgram = resourceManager->createShaderProgram();
    ssaoBlurProgram->name = "SSAO Blur Ambient Oclusion shading";
    ssaoBlurProgram->vertexShaderFilename = "res/shaders/ssao_blur_shading.vert";
    ssaoBlurProgram->fragmentShaderFilename = "res/shaders/ssao_blur_shading.frag";
    ssaoBlurProgram->includeForSerialization = false;

    blitProgram = resourceManager->createShaderProgram();
    blitProgram->name = "Blit";
    blitProgram->vertexShaderFilename = "res/shaders/blit.vert";
    blitProgram->fragmentShaderFilename = "res/shaders/blit.frag";
    blitProgram->includeForSerialization = false;

    // Create FBO

    fbo = new FramebufferObject;
    fbo->create();
    fboBloom1 = new FramebufferObject;
    fboBloom1->create();
    fboBloom2 = new FramebufferObject;
    fboBloom2->create();
    fboBloom3 = new FramebufferObject;
    fboBloom3->create();
    fboBloom4 = new FramebufferObject;
    fboBloom4->create();
    fboBloom5 = new FramebufferObject;
    fboBloom5->create();
    ssaoFBO = new FramebufferObject;
    ssaoFBO->create();
    ssaoBlurFBO = new FramebufferObject;
    ssaoBlurFBO->create();
}

void DeferredRenderer::finalize()
{
    fbo->destroy();
    delete fbo;
}

void DeferredRenderer::initializeBloom(int width, int height){
    //Bloom mipmap
    if (rtBright != 0) gl->glDeleteTextures(1, &rtBright);
    gl->glGenTextures(1, &rtBright);
    gl->glBindTexture(GL_TEXTURE_2D, rtBright);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, MIPMAP_BASE_LEVEL);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MIPMAP_MAX_LEVEL);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width/2, height/2, 0, GL_RGBA, GL_FLOAT, nullptr);
    gl->glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, width/4, height/4, 0, GL_RGBA, GL_FLOAT, nullptr);
    gl->glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA16F, width/8, height/8, 0, GL_RGBA, GL_FLOAT, nullptr);
    gl->glTexImage2D(GL_TEXTURE_2D, 3, GL_RGBA16F, width/16, height/16, 0, GL_RGBA, GL_FLOAT, nullptr);
    gl->glTexImage2D(GL_TEXTURE_2D, 4, GL_RGBA16F, width/32, height/32, 0, GL_RGBA, GL_FLOAT, nullptr);
    gl->glGenerateMipmap(GL_TEXTURE_2D);

    if (rtBloomH != 0) gl->glDeleteTextures(1, &rtBloomH);
    gl->glGenTextures(1, &rtBloomH);
    gl->glBindTexture(GL_TEXTURE_2D, rtBloomH);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, MIPMAP_BASE_LEVEL);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MIPMAP_MAX_LEVEL);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width/2, height/2, 0, GL_RGBA, GL_FLOAT, nullptr);
    gl->glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, width/4, height/4, 0, GL_RGBA, GL_FLOAT, nullptr);
    gl->glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA16F, width/8, height/8, 0, GL_RGBA, GL_FLOAT, nullptr);
    gl->glTexImage2D(GL_TEXTURE_2D, 3, GL_RGBA16F, width/16, height/16, 0, GL_RGBA, GL_FLOAT, nullptr);
    gl->glTexImage2D(GL_TEXTURE_2D, 4, GL_RGBA16F, width/32, height/32, 0, GL_RGBA, GL_FLOAT, nullptr);
    gl->glGenerateMipmap(GL_TEXTURE_2D);

    fboBloom1->bind();
    fboBloom1->addColorAttachment(0,rtBright,0);
    fboBloom1->addColorAttachment(1,rtBloomH,0);
    fboBloom1->checkStatus();
    fboBloom1->release();

    fboBloom2->bind();
    fboBloom2->addColorAttachment(0,rtBright,0);
    fboBloom2->addColorAttachment(1,rtBloomH,0);
    fboBloom2->checkStatus();
    fboBloom2->release();

    fboBloom3->bind();
    fboBloom3->addColorAttachment(0,rtBright,0);
    fboBloom3->addColorAttachment(1,rtBloomH,0);
    fboBloom3->checkStatus();
    fboBloom3->release();

    fboBloom4->bind();
    fboBloom4->addColorAttachment(0,rtBright,0);
    fboBloom4->addColorAttachment(1,rtBloomH,0);
    fboBloom4->checkStatus();
    fboBloom4->release();

    fboBloom5->bind();
    fboBloom5->addColorAttachment(0,rtBright,0);
    fboBloom5->addColorAttachment(1,rtBloomH,0);
    fboBloom5->checkStatus();
    fboBloom5->release();

}

void DeferredRenderer::resize(int w, int h)
{
    viewportWidth = w;
    viewportHeight = h;
    OpenGLErrorGuard guard("DeferredRenderer::resize()");

    // Regenerate render targets

    if (fboColor == 0) gl->glDeleteTextures(1, &fboColor);
    gl->glGenTextures(1, &fboColor);
    gl->glBindTexture(GL_TEXTURE_2D, fboColor);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    if (albedoColor == 0) gl->glDeleteTextures(1, &albedoColor);
    gl->glGenTextures(1, &albedoColor);
    gl->glBindTexture(GL_TEXTURE_2D, albedoColor);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    if (positionColor == 0) gl->glDeleteTextures(1, &positionColor);
    gl->glGenTextures(1, &positionColor);
    gl->glBindTexture(GL_TEXTURE_2D, positionColor);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    if (normalColor == 0) gl->glDeleteTextures(1, &normalColor);
    gl->glGenTextures(1, &normalColor);
    gl->glBindTexture(GL_TEXTURE_2D, normalColor);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    if (selectionColor == 0) gl->glDeleteTextures(1, &selectionColor);
    gl->glGenTextures(1, &selectionColor);
    gl->glBindTexture(GL_TEXTURE_2D, selectionColor);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    if (fboDepth == 0) gl->glDeleteTextures(1, &fboDepth);
    gl->glGenTextures(1, &fboDepth);
    gl->glBindTexture(GL_TEXTURE_2D, fboDepth);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    // Attach textures to the fbo

    fbo->bind();
    //fbo->addColorAttachment(0, fboColor);
    fbo->addColorAttachment(0, albedoColor);
    fbo->addColorAttachment(1, positionColor);
    fbo->addColorAttachment(2, normalColor);
    fbo->addColorAttachment(3, fboColor);
    fbo->addColorAttachment(4, selectionColor);
    fbo->addDepthAttachment(fboDepth);

    unsigned int atachment[5] = { GL_COLOR_ATTACHMENT0,
                                  GL_COLOR_ATTACHMENT1,
                                  GL_COLOR_ATTACHMENT2,
                                  GL_COLOR_ATTACHMENT3,
                                  GL_COLOR_ATTACHMENT4};
    gl->glDrawBuffers(5, atachment);

    fbo->checkStatus();
    fbo->release();

    initializeBloom(w, h);
    initializeSSAO();

}

void DeferredRenderer::render(Camera *camera)
{
    OpenGLErrorGuard guard("DeferredRenderer::render()");

    fbo->bind();

    GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 }; //Albedo, Normals
    gl->glDrawBuffers(5, buffers);

    // Clear color
    gl->glClearDepth(1.0);
    gl->glClearColor(miscSettings->backgroundColor.redF(),
                     miscSettings->backgroundColor.greenF(),
                     miscSettings->backgroundColor.blueF(),
                     1.0);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDrawBuffer(0);

    // Passes
    passBackground(camera);
    passMeshes(camera);
    passSSAO();
    passLights(camera->viewMatrix);
    if(miscSettings->show_selection_outline){
        passOutline();
    }
    if(miscSettings->show_grid){
        passGrid(camera);
    }
    if(miscSettings->use_bloom){
        passBloom();
    }

    fbo->release();

    gl->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    passBlit();
}

void DeferredRenderer::passMeshes(Camera *camera)
{
    QOpenGLShaderProgram &program = deferredProgram->program;

    GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT4 }; //Albedo, Normals
    gl->glDrawBuffers(4, buffers);

    if (program.bind())
    {
        program.setUniformValue("viewMatrix", camera->viewMatrix);
        program.setUniformValue("projectionMatrix", camera->projectionMatrix);
        program.setUniformValue("viewPos", camera->position);

        QVector<MeshRenderer*> meshRenderers;

        // Get components
        for (auto entity : scene->entities)
        {
            if (entity->active)
            {
                if (entity->meshRenderer != nullptr) { meshRenderers.push_back(entity->meshRenderer); }

            }
        }

        // Meshes
        for (auto meshRenderer : meshRenderers)
        {
            auto mesh = meshRenderer->mesh;

            if (mesh != nullptr)
            {
                QMatrix4x4 worldMatrix = meshRenderer->entity->transform->matrix();
                QMatrix4x4 worldViewMatrix = camera->viewMatrix * worldMatrix;
                QMatrix3x3 normalMatrix = worldViewMatrix.normalMatrix();

                program.setUniformValue("worldMatrix", worldMatrix);
                program.setUniformValue("worldViewMatrix", worldViewMatrix);
                program.setUniformValue("normalMatrix", normalMatrix);
                program.setUniformValue("is_selected", meshRenderer->entity->is_selected);

                int materialIndex = 0;
                for (auto submesh : mesh->submeshes)
                {
                    // Get material from the component
                    Material *material = nullptr;
                    if (materialIndex < meshRenderer->materials.size()) {
                        material = meshRenderer->materials[materialIndex];
                    }
                    if (material == nullptr) {
                        material = resourceManager->materialWhite;
                    }
                    materialIndex++;

#define SEND_TEXTURE(uniformName, tex1, tex2, texUnit) \
    program.setUniformValue(uniformName, texUnit); \
    if (tex1 != nullptr) { \
    tex1->bind(texUnit); \
                } else { \
    tex2->bind(texUnit); \
                }

                    // Send the material to the shader
                    program.setUniformValue("albedo", material->albedo);
                    program.setUniformValue("emissive", material->emissive);
                    program.setUniformValue("specular", material->specular);
                    program.setUniformValue("smoothness", material->smoothness);
                    program.setUniformValue("bumpiness", material->bumpiness);
                    program.setUniformValue("tiling", material->tiling);
                    SEND_TEXTURE("albedoTexture", material->albedoTexture, resourceManager->texWhite, 0);
                    SEND_TEXTURE("emissiveTexture", material->emissiveTexture, resourceManager->texBlack, 1);
                    SEND_TEXTURE("specularTexture", material->specularTexture, resourceManager->texBlack, 2);
                    SEND_TEXTURE("normalTexture", material->normalsTexture, resourceManager->texNormal, 3);
                    SEND_TEXTURE("bumpTexture", material->bumpTexture, resourceManager->texWhite, 4);

                    submesh->draw();
                }
            }
        }

        program.release();
    }
}

bool DeferredRenderer::passGrid(Camera *camera)
{
    QOpenGLShaderProgram &program = gridProgram->program;

     GLenum draw_buffers = GL_COLOR_ATTACHMENT3;
     gl->glDrawBuffer(draw_buffers);

     glEnable(GL_BLEND);
     glEnable(GL_DEPTH_TEST);
     glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

     if (program.bind())
     {
         QVector4D camera_parameters = camera->getLeftRightBottomTop();
         program.setUniformValue("left", camera_parameters.x());
         program.setUniformValue("right", camera_parameters.y());
         program.setUniformValue("bottom", camera_parameters.z());
         program.setUniformValue("top", camera_parameters.w());
         program.setUniformValue("z_near", camera->znear);
         program.setUniformValue("z_far", camera->zfar);
         program.setUniformValue("worldMatrix", camera->worldMatrix);
         program.setUniformValue("viewMatrix", camera->viewMatrix);
         program.setUniformValue("projectionMatrix", camera->projectionMatrix);

         resourceManager->quad->submeshes[0]->draw();

         program.release();
     }
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    return true;
}

void DeferredRenderer::passBackground(Camera *camera)
{
    QOpenGLShaderProgram &program = backgroundProgram->program;

    GLenum draw_buffers = GL_COLOR_ATTACHMENT3;
    glDrawBuffer(draw_buffers);
    gl->glDisable(GL_DEPTH_TEST);
    if (program.bind())
    {
        QVector4D viewport_parameters = camera->getLeftRightBottomTop();
        program.setUniformValue("viewportSize", QVector2D(viewportWidth, viewportHeight));
        program.setUniformValue("left", viewport_parameters.x());
        program.setUniformValue("right", viewport_parameters.y());
        program.setUniformValue("bottom", viewport_parameters.z());
        program.setUniformValue("top", viewport_parameters.w());
        program.setUniformValue("znear", camera->znear);
        program.setUniformValue("worldMatrix", camera->worldMatrix);
        program.setUniformValue("backgroundColor", miscSettings->backgroundColor);

        resourceManager->quad->submeshes[0]->draw();

        program.release();
    }
    gl->glEnable(GL_DEPTH_TEST);
}

void DeferredRenderer::passOutline()
{
    QOpenGLShaderProgram &program = outlineProgram->program;

    GLenum draw_buffers = GL_COLOR_ATTACHMENT3;
    glDrawBuffer(draw_buffers);

    glDisable(GL_DEPTH_TEST);

    if (program.bind())
    {
        //std::cout<<main_buffer->GetSelectionTexture()<<std::endl;

        gl->glActiveTexture(GL_TEXTURE0);
        gl->glBindTexture(GL_TEXTURE_2D, selectionColor);
        program.setUniformValue(program.uniformLocation("mask"), 0);
        //program_selection.setUniformValue(program_selection.uniformLocation("mask"));

        resourceManager->quad->submeshes[0]->draw();
        //std::cout<<"hol2a1"<<std::endl;
        program.release();
    }

   glEnable(GL_DEPTH_TEST);
}

void DeferredRenderer::passBlitBrightPixel(FramebufferObject *current_fbo, const QVector2D &viewportSize, GLenum colorAttachment, GLuint inputTexture, GLuint inputLod, float threshhold){
    current_fbo->bind();
    gl->glDrawBuffer(colorAttachment);

    gl->glViewport(0,0, viewportSize.x(), viewportSize.y());

    OpenGLState::reset();

    QOpenGLShaderProgram &program = blitBrightestPixelProgram->program;

    if(program.bind()){
        gl->glActiveTexture(GL_TEXTURE0);
        gl->glBindTexture(GL_TEXTURE_2D, inputTexture);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        program.setUniformValue("colorTexture", 0);

        resourceManager->quad->submeshes[0]->draw();

        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        program.release();
    }

}

void DeferredRenderer::passBlur(FramebufferObject *pfbo, const QVector2D &viewportSize, GLenum colorAttachment, GLuint inputTexture, GLuint inputLod, const QVector2D &direction){
    pfbo->bind();
    gl->glDrawBuffer(colorAttachment);
    gl->glViewport(0,0, viewportSize.x(), viewportSize.y());

    OpenGLState glState;
    glState.depthTest = false;
    glState.blending = false;
    glState.apply();

    QOpenGLShaderProgram &program = blur->program;

    if(program.bind()){
        gl->glActiveTexture(GL_TEXTURE0);
        gl->glBindTexture(GL_TEXTURE_2D, inputTexture);
        program.setUniformValue("colorMap", 0);
        program.setUniformValue("inputLod", inputLod);
        program.setUniformValue("direction", direction);

        resourceManager->quad->submeshes[0]->draw();
        program.release();
    }

    pfbo->release();

}

void DeferredRenderer::passBloom(){
#define LOD(x) x
    const QVector2D horizontal(1.0,0.0);
    const QVector2D vertical(0.0,1.0);

    const float w = viewportWidth;
    const float h = viewportHeight;

    //HorizontalBlur
    float threshold = 1.0;
    passBlitBrightPixel(fboBloom1, QVector2D(w/2,h/2), GL_COLOR_ATTACHMENT0, fboColor, LOD(0), threshold);
    gl->glBindTexture(GL_TEXTURE_2D, rtBright);
    gl->glGenerateMipmap(GL_TEXTURE_2D);

    //Horizontal Blur
    passBlur(fboBloom1, QVector2D(w/2,h/2), GL_COLOR_ATTACHMENT1, rtBright, LOD(0), horizontal);
    passBlur(fboBloom2, QVector2D(w/4,h/4), GL_COLOR_ATTACHMENT1, rtBright, LOD(1), horizontal);
    passBlur(fboBloom3, QVector2D(w/8,h/8), GL_COLOR_ATTACHMENT1, rtBright, LOD(2), horizontal);
    passBlur(fboBloom4, QVector2D(w/16,h/16), GL_COLOR_ATTACHMENT1, rtBright, LOD(3), horizontal);
    passBlur(fboBloom5, QVector2D(w/32,h/32), GL_COLOR_ATTACHMENT1, rtBright, LOD(4), horizontal);

    //Vertical Blur
    passBlur(fboBloom1, QVector2D(w/2,h/2), GL_COLOR_ATTACHMENT0, rtBloomH, LOD(0), vertical);
    passBlur(fboBloom2, QVector2D(w/4,h/4), GL_COLOR_ATTACHMENT0, rtBloomH, LOD(1), vertical);
    passBlur(fboBloom3, QVector2D(w/8,h/8), GL_COLOR_ATTACHMENT0, rtBloomH, LOD(2), vertical);
    passBlur(fboBloom4, QVector2D(w/16,h/16), GL_COLOR_ATTACHMENT0, rtBloomH, LOD(3), vertical);
    passBlur(fboBloom5, QVector2D(w/32,h/32), GL_COLOR_ATTACHMENT0, rtBloomH, LOD(4), vertical);

    passBloom2(fbo, GL_COLOR_ATTACHMENT3, rtBright, 4);


#undef LOD
}

void DeferredRenderer::passBloom2(FramebufferObject *current_fbo, GLenum colorAttachment, GLuint inputTexture, int maxLod){
    current_fbo->bind();
    gl->glDrawBuffer(colorAttachment);

    gl->glViewport(0,0,viewportWidth,viewportHeight);

    OpenGLState glState;
    glState.depthTest = false;
    glState.blending = true;
    glState.blendFuncDst = GL_ONE;
    glState.blendFuncSrc = GL_NONE;
    glState.apply();

    QOpenGLShaderProgram &program = bloomProgram->program;

    if(program.bind()){
        gl->glActiveTexture(GL_TEXTURE0);
        gl->glBindTexture(GL_TEXTURE_2D, inputTexture);
        program.setUniformValue("colorMap", 0);
        program.setUniformValue("maxLod", maxLod);

        resourceManager->quad->submeshes[0]->draw();
        program.release();
    }
    glDisable(GL_BLEND);
    current_fbo->release();
}

float lerp(float a, float b, float f)
{
    return a + f * (b - a);
}

void DeferredRenderer::initializeSSAO(){
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
    std::default_random_engine generator;

    ssaoFBO->bind();

    std::vector<QVector3D> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++)
    {
        QVector3D noise(
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            0.0f);
        ssaoNoise.push_back(noise);
    }

    glGenTextures(1, &ssaoNoiseTexture);
    glBindTexture(GL_TEXTURE_2D, ssaoNoiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenTextures(1, &ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, viewportWidth, viewportHeight, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    ssaoFBO->addColorAttachment(0,ssaoColorBuffer,0);
    ssaoFBO->addColorAttachment(1, ssaoNoiseTexture, 0);
    ssaoFBO->checkStatus();
    ssaoFBO->release();

    initializeSSAOBlur();
}

void DeferredRenderer::initializeSSAOBlur(){
    ssaoBlurFBO->bind();

    glGenTextures(1, &ssaoBlurColor);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, viewportWidth, viewportHeight, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    ssaoBlurFBO->addColorAttachment(0, ssaoBlurColor, 0);
    ssaoBlurFBO->checkStatus();
    ssaoBlurFBO->release();
}

void DeferredRenderer::passSSAO(){
     std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
     std::default_random_engine generator;
     std::vector<QVector3D> ssaoKernel;
     for (unsigned int i = 0; i < 64; ++i)
     {
         QVector3D sample(
             randomFloats(generator) * 2.0 - 1.0,
             randomFloats(generator) * 2.0 - 1.0,
             randomFloats(generator)
         );
         float scale = (float)i / 64.0;
         scale   = lerp(0.1f, 1.0f, scale * scale);
         sample.normalize();
         sample *= randomFloats(generator);
         sample *= scale;
         ssaoKernel.push_back(sample);
     }

     QOpenGLShaderProgram &program = ssaoProgram->program;

     ssaoFBO->bind();
     glClear(GL_COLOR_BUFFER_BIT);

     GLenum draw_buffers = GL_COLOR_ATTACHMENT0;
     gl->glDrawBuffer(draw_buffers);

     if(program.bind()){
        gl->glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, positionColor);
        program.setUniformValue(program.uniformLocation("gPosition"), 0);
        gl->glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalColor);
        program.setUniformValue(program.uniformLocation("gNormal"), 1);
        gl->glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, ssaoNoiseTexture);
        program.setUniformValue(program.uniformLocation("texNoise"), 2);
        program.setUniformValueArray("samples", &ssaoKernel[0], 64);
        program.setUniformValue("projection", camera->projectionMatrix);
        resourceManager->quad->submeshes[0]->draw();
        program.release();
     }
     ssaoFBO->release();
     passSSAOBlur();
     fbo->bind();

}

void DeferredRenderer::passSSAOBlur(){
    QOpenGLShaderProgram &program = ssaoBlurProgram->program;
    ssaoBlurFBO->bind();

    if(program.bind()){
        gl->glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
        program.setUniformValue(program.uniformLocation("ssaoInput"), 0);
        resourceManager->quad->submeshes[0]->draw();
        program.release();
    }

    ssaoBlurFBO->release();

}

void DeferredRenderer::passBlit()
{
    gl->glDisable(GL_DEPTH_TEST);

    QOpenGLShaderProgram &program = blitProgram->program;

    if (program.bind())
    {

        program.setUniformValue("colorTexture", 0);
        gl->glActiveTexture(GL_TEXTURE0);

        if (shownTexture() == "Final render") {
            gl->glBindTexture(GL_TEXTURE_2D, fboColor);
        }
        else if(shownTexture() == "White") {
            gl->glBindTexture(GL_TEXTURE_2D, resourceManager->texWhite->textureId());
        }
        else if(shownTexture() == "Black"){
            gl->glBindTexture(GL_TEXTURE_2D, resourceManager->texBlack->textureId());
        }
        else if(shownTexture() == "Position texture"){
            gl->glBindTexture(GL_TEXTURE_2D, positionColor);
        }
        else if(shownTexture() == "Normal texture"){
            gl->glBindTexture(GL_TEXTURE_2D, normalColor);
        }
        else if(shownTexture() == "Albedo"){
            gl->glBindTexture(GL_TEXTURE_2D, albedoColor);
        }

        resourceManager->quad->submeshes[0]->draw();

    }

    gl->glEnable(GL_DEPTH_TEST);
}
