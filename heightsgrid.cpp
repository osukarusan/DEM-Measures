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
                vtxId[x][y] = int(verts.size());
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
    glm::ivec2 pcoords = glm::ivec2((p - gridMin)/gridRes);
    float ph = grid[pcoords.x][pcoords.y];
    return getRadialStatistics(glm::vec3(p.x, p.y, ph), rad, hmin, hmax, hmean, hdev);
}

void HeightsGrid::getRadialStatistics(const glm::vec3 &p, float rad, glm::vec3 &hmin, glm::vec3 &hmax, float &hmean, float &hdev) const
{
    double hsum = 0;
    double ssum = 0;
    int N = 0;

    glm::vec2 p_xy = glm::vec2(p);
    glm::ivec2 pcoords = glm::ivec2((p_xy - gridMin)/gridRes);
    glm::ivec2 radOff = glm::ivec2(glm::ceil(glm::vec2(rad)/gridRes));
    glm::ivec2 ijMin = glm::max(pcoords - radOff, glm::ivec2(0));
    glm::ivec2 ijMax = glm::min(pcoords + radOff, gridSize);
    hmin = hmax = p;

    for (int i = ijMin.x; i < ijMax.x; i++) {
        for (int j = ijMin.x; j < ijMax.y; j++) {
            glm::vec2 pij = gridMin + glm::vec2(i + 0.5f, j + 0.5f)*gridRes;
            if (glm::distance(pij, p_xy) <= rad && grid[i][j] >= 0) {
                double h = static_cast<double>(grid[i][j]);
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

    hmean = float(hsum/double(N));
    hdev = float(glm::sqrt((ssum - hsum*hsum/double(N))/double(N - 1)));
}

float HeightsGrid::getIsolation(const glm::vec2 &p, float minDist, glm::vec3 &pIso, float minIsoArea, float hOffset) const
{
    glm::ivec2 pcoords = glm::ivec2((p - gridMin)/gridRes);
    float ph = grid[pcoords.x][pcoords.y];
    return getIsolation(glm::vec3(p.x, p.y, ph), minDist, pIso, minIsoArea, hOffset);
}

float HeightsGrid::getIsolation(const glm::vec3 &p, float minDist, glm::vec3 &pIso, float minIsoArea, float hOffset) const
{
    glm::vec2 p_xy = glm::vec2(p);
    glm::ivec2 pcoords = glm::ivec2((p_xy - gridMin)/gridRes);
    float ph = p.z + hOffset;
    pIso = p;
	float isoArea = 0;

    std::priority_queue<std::pair<float, std::pair<int,int> > > Q;
    std::vector<std::vector<bool> > Visited(gridSize.x, std::vector<bool>(gridSize.y, false));
    Q.push(std::make_pair(0.0f, std::make_pair(pcoords.x, pcoords.y)));
    while (!Q.empty()) {
        std::pair<float, std::pair<int, int> > qtop = Q.top(); Q.pop();
        int pcx = qtop.second.first;
        int pcy = qtop.second.second;
        if (pcx >= 0 && pcy >= 0 && pcx < gridSize.x && pcy < gridSize.y && !Visited[pcx][pcy]) {
            Visited[pcx][pcy] = true;
            float h = grid[pcx][pcy];
			glm::vec2 pij = gridMin + glm::vec2(pcx + 0.5f, pcy + 0.5f)*gridRes;
			float d = glm::distance(pij, p_xy);
            if (h > ph && d >= minDist) {
				// keep the last isolation point found
                pIso = glm::vec3(pij.x, pij.y, h);
				isoArea += gridRes.x * gridRes.y;
				if (isoArea > minIsoArea)
					break;
            }

            Q.push(std::make_pair(-d - std::sqrt(2.0f), std::make_pair(pcx - 1, pcy - 1)));
            Q.push(std::make_pair(-d - 1,               std::make_pair(pcx - 1, pcy    )));
            Q.push(std::make_pair(-d - std::sqrt(2.0f), std::make_pair(pcx - 1, pcy + 1)));
            Q.push(std::make_pair(-d - 1,               std::make_pair(pcx,     pcy - 1)));
            Q.push(std::make_pair(-d - 1,               std::make_pair(pcx,     pcy + 1)));
            Q.push(std::make_pair(-d - std::sqrt(2.0f), std::make_pair(pcx + 1, pcy - 1)));
            Q.push(std::make_pair(-d - 1,               std::make_pair(pcx + 1, pcy    )));
            Q.push(std::make_pair(-d - std::sqrt(2.0f), std::make_pair(pcx + 1, pcy + 1)));
        }
    }

    float dres = glm::distance(p_xy, glm::vec2(pIso));
    if (dres > minDist) return dres;
    else                return -1;
}

float HeightsGrid::getORS(const glm::vec2 &p, float radius) const
{
	glm::ivec2 pcoords = glm::ivec2((p - gridMin) / gridRes);
	float ph = grid[pcoords.x][pcoords.y];
	return getORS(glm::vec3(p.x, p.y, ph), radius);
}


inline double slopeNormalization(double u) {
	double atanu = atan(u);
	return (4.0 / pow(M_PI, 3))*(2*u*atanu - log(u*u + 1) - atanu*atanu);
}

float HeightsGrid::getORS(const glm::vec3 &p, float radius) const
{


	glm::vec2 p_xy = glm::vec2(p);
	double    h0 = p.z;
	glm::ivec2 pcoords = glm::ivec2((p_xy - gridMin) / gridRes);
	glm::ivec2 radOff = glm::ivec2(glm::ceil(glm::vec2(radius) / gridRes));
	glm::ivec2 ijMin = glm::max(pcoords - radOff, glm::ivec2(0));
	glm::ivec2 ijMax = glm::min(pcoords + radOff, gridSize);

	double dA = gridSize.x * gridSize.y;
	double intSum = 0;
	double areaSum = 0;

	for (int i = ijMin.x; i < ijMax.x; i++) {
		for (int j = ijMin.x; j < ijMax.y; j++) { 
			glm::vec2 pij = gridMin + glm::vec2(i + 0.5f, j + 0.5f)*gridRes;
			float pdist = glm::distance(pij, p_xy);
			if (pdist <= radius && grid[i][j] >= 0) {
				double h = static_cast<double>(grid[i][j]);

				// higher ground does not contribute
				if (h <= h0) {
					double y = h0 - h;
					double r = pdist;
					double f2 = slopeNormalization(y / r);
					intSum += f2*dA;
				}

				areaSum += dA;
			}
		}
	}
	
	double ors = sqrt((intSum/areaSum)/(2.0*M_PI));

	return float(ors);
}
