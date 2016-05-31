/***************************************************************************
    qgssimplefeaturegeometryengine.cpp
    ---------------------
    begin                : 03.03.2016
    author               : Alvaro Huarte
    email                : http://wiki.osgeo.org/wiki/Alvaro_Huarte
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgssimplefeaturegeometryengine.h"

QgsSimpleFeatureGeometryEngine::QgsSimpleFeatureGeometryEngine( const QgsGeometry& geometry, double precision )
    : mGeometry( geometry )
    , mPrecision( precision )
{
}

/***************************************************************************
 * This class is considered CRITICAL and any change MUST be accompanied    *
 * with full unit tests.                                                   *
 * See details in QEP #17                                                  *
 ***************************************************************************/
