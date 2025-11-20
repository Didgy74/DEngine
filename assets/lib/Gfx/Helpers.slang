// dont_precompile

#language slang latest

module Helpers;

namespace Gfx {
    public float4 TransformPos(column_major float4x4 mat, float3 input) {
        var temp = mul(mat, float4(input, 1.0));
	    temp.y = -temp.y;
        return temp;
    }
}
