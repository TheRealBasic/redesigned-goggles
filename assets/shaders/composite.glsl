#version 120

uniform sampler2D uAlbedoTex;
uniform sampler2D uLightTex;

void main() {
    vec2 uv = gl_TexCoord[0].xy;
    vec3 albedo = texture2D(uAlbedoTex, uv).rgb;
    vec3 light = texture2D(uLightTex, uv).rgb;
    vec3 color = albedo * light;
    color = clamp(color, vec3(0.0), vec3(1.0));
    color = color / (vec3(1.0) + color);
    gl_FragColor = vec4(color, 1.0);
}
