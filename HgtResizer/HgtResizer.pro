#-------------------------------------------------
#
# Project created by QtCreator 2011-05-15T21:33:11
#
#-------------------------------------------------

QT       += core

QT       += gui

TARGET = HgtResizer
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    CHgtFile.cpp \
    CCacheManager.cpp \
    CAvability.cpp \
    alglib/statistics.cpp \
    alglib/specialfunctions.cpp \
    alglib/solvers.cpp \
    alglib/optimization.cpp \
    alglib/linalg.cpp \
    alglib/interpolation.cpp \
    alglib/integration.cpp \
    alglib/fasttransforms.cpp \
    alglib/diffequations.cpp \
    alglib/dataanalysis.cpp \
    alglib/ap.cpp \
    alglib/alglibmisc.cpp \
    alglib/alglibinternal.cpp \
    CResizer.cpp

HEADERS += \
    CHgtFile.h \
    CCacheManager.h \
    CAvability.h \
    alglib/stdafx.h \
    alglib/statistics.h \
    alglib/specialfunctions.h \
    alglib/solvers.h \
    alglib/optimization.h \
    alglib/linalg.h \
    alglib/interpolation.h \
    alglib/integration.h \
    alglib/fasttransforms.h \
    alglib/diffequations.h \
    alglib/dataanalysis.h \
    alglib/ap.h \
    alglib/alglibmisc.h \
    alglib/alglibinternal.h \
    CResizer.h
