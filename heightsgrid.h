#ifndef HEIGHTSGRID_H
#define HEIGHTSGRID_H
#include <vector>
#include "glm/glm.hpp"

class HeightsGrid
{
public:
    HeightsGrid(const std::vector<std::vector<float> >& grid,
                const glm::vec2& gmin,
                const glm::vec2& gmax,
                const glm::vec2& gres,
                float gridNoVal = -9999.0f);

    glm::vec2  getGridMin() const;
    glm::vec2  getGridMax() const;
    glm::vec2  getGridRes() const;
    glm::vec2  getGridExtension() const;
    glm::ivec2 getGridSize() const;
    float      getGridNoValue() const;

    const std::vector<std::vector<float> >& data() const;

    void  buildTriangleModel(std::vector<glm::vec3>& verts, std::vector<glm::ivec3>& tris) const;

    float getHeightMin();
    float getHeightMax();
    void getRadialStatistics(const glm::vec2& p, float rad, glm::vec3& hmin, glm::vec3& hmax, float& hmean, float& hdev) const;
    void getRadialStatistics(const glm::vec3& p, float rad, glm::vec3& hmin, glm::vec3& hmax, float& hmean, float& hdev) const;
    float getIsolation(const glm::vec2& p, float minDist, glm::vec3& pIso, float minIsoArea = 0, float hOffset = 0);
    float getIsolation(const glm::vec3& p, float minDist, glm::vec3& pIso, float minIsoArea = 0, float hOffset = 0);

private:
    std::vector<std::vector<float> > grid;
    glm::vec2  gridMin, gridMax, gridRes;
    glm::ivec2 gridSize;
    float      gridNoValue;

    float heightMin, heightMax;
};

inline glm::vec2 HeightsGrid::getGridMin() const {
    return gridMin;
}

inline glm::vec2 HeightsGrid::getGridMax() const {
    return gridMax;
}

inline glm::vec2 HeightsGrid::getGridRes() const {
    return gridRes;
}

inline glm::vec2 HeightsGrid::getGridExtension() const {
    return gridMax - gridMin;
}

inline glm::ivec2 HeightsGrid::getGridSize() const {
    return gridSize;
}

inline float HeightsGrid::getGridNoValue() const {
    return gridNoValue;
}

inline const std::vector<std::vector<float> >& HeightsGrid::data() const {
    return grid;
}

#endif // HEIGHTSGRID_H
