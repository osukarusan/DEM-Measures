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
    ui->elvXmin->setValue(gridMin.x);
    ui->elvXmax->setMinimum(gridMin.x);
    ui->elvXmax->setMaximum(gridMax.x);
    ui->elvXmax->setValue(gridMax.x);
    ui->elvYmin->setMinimum(gridMin.y);
    ui->elvYmin->setMaximum(gridMax.y);
    ui->elvYmin->setValue(gridMin.y);
    ui->elvYmax->setMinimum(gridMin.y);
    ui->elvYmax->setMaximum(gridMax.y);
    ui->elvYmax->setValue(gridMax.y);
    ui->elvResolution->setMinimum(glm::max(gridRes.x, gridRes.y));
    ui->elvResolution->setSingleStep(glm::max(gridRes.x, gridRes.y));
	ui->queryIsolMinIsoArea->setSingleStep(gridRes.x*gridRes.y);
    emitUpdatedRegion();

    ui->viewX->setMinimum(gridMin.x);
    ui->viewX->setMaximum(gridMax.x);
    ui->viewY->setMinimum(gridMin.y);
    ui->viewY->setMaximum(gridMax.y);
    ui->viewRadius->setMaximum(glm::max(gridMax.x - gridMin.x, gridMax.y - gridMin.y));
	ui->queryStatsX->setValue(0.5*(gridMin.x + gridMax.x));
	ui->queryStatsY->setValue(0.5*(gridMin.y + gridMax.y));
	ui->queryStatsX->setMinimum(gridMin.x);
	ui->queryStatsX->setMaximum(gridMax.x);
	ui->queryStatsY->setMinimum(gridMin.y);
	ui->queryStatsY->setMaximum(gridMax.y);
	ui->queryIsolX->setValue(0.5*(gridMin.x + gridMax.x));
	ui->queryIsolY->setValue(0.5*(gridMin.y + gridMax.y));
	ui->queryIsolX->setMinimum(gridMin.x);
	ui->queryIsolX->setMaximum(gridMax.x);
	ui->queryIsolY->setMinimum(gridMin.y);
	ui->queryIsolY->setMaximum(gridMax.y);
	ui->queryOrsX->setValue(0.5*(gridMin.x + gridMax.x));
	ui->queryOrsY->setValue(0.5*(gridMin.y + gridMax.y));
	ui->queryOrsX->setMinimum(gridMin.x);
	ui->queryOrsX->setMaximum(gridMax.x);
	ui->queryOrsY->setMinimum(gridMin.y);
	ui->queryOrsY->setMaximum(gridMax.y);

	ui->buttonExportRegionORS->setEnabled(false);

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
		float elevScale = ui->elvScaleFactor->value();

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
                write_short(fout, short(elevScale*grid->data()[x][y] + 0.5));
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
            fout << grid->data()[0][gridPoints.y - 1 - y];
            for (int x = 1; x < gridPoints.x; x++) {
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
    glm::vec2 pmin = p - glm::vec2(rad, rad);
    glm::vec2 pmax = p + glm::vec2(rad, rad);

    HeightsGrid* gridArea = tileset->loadRegion(pmin, pmax, tileset->getTileRes());

    this->ui->statusBar->showMessage("Calculant estadístiques...");
    float hmean, hstdev;
    glm::vec3 hmin, hmax;
    gridArea->computeRadialStatistics(p, rad, hmin, hmax, hmean, hstdev);

    QString txt;
    ui->lineQstatsResMinX->setText(txt.sprintf("%.1f", hmin.x));
    ui->lineQstatsResMinY->setText(txt.sprintf("%.1f", hmin.y));
    ui->lineQstatsResMin->setText(txt.sprintf("%.1f", hmin.z));
    ui->lineQstatsResMaxX->setText(txt.sprintf("%.1f", hmax.x));
    ui->lineQstatsResMaxY->setText(txt.sprintf("%.1f", hmax.y));
    ui->lineQstatsResMax->setText(txt.sprintf("%.1f", hmax.z));
    ui->lineQstatsResMean->setText(txt.sprintf("%.1f", hmean));
    ui->lineQstatsResStdev->setText(txt.sprintf("%.1f", hstdev));

    delete gridArea;
    this->ui->statusBar->showMessage("Completat!", 5000);
    this->ui->tabWidget->setEnabled(true);
}


