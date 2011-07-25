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

#include <fstream>
#include <iostream>
#include <QDebug>
#include <QImage>
#include <QColor>
#include "CResizer.h"
#include "CHgtFile.h"
#include "alglib/interpolation.h"

using namespace std;

CResizer::CResizer()
{
}

bool CResizer::findSRTMFilesFor_L09_L13(const double &L09_L13_topLeftLon, const double &L09_L13_topLeftLat,
                                        int *SRTMfilesIndex, int *offsetLon, int *offsetLat)
{
    double degreeDeltaLon, degreeDeltaLat;
    double SRTMtopLeftLon, SRTMtopLeftLat;
    int x, y;
    int index;
    bool hasAtLeastOneSRTMFile;

    cacheManager.findTopLeftCorner(L09_L13_topLeftLon, L09_L13_topLeftLat, HGT_SOURCE_DEGREE_SIZE_SRTM, &SRTMtopLeftLon, &SRTMtopLeftLat);

    degreeDeltaLon = L09_L13_topLeftLon - SRTMtopLeftLon;
    degreeDeltaLat = L09_L13_topLeftLat - SRTMtopLeftLat;
    if (degreeDeltaLon<0.0) degreeDeltaLon *= -1.0;
    if (degreeDeltaLat<0.0) degreeDeltaLat *= -1.0;

    (*offsetLon) = (int)( degreeDeltaLon * 4.0 + 0.5 );
    (*offsetLat) = (int)( degreeDeltaLat * 4.0 + 0.5 );

    hasAtLeastOneSRTMFile = false;
    for (y=0; y<5; y++)
        for (x=0; x<5; x++) {
            cacheManager.convertTopLeft2AvabilityIndex(SRTMtopLeftLon + x*HGT_SOURCE_DEGREE_SIZE_SRTM,
                                                       SRTMtopLeftLat - y*HGT_SOURCE_DEGREE_SIZE_SRTM,
                                                       HGT_SOURCE_DEGREE_SIZE_SRTM,
                                                       &index);
            if (index>=360*180)
                index = -1;

            if (index!=-1 && cacheManager.avability_SRTM[index].available)
                hasAtLeastOneSRTMFile = true;

            SRTMfilesIndex[y*5 + x] = index;
        }

    return hasAtLeastOneSRTMFile;
}

void CResizer::buildL09_L13TerrainFromSRTMEntireEarth()
{
    int L09_L13_index;

    for (L09_L13_index=0; L09_L13_index<96*48; L09_L13_index++) {
        buildL09_L13TerrainFromSRTM(L09_L13_index);
    }
}

void CResizer::buildL09_L13TerrainFromSRTM(const double &lon, const double &lat)
{
    double tlLon, tlLat;
    int L09_L13_index;

    cacheManager.findTopLeftCorner(lon, lat, HGT_SOURCE_DEGREE_SIZE_L09_L13, &tlLon, &tlLat);
    cacheManager.convertTopLeft2AvabilityIndex(tlLon, tlLat, HGT_SOURCE_DEGREE_SIZE_L09_L13, &L09_L13_index);

    buildL09_L13TerrainFromSRTM(L09_L13_index);
}

void CResizer::buildL09_L13TerrainFromSRTM(int L09_L13_index)
{
    int *buffer = new int[301*301];
    CHgtFile hgtL09_L13;
    CHgtFile hgtL09_L13_resized;
    QString hgtL09_L13_resizedFilename;
    CHgtFile hgtSRTM;
    QString *SRTMfilename;
    QString SRTMfilenamePrevious;
    int x, y, i;
    bool hasAtLeastOneSRTMFile;
    int SRTMfilesIndex[25];
    int offsetLon, offsetLat;
    int fileOffsetLon, fileOffsetLat;
    int fileLon, fileLat;
    double L09_L13_topLeftLon, L09_L13_topLeftLat;
    alglib::real_2d_array real_2d_array;
    alglib::real_2d_array real_2d_array_resized;


    cacheManager.convertAvabilityIndex2TopLeft(L09_L13_index, HGT_SOURCE_DEGREE_SIZE_L09_L13, &L09_L13_topLeftLon, &L09_L13_topLeftLat);


    qDebug() << "Terrain resizing:  " << QString::number(L09_L13_topLeftLon, 'f', 2) << "  "
                                      << QString::number(L09_L13_topLeftLat, 'f', 2) << "  "
                                      << L09_L13_index;

    // find SRTM files that contains data for new terrain tile (L09-L13)
    qDebug() << "    Find & copy SRTM data...";
    hasAtLeastOneSRTMFile = findSRTMFilesFor_L09_L13(L09_L13_topLeftLon, L09_L13_topLeftLat, SRTMfilesIndex, &offsetLon, &offsetLat);
    if ( ! hasAtLeastOneSRTMFile) {
        qDebug() << "    Find & copy SRTM data... no files, skipping";
        delete []buffer;
        return;
    }
    // copy data from SRTM files
    SRTMfilenamePrevious = "";
    hgtL09_L13.init(4501, 4501);
    for (y=0; y<15; y++)
        for (x=0; x<15; x++) {
            fileOffsetLon = (x+offsetLon) % 4;
            fileOffsetLat = (y+offsetLat) % 4;
            fileLon = (x+offsetLon) / 4;
            fileLat = (y+offsetLat) / 4;

            if (SRTMfilesIndex[fileLat*5 + fileLon]!=-1 && cacheManager.avability_SRTM[SRTMfilesIndex[fileLat*5 + fileLon]].available) {
                // get real data...
                SRTMfilename = cacheManager.avability_SRTM[SRTMfilesIndex[fileLat*5 + fileLon]].name;
                if ((*SRTMfilename) != SRTMfilenamePrevious) {
                    SRTMfilenamePrevious = (*SRTMfilename);
                    hgtSRTM.loadFile(cacheManager.pathSRTM + (*SRTMfilename), 1201, 1201);
                }
                hgtSRTM.getHeightBlock(buffer, fileOffsetLon*300, fileOffsetLat*300, 301, 301, 1);
            } else {
                // ...or fill sea level if no file
                for (i=0; i<301*301; i++)
                    buffer[i] = 0;
            }

            // set copied data to new terrain tile (L09-L13)
            hgtL09_L13.setHeightBlock(buffer, x*300, y*300, 301, 301, 1);
        }
    qDebug() << "    Find & copy SRTM data... OK";


    qDebug() << "    [alglib] Load & resize data...";
    real_2d_array.setlength(4501, 4501);
    real_2d_array_resized.setlength(4097, 4097);
    for (y=0; y<4501; y++)
        for (x=0; x<4501; x++) {
            if (hgtL09_L13.getHeight(x, y)>9000)
                real_2d_array[y][x] = 10.0; else
                real_2d_array[y][x] = (double)hgtL09_L13.getHeight(x, y);
        }
    // bicubic resizing from 4501x4501 to 4097x4097
    alglib::spline2dresamplebicubic(real_2d_array, 4501, 4501, real_2d_array_resized, 4097, 4097);
    hgtL09_L13_resized.init(4097, 4097);
    for (y=0; y<4097; y++)
        for (x=0; x<4097; x++) {
            hgtL09_L13_resized.setHeight(x, y, (int)real_2d_array_resized[y][x]);
        }
    qDebug() << "    [alglib] Load & resize data... OK";

    // find terrain filename and save
    qDebug() << "    Save resized HGT file...";
    cacheManager.convertLonLatToFileName(L09_L13_topLeftLon, L09_L13_topLeftLat, &hgtL09_L13_resizedFilename);
    hgtL09_L13_resized.saveFile(cacheManager.pathL09_L13 + hgtL09_L13_resizedFilename);
    qDebug() << "    Save resized HGT file... OK";


    delete []buffer;
}

