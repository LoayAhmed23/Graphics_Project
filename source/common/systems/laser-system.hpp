#pragma once

#include "../ecs/world.hpp"
#include "../components/laser.hpp"
#include "../components/car-controller.hpp"
#include "../components/enemy-car.hpp"
#include "../components/mesh-renderer.hpp"
#include "../application.hpp"
#include "../asset-loader.hpp"

#include <glm/glm.hpp>
#include <vector>

// For audio (Windows)
#ifdef _WIN32
#include <windows.h>
#endif

namespace our
{
    class LaserSystem
    {
        Application *app;

        // Laser settings
        float laserSpeed = 100.0f;    // Speed of laser beam
        float laserLifetime = 2.0f;   // Max lifetime before despawn
        float laserCooldown = 0.3f;   // Cooldown between shots
        float currentCooldown = 0.0f; // Current cooldown timer

        // Laser dimensions for collision
        float laserHalfWidth = 0.2f;
        float laserHalfLength = 1.0f;

        // Enemy dimensions for collision (should match enemy-spawner)
        float enemyHalfWidth = 1.0f;
        float enemyHalfLength = 2.0f;

    public:
        LaserSystem() {}

        void enter(Application *app)
        {
            this->app = app;
            currentCooldown = 0.0f;
        }

        void update(World *world, float deltaTime)
        {
            // Update cooldown
            if (currentCooldown > 0.0f)
            {
                currentCooldown -= deltaTime;
            }

            // Find player car position
            Entity *playerEntity = nullptr;
            glm::vec3 playerPos(0.0f);

            for (auto entity : world->getEntities())
            {
                CarControllerComponent *car = entity->getComponent<CarControllerComponent>();
                if (car)
                {
                    playerEntity = entity;
                    playerPos = entity->localTransform.position;
                    break;
                }
            }

            // Check for laser fire input (L key)
            auto &keyboard = app->getKeyboard();
            if (keyboard.justPressed(GLFW_KEY_L) && currentCooldown <= 0.0f && playerEntity)
            {
                spawnLaser(world, playerPos);
                currentCooldown = laserCooldown;

                // Play laser sound
#ifdef _WIN32
                Beep(800, 50); // High pitch short beep for laser
#endif
            }

            // Collect all enemies for collision checking
            struct EnemyData
            {
                Entity *entity;
                glm::vec3 position;
            };
            std::vector<EnemyData> enemies;

            for (auto entity : world->getEntities())
            {
                EnemyCarComponent *enemy = entity->getComponent<EnemyCarComponent>();
                if (enemy)
                {
                    enemies.push_back({entity, entity->localTransform.position});
                }
            }

            // Update all lasers
            for (auto entity : world->getEntities())
            {
                LaserComponent *laser = entity->getComponent<LaserComponent>();
                if (!laser)
                    continue;

                // Update lifetime
                laser->currentLife += deltaTime;
                if (laser->currentLife >= laser->lifetime)
                {
                    world->markForRemoval(entity);
                    continue;
                }

                // Move laser forward (negative Z direction)
                entity->localTransform.position.z -= laser->speed * deltaTime;

                // Despawn if too far
                if (entity->localTransform.position.z < -100.0f)
                {
                    world->markForRemoval(entity);
                    continue;
                }

                // Check collision with enemies
                glm::vec3 laserPos = entity->localTransform.position;

                float laserMinX = laserPos.x - laserHalfWidth;
                float laserMaxX = laserPos.x + laserHalfWidth;
                float laserMinZ = laserPos.z - laserHalfLength;
                float laserMaxZ = laserPos.z + laserHalfLength;

                for (auto &enemyData : enemies)
                {
                    glm::vec3 enemyPos = enemyData.position;

                    float enemyMinX = enemyPos.x - enemyHalfWidth;
                    float enemyMaxX = enemyPos.x + enemyHalfWidth;
                    float enemyMinZ = enemyPos.z - enemyHalfLength;
                    float enemyMaxZ = enemyPos.z + enemyHalfLength;

                    // AABB collision check
                    bool hitX = (laserMinX <= enemyMaxX) && (laserMaxX >= enemyMinX);
                    bool hitZ = (laserMinZ <= enemyMaxZ) && (laserMaxZ >= enemyMinZ);

                    if (hitX && hitZ)
                    {
                        // Destroy enemy
                        world->markForRemoval(enemyData.entity);
                        // Destroy laser
                        world->markForRemoval(entity);

                        // Play explosion sound
#ifdef _WIN32
                        Beep(200, 100); // Low pitch for explosion
#endif
                        break;
                    }
                }
            }

            world->deleteMarkedEntities();
        }

        void spawnLaser(World *world, const glm::vec3 &playerPos)
        {
            // Create laser entity
            Entity *laser = world->add();

            // Position laser in front of the player car
            laser->localTransform.position = glm::vec3(playerPos.x, playerPos.y + 0.5f, playerPos.z - 3.0f);
            laser->localTransform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
            laser->localTransform.scale = glm::vec3(0.3f, 0.3f, 2.0f); // Elongated beam shape

            // Add mesh renderer - use cube stretched as laser beam
            MeshRendererComponent *meshRenderer = laser->addComponent<MeshRendererComponent>();
            meshRenderer->mesh = AssetLoader<Mesh>::get("cube");
            meshRenderer->material = AssetLoader<Material>::get("laser");

            // Add laser component
            LaserComponent *laserComp = laser->addComponent<LaserComponent>();
            laserComp->speed = laserSpeed;
            laserComp->lifetime = laserLifetime;
            laserComp->currentLife = 0.0f;
        }

        void exit()
        {
            // Nothing to clean up
        }
    };

}
