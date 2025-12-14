#pragma once

#include <application.hpp>

#include <ecs/world.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <systems/movement.hpp>
#include <systems/car-controller.hpp>
#include <systems/enemy-spawner.hpp>
#include <systems/coin-spawner.hpp>
#include <systems/laser-system.hpp>
#include <asset-loader.hpp>
#include <imgui.h>

// This state shows how to use the ECS framework and deserialization.
class Playstate : public our::State
{

    our::World world;
    our::ForwardRenderer renderer;
    our::FreeCameraControllerSystem cameraController;
    our::MovementSystem movementSystem;
    our::CarControllerSystem carController;
    our::EnemySpawnerSystem enemySpawner;
    our::CoinSpawnerSystem coinSpawner;
    our::LaserSystem laserSystem;

    void onInitialize() override
    {
        // First of all, we get the scene configuration from the app config
        auto &config = getApp()->getConfig()["scene"];
        // If we have assets in the scene config, we deserialize them
        if (config.contains("assets"))
        {
            our::deserializeAllAssets(config["assets"]);
        }
        // If we have a world in the scene config, we use it to populate our world
        if (config.contains("world"))
        {
            world.deserialize(config["world"]);
        }
        // We initialize the camera controller system since it needs a pointer to the app
        cameraController.enter(getApp());
        // Initialize the car controller system
        carController.enter(getApp());
        // Initialize enemy spawner
        enemySpawner.enter(getApp());
        // Initialize coin spawner
        coinSpawner.enter(getApp());
        // Initialize laser system
        laserSystem.enter(getApp());
        // Then we initialize the renderer
        auto size = getApp()->getFrameBufferSize();
        renderer.initialize(size, config["renderer"]);

        // Set up callback for speed boost effect (postprocess switching)
        coinSpawner.setSpeedBoostCallback([this](bool enabled)
                                          { renderer.setSpeedBoostEffect(enabled); });

        // Set up callback for speed multiplier changes (sync with enemy spawner)
        coinSpawner.setSpeedMultiplierCallback([this](float multiplier)
                                               { enemySpawner.setSpeedMultiplier(multiplier); });
    }

    void onDraw(double deltaTime) override
    {
        // Here, we just run a bunch of systems to control the world logic
        movementSystem.update(&world, (float)deltaTime);
        cameraController.update(&world, (float)deltaTime);
        carController.update(&world, (float)deltaTime);
        enemySpawner.update(&world, (float)deltaTime);
        coinSpawner.update(&world, (float)deltaTime);
        laserSystem.update(&world, (float)deltaTime);
        // And finally we use the renderer system to draw the scene
        renderer.render(&world);

        // Get a reference to the keyboard object
        auto &keyboard = getApp()->getKeyboard();

        if (keyboard.justPressed(GLFW_KEY_ESCAPE))
        {
            // If the escape  key is pressed in this frame, go to the play state
            getApp()->changeState("menu");
        }
    }

    void onImmediateGui() override
    {
        // Display score HUD
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_Always);
        ImGui::Begin("Score", nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoBackground);

        ImGui::SetWindowFontScale(2.0f);
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Score: %d", coinSpawner.getScore()+laserSystem.getScore());

        if (coinSpawner.isSpeedBoostActive())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "SPEED BOOST!");
        }

        ImGui::End();
    }

    void onDestroy() override
    {
        // Don't forget to destroy the renderer
        renderer.destroy();
        // On exit, we call exit for the camera controller system to make sure that the mouse is unlocked
        cameraController.exit();
        carController.exit();
        enemySpawner.exit();
        coinSpawner.exit();
        laserSystem.exit();
        // Clear the world
        world.clear();
        // and we delete all the loaded assets to free memory on the RAM and the VRAM
        our::clearAllAssets();
    }
};