void CResizer::connectL09_L13TerrainEntireEarth()
{
    int L09_L13_index;

    for (L09_L13_index=0; L09_L13_index<96*48; L09_L13_index++) {
        connectL09_L13Terrain(L09_L13_index);
    }
}

void CResizer::connectL09_L13Terrain(const double &lon, const double &lat)
{
    double tlLon, tlLat;
    int L09_L13_index;

    cacheManager.findTopLeftCorner(lon, lat, HGT_SOURCE_DEGREE_SIZE_L09_L13, &tlLon, &tlLat);
    cacheManager.convertTopLeft2AvabilityIndex(tlLon, tlLat, HGT_SOURCE_DEGREE_SIZE_L09_L13, &L09_L13_index);

    connectL09_L13Terrain(L09_L13_index);
}

void CResizer::connectL09_L13Terrain(int L09_L13_index)
{
    CHgtFile hgtNW, hgtN, hgtNE;
    CHgtFile hgtW, hgtBase, hgtE;
    CHgtFile hgtSW, hgtS, hgtSE;
    QString hgtNWfn, hgtNfn, hgtNEfn;
    QString hgtWfn, hgtBasefn, hgtEfn;
    QString hgtSWfn, hgtSfn, hgtSEfn;
    bool hgtNWav, hgtNav, hgtNEav;
    bool hgtWav, hgtBaseav, hgtEav;
    bool hgtSWav, hgtSav, hgtSEav;
    int hgtNWinx, hgtNinx, hgtNEinx;
    int hgtWinx, hgtBaseinx, hgtEinx;
    int hgtSWinx, hgtSinx, hgtSEinx;
    double L09_L13_topLeftLon, L09_L13_topLeftLat;
    double roundedHgt;
    int roundedHgtInt;
    int x, y;
    bool save = true;

    cacheManager.convertAvabilityIndex2TopLeft(L09_L13_index, HGT_SOURCE_DEGREE_SIZE_L09_L13, &L09_L13_topLeftLon, &L09_L13_topLeftLat);

    qDebug() << "Terrain connecting:  " << QString::number(L09_L13_topLeftLon, 'f', 2) << "  "
                                        << QString::number(L09_L13_topLeftLat, 'f', 2) << "  "
                                        << L09_L13_index;

    if ( ! cacheManager.avability_L09_L13[L09_L13_index].available) {
        qDebug() << "    No terrain... skipping";
        return;
    }

    // NW neighbor
    hgtNWinx = cacheManager.getNeighborAvabilityIndex(L09_L13_index, HGT_SOURCE_DEGREE_SIZE_L09_L13, -1, -1);
    if (hgtNWinx!=-1) hgtNWav = cacheManager.avability_L09_L13[hgtNWinx].available; else
                      hgtNWav = false;
    // N neighbor
    hgtNinx = cacheManager.getNeighborAvabilityIndex(L09_L13_index, HGT_SOURCE_DEGREE_SIZE_L09_L13, 0, -1);
    if (hgtNinx!=-1) hgtNav = cacheManager.avability_L09_L13[hgtNinx].available; else
                     hgtNav = false;
    // NE neighbor
    hgtNEinx = cacheManager.getNeighborAvabilityIndex(L09_L13_index, HGT_SOURCE_DEGREE_SIZE_L09_L13, 1, -1);
    if (hgtNEinx!=-1) hgtNEav = cacheManager.avability_L09_L13[hgtNEinx].available; else
                      hgtNEav = false;
    // W neighbor
    hgtWinx = cacheManager.getNeighborAvabilityIndex(L09_L13_index, HGT_SOURCE_DEGREE_SIZE_L09_L13, -1, 0);
    if (hgtWinx!=-1) hgtWav = cacheManager.avability_L09_L13[hgtWinx].available; else
                     hgtWav = false;
    // base terrain
    hgtBaseinx = L09_L13_index;
    if (hgtBaseinx!=-1) hgtBaseav = cacheManager.avability_L09_L13[hgtBaseinx].available; else
                        hgtBaseav = false;
    // E neighbor
    hgtEinx = cacheManager.getNeighborAvabilityIndex(L09_L13_index, HGT_SOURCE_DEGREE_SIZE_L09_L13, 1, 0);
    if (hgtEinx!=-1) hgtEav = cacheManager.avability_L09_L13[hgtEinx].available; else
                     hgtEav = false;
    // SW neighbor
    hgtSWinx = cacheManager.getNeighborAvabilityIndex(L09_L13_index, HGT_SOURCE_DEGREE_SIZE_L09_L13, -1, 1);
    if (hgtSWinx!=-1) hgtSWav = cacheManager.avability_L09_L13[hgtSWinx].available; else
                      hgtSWav = false;
    // S neighbor
    hgtSinx = cacheManager.getNeighborAvabilityIndex(L09_L13_index, HGT_SOURCE_DEGREE_SIZE_L09_L13, 0, 1);
    if (hgtSinx!=-1) hgtSav = cacheManager.avability_L09_L13[hgtSinx].available; else
                     hgtSav = false;
    // SE neighbor
    hgtSEinx = cacheManager.getNeighborAvabilityIndex(L09_L13_index, HGT_SOURCE_DEGREE_SIZE_L09_L13, 1, 1);
    if (hgtSEinx!=-1) hgtSEav = cacheManager.avability_L09_L13[hgtSEinx].available; else
                      hgtSEav = false;

    // get filenames
    if (hgtNWav) hgtNWfn = (*cacheManager.avability_L09_L13[hgtNWinx].name); else hgtNWfn = "                  ";
    if (hgtNav)  hgtNfn  = (*cacheManager.avability_L09_L13[hgtNinx].name);  else hgtNfn  = "                  ";
    if (hgtNEav) hgtNEfn = (*cacheManager.avability_L09_L13[hgtNEinx].name); else hgtNEfn = "                  ";
    if (hgtWav)    hgtWfn    = (*cacheManager.avability_L09_L13[hgtWinx].name);    else hgtWfn    = "                  ";
    if (hgtBaseav) hgtBasefn = (*cacheManager.avability_L09_L13[hgtBaseinx].name); else hgtBasefn = "                  ";
    if (hgtEav)    hgtEfn    = (*cacheManager.avability_L09_L13[hgtEinx].name);    else hgtEfn    = "                  ";
    if (hgtSWav) hgtSWfn = (*cacheManager.avability_L09_L13[hgtSWinx].name); else hgtSWfn = "                  ";
    if (hgtSav)  hgtSfn  = (*cacheManager.avability_L09_L13[hgtSinx].name);  else hgtSfn  = "                  ";
    if (hgtSEav) hgtSEfn = (*cacheManager.avability_L09_L13[hgtSEinx].name); else hgtSEfn = "                  ";

//    qDebug() << hgtNWfn << " " << hgtNfn    << " " << hgtNEfn;
//    qDebug() << hgtWfn  << " " << hgtBasefn << " " << hgtEfn;
//    qDebug() << hgtSWfn << " " << hgtSfn    << " " << hgtSEfn;

    // open files
    if (hgtNWav) hgtNW.fileOpen(cacheManager.pathL09_L13 + hgtNWfn, 4097, 4097);
    if (hgtNav)  hgtN.fileOpen(cacheManager.pathL09_L13 + hgtNfn, 4097, 4097);
    if (hgtNEav) hgtNE.fileOpen(cacheManager.pathL09_L13 + hgtNEfn, 4097, 4097);
    if (hgtWav)    hgtW.fileOpen(cacheManager.pathL09_L13 + hgtWfn, 4097, 4097);
    if (hgtBaseav) hgtBase.fileOpen(cacheManager.pathL09_L13 + hgtBasefn, 4097, 4097);
    if (hgtEav)    hgtE.fileOpen(cacheManager.pathL09_L13 + hgtEfn, 4097, 4097);
    if (hgtSWav) hgtSW.fileOpen(cacheManager.pathL09_L13 + hgtSWfn, 4097, 4097);
    if (hgtSav)  hgtS.fileOpen(cacheManager.pathL09_L13 + hgtSfn, 4097, 4097);
    if (hgtSEav) hgtSE.fileOpen(cacheManager.pathL09_L13 + hgtSEfn, 4097, 4097);

    // corner NW
    roundedHgt = 0.0;
    if (hgtBaseav) { roundedHgt = roundedHgt + (double)hgtBase.fileGetHeight(0, 0); }
    if (hgtNWav)   { roundedHgt = roundedHgt + (double)hgtNW.fileGetHeight(4096, 4096); }
    if (hgtWav)    { roundedHgt = roundedHgt + (double)hgtW.fileGetHeight(4096, 0); }
    if (hgtNav)    { roundedHgt = roundedHgt + (double)hgtN.fileGetHeight(0, 4096); }
    roundedHgtInt = (int)((roundedHgt / 4.0) + 0.5);
    if (hgtBaseav) { if (hgtBase.fileGetHeight(0, 0)!=roundedHgtInt)     { qDebug() << "[NW] Base: " << hgtBase.fileGetHeight(0, 0)     << "!=" << roundedHgtInt; if (save) hgtBase.fileSetHeight(0, 0, roundedHgtInt); } }
    if (hgtNWav)   { if (hgtNW.fileGetHeight(4096, 4096)!=roundedHgtInt) { qDebug() << "[NW] NW:   " << hgtNW.fileGetHeight(4096, 4096) << "!=" << roundedHgtInt; if (save) hgtNW.fileSetHeight(4096, 4096, roundedHgtInt); } };
    if (hgtWav)    { if (hgtW.fileGetHeight(4096, 0)!=roundedHgtInt)     { qDebug() << "[NW] W:    " << hgtW.fileGetHeight(4096, 0)     << "!=" << roundedHgtInt; if (save) hgtW.fileSetHeight(4096, 0, roundedHgtInt); } }
    if (hgtNav)    { if (hgtN.fileGetHeight(0, 4096)!=roundedHgtInt)     { qDebug() << "[NW] N:    " << hgtN.fileGetHeight(0, 4096)     << "!=" << roundedHgtInt; if (save) hgtN.fileSetHeight(0, 4096, roundedHgtInt); } }

    // corner NE
    roundedHgt = 0.0;
    if (hgtBaseav) { roundedHgt = roundedHgt + (double)hgtBase.fileGetHeight(4096, 0); }
    if (hgtNEav)   { roundedHgt = roundedHgt + (double)hgtNE.fileGetHeight(0, 4096); }
    if (hgtEav)    { roundedHgt = roundedHgt + (double)hgtE.fileGetHeight(0, 0); }
    if (hgtNav)    { roundedHgt = roundedHgt + (double)hgtN.fileGetHeight(4096, 4096); }
    roundedHgtInt = (int)((roundedHgt / 4.0) + 0.5);
    if (hgtBaseav) { if (hgtBase.fileGetHeight(4096, 0)!=roundedHgtInt)    { qDebug() << "[NE] Base: " << hgtBase.fileGetHeight(4096, 0)    << "!=" << roundedHgtInt; if (save) hgtBase.fileSetHeight(4096, 0, roundedHgtInt); } }
    if (hgtNEav)   { if (hgtNE.fileGetHeight(0, 4096)!=roundedHgtInt)   { qDebug() << "[NE] NE:   " << hgtNE.fileGetHeight(0, 4096)   << "!=" << roundedHgtInt; if (save) hgtNE.fileSetHeight(0, 4096, roundedHgtInt); } };
    if (hgtEav)    { if (hgtE.fileGetHeight(0, 0)!=roundedHgtInt)       { qDebug() << "[NE] E:    " << hgtE.fileGetHeight(0, 0)       << "!=" << roundedHgtInt; if (save) hgtE.fileSetHeight(0, 0, roundedHgtInt); } }
    if (hgtNav)    { if (hgtN.fileGetHeight(4096, 4096)!=roundedHgtInt) { qDebug() << "[NE] N:    " << hgtN.fileGetHeight(4096, 4096) << "!=" << roundedHgtInt; if (save) hgtN.fileSetHeight(4096, 4096, roundedHgtInt); } }

    // corner SW
    roundedHgt = 0.0;
    if (hgtBaseav) { roundedHgt = roundedHgt + (double)hgtBase.fileGetHeight(0, 4096); }
    if (hgtSWav)   { roundedHgt = roundedHgt + (double)hgtSW.fileGetHeight(4096, 0); }
    if (hgtWav)    { roundedHgt = roundedHgt + (double)hgtW.fileGetHeight(4096, 4096); }
    if (hgtSav)    { roundedHgt = roundedHgt + (double)hgtS.fileGetHeight(0, 0); }
    roundedHgtInt = (int)((roundedHgt / 4.0) + 0.5);
    if (hgtBaseav) { if (hgtBase.fileGetHeight(0, 4096)!=roundedHgtInt) { qDebug() << "[SW] Base: " << hgtBase.fileGetHeight(0, 4096) << "!=" << roundedHgtInt; if (save) hgtBase.fileSetHeight(0, 4096, roundedHgtInt); } }
    if (hgtSWav)   { if (hgtSW.fileGetHeight(4096, 0)!=roundedHgtInt)   { qDebug() << "[SW] SW:   " << hgtSW.fileGetHeight(4096, 0)   << "!=" << roundedHgtInt; if (save) hgtSW.fileSetHeight(4096, 0, roundedHgtInt); } };
    if (hgtWav)    { if (hgtW.fileGetHeight(4096, 4096)!=roundedHgtInt) { qDebug() << "[SW] W:    " << hgtW.fileGetHeight(4096, 4096) << "!=" << roundedHgtInt; if (save) hgtW.fileSetHeight(4096, 4096, roundedHgtInt); } }
    if (hgtSav)    { if (hgtS.fileGetHeight(0, 0)!=roundedHgtInt)       { qDebug() << "[SW] S:    " << hgtS.fileGetHeight(0, 0)       << "!=" << roundedHgtInt; if (save) hgtS.fileSetHeight(0, 0, roundedHgtInt); } }

    // corner SE
    roundedHgt = 0.0;
    if (hgtBaseav) { roundedHgt = roundedHgt + (double)hgtBase.fileGetHeight(4096, 4096); }
    if (hgtSEav)   { roundedHgt = roundedHgt + (double)hgtSE.fileGetHeight(0, 0); }
    if (hgtEav)    { roundedHgt = roundedHgt + (double)hgtE.fileGetHeight(0, 4096); }
    if (hgtSav)    { roundedHgt = roundedHgt + (double)hgtS.fileGetHeight(4096, 0); }
    roundedHgtInt = (int)((roundedHgt / 4.0) + 0.5);
    if (hgtBaseav) { if (hgtBase.fileGetHeight(4096, 4096)!=roundedHgtInt) { qDebug() << "[SE] Base: " << hgtBase.fileGetHeight(4096, 4096) << "!=" << roundedHgtInt; if (save) hgtBase.fileSetHeight(4096, 4096, roundedHgtInt); } }
    if (hgtSEav)   { if (hgtSE.fileGetHeight(0, 0)!=roundedHgtInt)         { qDebug() << "[SE] SE:   " << hgtSE.fileGetHeight(0, 0)         << "!=" << roundedHgtInt; if (save) hgtSE.fileSetHeight(0, 0, roundedHgtInt); } };
    if (hgtEav)    { if (hgtE.fileGetHeight(0, 4096)!=roundedHgtInt)       { qDebug() << "[SE] E:    " << hgtE.fileGetHeight(0, 4096)       << "!=" << roundedHgtInt; if (save) hgtE.fileSetHeight(0, 4096, roundedHgtInt); } }
    if (hgtSav)    { if (hgtS.fileGetHeight(4096, 0)!=roundedHgtInt)       { qDebug() << "[SE] S:    " << hgtS.fileGetHeight(4096, 0)       << "!=" << roundedHgtInt; if (save) hgtS.fileSetHeight(4096, 0, roundedHgtInt); } }

    // line N
    for (x=1; x<4096; x++) {
        roundedHgt = 0.0;
        if (hgtBaseav) { roundedHgt = roundedHgt + (double)hgtBase.fileGetHeight(x, 0); }
        if (hgtNav)    { roundedHgt = roundedHgt + (double)hgtN.fileGetHeight(x, 4096); }
        roundedHgtInt = (int)((roundedHgt / 2.0) + 0.5);
        if (hgtBaseav) { if (hgtBase.fileGetHeight(x, 0)!=roundedHgtInt) { qDebug() << "[N -" << x << "] Base: " << hgtBase.fileGetHeight(x, 0)     << "!=" << roundedHgtInt; if (save) hgtBase.fileSetHeight(x, 0, roundedHgtInt); } }
        if (hgtNav)    { if (hgtN.fileGetHeight(x, 4096)!=roundedHgtInt) { qDebug() << "[N -" << x << "] N:    " << hgtN.fileGetHeight(x, 4096)     << "!=" << roundedHgtInt; if (save) hgtN.fileSetHeight(x, 4096, roundedHgtInt); } }
    }

    // line S
    for (x=1; x<4096; x++) {
        roundedHgt = 0.0;
        if (hgtBaseav) { roundedHgt = roundedHgt + (double)hgtBase.fileGetHeight(x, 4096); }
        if (hgtSav)    { roundedHgt = roundedHgt + (double)hgtS.fileGetHeight(x, 0); }
        roundedHgtInt = (int)((roundedHgt / 2.0) + 0.5);
        if (hgtBaseav) { if (hgtBase.fileGetHeight(x, 4096)!=roundedHgtInt) { qDebug() << "[S -" << x << "] Base: " << hgtBase.fileGetHeight(x, 4096) << "!=" << roundedHgtInt; if (save) hgtBase.fileSetHeight(x, 4096, roundedHgtInt); } }
        if (hgtSav)    { if (hgtS.fileGetHeight(x, 0)!=roundedHgtInt)       { qDebug() << "[S -" << x << "] S:    " << hgtS.fileGetHeight(x, 0)       << "!=" << roundedHgtInt; if (save) hgtS.fileSetHeight(x, 0, roundedHgtInt); } }
    }

    // line W
    for (y=1; y<4096; y++) {
        roundedHgt = 0.0;
        if (hgtBaseav) { roundedHgt = roundedHgt + (double)hgtBase.fileGetHeight(0, y); }
        if (hgtWav)    { roundedHgt = roundedHgt + (double)hgtW.fileGetHeight(4096, y); }
        roundedHgtInt = (int)((roundedHgt / 2.0) + 0.5);
        if (hgtBaseav) { if (hgtBase.fileGetHeight(0, y)!=roundedHgtInt) { qDebug() << "[W -" << y << "] Base: " << hgtBase.fileGetHeight(0, y) << "!=" << roundedHgtInt; if (save) hgtBase.fileSetHeight(0, y, roundedHgtInt); } }
        if (hgtWav)    { if (hgtW.fileGetHeight(4096, y)!=roundedHgtInt) { qDebug() << "[W -" << y << "] W:    " << hgtW.fileGetHeight(4096, y) << "!=" << roundedHgtInt; if (save) hgtW.fileSetHeight(4096, y, roundedHgtInt); } }
    }

    // line E
    for (y=1; y<4096; y++) {
        roundedHgt = 0.0;
        if (hgtBaseav) { roundedHgt = roundedHgt + (double)hgtBase.fileGetHeight(4096, y); }
        if (hgtEav)    { roundedHgt = roundedHgt + (double)hgtE.fileGetHeight(0, y); }
        roundedHgtInt = (int)((roundedHgt / 2.0) + 0.5);
        if (hgtBaseav) { if (hgtBase.fileGetHeight(4096, y)!=roundedHgtInt) { qDebug() << "[E -" << y << "] Base: " << hgtBase.fileGetHeight(4096, y) << "!=" << roundedHgtInt; if (save) hgtBase.fileSetHeight(4096, y, roundedHgtInt); } }
        if (hgtEav)    { if (hgtE.fileGetHeight(0, y)!=roundedHgtInt)       { qDebug() << "[E -" << y << "] E:    " << hgtE.fileGetHeight(0, y)       << "!=" << roundedHgtInt; if (save) hgtE.fileSetHeight(0, y, roundedHgtInt); } }
    }

    // close files
    if (hgtNWav) hgtNW.fileClose();
    if (hgtNav)  hgtN.fileClose();
    if (hgtNEav) hgtNE.fileClose();
    if (hgtWav)    hgtW.fileClose();
    if (hgtBaseav) hgtBase.fileClose();
    if (hgtEav)    hgtE.fileClose();
    if (hgtSWav) hgtSW.fileClose();
    if (hgtSav)  hgtS.fileClose();
    if (hgtSEav) hgtSE.fileClose();
}

