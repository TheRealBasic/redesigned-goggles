#version 120

uniform vec2 uResolution;
uniform vec2 uIsoTile;
uniform vec2 uIsoOrigin;
uniform float uAmbient;
uniform vec3 uAmbientColor;
uniform vec4 uPlayerLight;
uniform vec4 uLampLight;
uniform vec4 uPlayerLightColor;
uniform vec4 uLampLightColor;

float lightContribution(vec2 tilePos, vec4 lightData, float falloffExponent) {
    vec2 delta = tilePos - lightData.xy;
    float dist = length(delta);
    float radius = max(0.001, lightData.z);
    float normalized = dist / radius;

    float attenuation;
    if (falloffExponent > 0.0) {
        attenuation = pow(clamp(1.0 - normalized, 0.0, 1.0), falloffExponent);
    } else {
        float k = 1.0 / (radius * radius);
        attenuation = 1.0 / (1.0 + k * dist * dist);
    }

    return attenuation * lightData.w;
}

void main() {
    vec2 screenPos = vec2(gl_FragCoord.x, uResolution.y - gl_FragCoord.y);

    float isoX = (screenPos.x - uIsoOrigin.x) / (uIsoTile.x * 0.5);
    float isoY = (screenPos.y - uIsoOrigin.y) / (uIsoTile.y * 0.5);
    vec2 tilePos = vec2((isoX + isoY) * 0.5, (isoY - isoX) * 0.5);

    float playerContribution = lightContribution(tilePos, uPlayerLight, uPlayerLightColor.w);
    float lampContribution = lightContribution(tilePos, uLampLight, uLampLightColor.w);

    vec3 light = uAmbientColor * uAmbient;
    light += uPlayerLightColor.rgb * playerContribution;
    light += uLampLightColor.rgb * lampContribution;

    light = light / (vec3(1.0) + light);
    light = clamp(light, vec3(0.0), vec3(1.0));

    gl_FragColor = vec4(light, 1.0);
}
