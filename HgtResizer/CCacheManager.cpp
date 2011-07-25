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

#include <QTime>
#include <math.h>
#include <QDebug>
#include <QDir>
#include <QFileInfoList>
#include <QFileInfo>
#include "CCacheManager.h"
#include "CHgtFile.h"

CCacheManager *CCacheManager::instance;

CCacheManager::CCacheManager()
{
    int i;

    // something like singleton :)
    instance = this;

    pathBase = ""; // "E:\\HgtReader_data\\";
    pathL00_L03 = pathBase + "L00-L03\\";
    pathL04_L08 = pathBase + "L04-L08\\";
    pathL09_L13 = pathBase + "L09-L13\\";
    pathSRTM = pathBase + "NASA_SRTM\\";
    pathL00_L03_index = pathBase + "L00-L03_index\\";
    pathL04_L08_index = pathBase + "L04-L08_index\\";
    pathL09_L13_index = pathBase + "L09-L13_index\\";
    pathSRTM_index = pathBase + "NASA_SRTM_index\\";

    // generate degree size of tile in each LOD
    LODdegreeSizeLookUp[0] = 60.0;
    for (i=1; i<=13; i++)
        LODdegreeSizeLookUp[i] = LODdegreeSizeLookUp[i-1]/2.0;

    // create look-up tables - source for each LOD
    for (i=0; i<=3; i++)  HGTsourceLookUp[i] = HGT_SOURCE_L00_L03;
    for (i=4; i<=8; i++)  HGTsourceLookUp[i] = HGT_SOURCE_L04_L08;
    for (i=9; i<=13; i++) HGTsourceLookUp[i] = HGT_SOURCE_L09_L13;

    // create look-up tables - source size (n x n) for each LOD
    for (i=0; i<=3; i++)  HGTsourceSizeLookUp[i] = HGT_SOURCE_SIZE_L00_L03;
    for (i=4; i<=8; i++)  HGTsourceSizeLookUp[i] = HGT_SOURCE_SIZE_L04_L08;
    for (i=9; i<=13; i++) HGTsourceSizeLookUp[i] = HGT_SOURCE_SIZE_L09_L13;

    // create look-up tables - source degree size for each LOD
    for (i=0; i<=3; i++)  HGTsourceDegreeSizeLookUp[i] = HGT_SOURCE_DEGREE_SIZE_L00_L03;
    for (i=4; i<=8; i++)  HGTsourceDegreeSizeLookUp[i] = HGT_SOURCE_DEGREE_SIZE_L04_L08;
    for (i=9; i<=13; i++) HGTsourceDegreeSizeLookUp[i] = HGT_SOURCE_DEGREE_SIZE_L09_L13;

    // create look-up tables - source degree size for each LOD
    for (i=0; i<=3; i++)  HGTsourceSkippingLookUp[i] = pow(2, 3-i);
    for (i=4; i<=8; i++)  HGTsourceSkippingLookUp[i] = pow(2, 8-i);
    for (i=9; i<=13; i++) HGTsourceSkippingLookUp[i] = pow(2, 13-i);

    // setup avability tables by reading each HGT files directory
    setupAvabilityTables();
}

CCacheManager *CCacheManager::getInstance()
{
    return instance;
}

