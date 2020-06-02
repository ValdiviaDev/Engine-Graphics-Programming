#version 330 core

uniform sampler2D gPosition;
//uniform sampler2D gDepth;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

//camera
//vec4 normalized_device_coordinate;
//uniform int viewport_width;
//uniform int viewport_height;
//uniform float near;
//uniform float far;
//uniform mat4 projection_matrix_transposed;
//uniform mat4 view_matrix_transposed;
//uniform vec3 camera_position;

// Lights
uniform int lightType;
uniform vec3 lightPosition;
uniform vec3 lightDirection;
uniform vec3 lightColor;
//uniform vec3 lightIntensity;

in vec2 vTexCoords;

layout(location=0) out vec4 outColor;

void main(void)
{
   //Calculate lighting
   vec3 lighting = texture(gAlbedoSpec, vTexCoords).rgb * 0.1;
   vec3 lightDir = normalize(lightPosition - texture(gPosition, vTexCoords).xyz);
   vec3 diffuse = max(dot(texture(gNormal, vTexCoords).xyz, lightDir), 0.0) * texture(gAlbedoSpec, vTexCoords).rgb * lightColor;
   lighting += diffuse;

    outColor = vec4(lighting, 1.0);
}

//vec3 FragmentToWorld()
//{
//        //World Depth
//        float depth_value = texture(gDepth, vTexCoords).z;
//
//        //Transform FragmentPos to WorldPos
//        vec2 viewport_size = vec2(viewport_width, viewport_height);
//        normalized_device_coordinate.xy = ((2.0 * gl_FragCoord.xy) / viewport_size.xy) - 1;
//        normalized_device_coordinate.z = (depth_value * 2) - 1;
//        normalized_device_coordinate.w = 1.0;
//
//        vec4 projpos_frag = projection_matrix_transposed  * normalized_device_coordinate;
//        projpos_frag = projpos_frag / projpos_frag.w;
//
//        vec3 world_space_fragment = (view_matrix_transposed * projpos_frag).xyz;
//
//        return world_space_fragment;
//}
//
//vec4 CalculateDirectional()
//{
//        vec3 light_dir_normalized = normalize(lightDirection);
//
//        vec4 albedo_color = texture(gAlbedoSpec, vTexCoords);
//        vec3 normal_vector = texture(gNormal, vTexCoords).xyz;
//        vec3 eye_dir = normalize(camera_position - FragmentToWorld());
//        vec3 H = normalize(eye_dir + light_dir_normalized);
//
//        vec4 ambient_color =  vec4(albedo_color.rgb * 0.1, 1.0) * lightColor;
//        vec4 difuse_color = albedo_color * max(dot(light_dir_normalized, normal_vector), 0.0) * lightColor;
//        vec4 specular =  0.5 * pow(max(dot(normal_vector, H), 0.0), 32)  * lightColor;
//
//        vec4 final_color = vec4((ambient_color + difuse_color + specular).rgb * (lightIntensity * 0.1), 1.0);
//
//        return final_color;
//}
//
//vec4 CalculatePoint()
//{
//        return vec4(1.0);
//}
//
//void main(void)
//{
//        if(lightType == 0)
//                outColor = CalculateDirectional();
//
//        if(lightType == 1)
//                outColor = CalculatePoint();
//}

