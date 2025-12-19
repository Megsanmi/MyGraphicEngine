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
//#include "PerlinNoise.hpp"

using namespace glm;
using namespace std;
using namespace nlohmann;

class GameObject; 
class Scene;

//Базовый класс для всез компонентов
//позволяет в одном векторе компонентов хранить любые типы этих компонентов
//является родителем для всех компонентов
class Component {
public:
    GameObject* gameObject = nullptr;
    virtual ~Component() {}
    virtual void Start() {}
    virtual void OnEnable() {}
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
    glm::vec3 rotationEuler{ 0.f,0.f,0.f };
    //glm::vec3 rotationAxis{ 0.f,1.f,0.f };
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
            ImGui::DragFloat3("rotationEuler", &rotationEuler.x, 0.5f);
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

        model = glm::rotate(model, glm::radians(rotationEuler.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(rotationEuler.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(rotationEuler.z), glm::vec3(0, 0, 1));
        
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
        c->OnEnable();
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
    Model* model = nullptr;
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

    void Update(float dt) {
        Draw(shaderProgram);
    }



    void drawInspector()
    {
        if (ImGui::CollapsingHeader("MeshRenderer"))
        {
            ImGui::Text("path", path.c_str());
            ImGui::Checkbox("isShaded", &isShaded);
            ImGui::Checkbox("UseNormalMap", &UseNormalMap);
            ImGui::Checkbox("UseSolidColor", &UseSolidColor);
        }

    };

    json Serialize() override {
        return{
            {"type","MeshRenderer"},
            {"path",path},
            {"isShaded",isShaded},
            { "UseNormalMap",UseNormalMap
 }
        };
    };

    void Deserialize(const json& j) override {
        std::string pathj = j["path"];
        path = pathj;
        model =new Model(path.c_str());
        isShaded = j["isShaded"];
    }

    void Draw(Renderer::ShaderProgram& shaderProgram) {
        if (model) {

            shaderProgram.use();
            shaderProgram.setMatrix4("model_matrix", gameObject->transform->GetMatrix());
            shaderProgram.setBool("isShaded", isShaded);
            shaderProgram.setBool("UseNormalMap", UseNormalMap);
            shaderProgram.setBool("UseSolidColor", UseSolidColor);



            model->Draw(shaderProgram);
        }
    }
};

class Terrain : public Component
{
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    Model* model;
    int W;
    int H;
    int S;

    Renderer::ShaderProgram& shaderProgram;

    unsigned int VAO, VBO, EBO;

    Terrain(Renderer::ShaderProgram& shader) : shaderProgram(shader) {};

    Terrain(int width, int height, float scale, Renderer::ShaderProgram& shader)
        : shaderProgram(shader),
        W(width),
        H(height),
        S(scale)
    {
        generate_chunk(W, H, S);
    }

    json Serialize() override {
        return{
                {"type","Terrain"},
                {"width",W},
                {"height",H},
                {"scale",S}
        };
    };

    void Deserialize(const json& j) override {
        
        W = j["width"];
        H = j["height"];
        S = j["scale"];
        generate_chunk(W, H, S);
        if (gameObject->GetComponent<MeshRenderer>())
        {
            gameObject->GetComponent<MeshRenderer>()->model = model;
        }
        else  gameObject->AddComponent<MeshRenderer>(model, shaderProgram);
    }

    void OnEnable()
    {
       
    };

    void generate_chunk(int width, int height, float scale)
    {
        for (int z = 0; z <= height; ++z)
        {
            for (int x = 0; x <= width; ++x)
            {
                Vertex v;
                v.Position = glm::vec3(x * scale - scale * width / 2.0, heightFunction(x, z) * scale, z * scale - scale * height / 2.0);
                v.Normal = glm::vec3(0, 1, 0);
                v.TexCoords = glm::vec2((float)x / width, (float)z / height);
                v.Tangent = glm::vec3(1, 0, 0);
                v.Bitangent = glm::vec3(0, 0, 1);




                vertices.push_back(v);
            }
        }

        for (int z = 0; z < height; ++z)
        {
            for (int x = 0; x < width; ++x)
            {
                int i = z * (width + 1) + x;

                indices.push_back(i);
                indices.push_back(i + width + 1);
                indices.push_back(i + 1);

                indices.push_back(i + 1);
                indices.push_back(i + width + 1);
                indices.push_back(i + width + 2);
            }
        }

        for (size_t i = 0; i < indices.size(); i += 3)
        {
            Vertex& v0 = vertices[indices[i]];
            Vertex& v1 = vertices[indices[i + 1]];
            Vertex& v2 = vertices[indices[i + 2]];

            glm::vec3 edge1 = v1.Position - v0.Position;
            glm::vec3 edge2 = v2.Position - v0.Position;
            glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

            // усредняем нормали
            v0.Normal += normal;
            v1.Normal += normal;
            v2.Normal += normal;
        }
        



        Texture texture;
        texture.id = model->TextureFromFile("textures/sand.png", "assets", false);
        texture.type = "texture_diffuse";
        textures.push_back(texture);
        model = new Model(vertices, indices, textures);

        
    }

    float heightFunction(int x, int z)
    {
        return sinf(x * 0.2f) * cosf(z * 0.2f) * 2.0f +
            sinf(x * 0.1f) * 3.0f +
            cosf(z * 0.05f) * 5.0f;
    }




    void Update(float dt)
    {
        
    }
};
