#include "Render/Vulkan/VulkanEngine.hpp"
#include "imgui.h"

#include <iostream>

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

static void TraceImpl(const char* inFMT, ...) {
    // Format the message
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);

    // Print to the TTY
    std::cout << buffer << std::endl;
}

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine) {
    // Print to the TTY
    std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << std::endl;

    // Breakpoint
    return true;
};

#endif// JPH_ENABLE_ASSERTS

namespace Layers {
    static constexpr JPH::ObjectLayer NON_MOVING = JPH::ObjectLayer(0);
    static constexpr JPH::ObjectLayer MOVING = JPH::ObjectLayer(1);
    static constexpr JPH::ObjectLayer NUM_LAYERS = JPH::ObjectLayer(2);
}// namespace Layers

class ObjectLayerFilterImpl : public JPH::ObjectLayerPairFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
        switch (inObject1) {
            case Layers::NON_MOVING:
                return inObject2 == Layers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

namespace BroadPhaseLayers {
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::uint NUM_LAYERS(2);
}// namespace BroadPhaseLayers

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
public:
    BPLayerInterfaceImpl() {
        // Create a mapping table from object to broad phase layer
        mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }

    virtual JPH::uint GetNumBroadPhaseLayers() const override {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
        switch ((JPH::BroadPhaseLayer::Type) inLayer) {
            case (JPH::BroadPhaseLayer::Type) BroadPhaseLayers::NON_MOVING:
                return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type) BroadPhaseLayers::MOVING:
                return "MOVING";
            default:
                JPH_ASSERT(false);
                return "INVALID";
        }
    }
