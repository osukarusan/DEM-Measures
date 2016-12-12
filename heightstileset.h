#ifndef HEIGHTSTILESET_H
#define HEIGHTSTILESET_H
#include <vector>
#include <string>
#include "glm/glm.hpp"
#include "heightsgrid.h"


class HeightsTileset
{
public:
    HeightsTileset(const std::string& pathToDescriptor);
    ~HeightsTileset();

    glm::vec2 getTilesetMin() const;
    glm::vec2 getTilesetMax() const;
    glm::vec2 getTileRes() const;
    glm::vec2 getTileExtension() const;
    glm::vec2 getTilesetExtension() const;

    HeightsGrid* loadRegion(const glm::vec2& pmin, const glm::vec2& pmax, const glm::vec2& res);

protected:
    std::vector<std::vector<float> > readTile(int ti, int tj, const glm::ivec2& outFactor);

private:
    // tileset properties
    std::string tilesFolder;
    glm::vec2   tsetMin, tsetMax, tsetExtension;
    glm::vec2   tileRes, tileExtension;
    float       hNoValue, hSeaValue, hSeaLevel;
    glm::ivec2  ppTile;
    glm::ivec2  numTiles;

    // tmp buffer
    float* loadBuffer;
};


inline glm::vec2 HeightsTileset::getTilesetMin() const {
    return tsetMin;
}

inline glm::vec2 HeightsTileset::getTilesetMax() const {
    return tsetMax;
}

inline glm::vec2 HeightsTileset::getTileRes() const {
    return tileRes;
}

inline glm::vec2 HeightsTileset::getTileExtension() const {
    return tileExtension;
}

inline glm::vec2 HeightsTileset::getTilesetExtension() const {
    return tsetExtension;
}


#endif // HEIGHTSTILESET_H
