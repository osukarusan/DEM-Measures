#ifndef LOADERPLY_H
#define LOADERPLY_H

#include <string>
#include <vector>
#include "glm/glm.hpp"

class LoaderPLY
{
public:

    static bool loadPLY(const std::string& filename, std::vector<glm::vec3>& verts, std::vector<glm::ivec3>& faces);
    static bool writePLY(const std::string& filename, const std::vector<glm::vec3>& verts, const std::vector<glm::ivec3>& faces);

};

#endif // LOADERPLY_H
