#include "Render/Vulkan/VulkanEngine.hpp"
#include "Render/Vulkan/VulkanFont.hpp"

#include "Audio/AudioEngine.hpp"

#include "Physics/GltfColliderFactory.hpp"
#include "Physics/JoltIncludes.hpp"

#include "Core/FileReader.hpp"
#include "Core/Ref.hpp"
#include "Core/RefCounted.hpp"

#include "Core/Resource/Lazy.hpp"
#include "Core/Resource/OrDefault.hpp"
#include "Core/Resource/Preload.hpp"
#include "Core/Resource/Test.hpp"


#include "Core/Task/Future.hpp"
#include "Core/Task/Scheduler.hpp"
#include "Core/Task/Utils.hpp"

#include "Core/Signal/Signal.hpp"
#include "Core/Signal/Slot.hpp"

#include "Core/Sync.hpp"

#include "Core/Expected.hpp"

#include "UI/BoxWidget.hpp"
#include "UI/TextWidget.hpp"

#include "Audio/AudioSource.hpp"
#include "Audio/StreamedOggProvider.hpp"
#include "Core/Resource/BinaryBuffer.hpp"

#include "imgui.h"

#include <iostream>


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

/*void testRefcount() {
    struct RefCountedObject : public moe::RefCounted<RefCountedObject> {
        RefCountedObject() {
            moe::Logger::debug("RefCountedObject created");
        }

        ~RefCountedObject() {
            moe::Logger::debug("RefCountedObject destroyed");
        }
    };

    auto ref1 = moe::Ref(new RefCountedObject());
    {
        auto ref2 = ref1;
        {
            auto ref3 = ref2;
            moe::Logger::debug("Ref count after 3 refs: {}", ref3->getRefCount());
        }
        moe::Logger::debug("Ref count after ref3 out of scope: {}", ref2->getRefCount());
    }
    moe::Logger::debug("Ref count after ref2 out of scope: {}", ref1->getRefCount());
}

void testLazy() {
    auto lazyValue = moe::Lazy<moe::OrDefault<moe::TestGenerator<int, true>>>(42, 123);
    moe::Logger::debug("Lazy value created");
    int value = lazyValue.generate().value();
    moe::Logger::debug("Lazy value generated: {}", value);
}

void testPreload() {
    auto preloadValue =
            moe::Preload<moe::Launch<moe::TestGeneratorAsync<int>>>(123, std::chrono::milliseconds(2000));
    moe::Logger::debug("Preload value created");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    moe::Logger::debug("Waiting for preload to complete...");
    int value = preloadValue.generate().value();
    moe::Logger::debug("Preload value generated: {}", value);

    auto preloadSecureValue =
            moe::Preload<moe::Secure<moe::TestGeneratorAsync<int>>>(456, std::chrono::milliseconds(2000));
    moe::Logger::debug("Preload secure value created");
    value = preloadSecureValue.generate().value();
    moe::Logger::debug("Preload secure value generated: {}", value);
}

void testExpect() {
    auto okValue = moe::Ok<int>(100);
    if (okValue.isOk()) {
        moe::Logger::debug(
                "Ok value: {}, andThen {}",
                okValue.unwrap(),
                okValue.andThen([](int x) { return x * 2.0f; }).unwrap());
    }

    enum class SampleError {
        ErrorA,
        ErrorB
    };

    auto errValue = moe::Expected<int, SampleError>(moe::Err(SampleError::ErrorA));
    if (errValue.isErr()) {
        moe::Logger::debug(
                "Error: {}, orElse {}",
                static_cast<int>(errValue.unwrapErr()),
                errValue.orElse(
                                [](SampleError x) {
                                    return static_cast<int>(x) + 1;
                                })
                        .unwrapErr());
    }
}

void testScheduler() {
    auto fut1 =
            moe::async(
                    []() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        moe::Logger::debug("Task running in scheduler");
                        return 42;
                    })
                    .then([](int x) {
                        return 12;
                    });
    auto fut2 =
            moe::async([]() {
                moe::Logger::debug("Another task running in scheduler");
                return std::string("Hello, Scheduler!");
            });
    moe::whenAny(fut1, fut2)
            .then([](auto result) {
                std::visit(
                        [](auto&& value) {
                            moe::Logger::debug("whenAny result: {}", value);
                        },
                        result);
            })
            .get();

    auto& scheduler = moe::ThreadPoolScheduler::getInstance();
    moe::Vector<moe::Future<moe::Expected<int, moe::Infallible>, moe::ThreadPoolScheduler>> futures;
    for (int i = 0; i < 5; ++i) {
        futures.push_back(
                moe::async([i]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50 * (5 - i)));
                    moe::Logger::debug("Task {} completed", i);
                    return moe::Ok(i * 10);
                }));
    }

    moe::whenAll(std::move(futures))
            .then([](auto results) {
                moe::Logger::debug("All tasks completed with results:");
                for (const auto& r: moe::collectExpected(std::move(results)).unwrap()) {
                    moe::Logger::debug("Result: {}", r);
                }
            })
            .get();
}

void testWrappedFuture() {
    auto fut = moe::async([]() {
        return moe::async([]() {
            return 1;
        });
    });
    int result = fut.get();
    moe::Logger::debug("Wrapped future result: {}", result);
}

void testDispatchOnMainThread() {
    auto& mainThreadDispatcher = moe::MainScheduler::getInstance();
    auto taskSubmittedPromise = std::promise<void>();
    auto taskSubmittedFuture = taskSubmittedPromise.get_future();
    std::thread workerThread([&mainThreadDispatcher, prom = std::move(taskSubmittedPromise)]() mutable {
        moe::Logger::debug("Scheduling task on main thread from worker thread");
        auto fut = moe::asyncOnMainThread(
                           []() {
                               moe::Logger::debug("Task running on main thread");
                               return 12;
                           })
                           .then([](int x) { return 2 * x; });
        prom.set_value();

        int result = fut.get();
        moe::Logger::debug("Result from main thread task: {}", result);
    });
    taskSubmittedFuture.wait();
    mainThreadDispatcher.processTasks();
    workerThread.join();
}

void testSync() {
    struct SharedCounter
        : public moe::RefCounted<SharedCounter>,
          public moe::RwSynchronized<SharedCounter> {
        SharedCounter() : counter(0) {}

        void increment() {
            auto writeLock = this->write();
            ++counter;
        }

        int get() const {
            auto readLock = this->read();
            return counter;
        }

    private:
        int counter;
    };

    auto counter = moe::Ref(new SharedCounter());
    const int numThreads = 10;
    const int incrementsPerThread = 1000;
    moe::Vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([counter, incrementsPerThread]() mutable {
            for (int j = 0; j < incrementsPerThread; ++j) {
                counter->increment();
            }
        });
    }

    for (auto& t: threads) {
        t.join();
    }

    moe::Logger::debug("Final counter value: {}", counter->get());
}

void testSignal() {
    struct TestSlot : public moe::Slot<TestSlot> {
    public:
        explicit TestSlot(int id) : slotId(id) {}
        void signal(int value) {
            moe::Logger::debug("TestSlot invoked with value: {}, slot id: {}", value, slotId);
        }

    private:
        int slotId = 0;
    };

    auto testSlot = TestSlot(99);
    TestSlot other = std::move(testSlot);

    auto signal = moe::Ref(new moe::Signal<TestSlot>());
    auto conn1 = signal->connect(TestSlot(0));
    auto conn2 = signal->connect(TestSlot(1));
    {
        auto conn3 = signal->connect(TestSlot(2));
        signal->emit(123);
    }
    {
        signal->emit(456);
        conn1.disconnect();
    }
    signal->emit(42);

    struct TestSyncSlot : public moe::SyncSlot<TestSyncSlot> {
    public:
        explicit TestSyncSlot(int id) : slotId(id) {}
        void signal(int value) {
            moe::Logger::debug("TestSyncSlot invoked with value: {}", value);
        }

    private:
        int slotId = 0;
    };

    auto syncSignal = moe::Ref(new moe::SyncSignal<TestSyncSlot>());
    auto syncConn1 = syncSignal->connect(TestSyncSlot(10));
    auto syncConn2 = syncSignal->connect(TestSyncSlot(20));
    syncSignal->emit(789);
}

void test() {
    testRefcount();
    testLazy();
    testPreload();
    testExpect();
    testScheduler();
    testWrappedFuture();
    testDispatchOnMainThread();
    testSync();
    testSignal();
}*/

