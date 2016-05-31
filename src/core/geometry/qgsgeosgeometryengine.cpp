/***************************************************************************
    qgsgeosgeometryengine.cpp
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

#include "qgsgeos.h"
#include "qgsgeosgeometryengine.h"
#include "qgsmessagelog.h"
#include "qgslogger.h"
#include "qgsmulticurvev2.h"
#include "qgsmultilinestringv2.h"
#include "qgsmultipointv2.h"
#include "qgsmultipolygonv2.h"


/***************************************************************************
 * This class is considered CRITICAL and any change MUST be accompanied    *
 * with full unit tests.                                                   *
 * See details in QEP #17                                                  *
 ***************************************************************************/

#define CATCH_GEOS(r) \
  catch (GEOSExceptionV2 &e) \
  { \
    QgsMessageLog::logMessage( QObject::tr( "Exception: %1" ).arg( e.what() ), QObject::tr("GEOS") ); \
    return r; \
  }

#define CATCH_GEOS_WITH_ERRMSG(r) \
  catch (GEOSExceptionV2 &e) \
  { \
    QgsMessageLog::logMessage( QObject::tr( "Exception: %1" ).arg( e.what() ), QObject::tr("GEOS") ); \
    if ( errorMsg ) \
    { \
      *errorMsg = e.what(); \
    } \
    return r; \
  }

/// @cond PRIVATE

static void throwGEOSException( const char *fmt, ... )
{
  va_list ap;
  char buffer[1024];

  va_start( ap, fmt );
  vsnprintf( buffer, sizeof buffer, fmt, ap );
  va_end( ap );

  QgsDebugMsg( QString( "GEOS exception: %1" ).arg( buffer ) );

  throw GEOSExceptionV2( QString::fromUtf8( buffer ) );
}

static void printGEOSNotice( const char *fmt, ... )
{
#if defined(QGISDEBUG)
  va_list ap;
  char buffer[1024];

  va_start( ap, fmt );
  vsnprintf( buffer, sizeof buffer, fmt, ap );
  va_end( ap );

  QgsDebugMsg( QString( "GEOS notice: %1" ).arg( QString::fromUtf8( buffer ) ) );
#else
  Q_UNUSED( fmt );
#endif
}

class GEOSInit
{
  public:
    GEOSContextHandle_t ctxt;

    GEOSInit()
    {
      ctxt = initGEOS_r( printGEOSNotice, throwGEOSException );
    }
    ~GEOSInit()
    {
      finishGEOS_r( ctxt );
    }

  private:
    GEOSInit( const GEOSInit& rh );
    GEOSInit& operator=( const GEOSInit& rh );
};

static GEOSInit geosinit;

///@endcond


/**
 * @brief Scoped GEOS pointer
 * @note not available in Python bindings
 */
class GEOSGeomScopedPtr
{
  public:
    explicit GEOSGeomScopedPtr( GEOSGeometry* geom = nullptr ) : mGeom( geom )
    {
    }
    ~GEOSGeomScopedPtr()
    {
      GEOSGeom_destroy_r( geosinit.ctxt, mGeom );
    }
    
    GEOSGeometry* get() const
    {
      return mGeom;
    }
    operator bool() const
    {
      return nullptr != mGeom;
    }
    void reset( GEOSGeometry* geom )
    {
      GEOSGeom_destroy_r( geosinit.ctxt, mGeom );
      mGeom = geom;
    }

  private:
    GEOSGeometry* mGeom;

  private:
    GEOSGeomScopedPtr( const GEOSGeomScopedPtr& rh );
    GEOSGeomScopedPtr& operator=( const GEOSGeomScopedPtr& rh );
};


/***************************************************************************
 *                  QgsGeosGeometryEngine implementation                   *
 ***************************************************************************/

QgsGeosGeometryEngine::QgsGeosGeometryEngine( const QgsGeometry& geometry, double precision )
    : QgsSimpleFeatureGeometryEngine( geometry, precision )
    , mGeosPrepared( nullptr )
{
}

QgsGeosGeometryEngine::~QgsGeosGeometryEngine()
{
  GEOSPreparedGeom_destroy_r( geosinit.ctxt, mGeosPrepared );
  mGeosPrepared = nullptr;
}

void QgsGeosGeometryEngine::geometryChanged()
{
  if ( mGeosPrepared )
  {
    prepareGeometry();
  }
}

