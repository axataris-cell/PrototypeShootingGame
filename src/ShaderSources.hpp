#pragma once
// Ma nguon GLSL cua game. Moi shader co cong tac uFx: 1 = hieu ung day du, 0 = che do don gian
// (Settings -> Shader ON/OFF).

namespace shader_src {

// ------------------------------------------------------------
// Vertex shader dung chung cho khoi hop / quad (world, enemy, decal)
// ------------------------------------------------------------
static const char* PRIMITIVE_VERT = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vUV = aUV;
    gl_Position = uProj * uView * world;
}
)";

// ------------------------------------------------------------
// Shader VAT THE: dat, tuong, vat can, tia dan
// ------------------------------------------------------------
static const char* WORLD_FRAG = R"(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
out vec4 FragColor;

uniform vec4  uColor;
uniform vec3  uLightDir;   // huong anh sang (tu nguon di xuong)
uniform vec3  uViewPos;
uniform vec3  uFogColor;
uniform int   uUnlit;      // 1 = khong tinh anh sang (tia dan...)
uniform int   uChecker;    // 1 = mat dat (ke o ca ro)
uniform int   uFx;         // 1 = shader day du
uniform float uTime;

float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float vnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1, 0)), f.x),
               mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), f.x), f.y);
}

void main() {
    if (uUnlit == 1) {
        FragColor = uColor;
        return;
    }

    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uViewPos - vWorldPos);
    vec3 base = uColor.rgb;
    if (uChecker == 1) {
        float c = mod(floor(vWorldPos.x) + floor(vWorldPos.z), 2.0);
        base *= mix(0.65, 1.0, c);
    }

    // ----- Che do don gian -----
    if (uFx == 0) {
        float d = max(dot(N, L), 0.0);
        FragColor = vec4(base * (0.4 + 0.7 * d), uColor.a);
        return;
    }

    // ----- Che do day du -----
    vec3 P = vWorldPos;
    float n = vnoise(P.xz * 2.5 + P.y * 1.3) * 0.6 + vnoise(P.xz * 9.0 + P.y * 4.0) * 0.4;
    base *= 0.84 + 0.32 * n;                       // van be mat

    if (uChecker == 0) {
        vec2 e = min(vUV, 1.0 - vUV);              // toi dan o canh hop
        base *= mix(0.70, 1.0, smoothstep(0.0, 0.035, min(e.x, e.y)));
        base *= mix(0.65, 1.0, smoothstep(0.0, 1.5, P.y));   // AO gia gan mat dat
    } else {
        base *= 0.9 + 0.2 * vnoise(P.xz * 0.6);    // mat dat loang lo
    }

    vec3 sky = vec3(0.50, 0.60, 0.78);
    vec3 gnd = vec3(0.30, 0.27, 0.22);
    vec3 amb = mix(gnd, sky, N.y * 0.5 + 0.5) * 0.55;   // anh sang moi truong ban cau

    float ndl = dot(N, L);
    float diff = max(ndl, 0.0);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 48.0) * 0.25 * step(0.0, ndl);
    float fres = pow(1.0 - max(dot(N, V), 0.0), 4.0);

    vec3 col = base * (amb + vec3(1.0, 0.95, 0.85) * diff * 0.95) + vec3(spec) + fres * sky * 0.12;

    float dist = length(uViewPos - vWorldPos);
    float fog = clamp((dist - 30.0) / 60.0, 0.0, 1.0);
    fog = fog * fog * (3.0 - 2.0 * fog);
    col = mix(col, uFogColor, fog);

    FragColor = vec4(col, uColor.a);
}
)";

// ------------------------------------------------------------
// Shader QUAI: toon 4 nac + vien do (rim) + mat phat sang + nhap nhay khi trung dan
// ------------------------------------------------------------
static const char* ENEMY_FRAG = R"(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
out vec4 FragColor;

