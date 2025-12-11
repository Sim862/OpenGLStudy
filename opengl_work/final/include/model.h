#ifndef MODEL_H
#define MODEL_H

#include <filesystem>
#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <mesh.h>
#include <shader_m.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

struct BoneInfo {
    int id;
    glm::mat4 offset;
};


unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);
unsigned int TextureFromMemory(const aiTexture* texture, bool gamma = false);
string ExtractFilename(const string& path);

glm::mat4 ConvertMatrix(const aiMatrix4x4& m) {
    glm::mat4 r;
    r[0][0] = m.a1; r[1][0] = m.a2; r[2][0] = m.a3; r[3][0] = m.a4;
    r[0][1] = m.b1; r[1][1] = m.b2; r[2][1] = m.b3; r[3][1] = m.b4;
    r[0][2] = m.c1; r[1][2] = m.c2; r[2][2] = m.c3; r[3][2] = m.c4;
    r[0][3] = m.d1; r[1][3] = m.d2; r[2][3] = m.d3; r[3][3] = m.d4;
    return r;
}


class Model
{
public:
    std::map<std::string, BoneInfo> m_BoneInfoMap;
    int m_BoneCount = 0;
    glm::mat4 m_GlobalInverseTransform;


    vector<Texture> textures_loaded;
    vector<Mesh>    meshes;
    string directory;
    bool gammaCorrection;
    const aiScene* scene;  // scene 포인터 저장

    Model(string const& path, bool gamma = false) : gammaCorrection(gamma), scene(nullptr)
    {
        loadModel(path);
    }

    void Draw(Shader& shader)
    {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

private:
    //-------------------------
    void SetVertexBoneData(Vertex& vertex, int boneID, float weight)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
            if (vertex.m_Weights[i] == 0.0f) {
                vertex.m_BoneIDs[i] = boneID;
                vertex.m_Weights[i] = weight;
                return;
            }
        }
        // 이미 4개 꽉 찬 경우는 일단 무시 (나중에 정교하게 바꿀 수 있음)
    }

    void ExtractBoneWeightsForMesh(aiMesh* mesh, std::vector<Vertex>& vertices)
    {
        for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++) {
            aiBone* aiBonePtr = mesh->mBones[boneIndex];
            std::string boneName = aiBonePtr->mName.C_Str();

            int boneID = -1;
            auto it = m_BoneInfoMap.find(boneName);
            if (it == m_BoneInfoMap.end()) {
                BoneInfo info;
                info.id = m_BoneCount;
                info.offset = ConvertMatrix(aiBonePtr->mOffsetMatrix);
                m_BoneInfoMap[boneName] = info;
                boneID = m_BoneCount;
                m_BoneCount++;
            }
            else {
                boneID = it->second.id;
            }

            // 이 본이 영향을 주는 정점들에 가중치 기록
            for (unsigned int weightIndex = 0; weightIndex < aiBonePtr->mNumWeights; weightIndex++) {
                int vertexID = aiBonePtr->mWeights[weightIndex].mVertexId;
                float weight = aiBonePtr->mWeights[weightIndex].mWeight;
                SetVertexBoneData(vertices[vertexID], boneID, weight);
            }
        }

        std::cout << "Mesh bones: " << mesh->mNumBones
            << ", total boneCount: " << m_BoneCount << std::endl;
    }

    //-------------------------------
    Assimp::Importer importer;  // 클래스 멤버로 이동 (scene 수명 유지)

    void loadModel(string const& path)
    {
        scene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            //aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace);
        

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }

        m_GlobalInverseTransform =
            glm::inverse(ConvertMatrix(scene->mRootNode->mTransformation));

        directory = path.substr(0, path.find_last_of('/'));

        // 임베디드 텍스처 정보 출력
        cout << "Embedded textures: " << scene->mNumTextures << endl;

        processNode(scene->mRootNode, scene);
    }

    void processNode(aiNode* node, const aiScene* scene)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector;

            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }

            if (mesh->mTextureCoords[0])
            {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;

                if (mesh->mTangents)
                {
                    vector.x = mesh->mTangents[i].x;
                    vector.y = mesh->mTangents[i].y;
                    vector.z = mesh->mTangents[i].z;
                    vertex.Tangent = vector;
                }
                if (mesh->mBitangents)
                {
                    vector.x = mesh->mBitangents[i].x;
                    vector.y = mesh->mBitangents[i].y;
                    vector.z = mesh->mBitangents[i].z;
                    vertex.Bitangent = vector;
                }
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }
        //-----------------------------------------
        // 
            // 본 ID/가중치 초기화
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            for (int j = 0; j < MAX_BONE_INFLUENCE; j++) {
                vertices[i].m_BoneIDs[j] = 0;
                vertices[i].m_Weights[j] = 0.0f;
            }
        }

        // 새로 추가: 본 데이터 추출
        ExtractBoneWeightsForMesh(mesh, vertices);

        //-----------------------------------------



        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", scene);
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal", scene);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height", scene);
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

        return Mesh(vertices, indices, textures);
    }

    // 수정된 loadMaterialTextures - scene 파라미터 추가
    vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName, const aiScene* scene)
    {
        vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);

            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }

            if (!skip)
            {
                Texture texture;

                // 1) 임베디드 텍스처 먼저 확인
                const aiTexture* embeddedTex = scene->GetEmbeddedTexture(str.C_Str());

                if (embeddedTex)
                {
                    // 임베디드 텍스처 로드
                    cout << "Loading embedded texture: " << str.C_Str() << endl;
                    texture.id = TextureFromMemory(embeddedTex, gammaCorrection);
                }
                else
                {
                    // 2) 외부 파일에서 로드 (파일명만 추출)
                    string filename = ExtractFilename(str.C_Str());
                    cout << "Loading external texture: " << filename << endl;
                    texture.id = TextureFromFile(filename.c_str(), this->directory, gammaCorrection);
                }

                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        return textures;
    }
};

