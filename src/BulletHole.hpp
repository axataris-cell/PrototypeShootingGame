#pragma once
#include "Mesh.hpp"
#include "Shader.hpp"

#include <deque>
#include <glm/glm.hpp>

// Lo dan: toi da MAX_BULLET_HOLES cung luc, vuot qua -> xoa lo cu nhat (FIFO)
class BulletHoles {
public:
    void add(const glm::vec3& pos, const glm::vec3& normal);
    void clear() { holes_.clear(); }
    size_t size() const { return holes_.size(); }
    void draw(Shader& decalShader, const Mesh& quad, GLuint texture) const;

private:
    struct Hole {
        glm::mat4 model;
    };
    std::deque<Hole> holes_;
};
