#pragma once
#include <glm.hpp>
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

using namespace glm;
using namespace std;
using namespace nlohmann;

class GameObject; 


//Базовый класс для всез компонентов
//позволяет в одном векторе компонентов хранить любые типы этих компонентов
//является родителем для всех компонентов

class Component {
public:
    GameObject* gameObject = nullptr;
    virtual ~Component() {}
    virtual void Start() {}
    virtual void Update(float dt) {}
    virtual json Serialize() //функция сохранения состояния и настроек из компонента 
    {
        return {};
    }
  
    virtual void Deserialize(const json& j) {}  //их загрузка 
    virtual void drawInspector() {}
};


// Трансформ не переместить ниже т.к он добавляется в GameObject

class Transform : public Component {
public:

    glm::vec3 position{ 0,0,0 };
    glm::vec3 rotationEuler{ 0.f,1.f,0.f };
    glm::vec3 rotationAxis{ 0.f,1.f,0.f };
    glm::vec3 scale{ 1,1,1 };


    void drawInspector()
    {
        ImGui::DragFloat3("position", &position.x, 1.0f);
        ImGui::DragFloat3("rotationEuler", &rotationEuler.x, 1.0f);
        ImGui::DragFloat3("scale", &scale.x, 1.0f);

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
        
        model = glm::rotate(model, glm::radians(rotationEuler.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(rotationEuler.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(rotationEuler.z), glm::vec3(0, 0, 1));
        
        model = glm::scale(model, scale);


        return model;
    }
};



class GameObject {
public:
    string name = "GameObject";
    Transform* transform;
    vector<std::unique_ptr<Component>> components;

    GameObject(const std::string& name = "GameObject")
        : name(name)
    {

        transform = new Transform();
        transform->gameObject = this;  
        components.emplace_back(transform);
    }


    template<typename T, typename... Args>
    T* AddComponent(Args&&... args)
    {
        T* c = new T(forward<Args>(args)...);
        c->gameObject = this;
        components.emplace_back(c);
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
            c->Update(dt);
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
    std::unique_ptr<Model> model;
    std::string path;
    Renderer::ShaderProgram& shaderProgram;
    bool isShaded = true;

    MeshRenderer(const std::string& modelPath, Renderer::ShaderProgram& shader) : path(modelPath), shaderProgram(shader){
        model = std::make_unique<Model>(path.c_str());
    }

    void Update(float dt) {
        Draw(shaderProgram);
    }

    void drawInspector()
    {
        ImGui::Text("path", path.c_str());
        ImGui::Checkbox("isShaded", &isShaded);

    };

    json Serialize() override { 
        return{
            {"type","MeshRenderer"},
            {"path",path},
            {"isShaded",isShaded}
        };
    };

    void Deserialize(const json& j) override {
        std::string pathj = j["path"];
        path = pathj;
        model = std::make_unique<Model>(path.c_str());
        isShaded = j["isShaded"];
    }
    
    void Draw(Renderer::ShaderProgram& shaderProgram) {
        if (model) {
            
            shaderProgram.use();
            shaderProgram.setMatrix4("model_matrix", gameObject->transform->GetMatrix());  
            shaderProgram.setBool("isShaded", isShaded);
            model->Draw(shaderProgram);
        }
    }
};
