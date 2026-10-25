#include "Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

GLuint textureFromRGBA(const unsigned char* rgba, int w, int h, bool mipmaps) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    if (mipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return tex;
}

GLuint textureFromMemory(const unsigned char* data, size_t size) {
    // Assimp da lat truc V cua UV (goc duoi-trai), nen lat anh khi nap de khop
    stbi_set_flip_vertically_on_load(1);
    int w = 0, h = 0, comp = 0;
    unsigned char* px = stbi_load_from_memory(data, static_cast<int>(size), &w, &h, &comp, 4);
    if (!px) {
        std::cerr << "Khong giai ma duoc texture: " << stbi_failure_reason() << std::endl;
        return 0;
    }
    GLuint tex = textureFromRGBA(px, w, h, true);
    stbi_image_free(px);
    return tex;
}

GLuint createWhiteTexture() {
    const unsigned char white[4] = {255, 255, 255, 255};
    return textureFromRGBA(white, 1, 1, false);
}

static float hash2(int x, int y) {
    unsigned h = static_cast<unsigned>(x) * 374761393u + static_cast<unsigned>(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return static_cast<float>((h ^ (h >> 16)) & 0xFFFF) / 65535.0f;
}

GLuint createBulletHoleTexture() {
    const int N = 64;
    std::vector<unsigned char> px(N * N * 4, 0);
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            float fx = (x + 0.5f) / N * 2.0f - 1.0f;
            float fy = (y + 0.5f) / N * 2.0f - 1.0f;
            float r = std::sqrt(fx * fx + fy * fy);
            float ang = std::atan2(fy, fx);

            // Be mat gap ghenh theo goc
            float jag = 0.5f + 0.5f * std::sin(ang * 5.0f + hash2(x / 6, y / 6) * 3.0f);
            float jag2 = hash2(static_cast<int>((ang + 3.15f) * 6.0f), 7);
            float rr = r * (1.0f + 0.22f * jag + 0.18f * jag2);

            float hole  = 1.0f - std::clamp((rr - 0.20f) / 0.07f, 0.0f, 1.0f);       // loi den
            float scorch = 1.0f - std::clamp((rr - 0.28f) / 0.45f, 0.0f, 1.0f);       // vet khet
            float chip = (rr > 0.24f && rr < 0.40f) ? 0.35f : 0.0f;                    // vanh vo vun

            float alpha = std::max(hole, scorch * 0.9f);
            float shade = 0.04f + 0.30f * (1.0f - hole) * (1.0f - scorch * 0.8f) + chip * 0.2f;
            shade += (hash2(x, y) - 0.5f) * 0.05f;
            shade = std::clamp(shade, 0.0f, 1.0f);

            unsigned char* p = &px[(y * N + x) * 4];
            p[0] = static_cast<unsigned char>(shade * 255.0f * 1.05f);
            p[1] = static_cast<unsigned char>(shade * 255.0f);
            p[2] = static_cast<unsigned char>(shade * 255.0f * 0.95f);
            p[3] = static_cast<unsigned char>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f);
        }
    }
    GLuint tex = textureFromRGBA(px.data(), N, N, true);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}
