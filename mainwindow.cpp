#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <fstream>
#include <sstream>
#include "loaderply.h"
#include "utils.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->glWidget->showRegion(ui->checkShowRegion->isChecked());

    tileset = new HeightsTileset("catalunya.tiles");
    gridMin = tileset->getTilesetMin();
    gridMax = tileset->getTilesetMax();
    gridRes = tileset->getTileRes();
    ui->elvXmin->setMinimum(gridMin.x);
    ui->elvXmin->setMaximum(gridMax.x);
    ui->elvXmax->setMinimum(gridMin.x);
    ui->elvXmax->setMaximum(gridMax.x);
    ui->elvYmin->setMinimum(gridMin.y);
    ui->elvYmin->setMaximum(gridMax.y);
    ui->elvYmax->setMinimum(gridMin.y);
    ui->elvYmax->setMaximum(gridMax.y);
    ui->elvResolution->setMinimum(glm::max(gridRes.x, gridRes.y));
    ui->elvResolution->setSingleStep(glm::max(gridRes.x, gridRes.y));
    emitUpdatedRegion();

    ui->viewX->setMinimum(gridMin.x);
    ui->viewX->setMaximum(gridMax.x);
    ui->viewY->setMinimum(gridMin.y);
    ui->viewY->setMaximum(gridMax.y);
    ui->viewRadius->setMaximum(glm::max(gridMax.x - gridMin.x, gridMax.y - gridMin.y));
    ui->queryStatsX->setValue(0.5*(gridMin.x + gridMax.x));
    ui->queryStatsY->setValue(0.5*(gridMin.y + gridMax.y));

    grid = nullptr;
    dirtyGrid = true;
}

MainWindow::~MainWindow()
{
    if (grid) delete grid;
    delete tileset;
    delete ui;
}

void MainWindow::setGridXmin(double f)
{
    gridMin.x = float(f);
    dirtyGrid = true;
    emitUpdatedRegion();
}

void MainWindow::setGridYmin(double f)
{
    gridMin.y = float(f);
    dirtyGrid = true;
    emitUpdatedRegion();
}

void MainWindow::setGridXmax(double f)
{
    gridMax.x = float(f);
    dirtyGrid = true;
    emitUpdatedRegion();
}

void MainWindow::setGridYmax(double f)
{
    gridMax.y = float(f);
    dirtyGrid = true;
    emitUpdatedRegion();
}

void MainWindow::setGridResolution(double f)
{
    gridRes = glm::vec2(float(f), float(f));
    dirtyGrid = true;
    emitUpdatedRegion();
}

void MainWindow::saveGridELV()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Desar elevacions per a WinProm"), QString(), tr("ELV (*.elv)"));
    if (!filename.isEmpty()) {
        ui->tabWidget->setEnabled(false);
        checkGrid();
        glm::ivec2 gridPoints = grid->getGridSize();

        this->ui->statusBar->showMessage("Desant ELV...");
        std::ofstream fout(filename.toStdString(), std::fstream::out | std::fstream::trunc | std::fstream::binary);

        write_long(fout, grid->getGridMin().y);                     // min lat
        write_long(fout, grid->getGridMin().y + gridPoints.y - 1);  // max lat
        write_long(fout, grid->getGridMin().x);                     // min lon
        write_long(fout, grid->getGridMin().x + gridPoints.x - 1);  // max lon
        write_short(fout, grid->getGridNoValue());                  // default
        write_long(fout, 0);    // xdim, ydim
        write_long(fout, 0);    // winprom throws error if they are not both 0
        write_long(fout, 2);    // WinProm assumes equat grid (code 2)
        write_long(fout, grid->getGridRes().y);     // lat_step
        write_long(fout, grid->getGridRes().x);     // lon_step
        write_long(fout, gridPoints.y);
        write_long(fout, gridPoints.x);

        for (int y = 0; y < gridPoints.y; y++) {
            for (int x = 0; x < gridPoints.x; x++) {
                write_short(fout, short(10*grid->data()[x][y] + 0.5));
            }
        }

        fout.close();

        this->ui->statusBar->showMessage("Completat!", 5000);
        ui->tabWidget->setEnabled(true);
    }
}

void MainWindow::saveGridPLY()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Desar model 3D"), QString(), tr("PLY (*.ply)"));
    if (!filename.isEmpty()) {
        ui->tabWidget->setEnabled(false);
        checkGrid();

        this->ui->statusBar->showMessage("Construint model...");
        std::vector<glm::vec3> verts;
        std::vector<glm::ivec3> tris;
        try {
            grid->buildTriangleModel(verts, tris);

            this->ui->statusBar->showMessage("Desant PLY...");
            if (LoaderPLY::writePLY(filename.toStdString(), verts, tris)) {
                this->ui->statusBar->showMessage("Completat!", 5000);
            }
            else {
                this->ui->statusBar->showMessage("No s'ha pogut desar el model PLY");
            }
        } catch (std::bad_alloc&) {
            this->ui->statusBar->showMessage("El model és massa gran! Proveu amb una resolució major.");
        }

        ui->tabWidget->setEnabled(true);
    }
}