void MainWindow::computePointIsolation()
{
	this->ui->tabWidget->setEnabled(false);

	this->ui->statusBar->showMessage("Carregant tiles...");
	glm::vec2 p(float(ui->queryIsolX->value()), float(ui->queryIsolY->value()));
	float refRadius = ui->queryIsolRadSummit->value();
	float minIsoArea = ui->queryIsolMinIsoArea->value();
	float minHeightOff = ui->queryIsolMinHeightDiff->value();
	float gridRad = ui->queryIsolMaxGrid->value()*1000.0f;
	glm::vec2 pmin = p - glm::vec2(gridRad, gridRad);
	glm::vec2 pmax = p + glm::vec2(gridRad, gridRad);

	HeightsGrid* gridArea = tileset->loadRegion(pmin, pmax, tileset->getTileRes());

	this->ui->statusBar->showMessage("Calculant aïllament...");

	// find peak
	glm::vec3 hmin, hmax;
	float hmean, hdev;
	gridArea->computeRadialStatistics(p, refRadius, hmin, hmax, hmean, hdev);
	glm::vec3 peak = hmax;

	glm::vec3 pIso;
	float dIso = gridArea->computeIsolation(peak, refRadius, pIso, minIsoArea, minHeightOff);

	QString txt;
	ui->lineQisolResPeak->setText(txt.sprintf("%.1f", peak.z));
	ui->lineQisolResPeakX->setText(txt.sprintf("%.1f", peak.x));
	ui->lineQisolResPeakY->setText(txt.sprintf("%.1f", peak.y));
	ui->lineQisolResIsoDist->setText(txt.sprintf("%.1f", dIso));
	ui->lineQisolResIsoX->setText(txt.sprintf("%.1f", pIso.x));
	ui->lineQisolResIsoY->setText(txt.sprintf("%.1f", pIso.y));

	delete gridArea;
	this->ui->statusBar->showMessage("Completat!", 5000);
	this->ui->tabWidget->setEnabled(true);
}


void MainWindow::computePointORS()
{
	this->ui->tabWidget->setEnabled(false);

	this->ui->statusBar->showMessage("Carregant tiles...");
	glm::vec2 p(float(ui->queryOrsX->value()), float(ui->queryOrsY->value()));
	float gridRad = ui->queryOrsRad->value();
	glm::vec2 pmin = p - glm::vec2(gridRad, gridRad);
	glm::vec2 pmax = p + glm::vec2(gridRad, gridRad);

	HeightsGrid* gridArea = tileset->loadRegion(pmin, pmax, tileset->getTileRes());

	this->ui->statusBar->showMessage("Calculant ORS...");

	float ors = gridArea->computeORS(p, gridRad);

	QString txt;
	ui->lineQorsValue->setText(txt.sprintf("%.2f", ors));

	delete gridArea;
	this->ui->statusBar->showMessage("Completat!", 5000);
	this->ui->tabWidget->setEnabled(true);
}

void MainWindow::computeRegionORS()
{
	float rad = ui->queryOrsRad->value();
	glm::vec2 pmin = glm::max(gridMin - glm::vec2(rad), tileset->getTilesetMin());
	glm::vec2 pmax = glm::min(gridMax + glm::vec2(rad), tileset->getTilesetMax());
	glm::ivec2 gridPoints = glm::ivec2(glm::ceil((gridMax - gridMin) / gridRes));
	if (gridPoints.x * gridPoints.y > 1000000000) {
		this->ui->statusBar->showMessage("ERROR: Regió massa gran per al càlcul d'ORS!");
		return;
	}

	this->ui->tabWidget->setEnabled(false);

	this->ui->statusBar->showMessage("Carregant tiles...");
	HeightsGrid* gridArea = tileset->loadRegion(pmin, pmax, tileset->getTileRes());

	QString txt;
	this->ui->statusBar->showMessage("Calculant ORS...");


	float maxOrs = 0;
	glm::vec2  pmaxOrs;
	float orsSum = 0;	
	orsGrid = std::vector<std::vector<float> >(gridPoints.x, std::vector<float>(gridPoints.y, 0));

	for (int i = 0; i < gridPoints.x; i++) {
		for (int j = 0; j < gridPoints.y; j++) {
			this->ui->statusBar->showMessage(txt.sprintf("Calculant ORS... %d de %d (%.1f%%)", 
				i*gridPoints.y + j + 1, gridPoints.x*gridPoints.y,  100*(i*gridPoints.y + j + 1)/float(gridPoints.x*gridPoints.y)));

			glm::vec2 p = gridMin + glm::vec2(i + 0.5, j + 0.5)*gridRes + glm::vec2(rad);
			
			float ors = gridArea->computeORS(p, rad);
			orsGrid[i][j] = ors;

			orsSum += ors;
			if (ors > maxOrs) {
				maxOrs = ors;
				pmaxOrs = p;
			}
		}
	}

	float orsMean = orsSum / float(gridPoints.x * gridPoints.y);
	ui->lineQorsResMax->setText(txt.sprintf("%.2f", maxOrs));
	ui->lineQorsResMaxX->setText(txt.sprintf("%.1f", pmaxOrs.x));
	ui->lineQorsResMaxY->setText(txt.sprintf("%.1f", pmaxOrs.y));
	ui->lineQorsResMean->setText(txt.sprintf("%.2f", orsMean));

	delete gridArea;
	this->ui->statusBar->showMessage("Completat!", 5000);
	this->ui->tabWidget->setEnabled(true);
	this->ui->buttonExportRegionORS->setEnabled(true);
}

