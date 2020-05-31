#include "interaction.h"
#include "globals.h"
#include "resources/mesh.h"
#include <QtMath>
#include <QVector2D>


bool Interaction::update()
{
    bool changed = false;

    switch (state)
    {
    case State::Idle:
        changed = idle();
        break;

    case State::MousePicking:
        changed = mousePicking();
        break;

    case State::Navigating:
        changed = navigate();
        break;

    case State::Focusing:
        changed = focus();
        break;
    }

    return changed;
}

bool Interaction::idle()
{
    if (input->mouseButtons[Qt::RightButton] == MouseButtonState::Pressed)
    {
        nextState = State::Navigating;
    }
    else if (input->mouseButtons[Qt::LeftButton] == MouseButtonState::Press)
    {
        // TODO: Left click
        nextState = State::MousePicking;
    }
    else if(selection->count > 0)
    {
        if (input->keys[Qt::Key_F] == KeyState::Press)
        {
            nextState = State::Focusing;
        }
    }

    return false;
}

bool Interaction::navigate()
{
    static float v = 0.0f; // Instant speed
    static const float a = 5.0f; // Constant acceleration
    static const float t = 1.0/60.0f; // Delta time

    bool pollEvents = input->mouseButtons[Qt::RightButton] == MouseButtonState::Pressed;
    bool cameraChanged = false;

    // Mouse delta smoothing
    static float mousex_delta_prev[3] = {};
    static float mousey_delta_prev[3] = {};
    static int curr_mousex_delta_prev = 0;
    static int curr_mousey_delta_prev = 0;
    float mousey_delta = 0.0f;
    float mousex_delta = 0.0f;
    if (pollEvents) {
        mousex_delta_prev[curr_mousex_delta_prev] = (input->mousex - input->mousex_prev);
        mousey_delta_prev[curr_mousey_delta_prev] = (input->mousey - input->mousey_prev);
        curr_mousex_delta_prev = curr_mousex_delta_prev % 3;
        curr_mousey_delta_prev = curr_mousey_delta_prev % 3;
        mousex_delta += mousex_delta_prev[0] * 0.33;
        mousex_delta += mousex_delta_prev[1] * 0.33;
        mousex_delta += mousex_delta_prev[2] * 0.33;
        mousey_delta += mousey_delta_prev[0] * 0.33;
        mousey_delta += mousey_delta_prev[1] * 0.33;
        mousey_delta += mousey_delta_prev[2] * 0.33;
    }

    float &yaw = camera->yaw;
    float &pitch = camera->pitch;

    // Camera navigation
    if (mousex_delta != 0 || mousey_delta != 0)
    {
        cameraChanged = true;
        yaw -= 0.5f * mousex_delta;
        pitch -= 0.5f * mousey_delta;
        while (yaw < 0.0f) yaw += 360.0f;
        while (yaw > 360.0f) yaw -= 360.0f;
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    }

    static QVector3D speedVector;
    speedVector *= 0.99;

    bool accelerating = false;
    if (input->keys[Qt::Key_W] == KeyState::Pressed) // Front
    {
        accelerating = true;
        speedVector += QVector3D(-sinf(qDegreesToRadians(yaw)) * cosf(qDegreesToRadians(pitch)),
                                        sinf(qDegreesToRadians(pitch)),
                                        -cosf(qDegreesToRadians(yaw)) * cosf(qDegreesToRadians(pitch))) * a * t;
    }
    if (input->keys[Qt::Key_A] == KeyState::Pressed) // Left
    {
        accelerating = true;
        speedVector -= QVector3D(cosf(qDegreesToRadians(yaw)),
                                        0.0f,
                                        -sinf(qDegreesToRadians(yaw))) * a * t;
    }
    if (input->keys[Qt::Key_S] == KeyState::Pressed) // Back
    {
        accelerating = true;
        speedVector -= QVector3D(-sinf(qDegreesToRadians(yaw)) * cosf(qDegreesToRadians(pitch)),
                                        sinf(qDegreesToRadians(pitch)),
                                        -cosf(qDegreesToRadians(yaw)) * cosf(qDegreesToRadians(pitch))) * a * t;
    }
    if (input->keys[Qt::Key_D] == KeyState::Pressed) // Right
    {
        accelerating = true;
        speedVector += QVector3D(cosf(qDegreesToRadians(yaw)),
                                        0.0f,
                                        -sinf(qDegreesToRadians(yaw))) * a * t;
    }
    if (input->keys[Qt::Key_Q] == KeyState::Pressed) // Front
    {
        accelerating = true;
        speedVector += QVector3D(0.0f,
                1.0f,
                0.0f) * a * t;
    }
    if (input->keys[Qt::Key_E] == KeyState::Pressed) // Front
    {
        accelerating = true;
        speedVector += QVector3D(0.0f,
                -1.0f,
                0.0f) * a * t;
    }

    if (!accelerating) {
        speedVector *= 0.9;
    }

    camera->position += speedVector * t;

    if (!(pollEvents ||
        speedVector.length() > 0.01f||
        qAbs(mousex_delta) > 0.1f ||
        qAbs(mousey_delta) > 0.1f))
    {
        nextState = State::Idle;
    }

    if(input->keys[Qt::Key_Z] == KeyState::Pressed)
        {
           // QMessageLogger("Yes", 89, 0).debug() << "error.description(";

            if((mousex_delta != 0 || mousey_delta != 0) && inspector->entity!= nullptr)
            {

               float len = QVector3D(camera->position - inspector->entity->transform->position).length();


               QVector3D front_vec = QVector3D(-sinf(qDegreesToRadians(yaw)) * cosf(qDegreesToRadians(pitch)),
                                                            sinf(qDegreesToRadians(pitch)),
                                                            -cosf(qDegreesToRadians(yaw)) * cosf(qDegreesToRadians(pitch)));
               front_vec *= len;

               camera->position += front_vec;

               yaw -= input->rotation_speed * mousex_delta;
               pitch -= input->rotation_speed * mousey_delta;
               while(yaw < 0.0f)yaw += 360.0f;
               while(yaw > 360.0f)yaw -= 360.0f;
               if(pitch > 89.0f)pitch = 89.0f;
               if(pitch < -89.0f)pitch = -89.0f;

               QVector3D back_vec = QVector3D(sinf(qDegreesToRadians(yaw)) * cosf(qDegreesToRadians(pitch)),
                                                            -sinf(qDegreesToRadians(pitch)),
                                                            cosf(qDegreesToRadians(yaw)) * cosf(qDegreesToRadians(pitch)));
               back_vec *= len;

               camera->position += back_vec;
           }

         }

    return true;
}

