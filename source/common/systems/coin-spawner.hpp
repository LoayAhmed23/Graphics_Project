#pragma once

#include "../ecs/world.hpp"
#include "../components/coin.hpp"
#include "../components/car-controller.hpp"
#include "../components/camera.hpp"
#include "../components/mesh-renderer.hpp"
#include "../application.hpp"
#include "../asset-loader.hpp"

#include <glm/glm.hpp>
#include <random>
#include <cmath>
#include <functional>

namespace our
{

    // System to spawn and collect coins
    class CoinSpawnerSystem
    {
        Application *app;
        float spawnTimer = 0.0f;
        float spawnInterval = 4.0f; // Spawn every 4 seconds consistently

        float spawnZ = -85.0f;  // Spawn far ahead (negative Z)
        float despawnZ = 60.0f; // Remove when past camera

        float minX = -10.0f; // Left boundary
        float maxX = 7.0f;   // Right boundary

        // Collision detection settings
        float collisionDistanceX = 2.5f;
        float collisionDistanceZ = 3.0f;

        std::random_device rd;
        std::mt19937 gen;

        // Game state
        int score = 0;
        bool speedBoostActive = false;
        float speedBoostTimer = 0.0f;
        float speedBoostDuration = 3.0f; // Boost lasts 3 seconds
        float currentSpeedMultiplier = 1.0f;

        // FOV settings for speed boost
        float baseFOV = 60.0f;  // Normal FOV (will be read from camera)
        float boostFOV = 90.0f; // Increased FOV during boost
        float currentFOV = 60.0f;
        float fovLerpSpeed = 5.0f; // How fast FOV transitions

        // Callbacks
        std::function<void(bool)> onSpeedBoostChanged;
        std::function<void(float)> onSpeedMultiplierChanged;

    public:
        CoinSpawnerSystem() : gen(rd()) {}

        void enter(Application *app)
        {
            this->app = app;
            spawnTimer = 2.0f; // Start with a coin spawning soon
            score = 0;
            speedBoostActive = false;
            speedBoostTimer = 0.0f;
            currentSpeedMultiplier = 1.0f;
            currentFOV = baseFOV;
        }

        // Set callback for when speed boost state changes (for postprocess switching)
        void setSpeedBoostCallback(std::function<void(bool)> callback)
        {
            onSpeedBoostChanged = callback;
        }

        // Set callback for speed multiplier changes (for enemy spawner)
        void setSpeedMultiplierCallback(std::function<void(float)> callback)
        {
            onSpeedMultiplierChanged = callback;
        }

        int getScore() const { return score; }
        bool isSpeedBoostActive() const { return speedBoostActive; }
        float getSpeedMultiplier() const { return currentSpeedMultiplier; }
        float getCurrentFOV() const { return currentFOV; }