void CResizer::buildL04_L08TerrainFromL09_L13EntireEarth()
{
    int L04_L08_index;

    for (L04_L08_index=0; L04_L08_index<24*12; L04_L08_index++) {
        buildL04_L08TerrainFromL09_L13(L04_L08_index);
    }
}

void CResizer::buildL04_L08TerrainFromL09_L13(const double &lon, const double &lat)
{
    double tlLon, tlLat;
    int L04_L08_index;

    cacheManager.findTopLeftCorner(lon, lat, HGT_SOURCE_DEGREE_SIZE_L04_L08, &tlLon, &tlLat);
    cacheManager.convertTopLeft2AvabilityIndex(tlLon, tlLat, HGT_SOURCE_DEGREE_SIZE_L04_L08, &L04_L08_index);

    buildL04_L08TerrainFromL09_L13(L04_L08_index);
}

void CResizer::buildL04_L08TerrainFromL09_L13(int L04_L08_index)
{
    double L04_L08_topLeftLon, L04_L08_topLeftLat;
    int *buffer = new int[129*129];
    bool hasAtLeastOneL09_L13;
    int index;
    int L09_L13_index;
    QString hgtFilename[4*4];
    QString hgtFilenameResult;
    CHgtFile hgt_L09_L13;
    CHgtFile hgt_L04_L08;
    int x, y, i;

    cacheManager.convertAvabilityIndex2TopLeft(L04_L08_index, HGT_SOURCE_DEGREE_SIZE_L04_L08, &L04_L08_topLeftLon, &L04_L08_topLeftLat);
    qDebug() << "L09_L13 to L04_L08:  " << QString::number(L04_L08_topLeftLon, 'f', 2) << "  "
                                        << QString::number(L04_L08_topLeftLat, 'f', 2) << "  "
                                        << L04_L08_index;

    // get most top left LO9_L13 file index
    cacheManager.convertTopLeft2AvabilityIndex(L04_L08_topLeftLon, L04_L08_topLeftLat, HGT_SOURCE_DEGREE_SIZE_L09_L13, &L09_L13_index);

    // get filenames of data in upper level
    hasAtLeastOneL09_L13 = false;
    for (y=0; y<4; y++)
        for (x=0; x<4; x++) {
            index = cacheManager.getNeighborAvabilityIndex(L09_L13_index, HGT_SOURCE_DEGREE_SIZE_L09_L13, x, y);

            hgtFilename[y*4 + x] = "";
            if (index!=-1)
                if (cacheManager.avability_L09_L13[index].available) {
                    hgtFilename[y*4 + x] = (*cacheManager.avability_L09_L13[index].name);
                    hasAtLeastOneL09_L13 = true;
                }
        }


    // if files was found copy data to lower LOD
    if (hasAtLeastOneL09_L13) {

        qDebug() << "    Copy data with skipping...";
        hgt_L04_L08.init(513, 513);
        for (y=0; y<4; y++)
            for (x=0; x<4; x++) {

                if (hgtFilename[y*4 + x]!="") {
                    hgt_L09_L13.loadFile(cacheManager.pathL09_L13 + hgtFilename[y*4 + x], 4097, 4097);
                    hgt_L09_L13.getHeightBlock(buffer, 0, 0, 129, 129, 32);
                } else {
                    for (i=0; i<129*129; i++)
                        buffer[i] = 0;
                }

                hgt_L04_L08.setHeightBlock(buffer, x*128, y*128, 129, 129, 1);
            }

        cacheManager.convertLonLatToFileName(L04_L08_topLeftLon, L04_L08_topLeftLat, &hgtFilenameResult);
        hgt_L04_L08.saveFile(cacheManager.pathL04_L08 + hgtFilenameResult);
        qDebug() << "    Copy data with skipping... OK";

    } else {

        qDebug() << "    No terrain in upper level... skipping";
        delete []buffer;
        return;

    }

    delete []buffer;
}

