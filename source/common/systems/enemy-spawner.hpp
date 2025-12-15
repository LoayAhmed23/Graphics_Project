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
#include <vector>

// For audio (Windows)
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif


namespace our
{
    class EnemySpawnerSystem
    {
        Application *app;
        float spawnTimer = 0.0f;
        float spawnInterval = 1.5f; // Spawn every 2 seconds
        float minSpawnInterval = 1.0f;
        float maxSpawnInterval = 10.0f;

        float spawnZ = -80.0f;  // Spawn far ahead (negative Z)
        float despawnZ = 60.0f; // Remove when past camera

        float minX = -10.0f; // Left lane boundary
        float maxX = 10.0f;   // Right lane boundary

        // AABB (Axis-Aligned Bounding Box) half-extents for collision
        // These represent half the width and half the length of each car's bounding box
        float playerHalfWidth = 1.0f;  // Half of car width (X axis)
        float playerHalfLength = 2.0f; // Half of car length (Z axis)
        float enemyHalfWidth = 1.0f;   // Half of enemy car width
        float enemyHalfLength = 2.0f;  // Half of enemy car length

        bool gameOver = false;
        float gameTime = 0.0f;
        float baseEnemySpeed = 15.0f;
        float speedIncreaseRate = 0.5f; // Speed increase per second of gameplay
        float maxEnemySpeed = 100.0f;

        // External speed multiplier (from coin system)
        float externalSpeedMultiplier = 1.0f;

        std::random_device rd;
        std::mt19937 gen;

    public:
        EnemySpawnerSystem() : gen(rd()) {}

        void enter(Application *app)
        {
            this->app = app;
            spawnTimer = 4.0f;
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
            if (gameOver)
                return;
            gameTime += deltaTime;

            // Update spawn timer
            spawnTimer -= deltaTime;

            // Only spawn when driving forward
            if (spawnTimer <= 0.0f)
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
            glm::vec3 playerPos(0.0f); // Initialize to zero
            
            for (auto entity : world->getEntities())
            {
                CarControllerComponent *car = entity->getComponent<CarControllerComponent>();
                if (car)
                {
                    playerEntity = entity;
                    playerSpeed = car->speed;
                    playerPos = entity->localTransform.position; // Now safe after assignment
                    break;
                }
            }

            // If no player found, skip collision detection
            if (!playerEntity)
                return;

            // First pass: Calculate new positions for all enemies
            struct EnemyUpdate
            {
                Entity* entity;
                glm::vec3 prevPos;
                glm::vec3 newPos;
            };
            std::vector<EnemyUpdate> enemyUpdates;

            for (auto entity : world->getEntities())
            {
                auto enemy = entity->getComponent<EnemyCarComponent>();
                if (!enemy) continue;

                glm::vec3 prev = entity->localTransform.position;

                float currentSpeed = getCurrentEnemySpeed();
                float relBoost = 0;

                relBoost = playerSpeed * externalSpeedMultiplier;

                float finalZspeed = currentSpeed + relBoost;

                glm::vec3 newPos = prev;
                newPos.z += finalZspeed * deltaTime;

                enemyUpdates.push_back({entity, prev, newPos});
            }

            // Second pass: Check collisions and apply movement
            for (auto& u : enemyUpdates)
            {
                glm::vec3 prev = u.prevPos;
                glm::vec3 next = u.newPos;

                // Swept AABB for Z
                float enemyMinZ = std::min(prev.z, next.z) - enemyHalfLength;
                float enemyMaxZ = std::max(prev.z, next.z) + enemyHalfLength;

                float enemyMinX = next.x - enemyHalfWidth;   // use final X
                float enemyMaxX = next.x + enemyHalfWidth;

                float playerMinX = playerPos.x - playerHalfWidth;
                float playerMaxX = playerPos.x + playerHalfWidth;

                float playerMinZ = playerPos.z - playerHalfLength;
                float playerMaxZ = playerPos.z + playerHalfLength;

                bool hitX = (playerMinX <= enemyMaxX) && (playerMaxX >= enemyMinX);
                bool hitZ = (playerMinZ <= enemyMaxZ) && (playerMaxZ >= enemyMinZ);

                if (hitX && hitZ)
                {
            #ifdef _WIN32
                    Beep(300, 200);
            #endif
                    gameOver = true;
                    app->changeState("menu");
                    return;
                }

                // apply movement
                u.entity->localTransform.position = next;

                // despawn
                if (next.z > despawnZ)
                    world->markForRemoval(u.entity);
            }

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
            enemy->localTransform.rotation = glm::vec3(
                -glm::half_pi<float>(),          // -90°
                glm::half_pi<float>(), // 90°
                0.0f);
            enemy->localTransform.scale = glm::vec3(0.5f, 0.5f, 0.5f);

            // Add mesh renderer
            MeshRendererComponent *meshRenderer = enemy->addComponent<MeshRendererComponent>();
            meshRenderer->mesh = AssetLoader<Mesh>::get("car");
            meshRenderer->material = AssetLoader<Material>::get("car_frame");

            // Add enemy car component
            EnemyCarComponent *enemyComp = enemy->addComponent<EnemyCarComponent>();
            enemyComp->speed = getCurrentEnemySpeed(); // Enemy speed
        }

        void exit()
        {
            // Nothing to clean up
        }
    };

}