void QgsGeosGeometryEngine::prepareGeometry()
{
  GEOSPreparedGeom_destroy_r( geosinit.ctxt, mGeosPrepared );
  mGeosPrepared = nullptr;

  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );
  if ( currentGeos )
  {
    mGeosPrepared = GEOSPrepare_r( geosinit.ctxt, currentGeos );
  }
}

QgsGeometry QgsGeosGeometryEngine::overlay( const QgsGeometry& other, Overlay op, QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );
  const GEOSGeometry* otherGeos = other.asGeos( mPrecision );

  if ( !currentGeos || !otherGeos )
  {
    return QgsGeometry();
  }

  try
  {
    GEOSGeomScopedPtr geos;

    switch ( op )
    {
      case INTERSECTION:
        geos.reset( GEOSIntersection_r( geosinit.ctxt, currentGeos, otherGeos ) );
        break;
      case DIFFERENCE:
        geos.reset( GEOSDifference_r( geosinit.ctxt, currentGeos, otherGeos ) );
        break;
      case UNION:
      {
        GEOSGeometry *unionGeometry = GEOSUnion_r( geosinit.ctxt, currentGeos, otherGeos );

        if ( unionGeometry && GEOSGeomTypeId_r( geosinit.ctxt, unionGeometry ) == GEOS_MULTILINESTRING )
        {
          GEOSGeometry *mergedLines = GEOSLineMerge_r( geosinit.ctxt, unionGeometry );
          if ( mergedLines )
          {
            GEOSGeom_destroy_r( geosinit.ctxt, unionGeometry );
            unionGeometry = mergedLines;
          }
        }
        geos.reset( unionGeometry );
        break;
      }
      case SYMDIFFERENCE:
        geos.reset( GEOSSymDifference_r( geosinit.ctxt, currentGeos, otherGeos ) );
        break;

      // unknown op
      default:
        throw GEOSExceptionV2( QString( "Unknown overlay operation: %1" ).arg( op ) );
    }

    return QgsGeometry( QgsGeos::fromGeos( geos.get() ) );
  }
  CATCH_GEOS_WITH_ERRMSG( QgsGeometry() )
}

QgsGeometry QgsGeosGeometryEngine::intersection( const QgsGeometry& other, QString* errorMsg ) const
{
  return overlay( other, INTERSECTION, errorMsg );
}

QgsGeometry QgsGeosGeometryEngine::difference( const QgsGeometry& other, QString* errorMsg ) const
{
  return overlay( other, DIFFERENCE, errorMsg );
}

QgsGeometry QgsGeosGeometryEngine::combine( const QgsGeometry& other, QString* errorMsg ) const
{
  return overlay( other, UNION, errorMsg );
}

QgsGeometry QgsGeosGeometryEngine::combine( const QList<QgsGeometry*>& geometryList, QString* errorMsg ) const
{
  if ( geometryList.size() == 0 )
    return QgsGeometry();

  if ( geometryList.size() == 1 )
    return *geometryList.at( 0 );

  QVector< GEOSGeometry* > geosGeometries;
  geosGeometries.resize( geometryList.size() );
  for ( int i = 0; i < geometryList.size(); ++i )
  {
    geosGeometries[i] = QgsGeos::asGeos( geometryList.at( i )->geometry(), mPrecision );
  }

  GEOSGeometry* geomUnion = nullptr;
  try
  {
    GEOSGeometry* geomCollection = createGeosCollection( GEOS_GEOMETRYCOLLECTION, geosGeometries );
    geomUnion = GEOSUnaryUnion_r( geosinit.ctxt, geomCollection );
    GEOSGeom_destroy_r( geosinit.ctxt, geomCollection );
  }
  CATCH_GEOS_WITH_ERRMSG( QgsGeometry() )

  QgsAbstractGeometryV2* result = QgsGeos::fromGeos( geomUnion );
  GEOSGeom_destroy_r( geosinit.ctxt, geomUnion );
  return result ? QgsGeometry( result ) : QgsGeometry();
}

QgsGeometry QgsGeosGeometryEngine::symDifference( const QgsGeometry& other, QString* errorMsg ) const
{
  return overlay( other, SYMDIFFERENCE, errorMsg );
}

