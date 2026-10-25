#include "Model.hpp"
#include "Shader.hpp"
#include "Texture.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstddef>

#include <algorithm>
#include <filesystem>
#include <iostream>

static glm::mat4 toGlm(const aiMatrix4x4& m) {
    return glm::transpose(glm::make_mat4(&m.a1));   // assimp: row-major
}

struct ModelLoader {
    Model& m;
    const aiScene* scene;
    std::unordered_map<std::string, GLuint> texCache;

    int addNode(const aiNode* n, int parent) {
        int idx = static_cast<int>(m.nodes_.size());
        m.nodes_.emplace_back();
        {
            ModelNode& node = m.nodes_[idx];
            node.name = n->mName.C_Str();
            node.parent = parent;
            node.local = toGlm(n->mTransformation);
            aiVector3D s, p;
            aiQuaternion q;
            n->mTransformation.Decompose(s, q, p);
            node.t = glm::vec3(p.x, p.y, p.z);
            node.s = glm::vec3(s.x, s.y, s.z);
            node.r = glm::normalize(glm::quat(q.w, q.x, q.y, q.z));
            for (unsigned i = 0; i < n->mNumMeshes; ++i) node.meshes.push_back(static_cast<int>(n->mMeshes[i]));
        }
        m.nameToNode_.emplace(m.nodes_[idx].name, idx);
        for (unsigned i = 0; i < n->mNumChildren; ++i) addNode(n->mChildren[i], idx);
        return idx;
    }