void CCacheManager::setupAvabilityTables()
{
    int i, index;
    double tlLon, tlLat;
    QDir dir;
    QFileInfo fileInfo;
    QFileInfoList list;
    int L00_L03_width  = (int)(360.0 / HGT_SOURCE_DEGREE_SIZE_L00_L03);
    int L00_L03_height = (int)(180.0 / HGT_SOURCE_DEGREE_SIZE_L00_L03);
    int L04_L08_width  = (int)(360.0 / HGT_SOURCE_DEGREE_SIZE_L04_L08);
    int L04_L08_height = (int)(180.0 / HGT_SOURCE_DEGREE_SIZE_L04_L08);
    int L09_L13_width  = (int)(360.0 / HGT_SOURCE_DEGREE_SIZE_L09_L13);
    int L09_L13_height = (int)(180.0 / HGT_SOURCE_DEGREE_SIZE_L09_L13);
    int SRTM_width     = (int)(360.0 / HGT_SOURCE_DEGREE_SIZE_SRTM);
    int SRTM_height    = (int)(180.0 / HGT_SOURCE_DEGREE_SIZE_SRTM);

    avability_L00_L03 = new CAvability[L00_L03_width * L00_L03_height];
    avability_L04_L08 = new CAvability[L04_L08_width * L04_L08_height];
    avability_L09_L13 = new CAvability[L09_L13_width * L09_L13_height];
    avability_SRTM    = new CAvability[SRTM_width * SRTM_height];

    // L00-L03 levels
    dir.setPath(pathL00_L03);
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    list = dir.entryInfoList();

    for (i=0; i<list.size(); i++) {
        fileInfo = list.at(i);
        if (fileInfo.size()==8450 && fileInfo.suffix()=="hgt") {
            convertFileNameToLonLat(fileInfo.fileName(), &tlLon, &tlLat);
            convertTopLeft2AvabilityIndex(tlLon, tlLat, HGT_SOURCE_DEGREE_SIZE_L00_L03, &index);
            avability_L00_L03[index].setAvailable(fileInfo.fileName());
        }
    }

    // L04-L08 levels
    dir.setPath(pathL04_L08);
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    list = dir.entryInfoList();

    for (i=0; i<list.size(); i++) {
        fileInfo = list.at(i);
        if (fileInfo.size()==526338 && fileInfo.suffix()=="hgt") {
            convertFileNameToLonLat(fileInfo.fileName(), &tlLon, &tlLat);
            convertTopLeft2AvabilityIndex(tlLon, tlLat, HGT_SOURCE_DEGREE_SIZE_L04_L08, &index);
            avability_L04_L08[index].setAvailable(fileInfo.fileName());
        }
    }

    // L09-L13 levels
    dir.setPath(pathL09_L13);
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    list = dir.entryInfoList();

    for (i=0; i<list.size(); i++) {
        fileInfo = list.at(i);
        if (fileInfo.size()==33570818 && fileInfo.suffix()=="hgt") {
            convertFileNameToLonLat(fileInfo.fileName(), &tlLon, &tlLat);
            convertTopLeft2AvabilityIndex(tlLon, tlLat, HGT_SOURCE_DEGREE_SIZE_L09_L13, &index);
            avability_L09_L13[index].setAvailable(fileInfo.fileName());
        }
    }

    // SRTM files
    dir.setPath(pathSRTM);
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    list = dir.entryInfoList();

    for (i=0; i<list.size(); i++) {
        fileInfo = list.at(i);
        if (fileInfo.size()==2884802 && fileInfo.suffix()=="hgt") {
            convertSRTMfileNameToLonLat(fileInfo.fileName(), &tlLon, &tlLat);
            convertTopLeft2AvabilityIndex(tlLon, tlLat, HGT_SOURCE_DEGREE_SIZE_SRTM, &index);
            avability_SRTM[index].setAvailable(fileInfo.fileName());
        }
    }
}

void CCacheManager::findTopLeftCorner(const double &lon, const double &lat, const double &degreeSize, double *tlLon, double *tlLat)
{
    double lonX, latY;
    double correctedLon;

    correctedLon = lon;
    if (correctedLon>=360.0) correctedLon -= 360.0;
    if (correctedLon<0.0) correctedLon += 360.0;

    convertLonLatToCartesian(correctedLon, lat, &lonX, &latY);       // convert to more friendly coordinate system

    lonX = floor(lonX / degreeSize) * degreeSize;     // find closest top left corner on 'sourceDegreeSize' Earth grid
    latY = floor(latY / degreeSize) * degreeSize;     // find closest top left corner on 'sourceDegreeSize' Earth grid

    convertCartesianToLonLat(lonX, latY, tlLon, tlLat);     // convert back to lon-lat format
}

