#pragma once
#pragma once

#include "../Gameobjects/RigidBody.hpp"
#include "../Gameobjects/Scene.hpp"

#define NodeArray BulletNodeArray
#include <btBulletDynamicsCommon.h>
#undef NodeArray
#include <BulletDynamics/Character/btKinematicCharacterController.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <GLFW/glfw3.h>


using json = nlohmann::json;

class CharacterController : public Component
{
public:
    // =========================
    // ÄÀÍÍÛÅ ÊÎÌÏÎÍÅÍÒÀ
    // =========================

    GLFWwindow* window;

    btPairCachingGhostObject* ghostObject = nullptr;
    btKinematicCharacterController* controller = nullptr;
    Collider* collider = nullptr;

    float stepHeight = 0.35f;
    float speed = 5.0f;
    float jumpSpeed = 10.0f;
    float height = 4.f;
    bool physicsCreated = false;

    // =========================
    // ÊÎÍÑÒÐÓÊÒÎÐ
    // =========================

    CharacterController() = default;
    ~CharacterController() override = default;

    // =========================
    // ÆÈÇÍÅÍÍÛÉ ÖÈÊË
    // =========================

    void Update(float dt)
    {
        gameObject->scene->camera->position = gameObject->GetComponent<Transform>()->position+vec3(0,height,0);
        float cameraYaw = gameObject->scene->camera->rotationEuler.y;
        btQuaternion rot;
        rot.setEuler(glm::radians(cameraYaw), 0, 0); 

        btTransform worldTrans = ghostObject->getWorldTransform();

        worldTrans.setRotation(rot);
        ghostObject->setWorldTransform(worldTrans);

        glm::vec3 inputDir = GetInputDirection();
        
        glm::mat4 rotationMat = glm::rotate(glm::mat4(1.0f), glm::radians(cameraYaw), glm::vec3(0, 1, 0));
        inputDir = glm::vec3(rotationMat * glm::vec4(inputDir, 1.0f));

        Move(inputDir, dt);
      
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            Jump();

        SyncTransformFromPhysics();
    }

    void OnEnable() {
        window = gameObject->scene->window;
        collider = gameObject->GetComponent<Collider>();
        if (!collider) return;
        TryCreatePhysics();
    }
    void OnDisable() {
        if (!physicsCreated) return;
        if (auto scene = gameObject->scene) {
            if (ghostObject) scene->dynamicsWorld->removeCollisionObject(ghostObject);
            if (controller) scene->dynamicsWorld->removeAction(controller);
        }

        delete controller;
        delete ghostObject;
        
        controller = nullptr;
        ghostObject = nullptr;
        physicsCreated = false;
    }
    

    // =========================
    // ÑÅÐÈÀËÈÇÀÖÈß
    // =========================

    json Serialize() override
    {
        return {
            { "type", "CharacterController" },
            { "speed", speed },
            { "enabled", enabled },
            { "stepHeight", stepHeight },
            { "jumpSpeed", jumpSpeed },
            { "height", height }
        };
    }

    void Deserialize(const json& j) override
    {
        if (j.contains("speed"))   speed = j["speed"];
        if (j.contains("enabled")) enabled = j["enabled"];
        if (j.contains("height")) height = j["height"];
        if (j.contains("stepHeight")) stepHeight = j["stepHeight"];
        if (j.contains("height")) jumpSpeed = j["jumpSpeed"];

    }

    // =========================
    // INSPECTOR (ImGui)
    // =========================

    void drawInspector() override
    {
        if (ImGui::CollapsingHeader("CharacterController"))
        {
            ImGui::Checkbox("Enabled", &enabled);
            ImGui::DragFloat("speed", &speed, 0.1f);
            ImGui::DragFloat("jump Speed", &jumpSpeed, 0.1f);
            ImGui::DragFloat("Height", &height, 0.1f);
        }
    }

    void TryCreatePhysics() {
        if (physicsCreated || !collider) return;
        auto scene = gameObject->scene;
        if (!scene) return;
        btConvexShape* convexShape = static_cast<btConvexShape*>(collider->shape);
        ghostObject = new btPairCachingGhostObject();
        Transform* t = gameObject->GetComponent<Transform>();
        btTransform startTransform;
        startTransform.setIdentity();
        startTransform.setOrigin(btVector3(t->position.x, t->position.y, t->position.z));
        ghostObject->setWorldTransform(startTransform);
        ghostObject->setCollisionShape(convexShape);
        ghostObject->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);

        

        controller = new btKinematicCharacterController(ghostObject, convexShape, stepHeight);
        scene->dynamicsWorld->addCollisionObject(
            ghostObject,
            btBroadphaseProxy::CharacterFilter,
            btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter
        );
        scene->dynamicsWorld->addAction(controller);

        physicsCreated = true;
        controller->setGravity(btVector3(0,-20,0));
    }

    void Jump() {
        if (controller && controller->canJump())
            controller->jump(btVector3(0, jumpSpeed, 0));
    }

    void Move(const glm::vec3& direction, float dt) {
        if (!controller) return;
        btVector3 walkDir(direction.x * speed * dt , 0, direction.z * speed * dt);
        controller->setWalkDirection(walkDir);
    }

    vec3 GetInputDirection() 
    {
        vec3 moveDir = vec3(0);
       

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir.z -= 1;

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir.z += 1;
        
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir.x += 1;
        
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir.x -= 1;
        return moveDir;
    };

    void SyncTransformFromPhysics() {
        if (!ghostObject) return;
        btTransform trans = ghostObject->getWorldTransform();
        Transform* t = gameObject->GetComponent<Transform>();
        t->position = glm::vec3(trans.getOrigin().x(), trans.getOrigin().y(), trans.getOrigin().z());
        btQuaternion rot = trans.getRotation();
        t->qrotation = glm::quat(rot.getW(), rot.getX(), rot.getY(), rot.getZ());
    }

};
