/***************************************************************************
 *  qgsgeometryduplicatecheck.cpp                                          *
 *  -------------------                                                    *
 *  copyright            : (C) 2014 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

#include "qgssimplefeaturegeometryengine.h"
#include "qgsgeometryduplicatecheck.h"
#include "qgsspatialindex.h"
#include "qgsgeometry.h"
#include "../utils/qgsfeaturepool.h"

void QgsGeometryDuplicateCheck::collectErrors( QList<QgsGeometryCheckError*>& errors, QStringList &messages, QAtomicInt* progressCounter , const QgsFeatureIds &ids ) const
{
  const QgsFeatureIds& featureIds = ids.isEmpty() ? mFeaturePool->getFeatureIds() : ids;
  Q_FOREACH ( QgsFeatureId featureid, featureIds )
  {
    if ( progressCounter ) progressCounter->fetchAndAddRelaxed( 1 );
    QgsFeature feature;
    if ( !mFeaturePool->get( featureid, feature ) )
    {
      continue;
    }
    QgsSimpleFeatureGeometryEngine* geomEngine = QgsGeomUtils::createGeometryEngineV2( *feature.geometry(), QgsGeometryCheckPrecision::tolerance() );

    QList<QgsFeatureId> duplicates;
    QgsFeatureIds ids = mFeaturePool->getIntersects( feature.geometry()->geometry()->boundingBox() );
    Q_FOREACH ( QgsFeatureId id, ids )
    {
      // > : only report overlaps once
      if ( id >= featureid )
      {
        continue;
      }
      QgsFeature testFeature;
      if ( !mFeaturePool->get( id, testFeature ) )
      {
        continue;
      }
      QString errMsg;
      QgsGeometry diffGeom = geomEngine->symDifference( *testFeature.geometry(), &errMsg );
      if ( !diffGeom.isEmpty() && diffGeom.geometry()->area() < QgsGeometryCheckPrecision::tolerance() )
      {
        duplicates.append( id );
      }
      else if ( diffGeom.isEmpty() )
      {
        messages.append( tr( "Duplicate check between features %1 and %2: %3" ).arg( feature.id() ).arg( testFeature.id() ).arg( errMsg ) );
      }
    }
    if ( !duplicates.isEmpty() )
    {
      qSort( duplicates );
      errors.append( new QgsGeometryDuplicateCheckError( this, featureid, feature.geometry()->geometry()->centroid(), duplicates ) );
    }
    delete geomEngine;
  }
}

void QgsGeometryDuplicateCheck::fixError( QgsGeometryCheckError* error, int method, int /*mergeAttributeIndex*/, Changes &changes ) const
{
  QgsFeature feature;
  if ( !mFeaturePool->get( error->featureId(), feature ) )
  {
    error->setObsolete();
    return;
  }

  if ( method == NoChange )
  {
    error->setFixed( method );
  }
  else if ( method == RemoveDuplicates )
  {
    QgsSimpleFeatureGeometryEngine* geomEngine = QgsGeomUtils::createGeometryEngineV2( *feature.geometry(), QgsGeometryCheckPrecision::tolerance() );

    QgsGeometryDuplicateCheckError* duplicateError = static_cast<QgsGeometryDuplicateCheckError*>( error );
    Q_FOREACH ( QgsFeatureId id, duplicateError->duplicates() )
    {
      QgsFeature testFeature;
      if ( !mFeaturePool->get( id, testFeature ) )
      {
        continue;
      }
      QgsGeometry diffGeom = geomEngine->symDifference( *testFeature.geometry() );
      if ( !diffGeom.isEmpty() && diffGeom.geometry()->area() < QgsGeometryCheckPrecision::tolerance() )
      {
        mFeaturePool->deleteFeature( testFeature );
        changes[id].append( Change( ChangeFeature, ChangeRemoved ) );
      }
    }
    delete geomEngine;
    error->setFixed( method );
  }
  else
  {
    error->setFixFailed( tr( "Unknown method" ) );
  }
}

const QStringList& QgsGeometryDuplicateCheck::getResolutionMethods() const
{
  static QStringList methods = QStringList()
                               << tr( "No action" )
                               << tr( "Remove duplicates" );
  return methods;
}
