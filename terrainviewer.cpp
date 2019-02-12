#include "terrainviewer.h"
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <glm/gtc/matrix_transform.hpp>
#include "loaderply.h"


TerrainViewer::TerrainViewer(QWidget *parent) : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::ClickFocus);
    program = nullptr;
    dtmVAO = nullptr;
    bufPos = bufIndex = texPaletteColor = 0;
    numPoints = numTriangles = 0;
    interaction = NONE;
	seaLevel = 0;
	selectedPoint = glm::vec2(0);
}

TerrainViewer::~TerrainViewer()
{
    if (program)    delete program;
    if (dtmVAO)     delete dtmVAO;
    if (bufPos)     glDeleteBuffers(1, &bufPos);
    if (bufIndex)   glDeleteBuffers(1, &bufIndex);
    if (texPaletteColor)    glDeleteTextures(1, &texPaletteColor);
    if (texPaletteGray)     glDeleteTextures(1, &texPaletteGray);
    if (texPaletteUniform)  glDeleteTextures(1, &texPaletteUniform);
}

void TerrainViewer::loadTerrain(const std::string &path)
{
    makeCurrent();

    if (bufPos)     glDeleteBuffers(1, &bufPos);
    if (bufIndex)   glDeleteBuffers(1, &bufIndex);

    std::vector<glm::vec3> verts;
    std::vector<glm::ivec3> tris;
    LoaderPLY::loadPLY(path, verts, tris);
    boxMin = verts[0];
    boxMax = verts[0];
    for (unsigned int i = 1; i < verts.size(); i++) {
        boxMin.x = glm::min(boxMin.x, verts[i].x);
        boxMin.y = glm::min(boxMin.y, verts[i].y);
        boxMin.z = glm::min(boxMin.z, verts[i].z);
        boxMax.x = glm::max(boxMax.x, verts[i].x);
        boxMax.y = glm::max(boxMax.y, verts[i].y);
        boxMax.z = glm::max(boxMax.z, verts[i].z);
    }
    camCtr = 0.5f*(boxMin + boxMax);
    camRadius = glm::round(glm::max(boxMax.x - camCtr.x, boxMax.y - camCtr.y)/1000.0f)*1000.0f;
    camDist = 2*boxMax.z;

    numPoints = static_cast<unsigned int>(verts.size());
    numTriangles = static_cast<unsigned int>(tris.size());

    program->bind();
    dtmVAO->bind();

    glGenBuffers(1, &bufPos);
    glBindBuffer(GL_ARRAY_BUFFER, bufPos);
    glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(glm::vec3),
                 reinterpret_cast<void*>(&verts[0].x), GL_STATIC_DRAW);
    glVertexAttribPointer(program->attributeLocation("vertex"), 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(program->attributeLocation("vertex"));

    glGenBuffers(1, &bufIndex);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIndex);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, tris.size()*sizeof(glm::ivec3),
                 reinterpret_cast<void*>(&tris[0][0]), GL_STATIC_DRAW);

    dtmVAO->release();
    program->release();
}

void TerrainViewer::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.5f, 0.7f, 1.0f, 1.0f);

    program = new QOpenGLShaderProgram(this);
    program->create();
    program->addShaderFromSourceFile(QOpenGLShader::Vertex, "data/terrain.vert");
    program->addShaderFromSourceFile(QOpenGLShader::Fragment, "data/terrain.frag");
    program->link();

    dtmVAO = new QOpenGLVertexArrayObject(this);
    dtmVAO->create();

    loadTerrain("data/dem.ply");

    QImage imgPal = QImage("data/palette.png");
    glGenTextures(1, &texPaletteColor);
    glBindTexture(GL_TEXTURE_1D, texPaletteColor);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, imgPal.width()*imgPal.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, imgPal.bits());
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_1D, 0);

    unsigned char graylevels[256*3];
    for (int i = 0; i < 256; i++) graylevels[3*i] = graylevels[3*i+1] = graylevels[3*i+2] = i;
    glGenTextures(1, &texPaletteGray);
    glBindTexture(GL_TEXTURE_1D, texPaletteGray);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, graylevels);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_1D, 0);

    glm::u8vec3 white(255);
    glGenTextures(1, &texPaletteUniform);
    glBindTexture(GL_TEXTURE_1D, texPaletteUniform);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, &white.x);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_1D, 0);


    currPaletteTex = texPaletteColor;
    paletteMin = 0;
    paletteMax = 3000;

    shadingEnabled = true;
    lightDir = glm::vec3(glm::cos(M_PI/4)*glm::cos(M_PI/3),
                         glm::sin(M_PI/4)*glm::cos(M_PI/3),
                         glm::sin(M_PI/3));
}

