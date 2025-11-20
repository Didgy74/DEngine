// dont_precompile

#language slang latest

module Camera;

namespace Gfx.Camera {
    // The data we store for each camera in the scene.
    // TODO: might want another level of indirection for per-scene data as well.
    public struct CameraData {
	    public column_major float4x4 matrix;
    }
}