// 파일명만 추출하는 함수
string ExtractFilename(const string& path)
{
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != string::npos)
        return path.substr(lastSlash + 1);
    return path;
}

// 임베디드 텍스처 로드 함수 (추가)
unsigned int TextureFromMemory(const aiTexture* texture, bool gamma)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = nullptr;

    if (texture->mHeight == 0)
    {
        // 압축된 포맷 (PNG, JPG 등) - mWidth가 데이터 크기(바이트)
        data = stbi_load_from_memory(
            reinterpret_cast<unsigned char*>(texture->pcData),
            texture->mWidth,
            &width, &height, &nrComponents, 0
        );
    }
    else
    {
        // 비압축 ARGB8888 포맷
        width = texture->mWidth;
        height = texture->mHeight;
        nrComponents = 4;
        data = reinterpret_cast<unsigned char*>(texture->pcData);
    }

    if (data)
    {
        GLenum format;
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

        // 압축 포맷일 때만 메모리 해제
        if (texture->mHeight == 0)
            stbi_image_free(data);
    }
    else
    {
        cout << "Embedded texture failed to load" << endl;
    }

    return textureID;
}

// 외부 파일 텍스처 로드 (여러 경로 시도)
unsigned int TextureFromFile(const char* path, const string& directory, bool gamma)
{
    string filename = ExtractFilename(string(path));

    // 시도할 경로 목록
    vector<string> searchPaths = {
        directory + '/' + filename,                      // 모델과 같은 폴더
        directory + '/' + string(path),                  // 원본 상대경로
        filename                                          // 현재 폴더
    };

    // .fbm 폴더도 검색 (FBX 텍스처 폴더)
    size_t lastSlash = directory.find_last_of("/\\");
    if (lastSlash != string::npos)
    {
        string parentDir = directory.substr(0, lastSlash);
        searchPaths.push_back(parentDir + '/' + filename);
    }

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = nullptr;

    // 각 경로 시도
    for (const auto& tryPath : searchPaths)
    {
        data = stbi_load(tryPath.c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            cout << "Texture loaded from: " << tryPath << endl;
            break;
        }
    }

    if (data)
    {
        GLenum format;
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
        cout << "Texture failed to load: " << filename << endl;
    }

    return textureID;
}

#endif