bool Interaction::focus()
{
    static bool idle = true;
    static float time = 0.0;
    static QVector3D initialCameraPosition;
    static QVector3D finalCameraPosition;
    if (idle) {
        idle = false;
        time = 0.0f;
        initialCameraPosition = camera->position;

        Entity *entity = selection->entities[0];

        float entityRadius = 0.5;
        if (entity->meshRenderer != nullptr && entity->meshRenderer->mesh != nullptr)
        {
            auto mesh = entity->meshRenderer->mesh;
            const QVector3D minBounds = entity->transform->matrix() * mesh->bounds.min;
            const QVector3D maxBounds = entity->transform->matrix() * mesh->bounds.max;
            entityRadius = (maxBounds - minBounds).length();
        }

        QVector3D entityPosition = entity->transform->position;
        QVector3D viewingDirection = QVector3D(camera->worldMatrix * QVector4D(0.0, 0.0, -1.0, 0.0));
        QVector3D displacement = - 1.5 * entityRadius * viewingDirection.normalized();
        finalCameraPosition = entityPosition + displacement;
    }

    const float focusDuration = 0.5f;
    time = qMin(focusDuration, time + 1.0f/60.0f); // TODO: Use frame delta time
    const float t = qPow(qSin(3.14159f * 0.5f * time / focusDuration), 0.5);

    camera->position = (1.0f - t) * initialCameraPosition + t * finalCameraPosition;

    if (t == 1.0f) {
        nextState = State::Idle;
        idle = true;;
    }

    return true;
}

bool Interaction::mousePicking(){

   //Throw a ray from the screen to the world
   camera->screenPointToWorldRay(input->mousex, input->mousey);



   return true;
}

void Interaction::postUpdate()
{
    state = nextState;
}
