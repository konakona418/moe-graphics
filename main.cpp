#include "Render/Vulkan/VulkanEngine.hpp"

struct CameraMovementMask {
    enum class CameraMovementDirection {
        Forward,
        Backward,
        Left,
        Right,
        Up,
        Down
    };

    uint8_t forward : 1;
    uint8_t backward : 1;
    uint8_t left : 1;
    uint8_t right : 1;
    uint8_t up : 1;
    uint8_t down : 1;

    CameraMovementMask() : forward(0), backward(0), left(0), right(0), up(0), down(0) {}

    bool isMoving() const {
        return forward || backward || left || right || up || down;
    }

    void set(CameraMovementDirection direction, bool value) {
        switch (direction) {
            case CameraMovementDirection::Forward:
                forward = value;
                break;
            case CameraMovementDirection::Backward:
                backward = value;
                break;
            case CameraMovementDirection::Left:
                left = value;
                break;
            case CameraMovementDirection::Right:
                right = value;
                break;
            case CameraMovementDirection::Up:
                up = value;
                break;
            case CameraMovementDirection::Down:
                down = value;
                break;
        }
    }

    glm::vec3 getDirection(glm::vec3 cameraFront, glm::vec3 cameraRight, glm::vec3 cameraUp) const {
        glm::vec3 dir(0.0f);
        if (forward) dir += cameraFront;
        if (backward) dir += -cameraFront;
        if (left) dir += -cameraRight;
        if (right) dir += cameraRight;
        if (up) dir += cameraUp;
        if (down) dir += -cameraUp;
        if (glm::length(dir) > 0.0f) {
            dir = glm::normalize(dir);
        }
        return dir;
    }
};

int main() {
    moe::VulkanEngine engine;
    engine.init();
    auto& camera = engine.getDefaultCamera();
    auto& illuminationBus = engine.getBus<moe::VulkanIlluminationBus>();
    camera.setPosition(glm::vec3(0.0f, 0.0f, 3.0f));
    camera.setYaw(-90.0f);
    constexpr float cameraRotDampening = 0.1f;
    constexpr float cameraMoveSpeed = 5.f;
    CameraMovementMask movementMask;

    std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();

    bool running = true;
    glm::vec3 cameraMovement = glm::vec3(0.0f);

    auto& inputBus = engine.getInputBus();
    inputBus.setMouseValid(false);

    bool isMouseValid = false;

    float elapsed = 0.0f;

    while (running) {
        engine.beginFrame();
        auto now = std::chrono::high_resolution_clock::now();
        auto deltaTime = (float) std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count() / 1000.0f;
        lastTime = now;

        elapsed += deltaTime;

        while (auto e = inputBus.pollEvent()) {
            if (e->is<moe::WindowEvent::Close>()) {
                running = false;
            }

            if (!isMouseValid) {
                if (auto mouse = e->getIf<moe::WindowEvent::MouseMove>()) {
                    //moe::Logger::info("Mouse move: {}, {}", mouse->deltaX, mouse->deltaY);
                    camera.setYaw(camera.getYaw() + mouse->deltaX * cameraRotDampening);
                    camera.setPitch(camera.getPitch() + -mouse->deltaY * cameraRotDampening);
                }
            }

            if (auto keyup = e->getIf<moe::WindowEvent::KeyUp>()) {
                if (keyup->keyCode == GLFW_KEY_ESCAPE) {
                    inputBus.setMouseValid(!isMouseValid);
                    isMouseValid = !isMouseValid;
                }
            }
        }

        if (inputBus.isKeyPressed(GLFW_KEY_W)) {
            movementMask.set(CameraMovementMask::CameraMovementDirection::Forward, true);
        } else {
            movementMask.set(CameraMovementMask::CameraMovementDirection::Forward, false);
        }
        if (inputBus.isKeyPressed(GLFW_KEY_S)) {
            movementMask.set(CameraMovementMask::CameraMovementDirection::Backward, true);
        } else {
            movementMask.set(CameraMovementMask::CameraMovementDirection::Backward, false);
        }
        if (inputBus.isKeyPressed(GLFW_KEY_A)) {
            movementMask.set(CameraMovementMask::CameraMovementDirection::Left, true);
        } else {
            movementMask.set(CameraMovementMask::CameraMovementDirection::Left, false);
        }
        if (inputBus.isKeyPressed(GLFW_KEY_D)) {
            movementMask.set(CameraMovementMask::CameraMovementDirection::Right, true);
        } else {
            movementMask.set(CameraMovementMask::CameraMovementDirection::Right, false);
        }
        if (inputBus.isKeyPressed(GLFW_KEY_Q)) {
            movementMask.set(CameraMovementMask::CameraMovementDirection::Up, true);
        } else {
            movementMask.set(CameraMovementMask::CameraMovementDirection::Up, false);
        }
        if (inputBus.isKeyPressed(GLFW_KEY_E)) {
            movementMask.set(CameraMovementMask::CameraMovementDirection::Down, true);
        } else {
            movementMask.set(CameraMovementMask::CameraMovementDirection::Down, false);
        }


        if (movementMask.isMoving()) {
            cameraMovement = movementMask.getDirection(camera.getFront(), camera.getRight(), camera.getUp());
            camera.setPosition(camera.getPosition() + cameraMovement * cameraMoveSpeed * deltaTime);
        } else {
            cameraMovement = glm::vec3(0.0f);
        }

        // reset dynamic lights
        illuminationBus.resetDynamicState();

        illuminationBus.setAmbient(glm::vec3(0.1f, 0.1f, 0.1f), 0.2f);

        {
            moe::VulkanCPULight light{};
            light.type = moe::LightType::Point;
            light.position = glm::vec3(2.0f, 2.0f, -2.0f) * glm::vec3(glm::cos(elapsed), 1.0f, glm::sin(elapsed));
            light.color = glm::vec3(1.0f, 0.5f, 0.0f);
            light.intensity = 4.0f;
            light.radius = 3.0f;
            illuminationBus.addDynamicLight(light);
        }
        {
            moe::VulkanCPULight light{};
            light.type = moe::LightType::Point;
            light.position = glm::vec3(-2.0f, 2.0f, 2.0f) * glm::vec3(glm::cos(elapsed), 1.0f, glm::sin(elapsed));
            light.color = glm::vec3(0.0f, 0.5f, 1.0f);
            light.intensity = 4.0f;
            light.radius = 3.0f;
            illuminationBus.addDynamicLight(light);
        }
        {
            moe::VulkanCPULight light{};
            light.type = moe::LightType::Directional;
            light.color = glm::vec3(1.0f, 1.0f, 1.0f);
            light.intensity = 2.0f;
            light.direction = glm::normalize(glm::vec3(1.0f, -1.0f, 0.0f));
            illuminationBus.setSunlight(light.direction, light.color, light.intensity);
        }

        engine.endFrame();
    }

    engine.cleanup();
    return 0;
}