void MainWindow::saveGridDATA()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Desar elevacions com a matriu"), QString(), tr("DATA (*.data)"));
    if (!filename.isEmpty()) {
        ui->tabWidget->setEnabled(false);

        checkGrid();
        glm::ivec2 gridPoints = grid->getGridSize();

        this->ui->statusBar->showMessage("Desant DATA...");
        std::ofstream fout(filename.toStdString(), std::fstream::out | std::fstream::trunc);
        for (int y = 0; y < gridPoints.y; y++) {
            fout << grid->data()[0][y];
            for (int x = 0; x < gridPoints.x; x++) {
                fout << " " << grid->data()[x][gridPoints.y - 1 - y];
            }
            fout << std::endl;
        }
        fout.close();

        this->ui->statusBar->showMessage("Completat!", 5000);
        ui->tabWidget->setEnabled(true);
    }
}

void MainWindow::computeRadialStats()
{
    this->ui->tabWidget->setEnabled(false);

    this->ui->statusBar->showMessage("Carregant tiles...");
    glm::vec2 p(float(ui->queryStatsX->value()), float(ui->queryStatsY->value()));
    float rad = float(ui->queryStatsRad->value());
    float refRadius = ui->queryStatsRadMin->value();
    float gridRad = ui->queryStatsMaxGrid->value()*1000.0f;
    glm::vec2 pmin = p - glm::vec2(gridRad, gridRad);
    glm::vec2 pmax = p + glm::vec2(gridRad, gridRad);

    HeightsGrid* gridArea = tileset->loadRegion(pmin, pmax, tileset->getTileRes());

    this->ui->statusBar->showMessage("Calculant estadístiques...");
    float hmean, hstdev;
    glm::vec3 hmin, hmax;
    gridArea->getRadialStatistics(p, rad, hmin, hmax, hmean, hstdev);
    glm::vec3 pIso;
    float dIso = gridArea->getIsolation(p, refRadius, pIso);

    QString txt;
    ui->lineQstatsResMinX->setText(txt.sprintf("%.1f", hmin.x));
    ui->lineQstatsResMinY->setText(txt.sprintf("%.1f", hmin.y));
    ui->lineQstatsResMin->setText(txt.sprintf("%.1f", hmin.z));
    ui->lineQstatsResMaxX->setText(txt.sprintf("%.1f", hmax.x));
    ui->lineQstatsResMaxY->setText(txt.sprintf("%.1f", hmax.y));
    ui->lineQstatsResMax->setText(txt.sprintf("%.1f", hmax.z));
    ui->lineQstatsResMean->setText(txt.sprintf("%.1f", hmean));
    ui->lineQstatsResStdev->setText(txt.sprintf("%.1f", hstdev));
    ui->lineQstatsResIsoDist->setText(txt.sprintf("%.1f", dIso));
    ui->lineQstatsResIsoX->setText(txt.sprintf("%.1f", pIso.x));
    ui->lineQstatsResIsoY->setText(txt.sprintf("%.1f", pIso.y));

    delete gridArea;
    this->ui->statusBar->showMessage("Completat!", 5000);
    this->ui->tabWidget->setEnabled(true);
}