void testUILayout() {
    auto boxWidget = moe::Ref(new moe::BoxWidget());
    boxWidget->setMargin({10.0f, 10.0f, 10.0f, 10.0f});
    boxWidget->setPadding({5.0f, 5.0f, 5.0f, 5.0f});
    auto textWidget = moe::Ref(new moe::TextWidget());
    textWidget->setText(U"测试文本测试文本测试文本");
    textWidget->setMargin({5.0f, 5.0f, 0.0f, 0.0f});
    textWidget->setFontSize(24.0f);
    boxWidget->addChild(textWidget);
    boxWidget->layout({
            0,
            0,
            50,
            1000,
    });
    auto boxBounds = boxWidget->calculatedBounds();
    auto textBounds = textWidget->calculatedBounds();
}

int main() {
    moe::MainScheduler::getInstance().init();
    moe::ThreadPoolScheduler::getInstance().init();
    moe::FileReader::initReader(new moe::DebugFileReader<moe::DefaultFileReader>());

    testUILayout();

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

    //auto floorShape = new JPH::BoxShape({10.0f, 0.1f, 10.0f});
    auto floorShape = moe::GltfColliderFactory::shapeFromGltf("phy/complex-scene/scene.gltf");
    auto floorSettings = JPH::BodyCreationSettings(
            floorShape,
            JPH::RVec3(0.0f, 0.0f, 0.0f),
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

    auto& audioEngine = moe::AudioEngine::getInstance();
    auto audioInterface = audioEngine.getInterface();

    size_t outFileSize;
    auto oggBuffer = moe::Ref<moe::BinaryBuffer>(new moe::BinaryBuffer(moe::FileReader::s_instance->readFile("bgm.ogg", outFileSize).value()));
    auto oggProvider = moe::Ref(new moe::StreamedOggProvider(oggBuffer, 1024 * 16));
    auto audioSource = audioInterface.createAudioSource();
    audioInterface.loadAudioSource(audioSource, oggProvider, true);
    audioInterface.playAudioSource(audioSource);

    moe::VulkanEngine engine;
    engine.init();

    auto zhcnGlyphRangeGenerator = []() -> moe::String {
        moe::Vector<char32_t> ss;
        for (char32_t c = 0x0000; c <= 0x007F; ++c) {
            if (!std::isprint(static_cast<char>(c))) {
                continue;
            }
            ss.push_back(c);
        }
        for (char32_t c = 0x4E00; c <= 0x9FFF; ++c) {
            ss.push_back(c);
        }
        for (char32_t c = 0x3000; c <= 0x303F; ++c) {
            ss.push_back(c);
        }
        for (char32_t c = 0xFF00; c <= 0xFFEF; ++c) {
            ss.push_back(c);
        }
        return utf8::utf32to8(std::u32string(ss.begin(), ss.end()));
    };
    moe::String zhcnGlyphRange = zhcnGlyphRangeGenerator();

    moe::FontId defaultFontId = engine.getResourceLoader().load("fonts/NotoSansSC-Regular.ttf", 24.0f, moe::StringView(zhcnGlyphRange), moe::Loader::Font);
    auto* font = engine.m_caches.fontCache.getRaw(defaultFontId).value();

    auto& camera = engine.getDefaultCamera();
    auto& spriteCamera = engine.getDefaultSpriteCamera();
    auto& illuminationBus = engine.getBus<moe::VulkanIlluminationBus>();
    auto& renderBus = engine.getBus<moe::VulkanRenderObjectBus>();
    camera.setPosition(glm::vec3(0.0f, 0.0f, 3.0f));
    camera.setYaw(-90.0f);
    constexpr float cameraRotDampening = 0.1f;
    constexpr float cameraMoveSpeed = 5.f;
    CameraMovementMask movementMask;

    auto lastTime = std::chrono::high_resolution_clock::now();

    bool running = true;
    glm::vec3 cameraMovement = glm::vec3(0.0f);

    auto& inputBus = engine.getInputBus();
    inputBus.setMouseValid(false);

    bool isMouseValid = false;

    float elapsed = 0.0f;

    auto& loader = engine.getResourceLoader();

    auto sceneId = loader.load("vrm0/vroid.glb", moe::Loader::Gltf);

    auto scene = engine.m_caches.objectCache.get(sceneId).value();
    auto* animatableRenderable = scene->checkedAs<moe::VulkanSkeletalAnimation>(moe::VulkanRenderableFeature::HasSkeletalAnimation).value();
    auto& animations = animatableRenderable->getAnimations();
    auto usedAnimation = animations.begin()->second;

    float animationTime = 0.0f;
    int animationIndex = 0;

    auto plane = loader.load("phy/complex-scene/scene.gltf", moe::Loader::Gltf);
    auto sphere = loader.load("phy/sphere/sphere.gltf", moe::Loader::Gltf);

    auto spriteImageId = loader.load("test_sprite.jpg", moe::Loader::Image);
    glm::vec2 spriteTexOffset(0.0f, 0.0f);
    moe::String text;

    glm::vec3 gizmoTranslation(0.0f);
    float gizmoHeight = 50.0f;
    float gizmoSize = 5.0f;

    moe::Logger::debug("Starting main loop");
    while (running) {
        engine.beginFrame();
        auto now = std::chrono::high_resolution_clock::now();
        auto deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(now - lastTime).count();
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
                ImGui::SliderFloat("UI Camera Pos X", &spriteCamera.position.x, -5000.0f, 5000.0f);
                ImGui::SliderFloat("UI Camera Pos Y", &spriteCamera.position.y, -5000.0f, 5000.0f);
                ImGui::SliderFloat("UI Camera Zoom", &spriteCamera.zoom, 0.1f, 10.0f);

                ImGui::SliderFloat("Sprite Tex Offset X", &spriteTexOffset.x, -640.0f, 1920.0f - 640.0f);
                ImGui::SliderFloat("Sprite Tex Offset Y", &spriteTexOffset.y, -360.0f, 1080.0f - 360.0f);

                char textBuffer[256];
                std::strncpy(textBuffer, text.c_str(), sizeof(textBuffer));
                ImGui::InputText("Text Input", textBuffer, sizeof(textBuffer));
                text = moe::String(textBuffer);

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

                ImGui::Separator();
                ImGui::Text("Skeletal Animation");
                ImGui::SliderFloat("Animation Time", &animationTime, 0.0f, 5.0f);
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
                }

                ImGui::Separator();
                ImGui::Text("Im3d Gizmo");
                ImGui::SliderFloat("Gizmo Height", &gizmoHeight, 10.0f, 200.0f);
                ImGui::SliderFloat("Gizmo Size", &gizmoSize, 1.0f, 20.0f);

                ImGui::End();
            }
        });

        engine.addIm3dDrawCommand([&] {
            Im3d::PushDrawState();
            Im3d::SetSize(10.0f);
            Im3d::BeginLineLoop();
            {
                Im3d::Vertex(-1.0f, 1.0f, 0.0f, Im3d::Color_Magenta);
                Im3d::Vertex(1.0f, 1.0f, 0.0f, Im3d::Color_Yellow);
                Im3d::Vertex(0.0f, 1.0f, 2.0f, Im3d::Color_Cyan);
            }
            Im3d::End();
            Im3d::PopDrawState();
            Im3d::PushDrawState();
            Im3d::SetSize(5.0f);
            Im3d::SetColor(Im3d::Color_Red);
            Im3d::DrawSphere(Im3d::Vec3(0.0f, 0.0f, 0.0f), 1.0f);
            Im3d::PopDrawState();
            Im3d::PushDrawState();
            Im3d::GetContext().m_gizmoHeightPixels = gizmoHeight;
            Im3d::GetContext().m_gizmoSizePixels = gizmoSize;
            if (Im3d::Gizmo("gizmo", (float*) &gizmoTranslation, nullptr, nullptr)) {
            }
            Im3d::PopDrawState();
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

        auto computeHandle = renderBus.submitComputeSkin(sceneId, usedAnimation, animationTime);
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                objectTransform.setPosition(glm::vec3((i - 2) * 2.0f, 0.0f, (j - 2) * 2.0f));
                renderBus.submitRender(sceneId, objectTransform, computeHandle);
            }
        }

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
                    font->getFontImageId(24.0f),
                    moe::Transform{},
                    moe::Colors::White,
                    glm::vec2(3822.0f, 3769.0f),
                    glm::vec2(0.0f, 0.0f),
                    glm::vec2(3822.0f, 3769.f));

            renderBus.submitSpriteRender(
                    spriteImageId,
                    moe::Transform{},
                    moe::Colors::White,
                    glm::vec2(640.0f, 360.0f),
                    spriteTexOffset,
                    glm::vec2(1920.0f, 1080.0f));

            renderBus.submitTextSpriteRender(
                    defaultFontId,
                    24.0f,
                    text,
                    moe::Transform{},
                    moe::Colors::Blue);
            renderBus.submitTextSpriteRender(
                    defaultFontId,
                    32.0f,
                    text,
                    moe::Transform{}.setPosition(glm::vec3(0.0f, 50.0f, 0.0f)),
                    moe::Colors::Red);
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
    moe::ThreadPoolScheduler::shutdown();
    moe::FileReader::destroyReader();

    bodyInterface.RemoveBody(floorBody);
    bodyInterface.RemoveBodies(sphereBodies.data(), (JPH::uint) sphereBodies.size());
    bodyInterface.DestroyBody(floorBody);
    bodyInterface.DestroyBodies(sphereBodies.data(), (JPH::uint) sphereBodies.size());

    JPH::UnregisterTypes();

    delete JPH::Factory::sInstance;
    return 0;
}