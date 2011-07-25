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

#ifndef CRESIZER_H
#define CRESIZER_H

#include "CCacheManager.h"

class CResizer
{
public:
    CCacheManager cacheManager;

    CResizer();
    void buildL09_L13TerrainFromSRTMEntireEarth();
    void buildL09_L13TerrainFromSRTM(const double &lon, const double &lat);
    void buildL09_L13TerrainFromSRTM(int L09_L13_index);

    void buildL04_L08TerrainFromL09_L13EntireEarth();
    void buildL04_L08TerrainFromL09_L13(const double &lon, const double &lat);
    void buildL04_L08TerrainFromL09_L13(int L04_L08_index);

    void buildL00_L03TerrainFromL04_L08EntireEarth();
    void buildL00_L03TerrainFromL04_L08(const double &lon, const double &lat);
    void buildL00_L03TerrainFromL04_L08(int L00_L03_index);

    void connectL09_L13TerrainEntireEarth();
    void connectL09_L13Terrain(const double &lon, const double &lat);
    void connectL09_L13Terrain(int L09_L13_index);

    void generateHtmlIndex(int hgtSource, bool createImages, double lon, double lat);

private:
    unsigned int getColor(int height);
    bool findSRTMFilesFor_L09_L13(const double &L09_L13_topLeftLon, const double &L09_L13_topLeftLat,
                                  int *SRTMfilesIndex, int *offsetLon, int *offsetLat);
};

#endif // CRESIZER_H
