#version 450 core
#include "Uniforms.glsl"

// Compute the SDF for a rounded rectangle with a specified center.
// Calculate signed distance from point to rectangle edges
float roundedRectSDF(vec2 p, vec2 size, float radius) {
    vec2 dist = abs(p) - size + radius;
    return min(max(dist.x, dist.y), 0.0) + length(max(dist, 0.0)) - radius;
}

// Compute the SDF for a rounded rectangle with a specified center.
// Calculate signed distance from point to rectangle edges
float roundedRectSDF2(vec2 p, vec2 size, vec4 radiusAll) {
    float radius = 0;
    // Check the quadrant
    if (p.x < 0 && p.y < 0) {
        radius = radiusAll[0];
    } else if (p.x < 0 && p.y >= 0) {
        radius = radiusAll[1];
    } else if (p.x >= 0 && p.y >= 0) {
        radius = radiusAll[2];
    } else {
        radius = radiusAll[3];
    }

    vec2 dist = abs(p) - size + radius;
    return min(max(dist.x, dist.y), 0.0) + length(max(dist, 0.0)) - radius;
}

float rectSDF(vec2 p, vec2 boxSize) {
    vec2 d = abs(p) - boxSize;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float test(vec2 p, vec2 rectTopLeft, vec2 rectSize, vec4 radius) {
    float topLeftRadius = radius[0];
    float bottomLeftRadius = radius[1];
    float bottomRightRadius = radius[2];
    float topRightRadius = radius[3];

    vec2 startPoint = rectTopLeft;
    vec2 endPoint = rectTopLeft + rectSize;
    vec2 topLeftPoint = startPoint + vec2(topLeftRadius);
    vec2 bottomLeftPoint = startPoint + vec2(bottomLeftRadius, rectSize.y - bottomLeftRadius);
    vec2 bottomRightPoint = endPoint - vec2(bottomRightRadius);
    vec2 topRightPoint = startPoint + vec2(rectSize.x - topRightRadius, topRightRadius);
    if (p.x < topLeftPoint.x && p.y < topLeftPoint.y) {
        return 1 - length(p - topLeftPoint) / topLeftRadius;
    } else if (p.x < bottomLeftPoint.x && p.y >= bottomLeftPoint.y) {
        return 1 - length(p - bottomLeftPoint) / bottomLeftRadius;
    } else if (p.x >= bottomRightPoint.x && p.y >= bottomRightPoint.y) {
        return 1 - length(p - bottomRightPoint) / bottomRightRadius;
    } else if (p.x >= topRightPoint.x && p.y < topRightPoint.y) {
        return 1 - length(p - topRightPoint) / topRightRadius;
    }

    if (p.x > startPoint.x && p.x <= endPoint.x && p.y > startPoint.y && p.y <= endPoint.y) {
        return 1.0;
    } else {
        return 0.0;
    }
}

// Range [0, 1]
layout(location = 0) in vec2 in_trianglePos;
layout(location = 1) in vec2 rectExtentIn;
layout(location = 2) in vec2 rectOffsetIn;
layout(location = 3) in vec2 in_startPoint;
layout(location = 4) in vec2 in_endPoint;

layout(location = 0) out vec4 outColor;

void main()
{
    int orientation = perWindowUniform.orientation;
    mat2 orientMat = OrientationToMat2(orientation);
    vec2 resolution = vec2(perWindowUniform.resolution);
    float aspect = resolution.x / resolution.y;

    // This gives position in terms of [0, 1].
    // Remember that these [0, 1] ranges are physically different
    // We have to set it so that Y has the range [0, 1]
    // and then X has range [0, 1 * aspect]

    // This starts in the range of [0, 1],
    // But we want it relative to the screens.
    // We want to work in the coordinate space x=[0, aspect] y=[0, 1]
    // Because this reflects the actual size or whatever
    vec2 rectStartPoint = in_startPoint;
    rectStartPoint.x *= aspect;
    vec2 rectEndPoint = in_endPoint;
    rectEndPoint.x *= aspect;
    vec2 extent = rectEndPoint - rectStartPoint;
    float rectAspect = extent.x / extent.y;
    vec2 rectPos = in_startPoint;
    vec2 pos = in_trianglePos;
    pos *= extent;
    //pos.x *= rectAspect;
    pos += in_startPoint;


    vec4 radius = CorrectedRadius();
    float dist = test(pos, rectPos, extent, radius);
    float alpha = dist;
    alpha = alpha > 0 ? 1 : 0;




    vec4 color = vec4(1);
    //color.z = 0;
    //color.xy = trianglePos;
    //color.xy = pos;
    //color = mix(vec4(0), vec4(1), alpha);

    color = pushConstData.color;
    color.w *= alpha;
    //color.w *= 0.5;
    //color.w = alpha;
    //color.w = 1;

    outColor = color;
}