    GLuint loadTexture(const aiMaterial* mat, glm::vec4& color) {
        color = glm::vec4(1.0f);
        aiColor4D c;
        if (mat->Get(AI_MATKEY_BASE_COLOR, c) == AI_SUCCESS) color = glm::vec4(c.r, c.g, c.b, c.a);
        else if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, c) == AI_SUCCESS) color = glm::vec4(c.r, c.g, c.b, c.a);
        color.a = 1.0f;

        const aiTextureType types[] = {aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE};
        for (aiTextureType type : types) {
            aiString path;
            if (mat->GetTexture(type, 0, &path) != AI_SUCCESS) continue;
            std::string key = path.C_Str();
            auto it = texCache.find(key);
            if (it != texCache.end()) return it->second;

            GLuint tex = 0;
            if (const aiTexture* et = scene->GetEmbeddedTexture(key.c_str())) {
                if (et->mHeight == 0) {   // nen (PNG/JPG), mWidth = so byte
                    tex = textureFromMemory(reinterpret_cast<const unsigned char*>(et->pcData), et->mWidth);
                } else {                  // raw texel (BGRA) -> RGBA, lat doc cho dung quy uoc
                    std::vector<unsigned char> px(static_cast<size_t>(et->mWidth) * et->mHeight * 4);
                    for (unsigned y = 0; y < et->mHeight; ++y)
                        for (unsigned x = 0; x < et->mWidth; ++x) {
                            const aiTexel& t = et->pcData[(et->mHeight - 1 - y) * et->mWidth + x];
                            unsigned char* o = &px[(static_cast<size_t>(y) * et->mWidth + x) * 4];
                            o[0] = t.r; o[1] = t.g; o[2] = t.b; o[3] = t.a;
                        }
                    tex = textureFromRGBA(px.data(), et->mWidth, et->mHeight);
                }
            }
            if (tex) m.ownedTextures_.push_back(tex);
            texCache[key] = tex;
            if (tex) return tex;
        }
        return m.white_;
    }

    void addMesh(const aiMesh* am) {
        ModelMesh mm;
        mm.name = am->mName.C_Str();
        std::vector<ModelVertex> verts(am->mNumVertices);
        for (unsigned i = 0; i < am->mNumVertices; ++i) {
            ModelVertex& v = verts[i];
            v.pos = glm::vec3(am->mVertices[i].x, am->mVertices[i].y, am->mVertices[i].z);
            if (am->HasNormals())
                v.normal = glm::vec3(am->mNormals[i].x, am->mNormals[i].y, am->mNormals[i].z);
            if (am->HasTextureCoords(0))
                v.uv = glm::vec2(am->mTextureCoords[0][i].x, am->mTextureCoords[0][i].y);
        }

        mm.skinned = am->mNumBones > 0;
        std::vector<int> fill(am->mNumVertices, 0);
        for (unsigned b = 0; b < am->mNumBones; ++b) {
            const aiBone* bone = am->mBones[b];
            auto it = m.nameToNode_.find(bone->mName.C_Str());
            mm.boneNode.push_back(it != m.nameToNode_.end() ? it->second : 0);
            mm.boneOffset.push_back(toGlm(bone->mOffsetMatrix));
            for (unsigned w = 0; w < bone->mNumWeights; ++w) {
                unsigned vid = bone->mWeights[w].mVertexId;
                int c = fill[vid];
                if (c < 4) {
                    verts[vid].bones[c] = static_cast<int>(b);
                    verts[vid].weights[c] = bone->mWeights[w].mWeight;
                    fill[vid] = c + 1;
                }
            }
        }
        if (mm.skinned && am->mNumBones > MODEL_MAX_BONES)
            std::cerr << "Canh bao: mesh " << mm.name << " co " << am->mNumBones
                      << " bone > " << MODEL_MAX_BONES << std::endl;
        if (mm.skinned) {
            for (auto& v : verts) {
                float sum = v.weights.x + v.weights.y + v.weights.z + v.weights.w;
                if (sum < 1e-5f) { v.bones = glm::ivec4(0); v.weights = glm::vec4(1, 0, 0, 0); }
                else v.weights /= sum;
            }
        }

        std::vector<unsigned> idx;
        idx.reserve(am->mNumFaces * 3);
        for (unsigned f = 0; f < am->mNumFaces; ++f) {
            if (am->mFaces[f].mNumIndices != 3) continue;
            idx.push_back(am->mFaces[f].mIndices[0]);
            idx.push_back(am->mFaces[f].mIndices[1]);
            idx.push_back(am->mFaces[f].mIndices[2]);
        }
        mm.indexCount = static_cast<GLsizei>(idx.size());

        mm.texture = loadTexture(scene->mMaterials[am->mMaterialIndex], mm.baseColor);

        glGenVertexArrays(1, &mm.vao);
        glGenBuffers(1, &mm.vbo);
        glGenBuffers(1, &mm.ebo);
        glBindVertexArray(mm.vao);
        glBindBuffer(GL_ARRAY_BUFFER, mm.vbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(ModelVertex), verts.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mm.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned), idx.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, pos));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, uv));
        glEnableVertexAttribArray(3);
        glVertexAttribIPointer(3, 4, GL_INT, sizeof(ModelVertex), (void*)offsetof(ModelVertex, bones));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, weights));
        glBindVertexArray(0);

        m.meshes_.push_back(std::move(mm));
    }

    void addClip(const aiAnimation* a) {
        Clip c;
        c.name = a->mName.C_Str();
        const float tps = a->mTicksPerSecond > 0.0 ? static_cast<float>(a->mTicksPerSecond) : 25.0f;
        float lo = 1e9f, hi = 0.0f;
        auto track = [&](float t) {
            if (t > 1e-4f) lo = std::min(lo, t);   // bo qua key gia o t=0 do assimp chen vao
            hi = std::max(hi, t);
        };
        for (unsigned i = 0; i < a->mNumChannels; ++i) {
            const aiNodeAnim* ch = a->mChannels[i];
            auto it = m.nameToNode_.find(ch->mNodeName.C_Str());
            if (it == m.nameToNode_.end()) continue;
            Channel cc;
            cc.node = it->second;
            for (unsigned k = 0; k < ch->mNumPositionKeys; ++k) {
                float t = static_cast<float>(ch->mPositionKeys[k].mTime) / tps;
                const aiVector3D& v = ch->mPositionKeys[k].mValue;
                cc.pos.push_back({t, glm::vec3(v.x, v.y, v.z)});
                track(t);
            }
            for (unsigned k = 0; k < ch->mNumRotationKeys; ++k) {
                float t = static_cast<float>(ch->mRotationKeys[k].mTime) / tps;
                const aiQuaternion& q = ch->mRotationKeys[k].mValue;
                cc.rot.push_back({t, glm::normalize(glm::quat(q.w, q.x, q.y, q.z))});
                track(t);
            }
            for (unsigned k = 0; k < ch->mNumScalingKeys; ++k) {
                float t = static_cast<float>(ch->mScalingKeys[k].mTime) / tps;
                const aiVector3D& v = ch->mScalingKeys[k].mValue;
                cc.scl.push_back({t, glm::vec3(v.x, v.y, v.z)});
                track(t);
            }
            c.channels.push_back(std::move(cc));
        }
        c.start = (lo > 1e8f) ? 0.0f : lo;
        c.end = std::max(hi, c.start);
        c.nodeChannel.assign(m.nodes_.size(), -1);
        for (size_t i = 0; i < c.channels.size(); ++i) c.nodeChannel[c.channels[i].node] = static_cast<int>(i);
        m.clips_.push_back(std::move(c));
    }
};

bool Model::load(const std::string& path) {
    destroy();
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals |
                  aiProcess_LimitBoneWeights | aiProcess_ImproveCacheLocality);
    if (!scene || !scene->mRootNode || scene->mNumMeshes == 0) {
        std::cerr << "Khong nap duoc model '" << path << "': " << importer.GetErrorString() << std::endl;
        return false;
    }

    white_ = createWhiteTexture();
    ownedTextures_.push_back(white_);

    ModelLoader loader{*this, scene, {}};
    loader.addNode(scene->mRootNode, -1);
    for (unsigned i = 0; i < scene->mNumMeshes; ++i) loader.addMesh(scene->mMeshes[i]);
    for (unsigned i = 0; i < scene->mNumAnimations; ++i) loader.addClip(scene->mAnimations[i]);

    std::cout << "Model " << std::filesystem::path(path).filename().string() << ": "
              << meshes_.size() << " mesh, " << nodes_.size() << " node, " << clips_.size() << " clip\n";
    for (const Clip& c : clips_)
        std::cout << "   clip '" << c.name << "' " << c.start << "s -> " << c.end << "s\n";
    return true;
}

