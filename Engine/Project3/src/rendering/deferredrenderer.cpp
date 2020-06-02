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

   //for (auto entity : scene->entities)
   //{
   //    if (entity->active && entity->lightSource != nullptr)
   //    {
   //        auto light = entity->lightSource;
   //        lightType.push_back(int(light->type));
   //        lightPosition.push_back(QVector3D(viewMatrix * entity->transform->matrix() * QVector4D(0.0, 0.0, 0.0, 1.0)));
   //        lightDirection.push_back(QVector3D(viewMatrix * entity->transform->matrix() * QVector4D(0.0, 1.0, 0.0, 0.0)));
   //        QVector3D color(light->color.redF(), light->color.greenF(), light->color.blueF());
   //        lightColor.push_back(color * light->intensity);
   //    }
   //}
   // if (lightPosition.size() > 0)
   // {
   //     program.setUniformValueArray("lightType", &lightType[0], lightType.size());
   //     program.setUniformValueArray("lightPosition", &lightPosition[0], lightPosition.size());
   //     program.setUniformValueArray("lightDirection", &lightDirection[0], lightDirection.size());
   //     program.setUniformValueArray("lightColor", &lightColor[0], lightColor.size());
   // }
   // program.setUniformValue("lightCount", lightPosition.size());

    for (auto entity : scene->entities)
    {
        if (entity->active && entity->lightSource != nullptr)
        {
            auto light = entity->lightSource;
            //lightType.push_back(int(light->type));
            program.setUniformValue("lightType", int(light->type));
            //lightPosition.push_back(QVector3D(viewMatrix * entity->transform->matrix() * QVector4D(0.0, 0.0, 0.0, 1.0)));
            program.setUniformValue("lightPosition", QVector3D(viewMatrix * entity->transform->matrix() * QVector4D(0.0, 0.0, 0.0, 1.0)));
            //lightDirection.push_back(QVector3D(viewMatrix * entity->transform->matrix() * QVector4D(0.0, 1.0, 0.0, 0.0)));
            program.setUniformValue("lightDirection", QVector3D(viewMatrix * entity->transform->matrix() * QVector4D(0.0, 1.0, 0.0, 0.0)));
            QVector3D color(light->color.redF(), light->color.greenF(), light->color.blueF());
            program.setUniformValue("lightColor", color * light->intensity);
            //lightColor.push_back(color * light->intensity);
            //for(auto submesh : resourceManager->quad->submeshes){
              //  submesh->draw();
            //}
            resourceManager->quad->submeshes[0]->draw();
        }
    }

    //for(auto submesh : resourceManager->quad->submeshes){
    //    submesh->draw();
    //}

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

    blitProgram = resourceManager->createShaderProgram();
    blitProgram->name = "Blit";
    blitProgram->vertexShaderFilename = "res/shaders/blit.vert";
    blitProgram->fragmentShaderFilename = "res/shaders/blit.frag";
    blitProgram->includeForSerialization = false;

    // Create FBO

    fbo = new FramebufferObject;
    fbo->create();
}

void DeferredRenderer::finalize()
{
    fbo->destroy();
    delete fbo;
}

void DeferredRenderer::resize(int w, int h)
{
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
    fbo->addDepthAttachment(fboDepth);

    unsigned int atachment[4] = { GL_COLOR_ATTACHMENT0,
                                  GL_COLOR_ATTACHMENT1,
                                  GL_COLOR_ATTACHMENT2,
                                  GL_COLOR_ATTACHMENT3};
    gl->glDrawBuffers(4, atachment);

    fbo->checkStatus();
    fbo->release();
}

void DeferredRenderer::render(Camera *camera)
{
    OpenGLErrorGuard guard("DeferredRenderer::render()");

    fbo->bind();

    GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 }; //Albedo, Normals
    gl->glDrawBuffers(4, buffers);

    // Clear color
    gl->glClearDepth(1.0);
    gl->glClearColor(miscSettings->backgroundColor.redF(),
                     miscSettings->backgroundColor.greenF(),
                     miscSettings->backgroundColor.blueF(),
                     1.0);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDrawBuffer(0);

    // Passes
    passMeshes(camera);
    passLights(camera->viewMatrix);

    fbo->release();

    gl->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    passBlit();
}

void DeferredRenderer::passMeshes(Camera *camera)
{
    QOpenGLShaderProgram &program = deferredProgram->program;

    GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 }; //Albedo, Normals
    gl->glDrawBuffers(3, buffers);

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

        //Pass the lights
        //passLights(camera->viewMatrix);

        program.release();
    }
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
