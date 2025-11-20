// dont_precompile

#language slang latest

module CameraUniform;

import Gfx.Camera;

namespace Gfx.Camera {
    public static const let cameraDataVulkanDescrSetIndex = 0;
    [[vk::binding(0, cameraDataVulkanDescrSetIndex)]]
    ConstantBuffer<CameraData> uniform_cameraData;

    public float4x4 GetMatrix() { return uniform_cameraData.matrix; }

    public float4x4 MakeMvp(column_major float4x4 input) { return mul(Camera::GetMatrix(), input); }
}