QgsGeometry QgsGeosGeometryEngine::buffer( double distance, int segments, QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );

  if ( !currentGeos )
  {
    return QgsGeometry();
  }

  try
  {
    GEOSGeomScopedPtr geos;
    geos.reset( GEOSBuffer_r( geosinit.ctxt, currentGeos, distance, segments ) );

    return QgsGeometry( QgsGeos::fromGeos( geos.get() ) );
  }
  CATCH_GEOS_WITH_ERRMSG( QgsGeometry() )
}

QgsGeometry QgsGeosGeometryEngine::buffer( double distance, int segments, int endCapStyle, int joinStyle, double mitreLimit, QString* errorMsg ) const
{
#if defined(GEOS_VERSION_MAJOR) && defined(GEOS_VERSION_MINOR) && \
 ((GEOS_VERSION_MAJOR>3) || ((GEOS_VERSION_MAJOR==3) && (GEOS_VERSION_MINOR>=3)))

  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );

  if ( !currentGeos )
  {
    return QgsGeometry();
  }

  try
  {
    GEOSGeomScopedPtr geos;
    geos.reset( GEOSBufferWithStyle_r( geosinit.ctxt, currentGeos, distance, segments, endCapStyle, joinStyle, mitreLimit ) );

    return QgsGeometry( QgsGeos::fromGeos( geos.get() ) );
  }
  CATCH_GEOS_WITH_ERRMSG( QgsGeometry() )
#else
  return buffer( distance, segments, errorMsg );
#endif
}

QgsGeometry QgsGeosGeometryEngine::simplify( double tolerance, bool preservingTopology, QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );

  if ( !currentGeos )
  {
    return QgsGeometry();
  }

  try
  {
    GEOSGeomScopedPtr geos;

    if ( preservingTopology )
    {
      geos.reset( GEOSTopologyPreserveSimplify_r( geosinit.ctxt, currentGeos, tolerance ) );
    }
    else
    {
      geos.reset( GEOSSimplify_r( geosinit.ctxt, currentGeos, tolerance ) );
    }

    return QgsGeometry( QgsGeos::fromGeos( geos.get() ) );
  }
  CATCH_GEOS_WITH_ERRMSG( QgsGeometry() )
}

QgsGeometry QgsGeosGeometryEngine::interpolate( double distance, QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );

  if ( !currentGeos )
  {
    return QgsGeometry();
  }

  try
  {
    GEOSGeomScopedPtr geos;
    geos.reset( GEOSInterpolate_r( geosinit.ctxt, currentGeos, distance ) );

    return QgsGeometry( QgsGeos::fromGeos( geos.get() ) );
  }
  CATCH_GEOS_WITH_ERRMSG( QgsGeometry() )
}

QgsGeometry QgsGeosGeometryEngine::envelope( QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );

  if ( !currentGeos )
  {
    return QgsGeometry();
  }

  try
  {
    GEOSGeomScopedPtr geos;
    geos.reset( GEOSEnvelope_r( geosinit.ctxt, currentGeos ) );

    return QgsGeometry( QgsGeos::fromGeos( geos.get() ) );
  }
  CATCH_GEOS_WITH_ERRMSG( QgsGeometry() )
}

bool QgsGeosGeometryEngine::centroid( QgsPointV2& point, QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );

  if ( !currentGeos )
  {
    return false;
  }

  try
  {
    GEOSGeomScopedPtr geos;
    geos.reset( GEOSGetCentroid_r( geosinit.ctxt, currentGeos ) );

    if ( !geos )
    {
      return false;
    }

    double x, y;
    GEOSGeomGetX_r( geosinit.ctxt, geos.get(), &x );
    GEOSGeomGetY_r( geosinit.ctxt, geos.get(), &y );
    point.setX( x );
    point.setY( y );
  }
  CATCH_GEOS_WITH_ERRMSG( false );

  return true;
}

bool QgsGeosGeometryEngine::pointOnSurface( QgsPointV2& point, QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );

  if ( !currentGeos )
  {
    return false;
  }
  
  try
  {
    GEOSGeomScopedPtr geos;
    geos.reset( GEOSPointOnSurface_r( geosinit.ctxt, currentGeos ) );

    if ( !geos || GEOSisEmpty_r( geosinit.ctxt, geos.get() ) != 0 )
    {
      return false;
    }

    double x, y;
    GEOSGeomGetX_r( geosinit.ctxt, geos.get(), &x );
    GEOSGeomGetY_r( geosinit.ctxt, geos.get(), &y );
    point.setX( x );
    point.setY( y );
  }
  CATCH_GEOS_WITH_ERRMSG( false );

  return true;
}

