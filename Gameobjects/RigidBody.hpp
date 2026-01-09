#pragma once

#include "GameObject.hpp"
#include "scene.hpp"
#define NodeArray BulletNodeArray
#include <btBulletDynamicsCommon.h>
#undef NodeArray

enum class BodyType
{
    Static = 0,
    Dynamic = 1,
    Kinematic = 2
};

enum class ColliderType
{
    Box = 0,
    Sphere = 1,
    Capsule = 2
};
class Collider;

class RigidBody : public Component
{
public:
    btRigidBody* body = nullptr;
    Collider* collider = nullptr;
    btDefaultMotionState* motionState = nullptr;
    
    float mass = 1.0f;
    BodyType bodyType = BodyType::Dynamic;

    RigidBody(float m = 1.0f);

    void OnEnable() override;
    void OnDisable() override;
    
    json Serialize() override;
    void Deserialize(const json& j) override;

    void Update(float dt) override;

    void SyncTransformFromPhysics();
    void SyncTransformToPhysics();
    void ApplyBodyType();
    void OnColliderChanged();
    void SetMass(float newMass);

    void drawInspector();

private:
    const char* bodyTypeNames[3] = { "Static", "Dynamic", "Kinematic" };
};

class Collider : public Component
{
public:
    ColliderType type = ColliderType::Box;

    // Параметры формы
    glm::vec3 size = { 1, 1, 1 }; // Box
    float radius = 0.5f;        // Sphere / Capsule
    float height = 1.0f;        // Capsule

    btCollisionShape* shape = nullptr;

    void OnEnable() override;
    void OnDisable() override;

    json Serialize() override;
    void Deserialize(const json& j) override;
    void RebuildShape();
    void drawInspector();
};