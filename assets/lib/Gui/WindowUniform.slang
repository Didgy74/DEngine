// dont_precompile

#language slang latest

module WindowUniform;

import Gui;
import Gfx.CameraUniform;
import Gfx.ObjectUniform;

namespace Gui {
    namespace Window {
        // The first two sets have to be object stuff...
        public static const int uniformDescrSetIndex = Gfx.Object.objectDataVulkanDescrSetIndex + 1;

        [[vk::binding(0, uniformDescrSetIndex)]]
        ConstantBuffer<WindowData> _uniformWindowData;

        public Orientation GetOrientation() { return _uniformWindowData.orientation; }

        public float2x2 GetOrientationMat() { return _uniformWindowData.OrientationMat(); }

        public int2 GetResolution() { return _uniformWindowData.resolution; }
        public int2 GetOrientedResolution() { return FlipForOrientation(_uniformWindowData.resolution); }

        public vector<T, 2> FlipForOrientation<T>(vector<T, 2> in) {
            let orient = GetOrientation();
            return (orient == Orientation::e90 || orient == Orientation::e270) ? in.yx : in;
        }

        public float2 ReorientPoint(float2 p) {
            let orient = GetOrientation();
            switch (orient) {
                case Orientation::e0: return p;
                case Orientation::e90: return { p.y, 1 - p.x };
                case Orientation::e180: return { 1 - p.x, 1 - p.y };
                case Orientation::e270: return { 1 - p.y, p.x };
                default: return {};
            }
        }

        public float4 TransformRectPointToFrag(int2 rectExtentPx, int2 rectPosPx, float2 inputPos) {
            let rectExtentNorm = float2(rectExtentPx) / Gui::Window::GetOrientedResolution();
            let rectPosNorm = float2(rectPosPx) / Gui::Window::GetOrientedResolution();

            var outPos = inputPos;

            outPos *= rectExtentNorm;
            outPos += rectPosNorm;
            // Normalize to screenspace
            outPos *= 2;
            outPos -= 1;
            // ?
            let orientMat = Gui::Window::GetOrientationMat();
            return float4(mul(orientMat, outPos), 0, 1);
        }
    }
}