#version 330 core

uniform float left;
uniform float right;
uniform float bottom;
uniform float top;
uniform float znear;
uniform mat4 worldMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform float z_far;
uniform float z_near;

in vec2 vTexCoords;
out vec4 outColor;

float grid(vec3 worldPos, float gridStep)
{
    // Compute world-space grid lines
    vec2 grid = fwidth(worldPos.xz) / mod(worldPos.xz, gridStep);
    return step(1.0, max(grid.x, grid.y)); // line
}

void main(void)
{
    outColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);

    // Eye Direction
    vec3 eyeDirEyeSpace;
    eyeDirEyeSpace.x = left + vTexCoords.x * (right -left);
    eyeDirEyeSpace.y = bottom + vTexCoords.y * (top - bottom);
    eyeDirEyeSpace.z = -z_near;
    vec3 eyeDirWorldSpace = normalize(mat3(worldMatrix) * eyeDirEyeSpace);

    // Eye Position
    vec3 eyePosEyeSpace = vec3(0, 0, 0);
    vec3 eyePosWorldSpace = vec3(worldMatrix * vec4(eyePosEyeSpace, 1.0));

    // Plane Parameters
    vec3 planeNormalWorldSpace = vec3(0, 1, 0);
    vec3 planePointWorldSpace = vec3(0, 0, 0);

    // Ray-plane intersection
    float rayPlaneIntersection = dot(planePointWorldSpace - eyePosWorldSpace, planeNormalWorldSpace) / dot(eyeDirWorldSpace, planeNormalWorldSpace);

    if(rayPlaneIntersection > 0.0) // Intersected in front of the eye
    {
        vec3 hitWorldSpace = eyePosWorldSpace + (eyeDirWorldSpace * rayPlaneIntersection);

        outColor = vec4(grid(hitWorldSpace, 1.0) * 0.5);
        outColor += vec4(grid(hitWorldSpace, 10.0) * 0.5);
        outColor += vec4(grid(hitWorldSpace, 100.0) * 0.5);
        vec4 hitClip = projectionMatrix * viewMatrix * vec4(hitWorldSpace, 1.0);
        float ndcDepth = hitClip.z / hitClip.w;
        gl_FragDepth = ((gl_DepthRange.diff * ndcDepth) * gl_DepthRange.near + gl_DepthRange.far) / 2.0;
    }
    else
    {
        gl_FragDepth = 0.0;
        discard;
    }
}
