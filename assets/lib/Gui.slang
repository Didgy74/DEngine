// dont_precompile

#language slang latest

module Gui;

namespace Gui {

    // Needs to match DEngine::Gfx::WindowRotation
    // TODO: Include a text file directly?
    public enum Orientation {
        e0,
        e90,
        e180,
        e270,
    }

    public float2x2 OrientationToMat(Orientation orient) {
        switch(orient) {
            case Orientation::e0: return { 1, 0, 0, 1 };
            case Orientation::e90: return { 0, 1, -1, 0 };
            case Orientation::e180: return { -1, 0, 0, -1 };
            case Orientation::e270: return { 0, -1, 1, 0 };
	    }
	    return {};
    }

    // Should only be used as a uniform input.
    public struct WindowData {
        public Orientation orientation;
        public int2 resolution;

        public float2x2 OrientationMat() { return OrientationToMat(orientation); }
    }

    public struct RectCornerRadiuses<T> where T : IArithmetic {
        public T topLeft;
        public T botLeft;
        public T botRight;
        public T topRight;
        
        public static RectCornerRadiuses<T> From(vector<T, 4> in) {
            RectCornerRadiuses<T> temp = {};
            temp.topLeft = in.x;
            temp.botLeft = in.y;
            temp.botRight = in.z;
            temp.topRight = in.w;
            return temp;
        }
    }
    public typealias RectCornerRadiusesF = RectCornerRadiuses<float>;

    // TODO: Move Sdf functionality into separate module.
    namespace Sdf {
        // Returns 0 if on the border of the shape. Returns positive numbers if outside. 
        // Negative if inside.
        //
        // p is position relative to rectangle center.
        // halfSize
        // Assumes Y down
        public float RoundedRect(float2 p, float2 halfSize, RectCornerRadiusesF radiuses) {
            float radius = 0;
            // Check the quadrant
            if (p.x < 0 && p.y < 0) {
                radius = radiuses.topLeft;
            } else if (p.x >= 0 && p.y < 0) {
                radius = radiuses.topRight;
            } else if (p.x >= 0 && p.y >= 0) {
                radius = radiuses.botRight;
            } else {
                radius = radiuses.botLeft;
            }

            float2 dist = abs(p) - halfSize + radius;
            return min(max(dist.x, dist.y), 0.0) + length(max(dist, 0.0)) - radius;
        }

        public float RoundedRect(float2 p, float2 halfSize, float radius) {
            RectCornerRadiusesF temp = {};
            temp.topLeft = radius;
            temp.topRight = radius;
            temp.botRight = radius;
            temp.botLeft = radius;
            return RoundedRect(p, halfSize, temp);
        }
    }
}