void CCacheManager::findTopLeftCornerOfHgtFile(const double &lon, const double &lat, const int &lod, double *tlLon, double *tlLat)
{
    double sourceDegreeSize = HGTsourceDegreeSizeLookUp[lod];    // get deg size of source terrain files belongs to LOD
    findTopLeftCorner(lon, lat, sourceDegreeSize, tlLon, tlLat);
}

void CCacheManager::findXYInHgtFile(const double &tlLon, const double &tlLat, const double &lon, const double &lat, const int &lod, int *x, int *y)
{
    double deltaLon, deltaLat;
    double tlLonInHgt, tlLatInHgt;
    double HGTsourceDegreeSize = HGTsourceDegreeSizeLookUp[lod];
    int    HGTsourceSize       = HGTsourceSizeLookUp[lod];
    double LODdegreeSize       = LODdegreeSizeLookUp[lod];

    // find top left corner on grid on current LOD
    findTopLeftCorner(lon, lat, LODdegreeSize, &tlLonInHgt, &tlLatInHgt);

    deltaLon = tlLonInHgt - tlLon;
    deltaLat = tlLat - tlLatInHgt;

    // find x/y pos in HGT file
    (*x) = (int)( (deltaLon / HGTsourceDegreeSize) * (HGTsourceSize - 1) );
    (*y) = (int)( (deltaLat / HGTsourceDegreeSize) * (HGTsourceSize - 1) );
}

void CCacheManager::convertTopLeft2AvabilityIndex(const double &tlLon, const double &tlLat, const double &degreeSize, int *index)
{
    double correctedLon;
    double tlLonX, tlLatY;
    int tlLonIndexX, tlLatIndexY;

    correctedLon = tlLon;
    if (correctedLon>360.0) correctedLon -= 360.0;
    if (correctedLon<0.0) correctedLon += 360.0;
    convertLonLatToCartesian(correctedLon, tlLat, &tlLonX, &tlLatY);       // convert to more friendly coordinate system

    tlLonIndexX = (int)( (tlLonX / degreeSize) + 0.5 );     // find closest top left corner on 'sourceDegreeSize' Earth grid
    tlLatIndexY = (int)( (tlLatY / degreeSize) + 0.5 );     // find closest top left corner on 'sourceDegreeSize' Earth grid

    (*index) = tlLatIndexY * ((int)( (360.0 / degreeSize) + 0.5 )) + tlLonIndexX;
}

void CCacheManager::convertAvabilityIndex2TopLeft(const int &index, const double &degreeSize, double *tlLon, double *tlLat)
{
    double tlLonX, tlLatY;
    int tmpX, tmpY;

    tmpX = index % ((int)( (360.0 / degreeSize) + 0.5 ));
    tmpY = index / ((int)( (360.0 / degreeSize) + 0.5 ));

    tlLonX = tmpX * degreeSize;
    tlLatY = tmpY * degreeSize;

    convertCartesianToLonLat(tlLonX, tlLatY, tlLon, tlLat);
}

void CCacheManager::convertSRTMfileNameToLonLat(const QString &name, double *lon, double *lat)
{
    QString tmp;
    tmp = name.right(11).left(7);
    QChar fnLatSide = tmp.at(0);
    QChar fnLonSide = tmp.at(3);
    QString fnLat = tmp.right(6).left(2);
    QString fnLon = tmp.right(3);

    (*lat) = fnLat.toInt();
    (*lon) = fnLon.toInt();

    if (fnLonSide==QChar('W')) {
        (*lon) = 360.0 - (*lon);
    }
    if (fnLatSide==QChar('S')) {
        (*lat) *= -1.0;
    }

    (*lat) = (*lat) + 1.0;          // lat+1.0   -> my mistake, SRTM lonlat in filename is >lower< left corner
                                    //              not upper left... :(
}