void MainWindow::computeListStats()
{
    unsigned int NUM_RADII = 5;
    const float radii[NUM_RADII] = {100, 200, 1000, 5000, 25000};
    float refRadius = ui->queryStatsRadMin->value();

    QString infile = QFileDialog::getOpenFileName(this, tr("Obrir llistat de punts"), QString(), tr("TXT (*.txt)"));
    if (!infile.isEmpty()) {

        QString filename = QFileDialog::getSaveFileName(this, tr("Desar mesures del llistat"), QString(), tr("CSV (*.csv)"));
        if (!filename.isEmpty()) {
            ui->tabWidget->setEnabled(false);

            std::fstream fin(infile.toStdString(), std::fstream::in);
            std::fstream fout(filename.toStdString(), std::fstream::out);

            fout << "X" << ", ";
            fout << "Y" << ", ";
            fout << "X ref" << ", ";
            fout << "Y ref" << ", ";
            fout << "Altitud ref" << ", ";
            for (unsigned int ri = 0; ri < NUM_RADII; ri++) {
                fout << "Mitja " << radii[ri] << ", ";
                fout << "Min " << radii[ri] << ", ";
                fout << "Max " << radii[ri] << ", ";
            }
            fout << "Aillament" << std::endl;
            fout.setf(std::ios_base::fixed, std::ios_base::floatfield);
            fout.precision(0);

            unsigned int pnum = 1;
            std::string line;
            while (std::getline(fin, line)) {
                this->ui->statusBar->showMessage("Processant punt #" + QString::number(pnum) + "...");

                std::istringstream iss(line);
                float px, py;
                iss >> px >> py;

                // load the biggest area (last radius assuming they are ordered)
                float rad = radii[NUM_RADII-1];
                glm::vec2 p(px, py);
                glm::vec2 pmin = p - glm::vec2(rad, rad);
                glm::vec2 pmax = p + glm::vec2(rad, rad);
                HeightsGrid* gridArea = tileset->loadRegion(pmin, pmax, tileset->getTileRes());

                // get reference point
                glm::vec3 hmin, hmax;
                float hmean, hdev;
                gridArea->getRadialStatistics(p, refRadius, hmin, hmax, hmean, hdev);
                glm::vec3 pref = hmax;
                fout << px << ", ";
                fout << py << ", ";
                fout << pref.x << ", ";
                fout << pref.y << ", ";
                fout << pref.z << ", ";

                // get radial queries
                for (unsigned int ri = 0; ri < NUM_RADII; ri++) {
                    gridArea->getRadialStatistics(glm::vec2(pref), radii[ri], hmin, hmax, hmean, hdev);
                    fout << hmean << ", ";
                    fout << hmin.z << ", ";
                    fout << hmax.z << ", ";
                }

                // get isolation
                glm::vec3 pIso;
                float isolation = gridArea->getIsolation(glm::vec2(pref), refRadius, pIso);
                fout << isolation << std::endl;

                delete gridArea;
                pnum++;
            }

            fout.close();
            fin.close();
            ui->tabWidget->setEnabled(true);
            this->ui->statusBar->showMessage("Completat!", 5000);
        }
    }
}

void MainWindow::selectPoint()
{
    ui->buttonClickPoint->setEnabled(false);
    ui->glWidget->doSelectPoint();
}

void MainWindow::pointSelected()
{
    ui->buttonClickPoint->setEnabled(true);
    glm::vec2 p = ui->glWidget->getSelectedPoint();
    ui->queryStatsX->setValue(p.x);
    ui->queryStatsY->setValue(p.y);
}

void MainWindow::centerViewToRadialStats()
{
    ui->viewX->setValue(ui->queryStatsX->value());
    ui->viewY->setValue(ui->queryStatsY->value());
    ui->viewRadius->setValue(ui->queryStatsRad->value());
}

void MainWindow::toggleShowRegion(bool b)
{
    ui->glWidget->showRegion(b);
}

void MainWindow::setSeaLevel(double v)
{
    ui->glWidget->setSeaLevel(float(v));
    ui->sliderSeaLevel->setValue(int(v));
}

void MainWindow::setSeaLevel(int v)
{
    ui->glWidget->setSeaLevel(float(v));
    ui->spinSeaLevel->setValue(double(v));
}

void MainWindow::checkGrid()
{
    if (dirtyGrid) {
        this->ui->statusBar->showMessage("Carregant tiles de la regió seleccionada...");
        if (grid) delete grid;
        grid = tileset->loadRegion(gridMin, gridMax, gridRes);
        dirtyGrid = false;
    }
}

void MainWindow::emitUpdatedRegion()
{
    glm::ivec2 gridPoints = glm::ivec2(glm::ceil((gridMax - gridMin)/gridRes));
    float megas = float(gridPoints.x)*float(gridPoints.y)*sizeof(float)/float(1024*1024);

    QString txt;
    emit changedGridWidth(txt.sprintf("%.1f km", (gridMax.x - gridMin.x)/1000.0f));
    emit changedGridHeight(txt.sprintf("%.1f km", (gridMax.y - gridMin.y)/1000.0f));
    emit changedGridPoints(txt.sprintf("%d x %d", gridPoints.x, gridPoints.y));
    emit changedGridMB(txt.sprintf("%.1f MB", megas));

    if (gridPoints.x <= 0 || gridPoints.y <= 0) {
        this->ui->buttonSaveELV->setEnabled(false);
        this->ui->buttonSavePLY->setEnabled(false);
        this->ui->buttonSaveDATA->setEnabled(false);
    }
    else {
        this->ui->buttonSaveELV->setEnabled(true);
        this->ui->buttonSavePLY->setEnabled(true);
        this->ui->buttonSaveDATA->setEnabled(true);
    }

    ui->glWidget->setRegion(gridMin, gridMax);
}
