#pragma once
// Model 3D nap bang Assimp (GLB/glTF): mesh, texture nhung, skeleton va animation.

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <unordered_map>
#include <vector>

class Shader;

constexpr int MODEL_MAX_BONES = 64;   // phai khop voi uBones[] trong VIEWMODEL_VERT

struct ModelVertex {
    glm::vec3 pos{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec2 uv{0.0f};
    glm::ivec4 bones{0};
    glm::vec4 weights{0.0f};
};

struct ModelMesh {
    std::string name;
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei indexCount = 0;
    GLuint texture = 0;
    glm::vec4 baseColor{1.0f};
    bool skinned = false;
    std::vector<int> boneNode;            // chi so bone cuc bo -> node
    std::vector<glm::mat4> boneOffset;    // inverse bind matrix
};

struct ModelNode {
    std::string name;
    int parent = -1;
    glm::mat4 local{1.0f};
    glm::vec3 t{0.0f};
    glm::quat r{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 s{1.0f};
    std::vector<int> meshes;
};

struct KeyVec { float time; glm::vec3 v; };
struct KeyQuat { float time; glm::quat q; };

struct Channel {
    int node = -1;
    std::vector<KeyVec> pos, scl;
    std::vector<KeyQuat> rot;
};

// Mot clip. GLB nay gop moi clip vao 1 timeline chung nen moi clip co [start, end] rieng (giay).
struct Clip {
    std::string name;
    float start = 0.0f, end = 0.0f;
    std::vector<Channel> channels;
    std::vector<int> nodeChannel;         // node -> chi so channel (-1 neu khong co)
    float duration() const { return end - start; }
};

class Model {
public:
    ~Model() { destroy(); }

    bool load(const std::string& path);
    void destroy();

    int findClip(const std::string& name) const;
    const Clip& clip(int i) const { return clips_[i]; }
    int clipCount() const { return static_cast<int>(clips_.size()); }
    int findNode(const std::string& name) const;
    size_t nodeCount() const { return nodes_.size(); }
    bool loaded() const { return !meshes_.empty(); }

    // Tinh ma tran global cua moi node tai thoi diem "time" (giay, tren timeline chung) cua clip ci.
    // ci < 0: pose mac dinh (bind).
    void evaluate(int ci, float time, std::vector<glm::mat4>& globals) const;

    // Ve toan bo model. shader phai la viewmodel shader.
    void draw(Shader& shader, const glm::mat4& modelMat, const std::vector<glm::mat4>& globals) const;

    // Vi tri (theo toa do model sau cay node) cua mot node
    glm::vec3 nodePosition(const std::vector<glm::mat4>& globals, int node) const;

private:
    std::vector<ModelNode> nodes_;
    std::vector<ModelMesh> meshes_;
    std::vector<Clip> clips_;
    std::unordered_map<std::string, int> nameToNode_;
    std::vector<GLuint> ownedTextures_;
    GLuint white_ = 0;

    friend struct ModelLoader;
};
