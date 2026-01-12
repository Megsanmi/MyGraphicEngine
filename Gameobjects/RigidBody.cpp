#include "RigidBody.hpp"
#include "../Renderer/camera.hpp"

RigidBody::RigidBody(float m) : mass(m)
{
}

void RigidBody::OnEnable()
{
    TryCreatePhysics();
}

void RigidBody::OnDisable()
{
    if (!physicsCreated)
        return;

    Scene* scene = gameObject->scene;
    if (scene && body)
        scene->dynamicsWorld->removeRigidBody(body);

    delete body;
    delete motionState;

    body = nullptr;
    motionState = nullptr;
    physicsCreated = false;
}

void RigidBody::Update(float dt)
{
    
}

void RigidBody::SyncTransformFromPhysics()
{
    if (!body || !body->getMotionState()) return;
    if (!physicsCreated) return;

    btTransform trans;
    body->getMotionState()->getWorldTransform(trans);

    Transform* t = gameObject->GetComponent<Transform>();
    t->position = glm::vec3(
        trans.getOrigin().x(),
        trans.getOrigin().y(),
        trans.getOrigin().z()
    );

    btQuaternion rot = trans.getRotation();
    t->qrotation = glm::quat(rot.getW(), rot.getX(), rot.getY(), rot.getZ());
}

void RigidBody::SyncTransformToPhysics()
{
    if (!physicsCreated) return;

    if (!body) return;
    Transform* t = gameObject->GetComponent<Transform>();
    if (!t) return;

    btTransform trans;
    trans.setIdentity();
    trans.setOrigin(btVector3(t->position.x, t->position.y, t->position.z));

    btQuaternion rot(t->qrotation.x, t->qrotation.y, t->qrotation.z, t->qrotation.w);
    trans.setRotation(rot);

    
    if (body->getMotionState()) body->getMotionState()->setWorldTransform(trans);

    body->setWorldTransform(trans);
    

    if (collider->shape)
        collider->shape->setLocalScaling(btVector3(t->scale.x, t->scale.y, t->scale.z));
}

void RigidBody::TryCreatePhysics()
{
    if (physicsCreated) return;

    
    collider = gameObject->GetComponent<Collider>();
    if (!collider || !collider->shape)
        return; 
    
    Scene* scene = gameObject->scene;
    Transform* t = gameObject->GetComponent<Transform>();
    if (!scene || !t)
        return;

    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(t->position.x, t->position.y, t->position.z));

    motionState = new btDefaultMotionState(startTransform);

    btVector3 inertia(0, 0, 0);
    if (mass > 0)
        collider->shape->calculateLocalInertia(mass, inertia);

    btRigidBody::btRigidBodyConstructionInfo info(mass, motionState, collider->shape, inertia);
    body = new btRigidBody(info);

    ApplyBodyType();

    scene->dynamicsWorld->addRigidBody(body);
    
    physicsCreated = true;
}


void RigidBody::ApplyBodyType()
{
    if (!body) return;
    

    int flags = body->getCollisionFlags();
    flags &= ~(btCollisionObject::CF_STATIC_OBJECT | btCollisionObject::CF_KINEMATIC_OBJECT);

    switch (bodyType)
    {
    case BodyType::Static:
        flags |= btCollisionObject::CF_STATIC_OBJECT;
        body->setMassProps(0, btVector3(0, 0, 0));
        body->setActivationState(DISABLE_DEACTIVATION);
        break;

    case BodyType::Kinematic:
        flags |= btCollisionObject::CF_KINEMATIC_OBJECT;
        body->setMassProps(0, btVector3(0, 0, 0));
        body->setActivationState(DISABLE_DEACTIVATION);
        break;

    case BodyType::Dynamic:
    {
        btVector3 inertia(0, 0, 0);
        if (mass > 0 && collider->shape)
            collider->shape->calculateLocalInertia(mass, inertia);
        body->setMassProps(mass, inertia);
        body->setActivationState(ACTIVE_TAG);
    }
    break;
    }

    body->setCollisionFlags(flags);
}

void RigidBody::OnColliderChanged()
{
  
    if (!body || !collider || !collider->shape)
        return;

    body->setCollisionShape(collider->shape);

    if (bodyType == BodyType::Dynamic)
    {
        btVector3 inertia(0, 0, 0);
        if (mass > 0)
            collider->shape->calculateLocalInertia(mass, inertia);

        body->setMassProps(mass, inertia);
        body->updateInertiaTensor();
    }
}
void RigidBody::SetMass(float newMass)
{
    
    mass = newMass;
    if (bodyType == BodyType::Dynamic && body && collider->shape)
    {
        btVector3 inertia(0, 0, 0);
        if (mass > 0)
            collider->shape->calculateLocalInertia(mass, inertia);
        body->setMassProps(mass, inertia);
        body->updateInertiaTensor();
    }
}