QgsGeometry QgsGeosGeometryEngine::convexHull( QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );

  if ( !currentGeos )
  {
    return QgsGeometry();
  }

  try
  {
    GEOSGeomScopedPtr geos;
    geos.reset( GEOSConvexHull_r( geosinit.ctxt, currentGeos ) );

    return QgsGeometry( QgsGeos::fromGeos( geos.get() ) );
  }
  CATCH_GEOS_WITH_ERRMSG( QgsGeometry() )
}

double QgsGeosGeometryEngine::distance( const QgsGeometry& other, QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );
  const GEOSGeometry* otherGeos = other.asGeos( mPrecision );

  if ( !currentGeos || !otherGeos )
  {
    return -1.0;
  }

  try
  {
    double distance = -1.0;

    if ( GEOSDistance_r( geosinit.ctxt, currentGeos, otherGeos, &distance ) != 1)
      return -1.0;

    return distance;
  }
  CATCH_GEOS_WITH_ERRMSG( -1.0 );
}

QgsGeometry QgsGeosGeometryEngine::offsetCurve( double distance, int segments, int joinStyle, double mitreLimit, QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );

  if ( !currentGeos )
  {
    return QgsGeometry();
  }

  try
  {
    GEOSGeomScopedPtr geos;
    geos.reset( GEOSOffsetCurve_r( geosinit.ctxt, currentGeos, distance, segments, joinStyle, mitreLimit ) );

    return QgsGeometry( QgsGeos::fromGeos( geos.get() ) );
  }
  CATCH_GEOS_WITH_ERRMSG( QgsGeometry() )
}

bool QgsGeosGeometryEngine::intersects( const QgsGeometry& other, QString* errorMsg ) const
{
  return relation( other, INTERSECTS, errorMsg );
}

bool QgsGeosGeometryEngine::touches( const QgsGeometry& other, QString* errorMsg ) const
{
  return relation( other, TOUCHES, errorMsg );
}

bool QgsGeosGeometryEngine::crosses( const QgsGeometry& other, QString* errorMsg ) const
{
  return relation( other, CROSSES, errorMsg );
}

bool QgsGeosGeometryEngine::within( const QgsGeometry& other, QString* errorMsg ) const
{
  return relation( other, WITHIN, errorMsg );
}

bool QgsGeosGeometryEngine::overlaps( const QgsGeometry& other, QString* errorMsg ) const
{
  return relation( other, OVERLAPS, errorMsg );
}

bool QgsGeosGeometryEngine::contains( const QgsGeometry& other, QString* errorMsg ) const
{
  return relation( other, CONTAINS, errorMsg );
}

bool QgsGeosGeometryEngine::disjoint( const QgsGeometry& other, QString* errorMsg ) const
{
  return relation( other, DISJOINT, errorMsg );
}