void MainWindow::exportRegionORS()
{
	QString filename = QFileDialog::getSaveFileName(this, tr("Desar ORS com a matriu"), QString(), tr("DATA (*.data)"));
	if (!filename.isEmpty()) {
		ui->tabWidget->setEnabled(false);

		checkGrid();
		glm::ivec2 gridPoints = glm::vec2(orsGrid.size(), orsGrid[0].size());

		this->ui->statusBar->showMessage("Desant ORS...");
		std::ofstream fout(filename.toStdString(), std::fstream::out | std::fstream::trunc);
		for (int y = 0; y < gridPoints.y; y++) {
			fout << orsGrid[0][gridPoints.y - 1 - y];
			for (int x = 1; x < gridPoints.x; x++) {
				fout << " " << orsGrid[x][gridPoints.y - 1 - y];
			}
			fout << std::endl;
		}
		fout.close();

		this->ui->statusBar->showMessage("Completat!", 5000);
		ui->tabWidget->setEnabled(true);
	}
}


void MainWindow::computeListStats()
{
    const unsigned int NUM_RADII = 9;
    const float radii[NUM_RADII] = {50, 100, 200, 500, 1000, 2000, 5000, 10000, 25000};
    float refRadius = ui->queryStatsRadSummit->value();
    bool givenHeights = ui->checkListStatsWithHeights->isChecked();

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
                fout << "Max " << radii[ri];
				if (ri < NUM_RADII - 1) fout << ", ";
				else                    fout << std::endl;
            }
            fout.setf(std::ios_base::fixed, std::ios_base::floatfield);
            fout.precision(0);

            unsigned int pnum = 1;
            std::string line;
            while (std::getline(fin, line)) {
                this->ui->statusBar->showMessage("Processant punt #" + QString::number(pnum) + "...");

                std::istringstream iss(line);
                float px, py, pz;
                iss >> px >> py;
                if (givenHeights) iss >> pz;

                // load the biggest area (last radius assuming they are ordered)
                float rad = radii[NUM_RADII-1];
                glm::vec2 p(px, py);
                glm::vec2 pmin = p - glm::vec2(rad, rad);
                glm::vec2 pmax = p + glm::vec2(rad, rad);
                HeightsGrid* gridArea = tileset->loadRegion(pmin, pmax, tileset->getTileRes());

                // variables
                glm::vec3 hmin, hmax;
                glm::vec3 pref;
                float hmean, hdev;

                // get reference point
                if (givenHeights) {
                    pref = glm::vec3(px, py, pz);
                }
                else {
                    gridArea->computeRadialStatistics(p, refRadius, hmin, hmax, hmean, hdev);
                    pref = hmax;
                }
                fout << px << ", ";
                fout << py << ", ";
                fout << pref.x << ", ";
                fout << pref.y << ", ";
                fout << pref.z << ", ";

                // get radial queries
                for (unsigned int ri = 0; ri < NUM_RADII; ri++) {
                    gridArea->computeRadialStatistics(pref, radii[ri], hmin, hmax, hmean, hdev);
                    fout << hmean << ", ";
                    fout << hmin.z << ", ";
					fout << hmax.z;
					if (ri < NUM_RADII - 1) fout << ", ";
					else                    fout << std::endl;
                }

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


void MainWindow::computeListIsolation()
{
	const unsigned int NUM_HEIGHTS = 6;
	const float heightOffsets[NUM_HEIGHTS] = { 0, 1, 5, 10, 15, 25 };
	float refRadius = ui->queryIsolRadSummit->value();
	float minIsoArea = ui->queryIsolMinIsoArea->value();
	float gridRad = ui->queryIsolMaxGrid->value()*1000.0f;
	bool givenHeights = ui->checkListIsolWithHeights->isChecked();

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
			fout << "Aillament" << ", ";
			fout << "X aill" << ", ";
			fout << "Y aill" << ", ";
			for (unsigned int hi = 0; hi < NUM_HEIGHTS; hi++) {
				fout << "Aillament net (+" << heightOffsets[hi] << "m), ";
				fout << "X aill net (+" << heightOffsets[hi] << "m), ";
				fout << "Y aill net (+" << heightOffsets[hi] << "m)";
				if (hi < NUM_HEIGHTS-1) fout << ", ";
				else                    fout << std::endl;
			}

			fout.setf(std::ios_base::fixed, std::ios_base::floatfield);
			fout.precision(0);

			unsigned int pnum = 1;
			std::string line;
			while (std::getline(fin, line)) {
				this->ui->statusBar->showMessage("Processant punt #" + QString::number(pnum) + "...");

				std::istringstream iss(line);
				float px, py, pz;
				iss >> px >> py;
				if (givenHeights) iss >> pz;

				// load the grid search area
				glm::vec2 p(px, py);
				glm::vec2 pmin = p - glm::vec2(gridRad, gridRad);
				glm::vec2 pmax = p + glm::vec2(gridRad, gridRad);
				HeightsGrid* gridArea = tileset->loadRegion(pmin, pmax, tileset->getTileRes());

				// variables
				glm::vec3 hmin, hmax;
				glm::vec3 pref;
				float hmean, hdev;

				// get reference point
				if (givenHeights) {
					pref = glm::vec3(px, py, pz);
				}
				else {
					gridArea->computeRadialStatistics(p, refRadius, hmin, hmax, hmean, hdev);
					pref = hmax;
				}
				fout << px << ", ";
				fout << py << ", ";
				fout << pref.x << ", ";
				fout << pref.y << ", ";
				fout << pref.z << ", ";

				// get isolation
				glm::vec3 pIso;
				float isolation = gridArea->computeIsolation(pref, refRadius, pIso, 0, 0);
				fout << isolation << ",";
				fout << pIso.x << ", ";
				fout << pIso.y << ", ";

				// get clean isolations (TODO! do it in one query)
				for (unsigned int hi = 0; hi < NUM_HEIGHTS; hi++) {
					float cleanIso = gridArea->computeIsolation(pref, refRadius, pIso, minIsoArea, heightOffsets[hi]);
					fout << cleanIso << ",";
					fout << pIso.x << ", ";
					fout << pIso.y;
					if (hi < NUM_HEIGHTS - 1) fout << ", ";
					else                    fout << std::endl;
				}

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


void MainWindow::computeListORS()
{
	float refRadius = ui->queryOrsRad->value();
	bool givenHeights = ui->checkListStatsWithHeights->isChecked();

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
			fout << "Altitud" << ", ";
			fout << "ORS" << std::endl;

			fout.setf(std::ios_base::fixed, std::ios_base::floatfield);
			fout.precision(0);

			unsigned int pnum = 1;
			std::string line;
			while (std::getline(fin, line)) {
				this->ui->statusBar->showMessage("Processant punt #" + QString::number(pnum) + "...");

				std::istringstream iss(line);
				float px, py, pz;
				iss >> px >> py;
				if (givenHeights) iss >> pz;

				// load the biggest area (last radius assuming they are ordered)
				float rad = refRadius;
				glm::vec2 p(px, py);
				glm::vec2 pmin = p - glm::vec2(rad, rad);
				glm::vec2 pmax = p + glm::vec2(rad, rad);
				HeightsGrid* gridArea = tileset->loadRegion(pmin, pmax, tileset->getTileRes());

				// variables
				glm::vec3 pref;

				// get reference point
				if (givenHeights) {
					pref = glm::vec3(px, py, pz);
				}
				else {					
					pref = glm::vec3(px, py, gridArea->getHeight(p));
				}
				fout << px << ", ";
				fout << py << ", ";
				fout << pref.x << ", ";
				fout << pref.y << ", ";
				fout << pref.z << ", ";

				// get ors
				float ors = gridArea->computeORS(pref, rad);
				fout << ors << std::endl;

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
	ui->buttonClickPointStats->setEnabled(false);
	ui->buttonCalcPointIsol->setEnabled(false);
    ui->glWidget->doSelectPoint();
}

void MainWindow::pointSelected()
{
	ui->buttonClickPointStats->setEnabled(true);
	ui->buttonCalcPointIsol->setEnabled(true);
    glm::vec2 p = ui->glWidget->getSelectedPoint();
	ui->queryStatsX->setValue(p.x);
	ui->queryStatsY->setValue(p.y);
	ui->queryIsolX->setValue(p.x);
	ui->queryIsolY->setValue(p.y);
	ui->queryOrsX->setValue(p.x);
	ui->queryOrsY->setValue(p.y);
}

void MainWindow::centerViewToRadialStats()
{
    ui->viewX->setValue(ui->queryStatsX->value());
    ui->viewY->setValue(ui->queryStatsY->value());
    ui->viewRadius->setValue(ui->queryStatsRad->value());
}

void MainWindow::centerViewToIsolation()
{
	ui->viewX->setValue(ui->queryIsolX->value());
	ui->viewY->setValue(ui->queryIsolY->value());
	ui->viewRadius->setValue(ui->queryIsolMaxGrid->value());
}

void MainWindow::centerViewToORS()
{
	ui->viewX->setValue(ui->queryOrsX->value());
	ui->viewY->setValue(ui->queryOrsY->value());
	ui->viewRadius->setValue(ui->queryOrsRad->value());
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
	ui->buttonExportRegionORS->setEnabled(false);
}
