#ifndef TERRAINVIEWER_H
#define TERRAINVIEWER_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

class TerrainViewer : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    TerrainViewer (QWidget *parent=0);
    ~TerrainViewer ();

    void loadTerrain(const std::string& path);
    void setRegion(const glm::vec2& rmin, const glm::vec2& rmax);

    void showRegion(bool);
    void setSeaLevel(float);
    void doSelectPoint();
    glm::vec2 getSelectedPoint() const;

public slots:
    void setViewX(double);
    void setViewY(double);
    void setViewRadius(double);

    void setPaletteMin(double);
    void setPaletteMax(double);
    void setPaletteModeColor(bool);
    void setPaletteModeGray(bool);
    void setPaletteModeUniform(bool);

    void enableShading(bool);
    void setShadingAngle(int);

signals:
    void changedViewX(double);
    void changedViewY(double);
    void changedViewRadius(double);
    void pointSelected();

protected:
    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int width, int height);
    void updateCamera();

    virtual void keyPressEvent (QKeyEvent *event);
    virtual void mousePressEvent (QMouseEvent *event);
    virtual void mouseReleaseEvent (QMouseEvent *event);
    virtual void mouseMoveEvent (QMouseEvent *event);
    virtual void wheelEvent(QWheelEvent *event);

private:
    QOpenGLShaderProgram*     program;
    QOpenGLVertexArrayObject* dtmVAO;
    GLuint bufPos, bufIndex;
    GLuint texPaletteColor, texPaletteGray, texPaletteUniform;
    unsigned int numPoints, numTriangles;

    glm::vec3 boxMin, boxMax;
    glm::vec3 camCtr;
    float camRadius, camDist;
    glm::mat4 camProj, camView;
    glm::vec3 lightDir;

    glm::vec2 regionMin, regionMax;
    bool showingRegion;
    float seaLevel;
    float paletteMin, paletteMax;
    GLuint currPaletteTex;
    bool shadingEnabled;

    typedef enum {NONE, ROTATE, PAN, ZOOM, SELECT} InteractiveAction;
    InteractiveAction interaction;
    int xClick, yClick;
    glm::vec2 selectedPoint;
};

inline void TerrainViewer::setRegion(const glm::vec2 &rmin, const glm::vec2 &rmax)
{
    regionMin = rmin;
    regionMax = rmax;
    update();
}

inline void TerrainViewer::showRegion(bool b)
{
    showingRegion = b;
    update();
}

inline void TerrainViewer::setSeaLevel(float f)
{
    seaLevel = f;
    update();
}

inline void TerrainViewer::doSelectPoint()
{
    interaction = SELECT;
}

inline glm::vec2 TerrainViewer::getSelectedPoint() const
{
    return selectedPoint;
}


#endif // TERRAINVIEWER_H