bool QgsGeosGeometryEngine::relation( const QgsGeometry& other, Relation r, QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );
  const GEOSGeometry* otherGeos = other.asGeos( mPrecision );

  if ( !currentGeos || !otherGeos )
  {
    return false;
  }

  bool result = false;
  try
  {
    if ( mGeosPrepared ) // use faster version with prepared geometry
    {
      switch ( r )
      {
        case INTERSECTS:
          result = ( GEOSPreparedIntersects_r( geosinit.ctxt, mGeosPrepared, otherGeos ) == 1 );
          break;
        case TOUCHES:
          result = ( GEOSPreparedTouches_r( geosinit.ctxt, mGeosPrepared, otherGeos ) == 1 );
          break;
        case CROSSES:
          result = ( GEOSPreparedCrosses_r( geosinit.ctxt, mGeosPrepared, otherGeos ) == 1 );
          break;
        case WITHIN:
          result = ( GEOSPreparedWithin_r( geosinit.ctxt, mGeosPrepared, otherGeos ) == 1 );
          break;
        case CONTAINS:
          result = ( GEOSPreparedContains_r( geosinit.ctxt, mGeosPrepared, otherGeos ) == 1 );
          break;
        case DISJOINT:
          result = ( GEOSPreparedDisjoint_r( geosinit.ctxt, mGeosPrepared, otherGeos ) == 1 );
          break;
        case OVERLAPS:
          result = ( GEOSPreparedOverlaps_r( geosinit.ctxt, mGeosPrepared, otherGeos ) == 1 );
          break;

        // unknown relation
        default:
          throw GEOSExceptionV2( QString( "Unknown relation operation: %1" ).arg( r ) );
      }
      return result;
    }

    switch ( r )
    {
      case INTERSECTS:
        result = ( GEOSIntersects_r( geosinit.ctxt, currentGeos, otherGeos ) == 1 );
        break;
      case TOUCHES:
        result = ( GEOSTouches_r( geosinit.ctxt, currentGeos, otherGeos ) == 1 );
        break;
      case CROSSES:
        result = ( GEOSCrosses_r( geosinit.ctxt, currentGeos, otherGeos ) == 1 );
        break;
      case WITHIN:
        result = ( GEOSWithin_r( geosinit.ctxt, currentGeos, otherGeos ) == 1 );
        break;
      case CONTAINS:
        result = ( GEOSContains_r( geosinit.ctxt, currentGeos, otherGeos ) == 1 );
        break;
      case DISJOINT:
        result = ( GEOSDisjoint_r( geosinit.ctxt, currentGeos, otherGeos ) == 1 );
        break;
      case OVERLAPS:
        result = ( GEOSOverlaps_r( geosinit.ctxt, currentGeos, otherGeos ) == 1 );
        break;

      // unknown relation
      default:
        throw GEOSExceptionV2( QString( "Unknown relation operation: %1" ).arg( r ) );
    }
    return result;
  }
  CATCH_GEOS_WITH_ERRMSG( false );
}

QString QgsGeosGeometryEngine::relate( const QgsGeometry& other, QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );
  const GEOSGeometry* otherGeos = other.asGeos( mPrecision );

  if ( !currentGeos || !otherGeos )
  {
    return QString();
  }

  try
  {
    QString result;

    char* r = GEOSRelate_r( geosinit.ctxt, currentGeos, otherGeos );
    if ( r )
    {
      result = QString( r );
      GEOSFree_r( geosinit.ctxt, r );
    }
    return result;
  }
  CATCH_GEOS_WITH_ERRMSG( QString() );
}

bool QgsGeosGeometryEngine::relatePattern( const QgsGeometry& other, const QString& pattern, QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );
  const GEOSGeometry* otherGeos = other.asGeos( mPrecision );

  if ( !currentGeos || !otherGeos )
  {
    return false;
  }

  try
  {
    return ( GEOSRelatePattern_r( geosinit.ctxt, currentGeos, otherGeos, pattern.toLocal8Bit().constData() ) == 1 );
  }
  CATCH_GEOS_WITH_ERRMSG( false );
}

double QgsGeosGeometryEngine::area( QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );

  if ( !currentGeos )
  {
    return -1.0;
  }

  try
  {
    double area = -1.0;

    if ( GEOSArea_r( geosinit.ctxt, currentGeos, &area ) != 1 )
      return -1.0;

    return area;
  }
  CATCH_GEOS_WITH_ERRMSG( -1.0 );
}

double QgsGeosGeometryEngine::length( QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );  

  if ( !currentGeos )
  {
    return -1.0;
  }

  try
  {
    double length = -1.0;

    if ( GEOSLength_r( geosinit.ctxt, currentGeos, &length ) != 1 )
      return -1.0;

    return length;
  }
  CATCH_GEOS_WITH_ERRMSG( -1.0 );
}

bool QgsGeosGeometryEngine::isValid( QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );

  if ( !currentGeos )
  {
    return false;
  }

  try
  {
    return GEOSisValid_r( geosinit.ctxt, currentGeos );
  }
  CATCH_GEOS_WITH_ERRMSG( false );
}

bool QgsGeosGeometryEngine::isEqual( const QgsGeometry& other, QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );
  const GEOSGeometry* otherGeos = other.asGeos( mPrecision );

  if ( !currentGeos || !otherGeos )
  {
    return false;
  }

  try
  {
    return GEOSEquals_r( geosinit.ctxt, currentGeos, otherGeos );
  }
  CATCH_GEOS_WITH_ERRMSG( false );
}

