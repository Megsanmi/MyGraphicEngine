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
    
    bool physicsCreated = false;

    float mass = 1.0f;
    BodyType bodyType = BodyType::Dynamic;

    RigidBody(float m = 1.0f);

    void OnEnable() override;
    void OnDisable() override;
    
    json Serialize() override;
    void Deserialize(const json& j) override;

    void Update(float dt) override;

    void drawInspector();
    
    void SyncTransformFromPhysics();
    
    void SyncTransformToPhysics();

    void TryCreatePhysics();
    
    void ApplyBodyType();
    
    void OnColliderChanged();

    void SetMass(float newMass);

private:
    const char* bodyTypeNames[3] = { "Static", "Dynamic", "Kinematic" };
};

class Collider : public Component
{
public:
    ColliderType type = ColliderType::Box;
    vec3 center = { 0,0,0 };

    glm::vec3 size = { 1, 1, 1 }; 
    float radius = 0.5f;        
    float height = 1.0f;        

    bool drawColision = false;
    
    btCollisionShape* shape = nullptr;
    void Update(float dt);
    void OnEnable() override;
    void OnDisable() override;

    json Serialize() override;
    void Deserialize(const json& j) override;
    void RebuildShape();
    void drawInspector();
};