json RigidBody::Serialize() 
{
    return{
        {"type","RigidBody"},
        {"mass",mass},
        {"bodyType",bodyType}
    };
};
void RigidBody::Deserialize(const json& j) {
    mass = j["mass"];
    bodyType = j["bodyType"];
    
    ApplyBodyType();
}

void RigidBody::drawInspector()
{
    if (ImGui::CollapsingHeader("RigidBody"))
    {
        int current = (int)bodyType;
        if (ImGui::Combo("Body Type", &current, bodyTypeNames, IM_ARRAYSIZE(bodyTypeNames)))
        {
            bodyType = (BodyType)current;
            ApplyBodyType();
        }

        if (bodyType == BodyType::Dynamic)
        {
            float m = mass;
            if (ImGui::DragFloat("Mass", &m, 0.1f, 0.0f, 1000.0f))
                SetMass(m);
        }
    }
}

void Collider::Update(float dt)
{
    if (drawColision)
    {
        gameObject->scene->GlobalShaderProgram.setVec3("solidColor", glm::vec3(0, 1, 0));
        gameObject->scene->GlobalShaderProgram.setMatrix4("model_matrix", glm::mat4(1));
        if (gameObject->GetComponent<RigidBody>())
        {
            btTransform trans = gameObject->GetComponent<RigidBody>()->body->getWorldTransform();
            gameObject->scene->dynamicsWorld->debugDrawObject(trans, gameObject->GetComponent<RigidBody>()->body->getCollisionShape(), btVector3(1, 0, 0));

            gameObject->scene->debugDrawer->render(gameObject->scene->camera->gameObject->GetComponent<Camera>()->get_projection_matrix(), gameObject->scene->camera->gameObject->GetComponent<Camera>()->get_view_matrix());
        }
        gameObject->scene->GlobalShaderProgram.setVec3("solidColor", glm::vec3(0.5, 0.5, 0.5));
    }

}

void Collider::OnEnable()
{
    RebuildShape();

    if (auto rb = gameObject->GetComponent<RigidBody>())
    {
        //rb->collider = this;
        rb->TryCreatePhysics();
        rb->OnColliderChanged();
    }
        
}

void Collider::OnDisable()
{
    delete shape;
    shape = nullptr;
}

json Collider::Serialize()
{
    return{
        {"type","Collider"},
        {"size",{size.x,size.y,size.z}},
        {"center",{center.x,center.y,center.z}},
        {"height",height},
        {"typeShape",type},
        {"radius",radius}
    };
};

void Collider::Deserialize(const json& j)
{
    size = glm::vec3(j["size"][0], j["size"][1], j["size"][2]);
    center = glm::vec3(j["center"][0], j["center"][1], j["center"][2]);
    height = j["height"];
    type = j["typeShape"];
    radius = j["radius"];

    RebuildShape();
}

void Collider::RebuildShape()
{
    
    if (shape) {
        delete shape;
    }

  
    switch (type)
    {
    case ColliderType::Box:
        shape = new btBoxShape(btVector3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f));
        break;
    case ColliderType::Sphere:
        shape = new btSphereShape(radius);
        break;
    case ColliderType::Capsule:
        shape = new btCapsuleShape(radius, height);
        break;
    }

  
    if (gameObject) {
        if (auto rb = gameObject->GetComponent<RigidBody>()) {
            rb->OnColliderChanged();
        }
    }
}


void Collider::drawInspector()
{
    if (!ImGui::CollapsingHeader("Collider")) return;
    ImGui::Checkbox("Draw collider", &drawColision);

    int current = (int)type;
    if (ImGui::Combo("Shape", &current, "Box\0Sphere\0Capsule\0"))
    {
        type = (ColliderType)current;
        RebuildShape();
    }

    switch (type)
    {
    case ColliderType::Box:
    {
        bool changed = false;
        changed |= ImGui::DragFloat3("Size", &size.x, 0.05f);
        //changed |= ImGui::DragFloat3("Offset", &center.x, 0.05f);
        if (changed)
            RebuildShape();
        break;
    }

    case ColliderType::Sphere:
    {
        bool changed = false;
        changed |= ImGui::DragFloat("Radius", &radius, 0.05f);
        //changed |= ImGui::DragFloat3("Offset", &center.x, 0.05f);
        if (changed)
            RebuildShape();
        break;
    }

    case ColliderType::Capsule:
    {
        bool changed = false;
        changed |= ImGui::DragFloat("Radius", &radius, 0.05f);
        changed |= ImGui::DragFloat("Height", &height, 0.05f);
        //changed |= ImGui::DragFloat3("Offset", &center.x, 0.05f);
        if (changed)
            RebuildShape();
        break;
    }

    }
}