bool QgsGeosGeometryEngine::isEmpty( QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );

  if ( !currentGeos )
  {
    return false;
  }

  try
  {
    return GEOSisEmpty_r( geosinit.ctxt, currentGeos );
  }
  CATCH_GEOS_WITH_ERRMSG( false );
}

/***************************************************************************
 *                     Other GEOS methods and utilities                    *
 ***************************************************************************/

GEOSContextHandle_t QgsGeosGeometryEngine::getGEOSHandler()
{
  return geosinit.ctxt;
}

QgsGeometry QgsGeosGeometryEngine::closestPoint( const QgsGeometry& other, QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );
  const GEOSGeometry* otherGeos = other.asGeos( mPrecision );

  if ( !currentGeos || !otherGeos )
  {
    return QgsGeometry();
  }

  try
  {
    GEOSCoordSequence* nearestCoord = GEOSNearestPoints_r( geosinit.ctxt, currentGeos, otherGeos );
    double nx = 0.0;
    double ny = 0.0;

    ( void )GEOSCoordSeq_getX_r( geosinit.ctxt, nearestCoord, 0, &nx );
    ( void )GEOSCoordSeq_getY_r( geosinit.ctxt, nearestCoord, 0, &ny );
    GEOSCoordSeq_destroy_r( geosinit.ctxt, nearestCoord );

    return QgsGeometry( new QgsPointV2( nx, ny ) );
  }
  CATCH_GEOS_WITH_ERRMSG( QgsGeometry() )
}

QgsGeometry QgsGeosGeometryEngine::shortestLine( const QgsGeometry& other, QString* errorMsg ) const
{
  const GEOSGeometry* currentGeos = mGeometry.asGeos( mPrecision );
  const GEOSGeometry* otherGeos = other.asGeos( mPrecision );

  if ( !currentGeos || !otherGeos )
  {
    return QgsGeometry();
  }

  try
  {
    GEOSCoordSequence* nearestCoord = GEOSNearestPoints_r( geosinit.ctxt, currentGeos, otherGeos );
    double nx1 = 0.0;
    double ny1 = 0.0;
    double nx2 = 0.0;
    double ny2 = 0.0;

    ( void )GEOSCoordSeq_getX_r( geosinit.ctxt, nearestCoord, 0, &nx1 );
    ( void )GEOSCoordSeq_getY_r( geosinit.ctxt, nearestCoord, 0, &ny1 );
    ( void )GEOSCoordSeq_getX_r( geosinit.ctxt, nearestCoord, 1, &nx2 );
    ( void )GEOSCoordSeq_getY_r( geosinit.ctxt, nearestCoord, 1, &ny2 );
    GEOSCoordSeq_destroy_r( geosinit.ctxt, nearestCoord );

    QgsLineStringV2* line = new QgsLineStringV2();
    line->addVertex( QgsPointV2( nx1, ny1 ) );
    line->addVertex( QgsPointV2( nx2, ny2 ) );
    return QgsGeometry( line );
  }
  CATCH_GEOS_WITH_ERRMSG( QgsGeometry() )
}

GEOSGeometry* QgsGeosGeometryEngine::createGeosCollection( int typeId, const QVector<GEOSGeometry*>& geoms )
{
  int nNullGeoms = geoms.count( nullptr );
  int nNotNullGeoms = geoms.size() - nNullGeoms;

  GEOSGeometry **geomarr = new GEOSGeometry*[ nNotNullGeoms ];
  if ( !geomarr )
  {
    return nullptr;
  }

  int i = 0;
  QVector<GEOSGeometry*>::const_iterator geomIt = geoms.constBegin();
  for ( ; geomIt != geoms.constEnd(); ++geomIt )
  {
    if ( *geomIt )
    {
      geomarr[i] = *geomIt;
      ++i;
    }
  }
  GEOSGeometry *geom = nullptr;

  try
  {
    geom = GEOSGeom_createCollection_r( geosinit.ctxt, typeId, geomarr, nNotNullGeoms );
  }
  catch ( GEOSException &e )
  {
    QgsMessageLog::logMessage( QObject::tr( "Exception: %1" ).arg( e.what() ), QObject::tr( "GEOS" ) );
  }

  delete [] geomarr;

  return geom;
}
