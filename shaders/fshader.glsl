#version 410

in vec3 fN;
in vec3 fL;
in vec3 fV;
in vec4 color;
in vec2 texCoord2D;
in float texCoord1D;
in vec3 FragPos;

uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform float Shininess;
uniform bool ShadingType;
uniform vec4 LightPosition;
uniform int RenderMode;
uniform bool textureType;
uniform samplerCube depthMap;
uniform sampler2D tex2D;
uniform float far_plane;

out vec4 fragColor;


float ShadowCalculation(vec3 fragPos)
{
    // get vector between fragment position and light position
    vec3 fragToLight = fragPos - LightPosition.xyz;
    // ise the fragment to light vector to sample from the depth map
    float closestDepth = texture(depthMap, fragToLight).r;
    // it is currently in linear range between [0,1], let's re-transform it back to original depth value
    closestDepth *= far_plane;
    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    // test for shadows
    vec3 lightDir = normalize(LightPosition.xyz - fragPos);
    float bias = max(1.0 * (1.0 - dot(normalize(fN), lightDir)), 0.1); // we use a much larger bias since depth is now in [near_plane, far_plane] range
    //float bias = 2.55;

    // display depth cubemap
    //fragColor = vec4(vec3(closestDepth / far_plane), 1.0);

    float shadow = 0.0;
    float samples = 4.0;
    float offset = 0.1;
    for (float x = -offset; x < offset; x += offset / (samples * 0.5))
    {
        for (float y = -offset; y < offset; y += offset / (samples * 0.5))
        {
            for (float z = -offset; z < offset; z += offset / (samples * 0.5))
            {
                float closestDepth = texture(depthMap, fragToLight + vec3(x, y, z)).r;
                closestDepth *= far_plane;   // undo mapping [0;1]
                if (currentDepth - bias > closestDepth)
                shadow += 1.0;
            }
        }
    }

    shadow /= (samples * samples * samples);
    return shadow;
}

void main()
{
    float shadow = ShadowCalculation(FragPos);
    vec4 textureColor = texture(tex2D, texCoord2D);
    if (ShadingType) {
        vec3 N = normalize(fN);
        vec3 V = normalize(fV);
        vec3 L = normalize(fL);

        vec3 H = normalize(L + V);
        vec4 ambient = AmbientProduct;

        float Kd = max(dot(L, N), 0.0);
        vec4 diffuse = Kd * DiffuseProduct;

        float Ks = pow(max(dot(N, H), 0.0), Shininess);
        vec4 specular = Ks * SpecularProduct;

        // discard the specular highlight if the light's behind the vertex
        if (dot(L, N) < 0.0) {
            specular = vec4(0.0, 0.0, 0.0, 1.0);
        }

        fragColor = textureColor * (ambient + (1.0 - shadow) * (diffuse + specular));

        fragColor.a = 1.0;

    } else {
        fragColor = textureColor * (1.0 - shadow) * color;
    }
}
