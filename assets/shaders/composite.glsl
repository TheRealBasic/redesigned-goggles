#version 120

uniform sampler2D uAlbedoTex;
uniform sampler2D uLightTex;

void main() {
    vec2 uv = gl_TexCoord[0].xy;
    vec3 albedo = texture2D(uAlbedoTex, uv).rgb;
    vec3 light = texture2D(uLightTex, uv).rgb;
    gl_FragColor = vec4(albedo * light, 1.0);
}