void CResizer::buildL00_L03TerrainFromL04_L08EntireEarth()
{
    int L00_L03_index;

    for (L00_L03_index=0; L00_L03_index<24*12; L00_L03_index++) {
        buildL00_L03TerrainFromL04_L08(L00_L03_index);
    }
}

void CResizer::buildL00_L03TerrainFromL04_L08(const double &lon, const double &lat)
{
    double tlLon, tlLat;
    int L00_L03_index;

    cacheManager.findTopLeftCorner(lon, lat, HGT_SOURCE_DEGREE_SIZE_L00_L03, &tlLon, &tlLat);
    cacheManager.convertTopLeft2AvabilityIndex(tlLon, tlLat, HGT_SOURCE_DEGREE_SIZE_L00_L03, &L00_L03_index);

    buildL00_L03TerrainFromL04_L08(L00_L03_index);
}

void CResizer::buildL00_L03TerrainFromL04_L08(int L00_L03_index)
{
    double L00_L03_topLeftLon, L00_L03_topLeftLat;
    int *buffer = new int[17*17];
    bool hasAtLeastOneL04_L08;
    int index;
    int L04_L08_index;
    QString hgtFilename[4*4];
    QString hgtFilenameResult;
    CHgtFile hgt_L04_L08;
    CHgtFile hgt_L00_L03;
    int x, y, i;

    cacheManager.convertAvabilityIndex2TopLeft(L00_L03_index, HGT_SOURCE_DEGREE_SIZE_L00_L03, &L00_L03_topLeftLon, &L00_L03_topLeftLat);
    qDebug() << "L04_L08 to L00_L03:  " << QString::number(L00_L03_topLeftLon, 'f', 2) << "  "
                                        << QString::number(L00_L03_topLeftLat, 'f', 2) << "  "
                                        << L00_L03_index;

    // get most top left L04_L08 file index
    cacheManager.convertTopLeft2AvabilityIndex(L00_L03_topLeftLon, L00_L03_topLeftLat, HGT_SOURCE_DEGREE_SIZE_L04_L08, &L04_L08_index);

    // get filenames of data in upper level
    hasAtLeastOneL04_L08 = false;
    for (y=0; y<4; y++)
        for (x=0; x<4; x++) {
            index = cacheManager.getNeighborAvabilityIndex(L04_L08_index, HGT_SOURCE_DEGREE_SIZE_L04_L08, x, y);

            hgtFilename[y*4 + x] = "";
            if (index!=-1)
                if (cacheManager.avability_L04_L08[index].available) {
                    hgtFilename[y*4 + x] = (*cacheManager.avability_L04_L08[index].name);
                    hasAtLeastOneL04_L08 = true;
                }
        }


    // if files was found copy data to lower LOD
    if (hasAtLeastOneL04_L08) {

        qDebug() << "    Copy data with skipping...";
        hgt_L00_L03.init(65, 65);
        for (y=0; y<4; y++)
            for (x=0; x<4; x++) {

                if (hgtFilename[y*4 + x]!="") {
                    hgt_L04_L08.loadFile(cacheManager.pathL04_L08 + hgtFilename[y*4 + x], 513, 513);
                    hgt_L04_L08.getHeightBlock(buffer, 0, 0, 17, 17, 32);
                } else {
                    for (i=0; i<17*17; i++)
                        buffer[i] = 0;
                }

                hgt_L00_L03.setHeightBlock(buffer, x*16, y*16, 17, 17, 1);
            }

        cacheManager.convertLonLatToFileName(L00_L03_topLeftLon, L00_L03_topLeftLat, &hgtFilenameResult);
        hgt_L00_L03.saveFile(cacheManager.pathL00_L03 + hgtFilenameResult);
        qDebug() << "    Copy data with skipping... OK";

    } else {

        qDebug() << "    No terrain in upper level... skipping";
        delete []buffer;
        return;

    }

    delete []buffer;
}

