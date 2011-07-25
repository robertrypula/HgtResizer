/*
 *   -------------------------------------------------------------------------
 *    HgtResizer v1.0
 *                                                    (c) Robert Rypula 156520
 *                                   Wroclaw University of Technology - Poland
 *                                                      http://www.pwr.wroc.pl
 *                                                           2011.01 - 2011.06
 *   -------------------------------------------------------------------------
 *
 *   What is this:
 *     - SRTM data resizer from 92.77m to 101.92m grid for more flexible LOD
 *       division in HgtReader program
 *     - html HGT index generator with jpg terrain presentation
 *     - part of my thesis "Rendering of complex 3D scenes"
 *
 *   What it use:
 *     - Nokia Qt cross-platform C++ application framework
 *     - NASA SRTM terrain elevation data:
 *         oryginal dataset
 *           http://dds.cr.usgs.gov/srtm/version2_1/SRTM3/
 *         corrected part of earth:
 *           http://www.viewfinderpanoramas.org/dem3.html
 *         SRTM v4 highest quality SRTM dataset avaiable:
 *           http://srtm.csi.cgiar.org/
 *     - ALGLIB cross-platform numerical analysis and data processing library
 *         ALGLIB website:
 *           http://www.alglib.net/
 *
 *   Contact to author:
 *            phone    +48 505-363-331
 *            e-mail   robert.rypula@gmail.com
 *            GG       1578139
 *
 *                                                   program under GNU licence
 *   -------------------------------------------------------------------------
 */

#include <iostream>
#include <QtCore/QCoreApplication>
#include <QDebug>
#include "CResizer.h"

using namespace std;

void executeTask(CResizer *resizer)
{
    double lon = -1.0, lat = -1.0;
    bool createImg = true;
    int createImgInt, entireEarthInt;
    int choose;

    cout << "-------------------------------------------------------" << endl;
    cout << " HgtResizer v1.0                                       " << endl;
    cout << "                                     (c) Robert Rypula " << endl;
    cout << "             Wroclaw University of Technology - Poland " << endl;
    cout << "                                http://www.pwr.wroc.pl " << endl;
    cout << "                                       2011.01-2011.06 " << endl;
    cout << "-------------------------------------------------------" << endl;
    cout << endl;
    cout << "Choose task:" << endl;
    cout << "  1. buildL09_L13TerrainFromSRTMEntireEarth();" << endl;
    cout << "  2. buildL09_L13TerrainFromSRTM(lon, lat);" << endl;
    cout << "  3. buildL04_L08TerrainFromL09_L13EntireEarth();" << endl;
    cout << "  4. buildL04_L08TerrainFromL09_L13(lon, lat);" << endl;
    cout << "  5. buildL00_L03TerrainFromL04_L08EntireEarth();" << endl;
    cout << "  6. buildL00_L03TerrainFromL04_L08(lon, lat);" << endl;
    cout << "  7. connectL09_L13TerrainEntireEarth();" << endl;
    cout << "  8. connectL09_L13Terrain(lon, lat);" << endl;
    cout << "  9. generateHtmlIndex(HGT_SOURCE_L00_L03);" << endl;
    cout << " 10. generateHtmlIndex(HGT_SOURCE_L04_L08);" << endl;
    cout << " 11. generateHtmlIndex(HGT_SOURCE_L09_L13);" << endl;
    cout << " 12. generateHtmlIndex(HGT_SOURCE_SRTM);" << endl;
    cout << endl;
    cout << " Your choose: ";
    cin >> choose;
    cout << endl << endl;
    cout << "----------------------------------------" << endl << endl;

    if (choose==2 || choose==4 || choose==6 || choose==8) {
        cout << "Longitude: "; cin >> lon;
        cout << "Latitude: "; cin >> lat;
    }

    if (choose>=9 && choose<=12) {
        cout << "Only HTML index file (0=no, 1=yes)? "; cin >> createImgInt;
        if (createImgInt) {
            createImg = false;
            lon = -1.0;
            lat = -1.0;
        } else {
            createImg = true;

            cout << "Images on entire earth (0=no, 1=yes)? "; cin >> entireEarthInt;
            if (entireEarthInt) {
                lon = -1.0;
                lat = -1.0;
            } else {
                cout << "Longitude: "; cin >> lon;
                cout << "Latitude: "; cin >> lat;
            }
        }
    }

    cout << endl;
    cout << "----------------------------------------" << endl << endl;

    switch (choose) {
        case 1:resizer->buildL09_L13TerrainFromSRTMEntireEarth(); break;
        case 2:resizer->buildL09_L13TerrainFromSRTM(lon, lat); break;
        case 3:resizer->buildL04_L08TerrainFromL09_L13EntireEarth(); break;
        case 4:resizer->buildL04_L08TerrainFromL09_L13(lon, lat); break;
        case 5:resizer->buildL00_L03TerrainFromL04_L08EntireEarth(); break;
        case 6:resizer->buildL00_L03TerrainFromL04_L08(lon, lat); break;
        case 7:resizer->connectL09_L13TerrainEntireEarth(); break;
        case 8:resizer->connectL09_L13Terrain(lon, lat); break;
        case 9:resizer->generateHtmlIndex(HGT_SOURCE_L00_L03, createImg, lon, lat); break;
        case 10:resizer->generateHtmlIndex(HGT_SOURCE_L04_L08, createImg, lon, lat); break;
        case 11:resizer->generateHtmlIndex(HGT_SOURCE_L09_L13, createImg, lon, lat); break;
        case 12:resizer->generateHtmlIndex(HGT_SOURCE_SRTM, createImg, lon, lat); break;
    }
}

int main(int argc, char *argv[])
{
    int stop;
    QCoreApplication a(argc, argv);
    CResizer resizer;

    executeTask(&resizer);

    qDebug() << ""; qDebug() << "";
    qDebug() << "All task complete!";
    cin >> stop;
    return 0;
}
