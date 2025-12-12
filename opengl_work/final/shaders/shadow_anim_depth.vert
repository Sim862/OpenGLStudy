// shaders/shadow_depth.vs
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in ivec4 aTangent;
layout(location = 4) in ivec4 aBitangent;
layout(location = 5) in ivec4 aBoneIDs;
layout(location = 6) in vec4 aWeights;


uniform mat4 lightSpaceMatrix;
uniform mat4 model;
uniform mat4 finalBonesMatrices[100];

void main()

{    // 1. 본 스킨닝(애니메이션 적용)
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
    gl_Position = lightSpaceMatrix * model * skinnedPos;
}
