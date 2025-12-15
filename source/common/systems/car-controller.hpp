#pragma once

#include "../ecs/world.hpp"
#include "../components/car-controller.hpp"
#include "../components/scrollable.hpp"
#include "../application.hpp"

#include <glm/glm.hpp>

namespace our
{

    // System to handle car movement with WASD keys
    class CarControllerSystem
    {
        Application *app;

    public:
        void enter(Application *app)
        {
            this->app = app;
        }

        void update(World *world, float deltaTime)
        {
            // First, find the car and get input
            CarControllerComponent *carComponent = nullptr;
            Entity *carEntity = nullptr;

            for (auto entity : world->getEntities())
            {
                CarControllerComponent *car = entity->getComponent<CarControllerComponent>();
                if (car)
                {
                    carComponent = car;
                    carEntity = entity;
                    break;
                }
            }

            if (!carComponent || !carEntity)
                return;

            // Get keyboard input
            auto &keyboard = app->getKeyboard();

            float forwardMovement = 0.0f;
            float horizontalMovement = 0.0f;

            // Always move forward
            forwardMovement = 1.0f; // Road moves backward (-Z), car appears to go forward
            // A/D - move the car left/right
            if (keyboard.isPressed(GLFW_KEY_A))
            {
                horizontalMovement = -1.0f;
            }
            if (keyboard.isPressed(GLFW_KEY_D))
            {
                horizontalMovement = 1.0f;
            }

            // Move car horizontally (left/right)
            carEntity->localTransform.position.x += horizontalMovement * carComponent->speed * deltaTime;

            // Clamp car position to road boundaries
            if (carEntity->localTransform.position.x < carComponent->minX)
                carEntity->localTransform.position.x = carComponent->minX;
            if (carEntity->localTransform.position.x > carComponent->maxX)
                carEntity->localTransform.position.x = carComponent->maxX;

            // Scroll all scrollable entities (environment moves opposite to give illusion)
            for (auto entity : world->getEntities())
            {
                ScrollableComponent *scrollable = entity->getComponent<ScrollableComponent>();
                if (scrollable)
                {
                    entity->localTransform.position.z += forwardMovement * carComponent->speed * deltaTime;

                    // Seamless wrap for infinite scrolling
                    // When road goes too far behind camera, wrap it ahead
                    // When road goes too far ahead, wrap it behind
                    float halfLength = scrollable->roadLength / 2.0f;

                    if (entity->localTransform.position.z < -halfLength)
                    {
                        entity->localTransform.position.z += scrollable->roadLength;
                    }
                    else if (entity->localTransform.position.z > halfLength)
                    {
                        entity->localTransform.position.z -= scrollable->roadLength;
                    }
                }
            }
        }

        void exit()
        {
            // Nothing to clean up
        }
    };

}
