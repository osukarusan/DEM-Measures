#include "heightstileset.h"
#include <fstream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>


HeightsTileset::HeightsTileset(const std::string& pathToDescriptor)
{
    std::ifstream fin(pathToDescriptor, std::fstream::in);
    if (fin.good()) {
        std::getline(fin, tilesFolder);
        std::string name;
        fin >> name >> tsetMin.x >> tsetMin.y;
        fin >> name >> tsetMax.x >> tsetMax.y;
        fin >> name >> tileRes.x >> tileRes.y;
        fin >> name >> ppTile.x >> ppTile.y;
        fin >> name >> hNoValue;
        fin >> name >> hSeaValue >> hSeaLevel;
        tsetExtension = tsetMax - tsetMin;
        tileExtension = glm::vec2(ppTile)*tileRes;
        numTiles = glm::ivec2(glm::ceil(tsetExtension/tileExtension));

        loadBuffer = new float[ppTile.x*ppTile.y];
    }
    fin.close();
}

HeightsTileset::~HeightsTileset()
{
    delete[] loadBuffer;
}


std::vector<std::vector<float> > HeightsTileset::readTile(int ti, int tj, const glm::ivec2 &outFactor)
{    
    std::ostringstream oss;
    oss << tilesFolder << "tile_";
    oss << std::setw(2) << std::setfill('0') << ti << "_";
    oss << std::setw(2) << std::setfill('0') << tj << ".bin";
    std::string tilePath = oss.str();

    unsigned int sx, sy;
    bool loadError = false;
    std::fstream fin(tilePath, std::fstream::in | std::fstream::binary);
    if (fin.good()) {
        fin.read((char*)(&sx), sizeof(unsigned int));
        fin.read((char*)(&sy), sizeof(unsigned int));
        fin.read((char*)(loadBuffer), sx*sy*sizeof(float));
        fin.close();
    }
    else {
        sx = int(glm::round(tileExtension.x/tileRes.x));
        sy = int(glm::round(tileExtension.y/tileRes.y));
        loadError = true;
    }

    glm::ivec2 osize = glm::ivec2(sx, sy)/outFactor;
    std::vector<std::vector<float> > H(osize.x, std::vector<float>(osize.y, hNoValue));

    if (loadError) {
        std::cerr << "Error loading " << tilePath << std::endl;
        return H;
    }

    for (int i = 0; i < osize.x; i++) {
        for (int j = 0; j < osize.y; j++) {
            float sumH = 0;
            int numH = 0;
            for (int ii = 0; ii < outFactor.x; ii++) {
                for (int jj = 0; jj < outFactor.y; jj++) {
                    if (i*outFactor.x + ii < int(sx) && j*outFactor.y + jj < int(sy)) {
                        float val = loadBuffer[(i*outFactor.x + ii)*sy + j*outFactor.y + jj];
                        if (val > hNoValue) {          // ignore no values
                            if (val <= hSeaValue) {    // replace sea values with desired sea level
                                val = hSeaLevel;
                            }
                            sumH += val;
                            numH++;
                        }
                    }
                }
            }
            if (numH > 0) {
                H[i][j] = sumH/float(numH);
            }
            else {
                H[i][j] = hSeaLevel;
            }
        }
    }
    return H;
}


HeightsGrid* HeightsTileset::loadRegion(const glm::vec2& dtmMin, const glm::vec2& dtmMax, const glm::vec2& outRes)
{
    glm::vec2 regionMin = dtmMin;//glm::max(dtmMin, tsetMin);
    glm::vec2 regionMax = dtmMax;//glm::min(dtmMax, tsetMax);
    glm::vec2  tileExtension = glm::vec2(ppTile)*tileRes;
    glm::ivec2 tileIni = glm::ivec2((regionMin - tsetMin)/tileExtension);
    glm::ivec2 tileEnd = glm::ivec2((regionMax - tsetMin)/tileExtension);

    glm::ivec2 reduceFactor = glm::ivec2(glm::round(outRes/tileRes));
    glm::ivec2 numPoints = glm::ivec2(glm::ceil((regionMax - regionMin)/outRes));
    glm::ivec2 outPPtile = ppTile/reduceFactor;

    std::vector<std::vector<float> > H(numPoints.x, std::vector<float>(numPoints.y, hNoValue));

    for (int ti = tileIni.x; ti <= tileEnd.x; ti++) {
        for (int tj = tileIni.y; tj <= tileEnd.y; tj++) {

            glm::vec2 tmin = tsetMin + glm::vec2(float(ti), float(tj))*tileExtension;
            std::vector<std::vector<float> > T = readTile(ti, tj, reduceFactor);

            for (int ii = 0; ii < outPPtile.x; ii++) {
                for (int jj = 0; jj < outPPtile.y; jj++) {
                    glm::vec2 p = tmin + glm::vec2(float(jj), float(outPPtile.x - 1 - ii))*outRes;
                    if (p.x >= regionMin.x && p.x < regionMax.x && p.y >= regionMin.y && p.y < regionMax.y) {
                        glm::ivec2 coords = glm::ivec2(glm::floor((p - regionMin)/outRes));
                        glm::clamp(coords, glm::ivec2(0,0), glm::ivec2(numPoints.x, numPoints.y));
                        H[coords.x][coords.y] = T[ii][jj];
                    }
                }
            }
        }
    }

    HeightsGrid* grid = new HeightsGrid(H, regionMin, regionMax, outRes, hNoValue);
    return grid;
}
