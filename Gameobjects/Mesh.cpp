    #include "Mesh.hpp"
    #include <assimp/Importer.hpp>
    #include <assimp/scene.h>
    #include <assimp/postprocess.h>
    #include <cassert>
    #include <iostream>

    #define STB_IMAGE_IMPLEMENTATION
    #include <stb_image.h>
    #include <ext/matrix_transform.hpp>



    using namespace glm;

    Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture*> textures, mat4 t)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
        this->transform = t;

        setupMesh();
    }

    void Mesh::setupMesh()
    {
        glGenVertexArrays(1, &VAO) ;
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
            &indices[0], GL_STATIC_DRAW);

        // Координаты вершин
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        // Нормали вершин
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

        // Текстурные координаты вершин
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        // Tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));

        // Bitangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

        //Bones
        glEnableVertexAttribArray(5);
        glVertexAttribIPointer( 5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneID));
    
        // Bone weights 
        glEnableVertexAttribArray(6);
        glVertexAttribPointer( 6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, weights));
    
        glBindVertexArray(0);

    }

    void Mesh::Draw(Renderer::ShaderProgram& shader)
    {

        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr = 1;

    
        shader.use();

        shader.setVec3("solidColor", material.albedo);
        shader.setFloat("matMetallic", material.metallic);
        shader.setFloat("matRoughness", material.roughness);
        shader.setFloat("matAo",0);
        shader.setFloat("Alpha", material.alpha);
     
        shader.setBool("hasAlbedoMap", material.hasAlbedoMap);
        shader.setBool("hasNormalMap", material.hasNormalMap);
        shader.setBool("hasMetallicMap", material.hasMetallicMap);
        shader.setBool("hasRoughnessMap", material.hasRoughnessMap);
        shader.setBool("hasMaterialAlpha", material.hasMaterialAlpha);

        shader.setMatrix4("mesh_matrix", transform);

        for (unsigned int i = 0; i < textures.size(); i++)
        {
        
            glActiveTexture(GL_TEXTURE0 + textures[i]->id);
            std::string number;


            std::string name = textures[i]->type;
            if (name == "diffuseTexture")
                number = std::to_string(diffuseNr++);
            else if (name == "texture_specular")
                number = std::to_string(specularNr++);
            else if (name == "normalMap")
                number = std::to_string(normalNr++);
   
            shader.setInt(name.c_str(), textures[i]->id);
            glBindTexture(GL_TEXTURE_2D, textures[i]->id);
        
        
        }
        glActiveTexture(GL_TEXTURE0);

        // Отрисовываем меш
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0); 
    }

    void Mesh::SetUV(float u0, float v0, float u1, float v1)
    {
        vertices[0].TexCoords = { u0, v1 };
        vertices[1].TexCoords = { u1, v1 };
        vertices[2].TexCoords = { u1, v0 };
        vertices[3].TexCoords = { u0, v0 };

        UpdateVBO();
    }

    void Mesh::UpdateVBO()
    {
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            vertices.size() * sizeof(Vertex),
            vertices.data()
        );

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void Model::Draw(Renderer::ShaderProgram& shader)
    {
    

        for (unsigned int i = 0; i < meshes.size(); i++)
        
            meshes[i]->Draw(shader);
    }

    void Model::SetUV(float u0, float v0, float u1, float v1)
    {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i]->SetUV(u0, v0, u1, v1);
    }

    void Model::loadModel(std::string path)
    {
        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate |
            aiProcess_FlipUVs |
            aiProcess_GenSmoothNormals |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ValidateDataStructure);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
            return;
        }
        directory = path.substr(0, path.find_last_of('/'));

        m_GlobalInverseTransform = glm::inverse(ConvertMatrix(scene->mRootNode->mTransformation));

        ExtractSkeleton(scene->mRootNode, -1);
    
        processNode(scene->mRootNode, scene, mat4(1));

        BuildBoneHierarchy(scene->mRootNode, -1);

        LoadAnimationsFromAssimp(scene);

        /*std::cout << "Meshes loaded: " << meshes.size() << std::endl;
        for (auto& mesh : meshes)
            std::cout << "Vertices: " << mesh->vertices.size()
            << ", Indices: " << mesh->indices.size()
            << ", Textures: " << mesh->textures.size() << std::endl;*/

    
    }

    void Model::ExtractSkeleton(aiNode* node, int parentIndex) {
        SkeletonNode skelNode;
        skelNode.name = node->mName.C_Str();
        skelNode.parent = parentIndex;

        skelNode.bindLocal = ConvertMatrix(node->mTransformation);

        int currentIndex = (int)skeleton.nodes.size();
        skeleton.nodes.push_back(skelNode);

        if (parentIndex != -1) {
            skeleton.nodes[parentIndex].children.push_back(currentIndex);
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            ExtractSkeleton(node->mChildren[i], currentIndex);
        }
    }

    void Model::processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform)
    {
        glm::mat4 nodeTransform = ConvertMatrix(node->mTransformation);
        glm::mat4 globalTransform = parentTransform * nodeTransform;
        // Обрабатываем все меши (если они есть) у выбранного узла
    
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(move(processMesh(mesh, scene, globalTransform)));
        }

        // И проделываем то же самое для всех дочерних узлов
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene, globalTransform);
        }

    }

    void Model::LoadAnimationsFromAssimp(const aiScene* scene)
    {
        animations.clear(); 
        for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
            aiAnimation* aiAnim = scene->mAnimations[i];
            Animation anim;

            anim.duration = (float)aiAnim->mDuration;
            anim.ticksPerSecond = (aiAnim->mTicksPerSecond != 0) ? (float)aiAnim->mTicksPerSecond : 25.0f;

            for (unsigned int c = 0; c < aiAnim->mNumChannels; c++) {
                aiNodeAnim* channel = aiAnim->mChannels[c];
                BoneAnimation boneAnim;

                // 1. Грузим позиции (их может быть 2)
                for (unsigned int k = 0; k < channel->mNumPositionKeys; k++) {
                    boneAnim.positions.push_back({
                        (float)channel->mPositionKeys[k].mTime,
                        glm::vec3(channel->mPositionKeys[k].mValue.x, channel->mPositionKeys[k].mValue.y, channel->mPositionKeys[k].mValue.z)
                        });
                }

                // 2. Грузим вращения (их может быть 150)
                for (unsigned int k = 0; k < channel->mNumRotationKeys; k++) {
                    boneAnim.rotations.push_back({
                        (float)channel->mRotationKeys[k].mTime,
                        glm::quat(channel->mRotationKeys[k].mValue.w, channel->mRotationKeys[k].mValue.x, channel->mRotationKeys[k].mValue.y, channel->mRotationKeys[k].mValue.z)
                        });
                }

                // 3. Грузим масштабы
                for (unsigned int k = 0; k < channel->mNumScalingKeys; k++) {
                    boneAnim.scales.push_back({
                        (float)channel->mScalingKeys[k].mTime,
                        glm::vec3(channel->mScalingKeys[k].mValue.x, channel->mScalingKeys[k].mValue.y, channel->mScalingKeys[k].mValue.z)
                        });
                }

                anim.channels[channel->mNodeName.C_Str()] = boneAnim;
            }
            animations.push_back(anim);
        }
    }

    void Model::BuildBoneHierarchy(aiNode* node, int parentBone)
    {
        std::string name = node->mName.C_Str();

        int currentBone = parentBone;

        auto it = boneMap.find(name);
        if (it != boneMap.end()) {
            currentBone = it->second;
            bones[currentBone].parent = parentBone;
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            BuildBoneHierarchy(node->mChildren[i], currentBone);
        }
    }

    std::unique_ptr<Mesh> Model::processMesh(aiMesh* mesh, const aiScene* scene, mat4 transform)
    {
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture*> textures;
        vector<Bone> bones;
        int BoneCounter = 0;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;

            // Обрабатываем координаты вершин, нормали и текстурные координаты

            glm::vec3 vector;
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
        


            vertex.Position = vector;
        
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;

            vertex.Normal = vector;  

            if (mesh->mTextureCoords[0]) 
            {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;

            }
        
            else vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }
    
        for (uint b = 0; b < mesh->mNumBones; b++)
        {
            aiBone* bone = mesh->mBones[b];

            int boneID = GetOrCreateBoneIndex(bone);

            for (uint w = 0; w < bone->mNumWeights; w++) {
                aiVertexWeight vw = bone->mWeights[w];

                int vertexId = vw.mVertexId;
                float weight = vw.mWeight;


                for (int i = 0; i < 4; i++) {

                    if (vertices[vertexId].weights[i] == 0.0f) {

                        vertices[vertexId].boneID[i] = boneID;
                        vertices[vertexId].weights[i] = weight;
                        break;
                    }
                }
         
            }

        }
   
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
   
        for (auto& v : vertices) {
            v.Tangent = glm::vec3(0.0f);
            v.Bitangent = glm::vec3(0.0f);
        }

   
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            Vertex& v0 = vertices[indices[i]];
            Vertex& v1 = vertices[indices[i + 1]];
            Vertex& v2 = vertices[indices[i + 2]];

            glm::vec3 edge1 = v1.Position - v0.Position;
            glm::vec3 edge2 = v2.Position - v0.Position;

            glm::vec2 deltaUV1 = v1.TexCoords - v0.TexCoords;
            glm::vec2 deltaUV2 = v2.TexCoords - v0.TexCoords;

            float f = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;

            // Если UV треугольника «схлопнуты», f будет 0. 
            // В этом случае этот конкретный треугольник не даст вклада в тангент.
            if (std::abs(f) > 1e-6f) {
                f = 1.0f / f;

                glm::vec3 t;
                t.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
                t.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
                t.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

                glm::vec3 b;
                b.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
                b.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
                b.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

                v0.Tangent += t; v1.Tangent += t; v2.Tangent += t;
                v0.Bitangent += b; v1.Bitangent += b; v2.Bitangent += b;
            }
        }

   
        for (auto& v : vertices) {
            if (glm::length(v.Tangent) < 0.001f) {
            
                glm::vec3 c1 = glm::cross(v.Normal, glm::vec3(0.0, 0.0, 1.0));
                glm::vec3 c2 = glm::cross(v.Normal, glm::vec3(0.0, 1.0, 0.0));
                v.Tangent = (glm::length(c1) > glm::length(c2)) ? glm::normalize(c1) : glm::normalize(c2);
                v.Bitangent = glm::normalize(glm::cross(v.Normal, v.Tangent));
            }
            else {           
                v.Tangent = glm::normalize(v.Tangent - v.Normal * glm::dot(v.Normal, v.Tangent));

                v.Bitangent = glm::normalize(glm::cross(v.Normal, v.Tangent));
            }
        }
    
        Material meshMaterial;
        if (mesh->mMaterialIndex >= 0)
        {
        
            aiColor4D baseColor{ 1.0f, 1.0f, 1.0f, 1.0f };

            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            if(material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
            {
                std::vector<Texture*> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "diffuseTexture", scene);
                textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

            }
            if (material->GetTextureCount(aiTextureType_SPECULAR) > 0)
            {
                std::vector<Texture*> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "specularMap", scene);
                textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
            }

            if (material->GetTextureCount(aiTextureType_METALNESS) > 0)
            {
                auto metallic = loadMaterialTextures(material, aiTextureType_METALNESS, "metallicMap", scene);
                textures.insert(textures.end(), metallic.begin(), metallic.end());
            }

            if (material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) > 0)
            {
                auto roughness = loadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, "roughnessMap", scene);
                textures.insert(textures.end(), roughness.begin(), roughness.end());          
            }

            if (material->GetTextureCount(aiTextureType_NORMALS) > 0)
            {

                std::vector<Texture*> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "normalMap", scene);
                textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
            }

            if (material->GetTextureCount(aiTextureType_NORMAL_CAMERA) > 0)
            {
                std::vector<Texture*> normalMapsEmbended = loadMaterialTextures(material, aiTextureType_NORMAL_CAMERA, "normalMap", scene);
                textures.insert(textures.end(), normalMapsEmbended.begin(), normalMapsEmbended.end());
            }

            if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &baseColor))
            {
                vec4 color = vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
            }

            aiColor3D color(1.f, 1.f, 1.f);
            material->Get(AI_MATKEY_BASE_COLOR, color);

            meshMaterial.albedo = glm::vec3(color.r, color.g, color.b);
        

            if (material->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS)
            {
                meshMaterial.albedo = glm::vec3(color.r, color.g, color.b);
            }
            else if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
            {
                meshMaterial.albedo = glm::vec3(color.r, color.g, color.b);
            }

            // Metallic
            float metallic = 0.0f;
            if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS)
            {
                meshMaterial.metallic = metallic;
            }

            // Roughness
            float roughness = 1.0f;
            if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
            {
                meshMaterial.roughness = roughness;
            }

            float alpha = 1.0f;

            if (material->Get(AI_MATKEY_OPACITY, alpha) == AI_SUCCESS)
            {
                meshMaterial.alpha = alpha;
                meshMaterial.hasMaterialAlpha = true;
                if (alpha <1)
                    cout<<"alpha mat " << alpha<< "\n";
            }
            else
            {
                meshMaterial.hasMaterialAlpha = false;
            }

            // AO
            float ao = 1.0f;
            //if (material->Get(AI_MATKEY_AMBIENT_OCCLUSION, ao) == AI_SUCCESS)
            //{
            //    meshMaterial.ao = ao;
            //}

            // Проверяем наличие текстур
            meshMaterial.hasAlbedoMap = material->GetTextureCount(aiTextureType_DIFFUSE) > 0;
            meshMaterial.hasNormalMap = material->GetTextureCount(aiTextureType_NORMALS) > 0;
            meshMaterial.hasMetallicMap = material->GetTextureCount(aiTextureType_METALNESS) > 0;
            meshMaterial.hasRoughnessMap = material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) > 0;

        }
        else 
        {
            std::cout << "no materials \n";
        }
        for (auto* t : textures)
        {
            std::cout << "texture: ";
            cout << t->type;
            cout << "\n";
        }
        auto finalMesh = std::make_unique<Mesh>(vertices, indices, textures, transform);
        finalMesh->material = meshMaterial;
        return finalMesh;
    }


    unsigned int TextureFromEmbedded(aiTexture* tex) {

        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char* data;
        if (tex->mHeight == 0) {
            // PNG/JPEG
            data = stbi_load_from_memory((unsigned char*)tex->pcData, tex->mWidth, &width, &height, &nrComponents, 0);
        }
        else {
            // RAW image
            data = (unsigned char*)tex->pcData;
            width = tex->mWidth;
            height = tex->mHeight;
            nrComponents = 4;
        }
        if (data)
        {
            GLenum format = GL_RGB;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        }
        else
        {
            stbi_image_free(data);
        
        }
        return textureID;
    }

    std::vector<Texture*> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene)
    {
        std::vector<Texture*> textures;

        unsigned int count = mat->GetTextureCount(type);
        //if (count == 0 && typeName == "diffuseTexture")
        //    useSolidColor = false;

        for (unsigned int i = 0; i < count; i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            std::string pathStr = str.C_Str();

            // Проверяем кэш
            auto it = textureCache.find(pathStr);
            if (it != textureCache.end()) {
                textures.push_back(it->second);
                continue;
            }

            Texture* texture = new Texture();
            const aiTexture* embeddedTex = scene->GetEmbeddedTexture(str.C_Str());

            if (embeddedTex) {
                // Для embedded-текстур используем pointer в качестве уникального ключа
                auto embeddedKey = reinterpret_cast<const char*>(embeddedTex);
                auto it2 = textureCache.find(embeddedKey);
                //if (it2 != textureCache.end()) {
                //    delete texture; // уже есть объект
                //    textures.push_back(it2->second);
                //    continue;
                //}

            
                texture->id = TextureFromEmbedded(const_cast<aiTexture*>(embeddedTex));
                texture->type = typeName;
                texture->path = "embedded";
            
                std::cout << "Loading embedded texture: " << str.C_Str() << texture->type << std::endl;
            
                textureCache[embeddedKey] = texture;
            }
            else {
                texture->id = TextureFromFile(str.C_Str(), directory, false);
                texture->type = typeName;
                texture->path = pathStr;

                textureCache[pathStr] = texture;
            }

            textures.push_back(texture);
        }

        return textures;
    }

    unsigned int Model::TextureFromFile(const char* path, const std::string& directory, bool gamma)
    {
        std::string filename = std::string(path);
        filename = directory + '/' + filename;

        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
        if (!data)
        {
            std::cout << "Texture missing: " << filename << ", using default\n";
        }
        if (data)
        {
            GLenum format = GL_RGB;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


            stbi_image_free(data);
        }
        else
        {

            std::cout << "Texture failed to load at path: " << path << std::endl;

            stbi_image_free(data);
        }

        return textureID;
    }