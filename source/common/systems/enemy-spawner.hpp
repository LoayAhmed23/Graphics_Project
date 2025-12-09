#pragma once

#include "../ecs/world.hpp"
#include "../components/enemy-car.hpp"
#include "../components/car-controller.hpp"
#include "../components/mesh-renderer.hpp"
#include "../application.hpp"
#include "../asset-loader.hpp"

#include <glm/glm.hpp>
#include <random>
#include <cmath>

namespace our
{

    // System to spawn and move enemy cars
    class EnemySpawnerSystem
    {
        Application *app;
        float spawnTimer = 0.0f;
        float spawnInterval = 2.0f; // Spawn every 2 seconds
        float minSpawnInterval = 1.0f;
        float maxSpawnInterval = 3.0f;

        float spawnZ = -80.0f;  // Spawn far ahead (negative Z)
        float despawnZ = 60.0f; // Remove when past camera

        float minX = -12.0f; // Left lane boundary
        float maxX = 9.0f;   // Right lane boundary

        // Collision detection settings
        float collisionDistanceX = 3.0f; 
        float collisionDistanceZ = 5.5f;

        bool gameOver = false;

        std::random_device rd;
        std::mt19937 gen;

    public:
        EnemySpawnerSystem() : gen(rd()) {}

        void enter(Application *app)
        {
            this->app = app;
            spawnTimer = spawnInterval;
            gameOver = false; // Reset game over state
        }

        void update(World *world, float deltaTime)
        {
            // If game over, don't update
            if (gameOver)
                return;

            // Update spawn timer
            spawnTimer -= deltaTime;

            // Check keyboard input
            auto &keyboard = app->getKeyboard();
            bool isMovingForward = keyboard.isPressed(GLFW_KEY_W);
            bool isMovingBackward = keyboard.isPressed(GLFW_KEY_S);

            // Only spawn when driving forward
            if (spawnTimer <= 0.0f && isMovingForward)
            {
                spawnEnemy(world);

                // Random interval for next spawn
                std::uniform_real_distribution<float> intervalDist(minSpawnInterval, maxSpawnInterval);
                spawnTimer = intervalDist(gen);
            }

            // Find player car position and speed
            Entity *playerEntity = nullptr;
            float playerSpeed = 0.0f;
            glm::vec3 playerPos(0.0f);
            for (auto entity : world->getEntities())
            {
                CarControllerComponent *car = entity->getComponent<CarControllerComponent>();
                if (car)
                {
                    playerEntity = entity;
                    playerSpeed = car->speed;
                    playerPos = entity->localTransform.position;
                    break;
                }
            }

            // Calculate speed modifier based on player movement
            float speedModifier = 0.0f;
            if (isMovingForward)
            {
                speedModifier = playerSpeed; // Add player speed when W pressed
            }
            else if (isMovingBackward)
            {
                speedModifier = -playerSpeed; // Subtract player speed when S pressed
            }

            // Move all enemy cars toward the player and check for collision
            auto entities = world->getEntities();
            for (auto entity : entities)
            {
                EnemyCarComponent *enemy = entity->getComponent<EnemyCarComponent>();
                if (enemy)
                {
                    // Enemy moves toward player with speed adjusted by player movement
                    float totalSpeed = enemy->speed + speedModifier;
                    entity->localTransform.position.z += totalSpeed * deltaTime;

                    // Check collision with player
                    if (playerEntity)
                    {
                        glm::vec3 enemyPos = entity->localTransform.position;
                        float distX = std::abs(playerPos.x - enemyPos.x);
                        float distZ = std::abs(playerPos.z - enemyPos.z);

                        if (distX < collisionDistanceX && distZ < collisionDistanceZ)
                        {
                            // Collision detected! Game over
                            gameOver = true;
                            app->changeState("menu");
                            return;
                        }
                    }

                    // Remove if past camera
                    if (entity->localTransform.position.z > despawnZ)
                    {
                        world->markForRemoval(entity);
                    }
                }
            }

            // Clean up marked entities
            world->deleteMarkedEntities();
        }

        void spawnEnemy(World *world)
        {
            // Create enemy entity
            Entity *enemy = world->add();

            // Random X position within road bounds
            std::uniform_real_distribution<float> xDist(minX, maxX);
            float xPos = xDist(gen);

            enemy->localTransform.position = glm::vec3(xPos, 0.0f, spawnZ);
            enemy->localTransform.rotation = glm::vec3(0.0f, 0.0f, 0.0f); // Face toward positive Z (toward player)
            enemy->localTransform.scale = glm::vec3(1.0f, 1.0f, 1.0f);

            // Add mesh renderer
            MeshRendererComponent *meshRenderer = enemy->addComponent<MeshRendererComponent>();
            meshRenderer->mesh = AssetLoader<Mesh>::get("car");
            meshRenderer->material = AssetLoader<Material>::get("car_frame");

            // Add enemy car component
            EnemyCarComponent *enemyComp = enemy->addComponent<EnemyCarComponent>();
            enemyComp->speed = 15.0f; // Enemy speed
        }

        void exit()
        {
            // Nothing to clean up
        }
    };

}
