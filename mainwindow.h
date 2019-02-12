#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "heightstileset.h"
#include "heightsgrid.h"
#include "glm/glm.hpp"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    // grid
    void setGridXmin(double);
    void setGridYmin(double);
    void setGridXmax(double);
    void setGridYmax(double);
    void setGridResolution(double);
    void saveGridELV();
    void saveGridPLY();
    void saveGridDATA();

    // height radial stats
	void computeRadialStats();
	void computeListStats();

	// isolations
	void computePointIsolation();
	void computeListIsolation();

	// ORS
	void computePointORS();
	void computeRegionORS();
	void computeListORS();
	void exportRegionORS();

	// point selection
	void selectPoint();
	void pointSelected();
	void centerViewToRadialStats();
	void centerViewToIsolation();
	void centerViewToORS();

    // render
    void toggleShowRegion(bool);
    void setSeaLevel(double);
    void setSeaLevel(int);

signals:
    // grid
    void changedGridWidth(const QString&);
    void changedGridHeight(const QString&);
    void changedGridPoints(const QString&);
    void changedGridMB(const QString&);

private:
    void checkGrid();
    void emitUpdatedRegion();

private:
    Ui::MainWindow *ui;

    HeightsTileset* tileset;
    HeightsGrid* grid;
    glm::vec2 gridMin, gridMax, gridRes;
    bool dirtyGrid;

	std::vector<std::vector<float> > orsGrid;
};

#endif // MAINWINDOW_H
