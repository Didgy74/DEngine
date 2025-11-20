// dont_precompile

#language slang latest

module Object;

import Gfx.Camera;
import Gfx.Helpers;

namespace Gfx.Object {
    // The data we store for each object in the scene.
    public struct ObjectData {
	    public column_major float4x4 matrix;
    }
}