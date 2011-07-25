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

#ifndef CCACHEMANAGER_H
#define CCACHEMANAGER_H

#include <QString>
#include "CAvability.h"

#define HGT_SOURCE_L00_L03                 0
#define HGT_SOURCE_L04_L08                 1
#define HGT_SOURCE_L09_L13                 2
#define HGT_SOURCE_SRTM                   10
#define HGT_SOURCE_SIZE_L00_L03           65
#define HGT_SOURCE_SIZE_L04_L08          513
#define HGT_SOURCE_SIZE_L09_L13         4097
#define HGT_SOURCE_SIZE_SRTM            1201
#define HGT_SOURCE_DEGREE_SIZE_L00_L03    60.00
#define HGT_SOURCE_DEGREE_SIZE_L04_L08    15.00
#define HGT_SOURCE_DEGREE_SIZE_L09_L13     3.75
#define HGT_SOURCE_DEGREE_SIZE_SRTM        1.00

class CCacheManager
{
private:
    static CCacheManager *instance;

public:
    QString pathBase;
    QString pathL00_L03;
    QString pathL04_L08;
    QString pathL09_L13;
    QString pathSRTM;
    QString pathL00_L03_index;
    QString pathL04_L08_index;
    QString pathL09_L13_index;
    QString pathSRTM_index;
    double LODdegreeSizeLookUp[14];
    int HGTsourceLookUp[14];
    double HGTsourceDegreeSizeLookUp[14];
    int HGTsourceSizeLookUp[14];
    int HGTsourceSkippingLookUp[14];

    CAvability *avability_L00_L03;    // tile size = 60.00 deg
    CAvability *avability_L04_L08;    // tile size = 15.00 deg
    CAvability *avability_L09_L13;    // tile size =  3.75 deg
    CAvability *avability_SRTM;       // tile size =  1.00 deg

    CCacheManager();
    static CCacheManager *getInstance();

    void findTopLeftCorner(const double &lon, const double &lat, const double &degreeSize, double *tlLon, double *tlLat);
    void findTopLeftCornerOfHgtFile(const double &lon, const double &lat, const int &lod, double *tlLon, double *tlLat);
    void findXYInHgtFile(const double &tlLon, const double &tlLat, const double &lon, const double &lat, const int &lod, int *x, int *y);
    void convertTopLeft2AvabilityIndex(const double &tlLon, const double &tlLat, const double &degreeSize, int *index);
    void convertAvabilityIndex2TopLeft(const int &index, const double &degreeSize, double *tlLon, double *tlLat);
    void convertSRTMfileNameToLonLat(const QString &name, double *lon, double *lat);
    void convertLonLatToSRTMfileName(const double &lon, const double &lat, QString *name);
    void convertFileNameToLonLat(const QString &name, double *lon, double *lat);
    void convertLonLatToFileName(const double &lon, const double &lat, QString *name);
    void convertLonLatToCartesian(const double &lon, const double &lat, double *lonX, double *latY);
    void convertCartesianToLonLat(const double &lonX, const double &latY, double *lon, double *lat);
    void setupAvabilityTables();
    int getNeighborAvabilityIndex(const int &baseIndex, const double &degreeSize, const int &dx, const int &dy);
};

#endif // CCACHEMANAGER_H
