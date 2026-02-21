#version 120

uniform vec2 uResolution;
uniform vec2 uIsoTile;
uniform vec2 uIsoOrigin;
uniform float uAmbient;
uniform vec4 uPlayerLight;
uniform vec4 uLampLight;

float smootherFalloff(float normalizedDistance) {
    float t = clamp(1.0 - normalizedDistance, 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

float lightContribution(vec2 tilePos, vec4 lightData) {
    vec2 delta = tilePos - lightData.xy;
    float dist = length(delta);
    float normalized = dist / max(0.001, lightData.z);
    return smootherFalloff(normalized) * lightData.w;
}

void main() {
    vec2 screenPos = vec2(gl_FragCoord.x, uResolution.y - gl_FragCoord.y);

    float isoX = (screenPos.x - uIsoOrigin.x) / (uIsoTile.x * 0.5);
    float isoY = (screenPos.y - uIsoOrigin.y) / (uIsoTile.y * 0.5);
    vec2 tilePos = vec2((isoX + isoY) * 0.5, (isoY - isoX) * 0.5);

    float light = uAmbient;
    light += lightContribution(tilePos, uPlayerLight);
    light += lightContribution(tilePos, uLampLight);

    gl_FragColor = vec4(vec3(clamp(light, 0.0, 1.0)), 1.0);
}