#endif// JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
        switch (inLayer1) {
            case Layers::NON_MOVING:
                return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

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
    JPH::RegisterDefaultAllocator();

    JPH::Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)

    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
    JPH::TempAllocatorImpl tempAllocator(10 * 1024 * 1024);
    JPH::JobSystemThreadPool jobSystem(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers);
    const JPH::uint cMaxBodies = 1024;
    const JPH::uint cNumBodyMutexes = 0;
    const JPH::uint cMaxBodyPairs = 1024;
    const JPH::uint cMaxContactConstraints = 1024;

    BPLayerInterfaceImpl broadPhaseLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;
    ObjectLayerFilterImpl objectLayerFilter;

    JPH::PhysicsSystem physicsSystem;
    physicsSystem.Init(
            cMaxBodies, cNumBodyMutexes,
            cMaxBodyPairs, cMaxContactConstraints,
            broadPhaseLayerInterface,
            objectVsBroadPhaseLayerFilter,
            objectLayerFilter);

    JPH::BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

    auto floorSettings = JPH::BodyCreationSettings(
            new JPH::BoxShape({10.0f, 0.1f, 10.0f}),
            JPH::RVec3(0.0f, -0.08f, 0.0f),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Static,
            Layers::NON_MOVING);

    auto floorBody =
            bodyInterface.CreateAndAddBody(
                    floorSettings,
                    JPH::EActivation::DontActivate);

    moe::Vector<JPH::BodyID> sphereBodies;

    constexpr float sphereScale = 0.1f;
    float sphereSpeed = 0.0;
    float planeFriction = 0.5f;
    float sphereRestitution = 0.6f;
    float sphereFriction = 0.5f;

    auto sphereFactory = [&](glm::vec3 pos, glm::vec3 velocity) {
        JPH::BodyCreationSettings sphereSettings(
                new JPH::SphereShape(1.5f * sphereScale),
                JPH::RVec3(pos.x, pos.y, pos.z),
                JPH::Quat::sIdentity(),
                JPH::EMotionType::Dynamic,
                Layers::MOVING);

        auto sphereBody =
                bodyInterface.CreateAndAddBody(
                        sphereSettings,
                        JPH::EActivation::Activate);

        bodyInterface.SetLinearVelocity(
                sphereBody,
                JPH::Vec3(velocity.x, velocity.y, velocity.z));

        bodyInterface.SetRestitution(sphereBody, sphereRestitution);
        bodyInterface.SetFriction(sphereBody, sphereFriction);

        sphereBodies.push_back(sphereBody);
    };

    physicsSystem.OptimizeBroadPhase();

    moe::VulkanEngine engine;
    engine.init();
    auto& camera = engine.getDefaultCamera();
    auto& spriteCamera = engine.getDefaultSpriteCamera();
    auto& illuminationBus = engine.getBus<moe::VulkanIlluminationBus>();
    auto& renderBus = engine.getBus<moe::VulkanRenderObjectBus>();
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

    /*auto sceneId = engine.loadGLTF("anim/anim.gltf");

    auto scene = engine.m_caches.objectCache.get(sceneId).value();
    auto* animatableRenderable = scene->checkedAs<moe::VulkanSkeletalAnimation>(moe::VulkanRenderableFeature::HasSkeletalAnimation).value();
    auto& animations = animatableRenderable->getAnimations();
    auto usedAnimation = animations.begin()->second;

    float animationTime = 0.0f;
    int animationIndex = 0;*/

    auto& loader = engine.getResourceLoader();

    auto plane = loader.load("phy/plane/plane.gltf", moe::Loader::Gltf);
    auto sphere = loader.load("phy/sphere/sphere.gltf", moe::Loader::Gltf);

    auto spriteImageId = loader.load("test_sprite.jpg", moe::Loader::Image);

    moe::Logger::debug("Starting main loop");
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

        engine.addImGuiDrawCommand([&] {
            if (ImGui::Begin("Settings")) {
                // draw set fxaa enabled
                ImGui::Checkbox("Enable FXAA", &engine.m_enableFxaa);

                // shadow map cam scale
                ImGui::BeginGroup();
                ImGui::Text("Shadow Map Camera Scale");
                ImGui::SliderFloat("Scale X", &engine.m_shadowMapCameraScale.x, 1.0f, 5.0f);
                ImGui::SliderFloat("Scale Y", &engine.m_shadowMapCameraScale.y, 1.0f, 5.0f);
                ImGui::SliderFloat("Scale Z", &engine.m_shadowMapCameraScale.z, 1.0f, 5.0f);
                ImGui::EndGroup();

                ImGui::Separator();
                ImGui::Text("UI Camera");
                ImGui::SliderFloat("UI Camera Pos X", &spriteCamera.position.x, -1000.0f, 1000.0f);
                ImGui::SliderFloat("UI Camera Pos Y", &spriteCamera.position.y, -1000.0f, 1000.0f);
                ImGui::SliderFloat("UI Camera Zoom", &spriteCamera.zoom, 0.1f, 10.0f);

                ImGui::Separator();
                ImGui::Text("Physics Debug");
                ImGui::Button("Generate a sphere", ImVec2(150, 0));
                if (ImGui::IsItemClicked()) {
                    moe::Logger::debug("Generate a sphere");
                    sphereFactory(
                            camera.getPosition() + glm::normalize(camera.getFront()) * 0.2f,
                            glm::normalize(camera.getFront()) * sphereSpeed);
                }
                ImGui::SliderFloat("Sphere Initial Speed", &sphereSpeed, 0.0f, 20.0f);
                ImGui::SliderFloat("Plane Friction", &planeFriction, 0.0f, 1.0f);
                ImGui::SliderFloat("Sphere Restitution", &sphereRestitution, 0.0f, 1.0f);
                ImGui::SliderFloat("Sphere Friction", &sphereFriction, 0.0f, 1.0f);
                /*ImGui::SliderFloat("Animation Time", &animationTime, 0.0f, 1.0f);
                if (ImGui::BeginListBox("Animation Select")) {
                    int i = 0;
                    for (auto& [name, animation]: animations) {
                        if (ImGui::Selectable(name.c_str(), animationIndex == i)) {
                            usedAnimation = animation;
                            animationIndex = i;
                        }
                        i++;
                    }
                    ImGui::EndListBox();
                }*/
                ImGui::End();
            }
        });


        if (movementMask.isMoving()) {
            glm::vec3 frontProjected = camera.getFront();
            frontProjected.y = 0.0f;
            cameraMovement = movementMask.getDirection(frontProjected, camera.getRight(), camera.getUp());
            camera.setPosition(camera.getPosition() + cameraMovement * cameraMoveSpeed * deltaTime);
        } else {
            cameraMovement = glm::vec3(0.0f);
        }

        moe::Transform objectTransform{};
        objectTransform.setScale(glm::vec3(1.0f));

        /*auto computeHandle = renderBus.submitComputeSkin(sceneId, usedAnimation, animationTime);
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                objectTransform.setPosition(glm::vec3((i - 2) * 2.0f, 0.0f, (j - 2) * 2.0f));
                renderBus.submitRender(sceneId, objectTransform, computeHandle);
            }
        }*/

        bodyInterface.SetFriction(floorBody, planeFriction);
        renderBus.submitRender(plane, moe::Transform{});

        for (auto bodyId: sphereBodies) {
            glm::vec3 position(0);
            glm::quat rotation(1, 0, 0, 0);
            JPH::RVec3 comPosition = bodyInterface.GetCenterOfMassPosition(bodyId);
            JPH::Quat comOrientation = bodyInterface.GetRotation(bodyId);

            position = glm::vec3(comPosition.GetX(), comPosition.GetY(), comPosition.GetZ());
            rotation = glm::quat(comOrientation.GetW(), comOrientation.GetX(), comOrientation.GetY(), comOrientation.GetZ());

            moe::Transform sphereTransform{};
            sphereTransform.setPosition(position);
            sphereTransform.setScale(glm::vec3(sphereScale));
            sphereTransform.setRotation(rotation);
            renderBus.submitRender(sphere, sphereTransform);
        }

        physicsSystem.Update(deltaTime, 3, &tempAllocator, &jobSystem);

        {
            renderBus.submitSpriteRender(
                    spriteImageId,
                    moe::Transform{},
                    moe::Colors::White,
                    glm::vec2(640.0f, 360.0f),
                    glm::vec2(0.0f, 0.0f),
                    glm::vec2(1920.0f, 1080.0f));
        }

        illuminationBus.setAmbient(glm::vec3(1.f, 1.f, 1.f), 0.2f);

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

    bodyInterface.RemoveBody(floorBody);
    bodyInterface.RemoveBodies(sphereBodies.data(), (JPH::uint) sphereBodies.size());
    bodyInterface.DestroyBody(floorBody);
    bodyInterface.DestroyBodies(sphereBodies.data(), (JPH::uint) sphereBodies.size());

    JPH::UnregisterTypes();

    delete JPH::Factory::sInstance;
    return 0;
}