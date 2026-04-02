#include "Animator.hpp"
#include <ext.hpp>

Animator::Animator()
    
{
    
}

Animator::~Animator()
{

}

void Animator::UpdateAnim(float dt)
{

    if (!currentAnimation) return;

    animationTime += dt * currentAnimation->ticksPerSecond ;
    animationTime = fmod(animationTime, currentAnimation->duration);

    setMat();

    TraverseNode(model->skeleton.rootIndex, glm::mat4(1.0f));

}
void Animator::PlayAnimation(int index)
{
    if (!model)
    {
        cout << "no model";
        return;
    }
    if (index < 0 || index >= model->animations.size()) return;
    currentAnimation = &model->animations[index];
    animationTime = 0.0f;
}

void Animator::TraverseNode(int nodeIndex, const glm::mat4& parentTransform)
{
    const SkeletonNode& node = model->skeleton.nodes[nodeIndex];
    std::string nodeName = node.name;

    // По умолчанию берем локальную матрицу из узла (если нет анимации)
    glm::mat4 nodeTransform = node.bindLocal;

    // Если для этого узла есть анимационный канал — используем Sample
    if (currentAnimation && currentAnimation->channels.count(nodeName)) {
        nodeTransform = currentAnimation->channels[nodeName].Sample(animationTime);
    }

    glm::mat4 globalTransform = parentTransform * nodeTransform;

    // Если этот узел является костью — вычисляем финальную матрицу
    if (model->boneMap.count(nodeName)) {
        int boneID = model->boneMap[nodeName];
        finalBoneMatrices[boneID] = model->m_GlobalInverseTransform * globalTransform * model->bones[boneID].inverseBindPose;
    }

    for (int childIndex : node.children) {
        TraverseNode(childIndex, globalTransform);
    }
}