uniform vec4  uColor;
uniform vec3  uEmissive;
uniform float uHitFlash;   // 0..1
uniform vec3  uLightDir;
uniform vec3  uViewPos;
uniform vec3  uFogColor;
uniform int   uFx;
uniform float uTime;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uViewPos - vWorldPos);
    vec3 base = uColor.rgb;
    float ndl = dot(N, L);
    vec3 col;

    if (uFx == 0) {
        col = base * (0.4 + 0.7 * max(ndl, 0.0)) + uEmissive;
    } else {
        float t = clamp(ndl * 0.5 + 0.5, 0.0, 1.0);
        float band = floor(t * 3.0 + 0.5) / 3.0;           // toon
        float lit = mix(0.35, 1.05, band);
        float rim = pow(1.0 - max(dot(N, V), 0.0), 2.5);
        float grime = 0.85 + 0.15 * smoothstep(0.0, 1.8, vWorldPos.y);   // chan toi hon dau
        col = base * lit * grime + rim * vec3(0.9, 0.2, 0.15) * 0.55;
        col += uEmissive * (0.75 + 0.25 * sin(uTime * 6.0));

        float dist = length(uViewPos - vWorldPos);
        float fog = clamp((dist - 30.0) / 60.0, 0.0, 1.0);
        col = mix(col, uFogColor, fog * fog * (3.0 - 2.0 * fog));
    }

    col = mix(col, vec3(1.0), uHitFlash);
    FragColor = vec4(col, uColor.a);
}
)";

// ------------------------------------------------------------
// Shader NGUOI CHOI (viewmodel): sung + tay, skinning + texture
// ------------------------------------------------------------
static const char* VIEWMODEL_VERT = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
layout (location = 3) in ivec4 aBoneIds;
layout (location = 4) in vec4 aWeights;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uBones[64];
uniform int  uSkinned;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

void main() {
    mat4 skin = mat4(1.0);
    if (uSkinned == 1) {
        skin = aWeights.x * uBones[aBoneIds.x] + aWeights.y * uBones[aBoneIds.y] +
               aWeights.z * uBones[aBoneIds.z] + aWeights.w * uBones[aBoneIds.w];
    }
    mat4 m = uModel * skin;
    vec4 world = m * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    vNormal = transpose(inverse(mat3(m))) * aNormal;
    vUV = aUV;
    gl_Position = uProj * uView * world;
}
)";

static const char* VIEWMODEL_FRAG = R"(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uTex;
uniform vec4  uBaseColor;
uniform vec3  uLightDir;
uniform vec3  uViewPos;
uniform int   uFx;
uniform float uFlash;      // anh lua dau nong 0..1

void main() {
    vec3 albedo = texture(uTex, vUV).rgb * uBaseColor.rgb;
    vec3 N = normalize(vNormal);
    if (!gl_FrontFacing) N = -N;
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uViewPos - vWorldPos);
    float ndl = dot(N, L);
    vec3 col;

    if (uFx == 0) {
        col = albedo * (0.6 + 0.5 * max(ndl, 0.0));
    } else {
        vec3 amb = mix(vec3(0.35, 0.32, 0.30), vec3(0.55, 0.62, 0.75), N.y * 0.5 + 0.5);
        float wrap = max((ndl + 0.35) / 1.35, 0.0);          // wrap lighting cho da / vai
        vec3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), 40.0) * 0.35;
        float rim = pow(1.0 - max(dot(N, V), 0.0), 3.0);
        col = albedo * (amb * 0.8 + vec3(1.0, 0.95, 0.88) * wrap * 0.85)
            + vec3(spec) + rim * vec3(0.5, 0.6, 0.8) * 0.18;
        col += albedo * uFlash * vec3(1.2, 0.7, 0.3) * 0.7;  // phan chieu lua dau nong
    }
    FragColor = vec4(col, 1.0);
}
)";

// ------------------------------------------------------------
// Decal (lo dan)
// ------------------------------------------------------------
static const char* DECAL_FRAG = R"(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uTex;
uniform float uAlpha;

void main() {
    vec4 t = texture(uTex, vUV);
    FragColor = vec4(t.rgb, t.a * uAlpha);
    if (FragColor.a < 0.01) discard;
}
)";

// ------------------------------------------------------------
// UI 2D (HUD, menu, chu)
// ------------------------------------------------------------
static const char* UI_VERT = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aCol;
uniform mat4 uProj;
out vec4 vCol;
void main() {
    vCol = aCol;
    gl_Position = uProj * vec4(aPos, 0.0, 1.0);
}
)";

static const char* UI_FRAG = R"(
#version 330 core
in vec4 vCol;
out vec4 FragColor;
void main() { FragColor = vCol; }
)";

} // namespace shader_src
