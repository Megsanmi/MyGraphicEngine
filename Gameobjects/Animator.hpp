#pragma once
#include "scene.hpp"
#include <unordered_map>
#include <vector>


class Animator : public Component {
public:
    
    int animN = 0;
    float speed = 1.f;

    Animator();
    ~Animator();

    void UpdateAnim(float dt);
    void PlayAnimation(int index);

    void drawInspector()
    {
        if (ImGui::CollapsingHeader("Animator"))
        {
            ImGui::DragInt("Animation number: ", &animN, 0.1f);
            if (ImGui::Button("Select animation"))
            {
                PlayAnimation(animN);
            }
            ImGui::DragFloat("Tick per second: ", &speed, 0.005f);
        }
    }
    
    void OnEnable()
    {
        if (gameObject->GetComponent<MeshRenderer>()->model)
        {
            model = gameObject->GetComponent<MeshRenderer>()->model;
            finalBoneMatrices.resize(model->bones.size(), glm::mat4(1.0f));

        }
        else
        {
            cout << "no model for animation";
        };
      
    }

    json Serialize()
    {
        return{
            {"type","Animator"},
            {"animN",animN},
        };
    };

    void Deserialize(const json& j)
    {
        if(j.contains("animN"))animN = j["animN"];
        PlayAnimation(animN);
    }

    std::vector<glm::mat4> finalBoneMatrices;

    void setMat()
    {

        for (size_t i = 0; i < finalBoneMatrices.size(); i++) {
            gameObject->scene->GlobalShaderProgram->use();

            gameObject->scene->GlobalShaderProgram->setMatrix4(
                ("bones[" + std::to_string(i) + "]").c_str(),
                finalBoneMatrices[i]
            );

            gameObject->scene->ShadowShader->use();
            gameObject->scene->ShadowShader->setMatrix4(
                ("bones[" + std::to_string(i) + "]").c_str(),
                finalBoneMatrices[i]
            );
        }
        gameObject->scene->GlobalShaderProgram->use();
    }
private:
    // ===== DATA =====
    Model* model = nullptr;

    Animation* currentAnimation = nullptr;

    float animationTime = 0.0f;

    // ===== CORE =====
    void TraverseNode(int nodeIndex, const glm::mat4& parent);
    glm::mat4 InterpolateBone(const BoneAnimation& boneAnim);
};