void Model::destroy() {
    for (auto& m : meshes_) {
        if (m.vao) glDeleteVertexArrays(1, &m.vao);
        if (m.vbo) glDeleteBuffers(1, &m.vbo);
        if (m.ebo) glDeleteBuffers(1, &m.ebo);
    }
    if (!ownedTextures_.empty()) glDeleteTextures(static_cast<GLsizei>(ownedTextures_.size()), ownedTextures_.data());
    meshes_.clear();
    nodes_.clear();
    clips_.clear();
    nameToNode_.clear();
    ownedTextures_.clear();
    white_ = 0;
}

int Model::findClip(const std::string& name) const {
    for (size_t i = 0; i < clips_.size(); ++i)
        if (clips_[i].name == name) return static_cast<int>(i);
    return -1;
}

int Model::findNode(const std::string& name) const {
    auto it = nameToNode_.find(name);
    return it == nameToNode_.end() ? -1 : it->second;
}

static glm::vec3 sampleVec(const std::vector<KeyVec>& k, float t, const glm::vec3& def) {
    if (k.empty()) return def;
    if (k.size() == 1 || t <= k.front().time) return k.front().v;
    if (t >= k.back().time) return k.back().v;
    auto it = std::upper_bound(k.begin(), k.end(), t, [](float v, const KeyVec& a) { return v < a.time; });
    const KeyVec& b = *it;
    const KeyVec& a = *(it - 1);
    float f = (t - a.time) / std::max(b.time - a.time, 1e-6f);
    return glm::mix(a.v, b.v, f);
}

static glm::quat sampleQuat(const std::vector<KeyQuat>& k, float t, const glm::quat& def) {
    if (k.empty()) return def;
    if (k.size() == 1 || t <= k.front().time) return k.front().q;
    if (t >= k.back().time) return k.back().q;
    auto it = std::upper_bound(k.begin(), k.end(), t, [](float v, const KeyQuat& a) { return v < a.time; });
    const KeyQuat& b = *it;
    const KeyQuat& a = *(it - 1);
    float f = (t - a.time) / std::max(b.time - a.time, 1e-6f);
    return glm::normalize(glm::slerp(a.q, b.q, f));
}

void Model::evaluate(int ci, float time, std::vector<glm::mat4>& g) const {
    g.resize(nodes_.size());
    for (size_t i = 0; i < nodes_.size(); ++i) {
        const ModelNode& n = nodes_[i];
        glm::mat4 local = n.local;
        if (ci >= 0) {
            int chi = clips_[ci].nodeChannel[i];
            if (chi >= 0) {
                const Channel& ch = clips_[ci].channels[chi];
                glm::vec3 t = sampleVec(ch.pos, time, n.t);
                glm::quat r = sampleQuat(ch.rot, time, n.r);
                glm::vec3 s = sampleVec(ch.scl, time, n.s);
                local = glm::translate(glm::mat4(1.0f), t) * glm::mat4_cast(r) * glm::scale(glm::mat4(1.0f), s);
            }
        }
        g[i] = n.parent < 0 ? local : g[n.parent] * local;   // cha luon dung truoc con
    }
}

void Model::draw(Shader& sh, const glm::mat4& modelMat, const std::vector<glm::mat4>& globals) const {
    sh.setI("uTex", 0);
    glActiveTexture(GL_TEXTURE0);
    std::vector<glm::mat4> bones;
    for (size_t i = 0; i < nodes_.size(); ++i) {
        for (int mi : nodes_[i].meshes) {
            const ModelMesh& m = meshes_[mi];
            if (m.skinned) {
                sh.setM("uModel", modelMat);   // mesh skin: bo qua transform cua node chua no
                sh.setI("uSkinned", 1);
                bones.resize(std::min<size_t>(m.boneNode.size(), MODEL_MAX_BONES));
                for (size_t b = 0; b < bones.size(); ++b) bones[b] = globals[m.boneNode[b]] * m.boneOffset[b];
                if (!bones.empty()) sh.setMv("uBones", bones.data(), static_cast<int>(bones.size()));
            } else {
                sh.setM("uModel", modelMat * globals[i]);
                sh.setI("uSkinned", 0);
            }
            sh.set4("uBaseColor", m.baseColor);
            glBindTexture(GL_TEXTURE_2D, m.texture);
            glBindVertexArray(m.vao);
            glDrawElements(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, nullptr);
        }
    }
    glBindVertexArray(0);
}

glm::vec3 Model::nodePosition(const std::vector<glm::mat4>& globals, int node) const {
    if (node < 0 || node >= static_cast<int>(globals.size())) return glm::vec3(0.0f);
    return glm::vec3(globals[node][3]);
}
