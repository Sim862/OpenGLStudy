#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in ivec4 aTangent;
layout(location = 4) in ivec4 aBitangent;
layout(location = 5) in ivec4 aBoneIDs;
layout(location = 6) in vec4 aWeights;

out VS_OUT{
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform mat4 finalBonesMatrices[100];

void main()
{
    // 1. 본 스킨닝(애니메이션 적용)
    vec4 skinnedPos = vec4(0.0);
    for (int i = 0; i < 4; ++i)
    {
        int   boneID = aBoneIDs[i];
        float weight = aWeights[i];
        if (weight > 0.0)
        {
            mat4 boneMatrix = finalBonesMatrices[boneID];
            skinnedPos += boneMatrix * vec4(aPos, 1.0) * weight;
        }
    }

    // 월드 좌표계로 변환
    vec4 worldPos = model * skinnedPos;
    vs_out.FragPos = vec3(worldPos);

    // 노멀(간단히 model에서만 변환; 필요하면 본 스킨닝 노멀도 구현 가능)
    vs_out.Normal = mat3(transpose(inverse(model))) * aNormal;

    // UV 그대로 전달
    vs_out.TexCoords = aTexCoords;

    // 쉐도우
    vs_out.FragPosLightSpace = lightSpaceMatrix * worldPos;

    // MVP 적용
    gl_Position = projection * view * worldPos;
}