void CCacheManager::convertLonLatToSRTMfileName(const double &lon, const double &lat, QString *name)
{
    double tmpLon, tmpLat;
    QChar fnLonSide, fnLatSide;
    QString fnLon, fnLat;

    if (lon>=180.0) {
        tmpLon = 360.0 - lon;
        fnLonSide = 'W';
    } else {
        tmpLon = lon;
        fnLonSide = 'E';
    }

    if ((lat-1.0)>=0.0) {          // lat-1.0   -> my mistake, SRTM lonlat in filename is >lower< left corner
        tmpLat = (lat-1.0);        //              not upper left... :(
        fnLatSide = 'N';
    } else {
        tmpLat = -1.0 * (lat-1.0);
        fnLatSide = 'S';
    }

    fnLon = QString::number(tmpLon, 'f', 0).rightJustified(3, QChar('0'));
    fnLat = QString::number(tmpLat, 'f', 0).rightJustified(2, QChar('0'));

    (*name) = fnLatSide + fnLat + fnLonSide + fnLon + ".hgt";
}

void CCacheManager::convertFileNameToLonLat(const QString &name, double *lon, double *lat)
{
    QString tmp;
    tmp = name.right(18).left(14);
    QChar fnLatSide = tmp.at(0);
    QChar fnLonSide = tmp.at(7);
    QString fnLat = tmp.right(13).left(5);
    QString fnLon = tmp.right(6);

    (*lat) = fnLat.toDouble();
    (*lon) = fnLon.toDouble();

    if (fnLonSide==QChar('W')) {
        (*lon) = 360.0 - (*lon);
    }
    if (fnLatSide==QChar('S')) {
        (*lat) *= -1.0;
    }
}

void CCacheManager::convertLonLatToFileName(const double &lon, const double &lat, QString *name)
{
    double tmpLon, tmpLat;
    QChar fnLonSide, fnLatSide;
    QString fnLon, fnLat;

    if (lon>=180.0) {
        tmpLon = 360.0 - lon;
        fnLonSide = 'W';
    } else {
        tmpLon = lon;
        fnLonSide = 'E';
    }

    if (lat>=0.0) {
        tmpLat = lat;
        fnLatSide = 'N';
    } else {
        tmpLat = -1.0 * lat;
        fnLatSide = 'S';
    }

    fnLon = QString::number(tmpLon, 'f', 2).rightJustified(6, QChar('0'));
    fnLat = QString::number(tmpLat, 'f', 2).rightJustified(5, QChar('0'));

    (*name) = fnLatSide + fnLat + '_' + fnLonSide + fnLon + ".hgt";
    (*name)[3] = ',';
    (*name)[11] = ',';
}

void CCacheManager::convertLonLatToCartesian(const double &lon, const double &lat, double *lonX, double *latY)
{
    (*lonX) = lon;
    (*latY) = 90.0 - lat;
}

void CCacheManager::convertCartesianToLonLat(const double &lonX, const double &latY, double *lon, double *lat)
{
    (*lon) = lonX;
    (*lat) = 90.0 - latY;
}

int CCacheManager::getNeighborAvabilityIndex(const int &baseIndex, const double &degreeSize, const int &dx, const int &dy)
{
    int baseX, baseY;
    int newX, newY;
    int maxX, maxY;

    maxX = ((int)( (360.0 / degreeSize) + 0.5 )) - 1;
    maxY = ((int)( (180.0 / degreeSize) + 0.5 )) - 1;
    baseX = baseIndex % ((int)( (360.0 / degreeSize) + 0.5 ));
    baseY = baseIndex / ((int)( (360.0 / degreeSize) + 0.5 ));

    newX = baseX + dx;
    newY = baseY + dy;

    if (newX<0) newX = newX + (maxX + 1);
    if (newX>maxX) newX = newX - (maxX + 1);

    if (newY<0) return -1;
    if (newY>maxY) return -1;

    return newY*(maxX+1) + newX;
}


