#pragma once
#include <glad/gl.h>
#include <cstddef>

// Giai ma PNG/JPG tu bo nho (texture nhung trong GLB). Tra ve 0 neu loi.
GLuint textureFromMemory(const unsigned char* data, size_t size);
// Tao texture tu mang RGBA8 (da o thu tu "dong dau tien o tren")
GLuint textureFromRGBA(const unsigned char* rgba, int w, int h, bool mipmaps = true);
GLuint createWhiteTexture();
// Texture lo dan sinh bang code (khong can file)
GLuint createBulletHoleTexture();
