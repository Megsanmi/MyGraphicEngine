#pragma once
#include "GameObject.hpp"
#include "scene.hpp"
#include <unordered_map>
#include <vector>


class Animator : public Component {
public:
    Animator(Model* model);

    void Update(float dt);
    void PlayAnimation(int index);

    std::vector<glm::mat4> finalBoneMatrices;

private:
    // ===== DATA =====
    Model* model = nullptr;
   

    
    Animation* currentAnimation = nullptr;

    float animationTime = 0.0f;

    // ===== CORE =====
    void TraverseNode(int nodeIndex, const glm::mat4& parent);
    glm::mat4 InterpolateBone(const BoneAnimation& boneAnim);
};
