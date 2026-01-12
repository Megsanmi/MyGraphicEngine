#pragma once
#include <glm.hpp>

#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp> 
#include <ext/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <memory> 
#include "Mesh.hpp"
#include "json.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "../Renderer/ShadowMap.hpp"

#include "PerlinNoise.hpp"

using namespace glm;
using namespace std;
using namespace nlohmann;

class GameObject; 
class Scene;

//Базовый класс для всех компонентов
//позволяет в одном векторе компонентов хранить любые типы этих компонентов
//является родителем для всех компонентов

class Component {
public:
    bool enabled = true;
    bool lastEnabled = true;

    GameObject* gameObject = nullptr;
    virtual ~Component() {}
    virtual void Start() {}
    virtual void OnEnable() {}
    virtual void OnDisable() {}
    virtual void Update(float dt) {}
    virtual json Serialize() { return {}; }
    virtual void Deserialize(const json& j) {}  //их загрузка 
    virtual void drawInspector() {}
    void syncEnabledState() {
        if (enabled != lastEnabled)
        {
            if (enabled) OnEnable();
            else OnDisable();

            lastEnabled = enabled;
        }
    }

    struct ComponentFactory {
        std::string name; // имя для UI
        std::function<Component* (GameObject*)> createFunc; // функция создания компонента
    };

};


// Трансформ не переместить ниже т.к он добавляется в GameObject

class Transform : public Component {
public:

    glm::vec3 position{ 0,0,0 };
    glm::vec3 rotationEuler{ 0.f,0.f,0.f };
    glm::vec3 scale{ 1,1,1 };
    glm::quat qrotation{1,0,0,0};



    void Update()
    {
        
    };
    void drawInspector()
    {
        if (ImGui::CollapsingHeader("Transform")) 
        {
            ImGui::DragFloat3("position", &position.x, 0.1f);
            if (ImGui::DragFloat3("rotationEuler", &rotationEuler.x, 0.5f))
            {
                qrotation = glm::normalize(glm::quat(glm::radians(rotationEuler)));
                
            }
            ImGui::DragFloat3("scale", &scale.x, 0.01f);
        }
    };

    
    json Serialize() override {
        return{
            {"type","Transform"},
            {"position",{position.x,position.y,position.z}},
            {"rotationEuler",{rotationEuler.x,rotationEuler.y,rotationEuler.z}},
            {"scale",{scale.x,scale.y,scale.z}}
        };
    };


    void Deserialize(const json& j) override {
        position = glm::vec3(j["position"][0], j["position"][1], j["position"][2]);
        rotationEuler = glm::vec3(j["rotationEuler"][0], j["rotationEuler"][1], j["rotationEuler"][2]);
        scale = glm::vec3(j["scale"][0], j["scale"][1], j["scale"][2]);
    }

    mat4 GetMatrix() {
        
        glm::mat4 model(1.0f);

        model = glm::translate(model, position);
        
        model *= glm::toMat4(qrotation);
        
        model = glm::scale(model, scale);


        return model;
    }
};



class GameObject {
public:

    Scene* scene = nullptr;
    GameObject* parent = nullptr;
    string name = "GameObject";
    Transform* transform;
    vector<std::unique_ptr<Component>> components;
    std::vector<GameObject*> children;

    GameObject(const std::string& name = "GameObject")
        : name(name)
    {

        transform = new Transform();
        transform->gameObject = this;  
        components.emplace_back(transform);
    }


    glm::mat4 GetWorldMatrix() const {
        if (parent)
            return parent->GetWorldMatrix() * transform->GetMatrix();
        else
            return transform->GetMatrix();
    }

    void AddChild(GameObject* child) {
        child->parent = this;
        children.push_back(child);
    }

    template<typename T, typename... Args>
    T* AddComponent(Args&&... args)
    {
        T* c = new T(forward<Args>(args)...);

        c->gameObject = this;

        components.emplace_back(c);
        if (c->enabled) {
            c->OnEnable();
        }
        return c;
    }


    template<typename T>
    T* GetComponent() {
        for (auto& c : components)
            if (auto t = dynamic_cast<T*>(c.get()))
                return t;
        return nullptr;
    }

    void Update(float dt) {
        for (auto& c : components)
        {
            c->syncEnabledState();

            if (c->enabled) c->Update(dt);
        }
    }

    void drawInspector()
    {
        for (auto& c : components)
            c->drawInspector();
    }

};

//******************      КОМПОНЕНТЫ      **********************



class MeshRenderer : public Component {
public:
  

    Model* model = nullptr;
    vec3 meshOffset = vec3{ 0,0,0 };
    std::string path;
    Renderer::ShaderProgram& shaderProgram;

    bool UseNormalMap = false;
    bool isShaded = true;
    bool UseSolidColor = false;


    MeshRenderer(const std::string& modelPath, Renderer::ShaderProgram& shader) : path(modelPath), shaderProgram(shader) {
        model = new Model(path.c_str());
    }

    MeshRenderer(Model* m, Renderer::ShaderProgram& shader)
        : shaderProgram(shader)
    {
        this->model = m;
    }
    
    void SetUV(float u0, float v0, float u1, float v1)
    {
        model->SetUV(u0, v0, u1, v1);
    }

    void OnEnable() {};

    void OnDisable() {};

    void Update(float dt) {
        Draw(shaderProgram);
    }


    void drawInspector()
    {
        if (ImGui::CollapsingHeader("MeshRenderer"))
        {
            ImGui::Checkbox("enabled", &enabled);
            ImGui::Text("path", path.c_str());
            ImGui::Checkbox("isShaded", &isShaded);
            ImGui::DragFloat3("Offset", &meshOffset.x,0.05);
            ImGui::Checkbox("UseNormalMap", &UseNormalMap);
            ImGui::Checkbox("UseSolidColor", &UseSolidColor);
        }

    };

    json Serialize() override {
        return{
            {"type","MeshRenderer"},
            {"path",path},
            {"isShaded",isShaded},
            {"meshOffset",{meshOffset.x,meshOffset.y,meshOffset.z} },
            { "UseNormalMap",UseNormalMap}
        };
    };

    void Deserialize(const json& j) override {
        std::string pathj = j["path"];
        path = pathj;
        meshOffset = glm::vec3(j["meshOffset"][0], j["meshOffset"][1], j["meshOffset"][2]);
        //model =new Model(path.c_str());
        isShaded = j["isShaded"];
    }

    void Draw(Renderer::ShaderProgram& shaderProgram) {
        if (model) {

            shaderProgram.use();

            mat4 modelMatrix = glm::translate(gameObject->transform->GetMatrix(), meshOffset);

            shaderProgram.setMatrix4("model_matrix", modelMatrix);
            shaderProgram.setBool("isShaded", isShaded);
            shaderProgram.setBool("UseNormalMap", UseNormalMap);
            shaderProgram.setBool("UseSolidColor", UseSolidColor);



            model->Draw(shaderProgram);
        }
    }
};