void TerrainViewer::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    program->bind();
    dtmVAO->bind();

    glUniformMatrix4fv(program->uniformLocation("ProjMatrix"), 1, GL_FALSE, &camProj[0][0]);
    glUniformMatrix4fv(program->uniformLocation("ViewMatrix"), 1, GL_FALSE, &camView[0][0]);
    glUniform1f(program->uniformLocation("minHeight"), paletteMin);
    glUniform1f(program->uniformLocation("maxHeight"), paletteMax);
    glUniform1f(program->uniformLocation("seaLevel"), seaLevel);
    glUniform1f(program->uniformLocation("regionAlpha"), showingRegion ? 0.3f : 0.0f);
    glUniform2fv(program->uniformLocation("regionMin"), 1, &regionMin.x);
    glUniform2fv(program->uniformLocation("regionMax"), 1, &regionMax.x);
    glUniform2fv(program->uniformLocation("selectedPoint"), 1, &selectedPoint.x);
    glUniform1f(program->uniformLocation("pointSize"), glm::min(camRadius/10.0f, 5000.0f));
    glUniform1ui(program->uniformLocation("heightPalette"), 0);
    glUniform3fv(program->uniformLocation("lightDir"), 1, &lightDir.x);
    glUniform1f(program->uniformLocation("shadingFactor"), shadingEnabled ? 0.75f : 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, currPaletteTex);

    glDrawElements(GL_TRIANGLES, numTriangles*3, GL_UNSIGNED_INT, 0);

    dtmVAO->release();
    program->release();
}

void TerrainViewer::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    updateCamera();
}

void TerrainViewer::updateCamera()
{
    float ar = float(width())/float(height());
    if (ar < 1) {
        camProj = glm::ortho(-camRadius, camRadius, -camRadius/ar, camRadius/ar, 0.5f*camDist, 1.5f*camDist);
    }
    else {
        camProj = glm::ortho(-camRadius*ar, camRadius*ar, -camRadius, camRadius, 0.5f*camDist, 1.5f*camDist);
    }

    camView = glm::lookAt(glm::vec3(camCtr.x, camCtr.y, camDist),
                          glm::vec3(camCtr.x, camCtr.y, 0),
                          glm::vec3(0, 1, 0));
    update();

    emit changedViewX(camCtr.x);
    emit changedViewY(camCtr.y);
    emit changedViewRadius(camRadius);
}


void TerrainViewer::setViewX(double d)
{
    camCtr.x = float(d);
    updateCamera();
}

void TerrainViewer::setViewY(double d)
{
    camCtr.y = float(d);
    updateCamera();
}

void TerrainViewer::setViewRadius(double d)
{
    camRadius = float(d);
    updateCamera();
}

void TerrainViewer::setPaletteMin(double d)
{
    paletteMin = float(d);
    update();
}

void TerrainViewer::setPaletteMax(double d)
{
    paletteMax = float(d);
    update();
}

void TerrainViewer::setPaletteModeColor(bool b)
{
    if (b) currPaletteTex = texPaletteColor;
    update();
}

void TerrainViewer::setPaletteModeGray(bool b)
{
    if (b) currPaletteTex = texPaletteGray;
    update();
}

void TerrainViewer::setPaletteModeUniform(bool b)
{
    if (b) currPaletteTex = texPaletteUniform;
    update();
}

void TerrainViewer::enableShading(bool b)
{
    shadingEnabled = b;
    update();
}

void TerrainViewer::setShadingAngle(int v)
{
    const float altitude = float(M_PI/3);
    float azimuth = M_PI*float(v - 90)/180.0f;
    lightDir = glm::vec3(glm::cos(azimuth)*glm::cos(altitude),
                         glm::sin(azimuth)*glm::cos(altitude),
                         glm::sin(altitude));
    update();
}

void TerrainViewer::keyPressEvent(QKeyEvent* e)
{
    switch (e->key()) {
        default:
            e->ignore();
            break;
    }
    update();
}

void TerrainViewer::mousePressEvent(QMouseEvent *e)
{
    xClick = e->x();
    yClick = e->y();
    if (interaction == SELECT) return;
    if (e->button()&Qt::LeftButton) {
        interaction = PAN;
    }
    if (e->button()&Qt::RightButton) {
        interaction = ZOOM;
    }
}

void TerrainViewer::mouseReleaseEvent(QMouseEvent *e)
{
    int mx = e->x();
    int my = e->y();
    if (interaction == SELECT) {
        glm::vec3 vwin(mx + 0.5f, height() - my - 0.5f, 1.0f);
        glm::vec4 viewport(0, 0, width(), height());
        glm::vec3 vmap = glm::unProject(vwin, camView, camProj, viewport);
        selectedPoint = glm::vec2(vmap.x, vmap.y);
        emit pointSelected();
        update();
    }

    interaction = NONE;
}

void TerrainViewer::mouseMoveEvent(QMouseEvent *e)
{
    const float PAN_FACTOR = 500.0f;
    const float ZOOM_FACTOR = 500.0f;
    const float MIN_RADIUS = 10000;

    float radius0 = glm::max(boxMax.x - camCtr.x, boxMax.y - camCtr.y);

    if (interaction == PAN) {
        int dx = e->x() - xClick;
        int dy = e->y() - yClick;
        camCtr += PAN_FACTOR*(camRadius/radius0)*glm::vec3(-dx, dy, 0);
        updateCamera();
    }
    else if (interaction == ZOOM) {
        int dy = e->y() - yClick;
        camRadius += ZOOM_FACTOR*dy;
        if (camRadius < MIN_RADIUS) camRadius = MIN_RADIUS;
        updateCamera();
    }

    xClick = e->x();
    yClick = e->y();
}

void TerrainViewer::wheelEvent(QWheelEvent *e)
{
    const float MIN_RADIUS = 10000;
    const float ZOOM_FACTOR = 50.0f;

    QPoint degrees = e->angleDelta();
    camRadius -= ZOOM_FACTOR*degrees.y();
    if (camRadius < MIN_RADIUS) camRadius = MIN_RADIUS;
    updateCamera();
}