unsigned int CResizer::getColor(int height)
{
    QColor col;
    double val, hue;
    int r, g, b, a;

    if (height>9000) height = 10;

    if (height==0) {
        col.setRedF(0.2784);
        col.setGreenF(0.6431);
        col.setBlueF(0.7216);
    } else {
        val = 240.0;
        hue = 170.0 - 170.0 * ( ((double)height)/1500.0 );
        if (hue<0.0) {
            hue = 0.0;
            hue = 360.0 - 100.0 * (((double)(height-1500))/1500.0);
            if (hue<260.0) {
                hue = 260.0;
                val = 240.0 - 200.0 * (((double)(height-3000))/5000.0);
                if (val<40.0) {
                    val = 40.0 + 215.0 * (((double)(height-8000))/850.0);
                }
            }
        }
        col.setHsv((int)hue, 170, (int)val);
    }
    col.getRgb(&r, &g, &b, &a);

    return qRgb(r, g, b);
}

void CResizer::generateHtmlIndex(int hgtSource, bool createImages, double lon = -1.0, double lat = -1.0)
{
    CHgtFile hgtFile;
    QImage imageTH;
    QString filename;
    fstream fileHTML;
    double tlLon, tlLat;
    int index, x, y;
    double DEGsize = 8;
    double onePhotoTlLon, onePhotoTlLat;
    int onePhotoIndex;
    bool onePhoto;

    CAvability *avab = 0;
    double hgtSourceDegree;
    double THsize = 0.0;
    int earthSizeX = 0, earthSizeY = 0;
    int hgtSourceSize = 0;
    QString pathDir, pathDirIndex, info;

    onePhoto = false;

    switch (hgtSource) {
        case HGT_SOURCE_L00_L03: avab = cacheManager.avability_L00_L03;
                                 hgtSourceDegree = HGT_SOURCE_DEGREE_SIZE_L00_L03;
                                 THsize = 480;
                                 earthSizeX = 6; earthSizeY = 3;
                                 hgtSourceSize = HGT_SOURCE_SIZE_L00_L03;
                                 pathDir = cacheManager.pathL00_L03;
                                 pathDirIndex = cacheManager.pathL00_L03_index;
                                 info = "L00-L03";
                                 if (lon!=-1.0 && lat!=-1.0) {
                                     cacheManager.findTopLeftCorner(lon, lat, HGT_SOURCE_DEGREE_SIZE_L00_L03, &onePhotoTlLon, &onePhotoTlLat);
                                     cacheManager.convertTopLeft2AvabilityIndex(onePhotoTlLon, onePhotoTlLat, HGT_SOURCE_DEGREE_SIZE_L00_L03, &onePhotoIndex);
                                     onePhoto = true;
                                 }
                                 break;
       case HGT_SOURCE_L04_L08:  avab = cacheManager.avability_L04_L08;
                                 hgtSourceDegree = HGT_SOURCE_DEGREE_SIZE_L04_L08;
                                 THsize = 120;
                                 earthSizeX = 24; earthSizeY = 12;
                                 hgtSourceSize = HGT_SOURCE_SIZE_L04_L08;
                                 pathDir = cacheManager.pathL04_L08;
                                 pathDirIndex = cacheManager.pathL04_L08_index;
                                 info = "L04-L08";
                                 if (lon!=-1.0 && lat!=-1.0) {
                                     cacheManager.findTopLeftCorner(lon, lat, HGT_SOURCE_DEGREE_SIZE_L04_L08, &onePhotoTlLon, &onePhotoTlLat);
                                     cacheManager.convertTopLeft2AvabilityIndex(onePhotoTlLon, onePhotoTlLat, HGT_SOURCE_DEGREE_SIZE_L04_L08, &onePhotoIndex);
                                     onePhoto = true;
                                 }
                                 break;
       case HGT_SOURCE_L09_L13:  avab = cacheManager.avability_L09_L13;
                                 hgtSourceDegree = HGT_SOURCE_DEGREE_SIZE_L09_L13;
                                 THsize = 30;
                                 earthSizeX = 96; earthSizeY = 48;
                                 hgtSourceSize = HGT_SOURCE_SIZE_L09_L13;
                                 pathDir = cacheManager.pathL09_L13;
                                 pathDirIndex = cacheManager.pathL09_L13_index;
                                 info = "L09-L13";
                                 if (lon!=-1.0 && lat!=-1.0) {
                                     cacheManager.findTopLeftCorner(lon, lat, HGT_SOURCE_DEGREE_SIZE_L09_L13, &onePhotoTlLon, &onePhotoTlLat);
                                     cacheManager.convertTopLeft2AvabilityIndex(onePhotoTlLon, onePhotoTlLat, HGT_SOURCE_DEGREE_SIZE_L09_L13, &onePhotoIndex);
                                     onePhoto = true;
                                 }
                                 break;
       case HGT_SOURCE_SRTM:     avab = cacheManager.avability_SRTM;
                                 hgtSourceDegree = HGT_SOURCE_DEGREE_SIZE_SRTM;
                                 THsize = 8;
                                 earthSizeX = 360; earthSizeY = 180;
                                 hgtSourceSize = HGT_SOURCE_SIZE_SRTM;
                                 pathDir = cacheManager.pathSRTM;
                                 pathDirIndex = cacheManager.pathSRTM_index;
                                 info = "SRTM";
                                 if (lon!=-1.0 && lat!=-1.0) {
                                     cacheManager.findTopLeftCorner(lon, lat, HGT_SOURCE_DEGREE_SIZE_SRTM, &onePhotoTlLon, &onePhotoTlLat);
                                     cacheManager.convertTopLeft2AvabilityIndex(onePhotoTlLon, onePhotoTlLat, HGT_SOURCE_DEGREE_SIZE_SRTM, &onePhotoIndex);
                                     onePhoto = true;
                                 }
                                 break;
    }

    QImage image(hgtSourceSize, hgtSourceSize, QImage::Format_RGB32);


    if (createImages) {
        for (index=0; index<earthSizeX*earthSizeY; index++) {

            if (onePhoto) {
                index = onePhotoIndex;
            }

            if (avab[index].available) {
                cacheManager.convertAvabilityIndex2TopLeft(index, hgtSourceDegree, &tlLon, &tlLat);
                qDebug() << info << " index " << QString::number(tlLon, 'f', 2) << " " << QString::number(tlLat, 'f', 2) << " " << index;
                qDebug() << "    Create img...";
                hgtFile.loadFile(pathDir + (*avab[index].name), hgtSourceSize, hgtSourceSize);
                for (y=0; y<hgtSourceSize; y++)
                    for (x=0; x<hgtSourceSize; x++) {
                        image.setPixel(x, y, getColor(hgtFile.getHeight(x, y)));
                    }
                cacheManager.convertLonLatToFileName(tlLon, tlLat, &filename);
                image.save(pathDirIndex + filename + ".jpg", 0, 90);
                imageTH = image.scaledToWidth(THsize, Qt::SmoothTransformation);
                imageTH.save(pathDirIndex + filename + "_th.jpg", 0, 90);
                qDebug() << "    Create img... OK";
            }

            if (onePhoto)
                break;
        }
    }

    qDebug() << info << " HTML index...";
    fileHTML.open(QString(pathDirIndex + "!_index.html").toAscii(), fstream::out);
    fileHTML << "<html><head><title></title>";
    fileHTML << "<style type=\"text/css\"> div > a { display: block; position: absolute; width: "
             << THsize << "px; height: " << THsize
             << "px; border: 1px solid #444; } div > span { position: absolute; display: block; opacity: 0.3; background-color: black; } </style>";
    fileHTML << "</head><body>";

    fileHTML << "<div style=\"width: " << earthSizeX*THsize << "; height: " << earthSizeY*THsize << "; position: relative; border: 1px solid red;\">";

    for (y=0; y<earthSizeY; y++)
        for (x=0; x<earthSizeX; x++) {
            index = cacheManager.getNeighborAvabilityIndex(earthSizeY, hgtSourceDegree, x, y);
            if (avab[index].available) {
                cacheManager.convertAvabilityIndex2TopLeft(index, hgtSourceDegree, &tlLon, &tlLat);
                cacheManager.convertLonLatToFileName(tlLon, tlLat, &filename);
                fileHTML << "<a style=\"top: " << y*THsize << "px; left: " << x*THsize << "px;\" href=\"" << filename.toStdString() << ".jpg\">";
                fileHTML << "<img src=\"" << filename.toStdString() << "_TH.jpg\" alt=\"" << filename.toStdString() << "\" />";
                fileHTML << "</a>";
            }
        }


    for (x=0; x<360; x++) {
        fileHTML << "<span style=\"width: 1px; height: " << DEGsize*180 << "px; top: 0px; left: " << x*DEGsize << "px;\"></span>";
    }
    for (y=0; y<180; y++) {
        fileHTML << "<span style=\"height: 1px; width: " << DEGsize*360 << "px; left: 0px; top: " << y*DEGsize << "px;\"></span>";
    }

    fileHTML << "</div>";
    fileHTML << "</body></html>";
    fileHTML.close();
    qDebug() << info << " HTML index... OK";
}
