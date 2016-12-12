#include "heightsgrid.h"
#include <cmath>
#include <queue>
#include <utility>


HeightsGrid::HeightsGrid(const std::vector<std::vector<float> >& grid,
                         const glm::vec2& gmin,
                         const glm::vec2& gmax,
                         const glm::vec2& gres,
                         float gridNoVal)
{
    this->grid = grid;
    this->gridMin = gmin;
    this->gridMax = gmax;
    this->gridRes = gres;
    this->gridSize = glm::ivec2((gmax - gmin)/gres);
    this->gridNoValue = gridNoVal;
    heightMin = heightMax = gridNoValue;
}

void HeightsGrid::buildTriangleModel(std::vector<glm::vec3> &verts, std::vector<glm::ivec3> &tris) const
{
    // build vertices
    verts.reserve(gridSize.x*gridSize.y);
    std::vector<std::vector<int> > vtxId(gridSize.x, std::vector<int>(gridSize.y, -1));
    for (int x = 0; x < gridSize.x; x++) {
        for (int y = 0; y < gridSize.y; y++) {
            if (grid[x][y] > gridNoValue) {
                glm::vec3 p(x*gridRes.x + gridMin.x, y*gridRes.y + gridMin.y, grid[x][y]);
                vtxId[x][y] = verts.size();
                verts.push_back(p);
            }
        }
    }

    // build triangles
    tris.reserve(2*verts.size());
    for (int x = 0; x < gridSize.x-1; x++) {
        for (int y = 0; y < gridSize.y-1; y++) {
            int v00 = vtxId[x][y];
            int v01 = vtxId[x][y+1];
            int v10 = vtxId[x+1][y];
            int v11 = vtxId[x+1][y+1];
            if (v00 >= 0 && v01 >= 0 && v10 >= 0 && v11 >= 0) {
                tris.push_back(glm::ivec3(v00, v10, v01));
                tris.push_back(glm::ivec3(v01, v10, v11));
            }
            else if (v00 >= 0 && v10 >= 0 && v11 >= 0) {
                tris.push_back(glm::ivec3(v00, v10, v11));
            }
            else if (v00 >= 0 && v11 >= 0 && v01 >= 0) {
                tris.push_back(glm::ivec3(v00, v11, v01));
            }
            else if (v00 >= 0 && v10 >= 0 && v01 >= 0) {
                tris.push_back(glm::ivec3(v00, v10, v01));
            }
            else if (v01 >= 0 && v10 >= 0 && v11 >= 0) {
                tris.push_back(glm::ivec3(v01, v10, v11));
            }
        }
    }
}

float HeightsGrid::getHeightMin() {
    if (heightMin <= gridNoValue) {
        heightMin = grid[0][0];
        for (int i = 0; i < gridSize.x; i++) {
            for (int j = 0; j < gridSize.y; j++) {
                heightMin = glm::min(heightMin, grid[i][j]);
            }
        }
    }
    return heightMin;
}

float HeightsGrid::getHeightMax() {
    if (heightMax <= gridNoValue) {
        heightMax = grid[0][0];
        for (int i = 0; i < gridSize.x; i++) {
            for (int j = 0; j < gridSize.y; j++) {
                heightMax = glm::max(heightMax, grid[i][j]);
            }
        }
    }
    return heightMax;
}

