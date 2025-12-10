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
#include <algorithm>

// For audio (Windows)
#ifdef _WIN32
#include <windows.h>
#endif

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

        float collisionDistanceX = 1.9f; // Slightly less than 2.0 to allow grazing
        float collisionDistanceZ = 3.8f; // Close to 4.0 to detect bumper-to-bumper contact

        bool gameOver = false;

        // Time-based difficulty progression
        float gameTime = 0.0f;
        float baseEnemySpeed = 15.0f;
        float speedIncreaseRate = 0.5f; // Speed increase per second of gameplay
        float maxEnemySpeed = 50.0f;

        // External speed multiplier (from coin system)
        float externalSpeedMultiplier = 1.0f;

        std::random_device rd;
        std::mt19937 gen;

    public:
        EnemySpawnerSystem() : gen(rd()) {}

        void enter(Application *app)
        {
            this->app = app;
            spawnTimer = spawnInterval;
            gameOver = false;
            gameTime = 0.0f;
            externalSpeedMultiplier = 1.0f;
        }

        // Set external speed multiplier (called by coin system during speed boost)
        void setSpeedMultiplier(float multiplier) { externalSpeedMultiplier = multiplier; }
        float getSpeedMultiplier() const { return externalSpeedMultiplier; }

        // Get current enemy speed based on game progression
        float getCurrentEnemySpeed() const
        {
            float timeBasedSpeed = baseEnemySpeed + (gameTime * speedIncreaseRate);
            return std::min(timeBasedSpeed, maxEnemySpeed);
        }

        void update(World *world, float deltaTime)
        {
            // If game over, don't update
            if (gameOver)
                return;

            // Update game time for difficulty progression
            gameTime += deltaTime;

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

                // Random interval for next spawn (gets shorter over time)
                float timeScale = std::max(0.5f, 1.0f - gameTime * 0.01f); // Spawn faster over time
                std::uniform_real_distribution<float> intervalDist(minSpawnInterval * timeScale, maxSpawnInterval * timeScale);
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

            // Calculate speed modifier based on player movement and external multiplier
            float speedModifier = 0.0f;
            if (isMovingForward)
            { // But we need to be careful not to make them too fast relative to the world
                speedModifier = playerSpeed * externalSpeedMultiplier;
            }
            else if (isMovingBackward)
            {
                // When player moves backward, enemies appear to move slower (or away)
                speedModifier = -playerSpeed * externalSpeedMultiplier;
            }
            auto entities = world->getEntities();
            for (auto entity : entities)
            {
                EnemyCarComponent *enemy = entity->getComponent<EnemyCarComponent>();
                if (enemy)
                {
                    // Enemy moves toward player with time-based speed increase
                    float currentSpeed = getCurrentEnemySpeed();
                    float totalSpeed = currentSpeed + speedModifier;

                    // Store previous position for swept collision detection
                    float prevZ = entity->localTransform.position.z;
                    entity->localTransform.position.z += totalSpeed * deltaTime;
                    float newZ = entity->localTransform.position.z;

                    // Check collision with player using swept collision check
                    if (playerEntity)
                    {
                        glm::vec3 enemyPos = entity->localTransform.position;

                        // Calculate distance between car centers on X axis
                        float distX = std::abs(playerPos.x - enemyPos.x);

                        // For Z axis, check if the enemy's movement path intersects with the player's collision box
                        // This handles cases where the enemy moves so fast it "skips" over the player in one frame
                        float minZ = std::min(prevZ, newZ);
                        float maxZ = std::max(prevZ, newZ);

                        float playerMinZ = playerPos.z - collisionDistanceZ;
                        float playerMaxZ = playerPos.z + collisionDistanceZ;

                        // Check for overlap between intervals [minZ, maxZ] and [playerMinZ, playerMaxZ]
                        bool collisionZ = (minZ <= playerMaxZ && playerMinZ <= maxZ);

                        // Both X AND Z must be within collision distance (or path intersected Z)
                        if (distX < collisionDistanceX && collisionZ)
                        {
// Collision detected! Play crash sound and game over
#ifdef _WIN32
                            // Use system beep for crash sound (no audio file needed)
                            Beep(300, 200); // Low frequency beep for crash
#endif

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
            enemy->localTransform.rotation = glm::vec3(0.0f, 0.0f, 0.0f); // Front faces positive Z (toward player/camera)
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