        void update(World *world, float deltaTime)
        {
            // Update FOV smoothly
            float targetFOV = speedBoostActive ? boostFOV : baseFOV;
            currentFOV = currentFOV + (targetFOV - currentFOV) * fovLerpSpeed * deltaTime;

            // Apply FOV to camera
            for (auto entity : world->getEntities())
            {
                CameraComponent *cam = entity->getComponent<CameraComponent>();
                if (cam)
                {
                    cam->fovY = glm::radians(currentFOV);
                    break;
                }
            }

            // Update speed boost timer
            if (speedBoostActive)
            {
                speedBoostTimer -= deltaTime;
                if (speedBoostTimer <= 0.0f)
                {
                    // Speed boost ended
                    speedBoostActive = false;
                    currentSpeedMultiplier = 1.0f;

                    // Notify callbacks
                    if (onSpeedBoostChanged)
                    {
                        onSpeedBoostChanged(false);
                    }
                    if (onSpeedMultiplierChanged)
                    {
                        onSpeedMultiplierChanged(currentSpeedMultiplier);
                    }
                }
            }

            // Update spawn timer - coins spawn consistently regardless of player movement
            spawnTimer -= deltaTime;

            // Check keyboard input
            auto &keyboard = app->getKeyboard();
            bool isMovingForward = keyboard.isPressed(GLFW_KEY_W);

            // Spawn coins consistently (always spawn when timer expires, not just when moving)
            if (spawnTimer <= 0.0f)
            {
                spawnCoin(world);
                spawnTimer = spawnInterval;
            }

            // Find player car position
            Entity *playerEntity = nullptr;
            glm::vec3 playerPos(0.0f);
            float playerSpeed = 0.0f;
            for (auto entity : world->getEntities())
            {
                CarControllerComponent *car = entity->getComponent<CarControllerComponent>();
                if (car)
                {
                    playerEntity = entity;
                    playerPos = entity->localTransform.position;
                    playerSpeed = car->speed;
                    break;
                }
            }

            // Calculate speed modifier based on player movement and boost
            float speedModifier = 0.0f;
            if (isMovingForward)
            {
                speedModifier = playerSpeed * currentSpeedMultiplier;
            }
            else if (keyboard.isPressed(GLFW_KEY_S))
            {
                speedModifier = -playerSpeed * currentSpeedMultiplier;
            }

            // Move all coins and check for collection
            auto entities = world->getEntities();
            for (auto entity : entities)
            {
                CoinComponent *coin = entity->getComponent<CoinComponent>();
                if (coin)
                {
                    // Move coin toward player
                    entity->localTransform.position.z += speedModifier * deltaTime;

                    // Rotate coin for visual effect
                    entity->localTransform.rotation.y += coin->rotationSpeed * deltaTime;

                    // Check collision with player
                    if (playerEntity)
                    {
                        glm::vec3 coinPos = entity->localTransform.position;
                        float distX = std::abs(playerPos.x - coinPos.x);
                        float distZ = std::abs(playerPos.z - coinPos.z);

                        if (distX < collisionDistanceX && distZ < collisionDistanceZ)
                        {
                            // Coin collected!
                            score += coin->value;

                            // Activate speed boost
                            speedBoostActive = true;
                            speedBoostTimer = speedBoostDuration;
                            currentSpeedMultiplier = 1.8f; // 80% speed boost during boost

                            // Notify callbacks
                            if (onSpeedBoostChanged)
                            {
                                onSpeedBoostChanged(true);
                            }
                            if (onSpeedMultiplierChanged)
                            {
                                onSpeedMultiplierChanged(currentSpeedMultiplier);
                            }

                            world->markForRemoval(entity);
                            continue;
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

        void spawnCoin(World *world)
        {
            // Create coin entity
            Entity *coin = world->add();

            // Random X position within road bounds
            std::uniform_real_distribution<float> xDist(minX, maxX);
            float xPos = xDist(gen);

            coin->localTransform.position = glm::vec3(xPos, 1.5f, spawnZ); // Slightly above ground
            coin->localTransform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
            coin->localTransform.scale = glm::vec3(1.0f, 1.0f, 1.0f);

            // Add mesh renderer - use sphere as coin for now
            MeshRendererComponent *meshRenderer = coin->addComponent<MeshRendererComponent>();
            meshRenderer->mesh = AssetLoader<Mesh>::get("sphere");
            meshRenderer->material = AssetLoader<Material>::get("coin");

            // Add coin component
            CoinComponent *coinComp = coin->addComponent<CoinComponent>();
            coinComp->value = 1;
            coinComp->rotationSpeed = 180.0f;
        }

        void exit()
        {
            // Ensure any active speed-boost effects are cleared when exiting (e.g., on game over)
            speedBoostActive = false;
            speedBoostTimer = 0.0f;
            currentSpeedMultiplier = 1.0f;
            // Notify callbacks so renderer and other systems reset their state
            if(onSpeedBoostChanged) onSpeedBoostChanged(false);
            if(onSpeedMultiplierChanged) onSpeedMultiplierChanged(currentSpeedMultiplier);
        }
    };

}