void HeightsGrid::getRadialStatistics(const glm::vec2 &p, float rad, glm::vec3 &hmin, glm::vec3 &hmax, float &hmean, float &hdev) const
{
    float hsum = 0;
    float ssum = 0;
    int N = 0;

    glm::ivec2 pcoords = glm::ivec2((p - gridMin)/gridRes);
    glm::ivec2 radOff = glm::ivec2(glm::ceil(glm::vec2(rad)/gridRes));
    glm::ivec2 ijMin = glm::max(pcoords - radOff, glm::ivec2(0));
    glm::ivec2 ijMax = glm::min(pcoords + radOff, gridSize);
    hmin = hmax = glm::vec3(p.x, p.y, grid[pcoords.x][pcoords.y]);

    for (int i = ijMin.x; i < ijMax.x; i++) {
        for (int j = ijMin.x; j < ijMax.y; j++) {
            glm::vec2 pij = gridMin + glm::vec2(i + 0.5f, j + 0.5f)*gridRes;
            if (glm::distance(pij, p) <= rad && grid[i][j] >= 0) {
                float h = grid[i][j];
                if (h < hmin.z) {
                    hmin = glm::vec3(gridMin.x + i*gridRes.x, gridMin.y + j*gridRes.y, h);
                }
                if (h > hmax.z) {
                    hmax = glm::vec3(gridMin.x + i*gridRes.x, gridMin.y + j*gridRes.y, h);
                }
                hsum += h;
                ssum += h*h;
                N++;
            }
        }
    }

    hmean = hsum/float(N);
    hdev = glm::sqrt((ssum - hsum*hsum/float(N))/float(N - 1));
}

float HeightsGrid::getIsolation(const glm::vec2 &p, float minDist, glm::vec3 &pIso)
{
    glm::ivec2 pcoords = glm::ivec2((p - gridMin)/gridRes);
    float ph = grid[pcoords.x][pcoords.y];
    pIso = glm::vec3(p.x, p.y, ph);

    std::priority_queue<std::pair<float, std::pair<int,int> > > Q;
    std::vector<std::vector<bool> > Visited(gridSize.x, std::vector<bool>(gridSize.y, false));
    Q.push(std::make_pair(0.0f, std::make_pair(pcoords.x, pcoords.y)));
    while (!Q.empty()) {
        std::pair<float, std::pair<int, int> > qtop = Q.top(); Q.pop();
        float pd = qtop.first;
        int pcx = qtop.second.first;
        int pcy = qtop.second.second;
        if (pcx >= 0 && pcy >= 0 && pcx < gridSize.x && pcy < gridSize.y && !Visited[pcx][pcy]) {
            Visited[pcx][pcy] = true;
            glm::vec2 pij = gridMin + glm::vec2(pcx + 0.5f, pcy + 0.5f)*gridRes;
            float d = glm::distance(pij, p);
            float h = grid[pcx][pcy];
            if (h > ph && d >= minDist) {
                pIso = glm::vec3(pij.x, pij.y, h);
                break;
            }

            Q.push(std::make_pair(pd - std::sqrt(2.0),  std::make_pair(pcx - 1, pcy - 1)));
            Q.push(std::make_pair(pd - 1,               std::make_pair(pcx - 1, pcy    )));
            Q.push(std::make_pair(pd - std::sqrt(2.0),  std::make_pair(pcx - 1, pcy + 1)));
            Q.push(std::make_pair(pd - 1,               std::make_pair(pcx,     pcy - 1)));
            Q.push(std::make_pair(pd - 1,               std::make_pair(pcx,     pcy + 1)));
            Q.push(std::make_pair(pd - std::sqrt(2.0),  std::make_pair(pcx + 1, pcy - 1)));
            Q.push(std::make_pair(pd - 1,               std::make_pair(pcx + 1, pcy    )));
            Q.push(std::make_pair(pd - std::sqrt(2.0),  std::make_pair(pcx + 1, pcy + 1)));
        }
    }

    /*
    float dIso = glm::distance(gridMin, gridMax) + 1;
    for (int i = 0; i < gridSize.x; i++) {
        for (int j = 0; j < gridSize.y; j++) {
            glm::vec2 pij = gridMin + glm::vec2(i + 0.5f, j + 0.5f)*gridRes;
            float d = glm::distance(pij, p);
            float h = grid[i][j];
            if (h > ph && d >= minDist && d < dIso) {
                pIso = glm::vec3(pij.x, pij.y, h);
                dIso = d;
            }
        }
    }
    */

    return glm::distance(p, glm::vec2(pIso));
}