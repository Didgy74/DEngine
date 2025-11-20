// dont_precompile

#language slang latest

module ObjectUniform;

import Gfx.CameraUniform;
import Gfx.Object;

namespace Gfx.Object {
    public static const let objectDataVulkanDescrSetIndex = Gfx.Camera.cameraDataVulkanDescrSetIndex + 1;
    [[vk::binding(0, objectDataVulkanDescrSetIndex)]]
    ConstantBuffer<ObjectData> uniform_objectData;

    public float4x4 GetMatrix() { return uniform_objectData.matrix; }

    public float4x4 GetMvp() { return Camera::MakeMvp(uniform_objectData.matrix); }

    public float4 TransformPos(float3 input) {
        var temp = mul(GetMvp(), float4(input, 1.0));
	    temp.y = -temp.y;
        return temp;
    }
}