/***************************************************************************
    qgsgeosgeometryengine.h
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

#ifndef QGSGEOSGEOMETRYENGINE_H
#define QGSGEOSGEOMETRYENGINE_H

#include "qgsgeometry.h"
#include "qgssimplefeaturegeometryengine.h"
#include <geos_c.h>

/**
 * This class implements QgsSimpleFeatureGeometryEngine using the GEOS library
 * according to the Open GIS Simple Feature Specification.
 */
class QgsGeosGeometryEngine : public QgsSimpleFeatureGeometryEngine
{
  public:
    /**
     * The caller is responsible that the geometry is available and unchanged
     * for the whole lifetime of this object.
     * @param geometry
     */
    QgsGeosGeometryEngine( const QgsGeometry& geometry, double precision = 0 );
    ~QgsGeosGeometryEngine();

  public:

    /**
     * Occurs when the geometry is changed.
     */
    virtual void geometryChanged() override;
    
    /**
     * Prepares the geometry in order to optimize the performance of repeated calls 
     * to specific geometric operations.
     */
    virtual void prepareGeometry() override;

    /**
     * Computes a geometry representing the point-set which is
     * common to both this geometry and the other geometry.
     *
     * The intersection of two geometries of different dimension produces 
     * a result geometry of dimension less than or equal to the minimum 
     * dimension of the input geometries.
     * The result geometry may be a heterogenous geometry collection.
     *
     * @param other the geometry with which to compute the intersection
     * @param errorMsg destination storage for any error message
     * @returns a geometry representing the point-set common to the two geometries
     */
    virtual QgsGeometry intersection( const QgsGeometry& other, QString* errorMsg = nullptr ) const override;

    /**
     * Computes a geometry representing the closure of the point-set
     * of the points contained in this geometry that are not contained in 
     * the other geometry. 
     * 
     * @param other the geometry with which to compute the difference
     * @param errorMsg destination storage for any error message
     * @returns a geometry representing the point-set difference of this geometry withother
     */
    virtual QgsGeometry difference( const QgsGeometry& other, QString* errorMsg = nullptr ) const override;
    
    /**
     * Computes a geometry representing the point-set which is contained in both this
     * geometry and the other geometry.
     * 
     * The union of two geometries of different dimension produces a result
     * geometry of dimension equal to the maximum dimension of the input geometries. 
     * The result geometry may be a heterogenous geometry collection.
     * 
     * @param other the geometry with which to compute the union
     * @param errorMsg destination storage for any error message
     * @returns a point-set combining the points of this geometry and the points of other
     */
    virtual QgsGeometry combine( const QgsGeometry& other, QString* errorMsg = nullptr ) const override;
    
    /**
     * Computes a geometry representing the point-set which is contained in all input geometries.
     * 
     * The union of several geometries of different dimension produces a result
     * geometry of dimension equal to the maximum dimension of the input geometries. 
     * The result geometry may be a heterogenous geometry collection.
     * 
     * @param geometryList the list of geometries to compute the union
     * @param errorMsg destination storage for any error message
     * @returns a point-set combining the points of all geometries
     */
    virtual QgsGeometry combine( const QList<QgsGeometry*>& geometryList, QString* errorMsg = nullptr ) const override;

    /**
     * Computes a geometry representing the closure of the point-set 
     * which is the union of the points in this geometry which are not 
     * contained in the other geometry, with the points in the other geometry 
     * not contained in this geometry. 
     *
     * @param other the geometry with which to compute the symmetric difference
     * @param errorMsg destination storage for any error message
     * @returns a geometry representing the point-set symmetric difference of this geometry with other
     */
    virtual QgsGeometry symDifference( const QgsGeometry& other, QString* errorMsg = nullptr ) const override;

    /**
     * Computes a buffer area around this geometry having the given width and with
     * a specified accuracy of approximation for circular arcs.
     * 
     * Mathematically-exact buffer area boundaries can contain circular arcs. 
     * To represent these arcs using linear geometry they must be approximated with line segments. 
     * The quadrantSegments argument allows controlling the accuracy of the approximation 
     * by specifying the number of line segments used to represent a quadrant of a circle.
     * 
     * The buffer operation always returns a polygonal result. The negative or
     * zero-distance buffer of lines and points is always an empty polygon.
     * This is also the result for the buffers of degenerate (zero-area) polygons.
     * 
     * @param distance the width of the buffer (may be positive, negative or 0)
     * @param segments the number of line segments used to represent a quadrant of a circle
     * @param errorMsg destination storage for any error message
     * @returns a polygonal geometry representing the buffer region (which may be empty)
     */
    virtual QgsGeometry buffer( double distance, int segments, QString* errorMsg = nullptr ) const override;

    /**
     * Computes a buffer area around this geometry having the given width and with 
     * a specified accuracy of approximation for circular arcs, 
     * and using a specified styles.
     * 
     * Mathematically-exact buffer area boundaries can contain circular arcs.
     * To represent these arcs using linear geometry they must be approximated with line segments.
     * The quadrantSegments argument allows controlling the accuracy of the approximation
     * by specifying the number of line segments used to represent a quadrant of a circle.
     * 
     * The buffer operation always returns a polygonal result. The negative or
     * zero-distance buffer of lines and points is always an empty polygon.
     * This is also the result for the buffers of degenerate (zero-area) polygons.
     *
     * @param distance the width of the buffer (may be positive, negative or 0)
     * @param segments the number of line segments used to represent a quadrant of a circle
     * @param endCapStyle the end cap style to use
     * @param joinStyle the join style
     * @param mitreLimit the mitre limit
     * @param errorMsg destination storage for any error message
     * @returns a polygonal geometry representing the buffer region (which may be empty)
     */
    virtual QgsGeometry buffer( double distance, int segments, int endCapStyle, int joinStyle, double mitreLimit, QString* errorMsg = nullptr ) const override;

    /**
     * Simplifies a geometry, ensuring that the result is a valid geometry 
     * having the same dimension and number of components as the input.
     *
     * Note that in general D-P does not preserve topology - e.g. polygons can be split, 
     * collapse to lines or disappear holes can be created or disappear, 
     * and lines can cross. 
     *
     * To simplify geometry while preserving topology enable "preservingTopology" parameter. 
     * (However, using D-P is significantly faster).
     *
     * @param tolerance distance tolerance for the simplification
     * @param preservingTopology preserves topology of the simplified geometry when supported
     * @param errorMsg destination storage for any error message
     * @returns the simplified geometry
     */
    virtual QgsGeometry simplify( double tolerance, bool preservingTopology = true, QString* errorMsg = nullptr ) const override;

    /**
     * Given a distance, returns the point (or closest point) within the geometry 
     * (LineString or MultiLineString) at that distance.
     *
     * @param distance the distance to interpolate
     * @param errorMsg destination storage for any error message
     * @returns the point
     */
    virtual QgsGeometry interpolate( double distance, QString* errorMsg = nullptr ) const override;

    /**
     * Returns the minimum bounding box for this geometry.
     *
     * @param errorMsg destination storage for any error message
     * @returns The envelope
     */
    virtual QgsGeometry envelope( QString* errorMsg = nullptr ) const override;

    /**
     * Computes the centroid of this geometry.
     *
     * The centroid is equal to the centroid of the set of component geometries 
     * of highest dimension (since the lower-dimension geometries contribute 
     * zero "weight" to the centroid).
     *
     * @param errorMsg destination storage for any error message
     * @returns a point which is the centroid of this geometry
     */
    virtual bool centroid( QgsPointV2& point, QString* errorMsg = nullptr ) const override;
    
    /**
     * Computes an interior point of this geometry.
     *
     * An interior point is guaranteed to lie in the interior of the geometry,
     * if it possible to calculate such a point exactly. 
     * Otherwise, the point may lie on the boundary of the geometry.
     * 
     * @param errorMsg destination storage for any error message
     * @returns a point which is in the interior of this geometry
     */
    virtual bool pointOnSurface( QgsPointV2& point, QString* errorMsg = nullptr ) const override;

    /**
     * Returns a gometry that represents the convex hull of this geometry.
     *
     * @param errorMsg destination storage for any error message
     * @returns The convex hull
     */
    virtual QgsGeometry convexHull( QString* errorMsg = nullptr ) const override;
    
    /**
     * Returns the minimum distance between this geometry and another geometry.
     *
     * @param other the geometry from which to compute the distance
     * @param errorMsg destination storage for any error message
     * @returns the distance between the geometries, 0 if either input geometry is empty
     */
    virtual double distance( const QgsGeometry& other, QString* errorMsg = nullptr ) const override;

    /**
     * Builds a offset curve, that is, a line parallel to the provided 
     * geometry at a given distance. 
     *
     * An offset curve is not the same as a buffer, the line is built only on one side
     * of the original geometry, and will self intersect if the original geometry 
     * does the same.
     *
     * @param distance the width of the buffer (may be positive, negative or 0)
     * @param segments the number of line segments used to represent a quadrant of a circle
     * @param joinStyle the join style to use
     * @param mitreLimit the mitre limit
     * @param errorMsg destination storage for any error message
     * @returns the offsets curve
     */
    virtual QgsGeometry offsetCurve( double distance, int segments, int joinStyle, double mitreLimit, QString* errorMsg = nullptr ) const override;

    /**
     * Tests whether this geometry intersects the argument geometry.
     * 
     * The intersects predicate has the following equivalent definitions:
     * - The two geometries have at least one point in common.
     * - The DE-9IM Intersection Matrix for the two geometries matches
     *   at least one of the patterns.
     *   [T********]
     *   [*T*******]
     *   [***T*****]
     *   [****T****]
     *
     * @param other the geometry with which to compare this geometry
     * @param errorMsg destination storage for any error message
     * @returns true if the two geometries intersect
     */
    virtual bool intersects( const QgsGeometry& other, QString* errorMsg = nullptr ) const override;

    /**
     * Tests whether this geometry touches the argument geometry.
     * 
     * The touches predicate has the following equivalent definitions:
     * - The geometries have at least one point in common, but their interiors 
     *   do not intersect.
     * - The DE-9IM Intersection Matrix for the two geometries matches
     *   at least one of the following patterns.
     *   [FT*******]
     *   [F**T*****]
     *   [F***T****]
     *
     * If both geometries have dimension 0, this predicate returns false.
     *
     * @param other the geometry with which to compare this geometry
     * @param errorMsg destination storage for any error message
     * @returns true if the two geometries touch, false if both geometries are points
     */
    virtual bool touches( const QgsGeometry& other, QString* errorMsg = nullptr ) const override;

    /**
     * Tests whether this geometry crosses the argument geometry.
     *
     * The crosses predicate has the following equivalent definitions:
     * - The geometries have some but not all interior points 
     *   in common.
     * - The DE-9IM Intersection Matrix for the two geometries matches 
     *   one of the following patterns:
     *   [T*T******] (for P/L, P/A, and L/A situations)
     *   [T*****T**] (for L/P, A/P, and A/L situations)
     *   [0********] (for L/L situations)
     *
     * For any other combination of dimensions this predicate returns false.
     * The SFS defined this predicate only for P/L, P/A, L/L, and L/A situations.
     *
     * @param other the geometry with which to compare this geometry
     * @param errorMsg destination storage for any error message
     * @returns true if the two geometries cross.
     */
    virtual bool crosses( const QgsGeometry& other, QString* errorMsg = nullptr ) const override;

    /**
     * Tests whether this geometry is within the argument geometry.
     *
     * The within predicate has the following equivalent definitions:
     * - Every point of this geometry is a point of the other geometry,
     *   and the interiors of the two geometries have at least 
     *   one point in common.
     * - The DE-9IM Intersection Matrix for the two geometries matches
     *   [T*F**F***]
     *
     * An implication of the definition is that "The boundary of a Geometry is not 
     * within the Geometry". In other words, if a geometry A is a subset of
     * the points in the boundary of a geomtry B, A.within(B) = false
     *
     * @param other the geometry with which to compare this geometry
     * @param errorMsg destination storage for any error message
     * @returns true if this geometry is within
     */
    virtual bool within( const QgsGeometry& other, QString* errorMsg = nullptr ) const override;

    /**
     * Tests whether this geometry overlaps the argument geometry.
     *
     * The overlaps predicate has the following equivalent definitions:
     * - The geometries have at least one point each not shared by the other
     *   (or equivalently neither covers the other), they have the same dimension,
     *   and the intersection of the interiors of the two geometries has
     *   the same dimension as the geometries themselves.
     * - The DE-9IM Intersection Matrix for the two geometries matches
     *   [T*T***T**] (for two points or two surfaces) or 
     *   [1*T***T**] (for two curves)
     *
     * If the geometries are of different dimension this predicate returns false.
     *
     * @param other the geometry with which to compare this geometry
     * @param errorMsg destination storage for any error message
     * @returns true if the two geometries overlap.
     */
    virtual bool overlaps( const QgsGeometry& other, QString* errorMsg = nullptr ) const override;

    /**
     * Tests whether this geometry contains the argument geometry.
     *
     * The contains predicate has the following equivalent definitions:
     * - Every point of the other geometry is a point of this geometry, 
     *   and the interiors of the two geometries have at least 
     *   one point in common.
     * - The DE-9IM Intersection Matrix for the two geometries matches 
     *   the pattern [T*****FF*]
     *
     * An implication of the definition is that "Geometries do not contain 
     * their boundary". In other words, if a geometry A is a subset of 
     * the points in the boundary of a geometry B, B.contains(A) = false.
     *
     * @param other the geometry with which to compare this geometry
     * @param errorMsg destination storage for any error message
     * @returns true if this geometry contains other
     */
    virtual bool contains( const QgsGeometry& other, QString* errorMsg = nullptr ) const override;

    /**
     * Tests whether this geometry is disjoint from the argument geometry.
     *
     * The disjoint predicate has the following equivalent definitions:
     * - The two geometries have no point in common
     * - The DE-9IM Intersection Matrix for the two geometries matches
     *   [FF*FF****]
     *
     * @param other the geometry with which to compare this geometry
     * @param errorMsg destination storage for any error message
     * @returns true if the two geometries are disjoint
     */
    virtual bool disjoint( const QgsGeometry& other, QString* errorMsg = nullptr ) const override;

    /** 
     * Returns the Dimensional Extended 9 Intersection Model (DE-9IM) representation of the
     * relationship between the geometries.
     *
     * @param other geometry to relate to
     * @param errorMsg destination storage for any error message
     * @returns DE-9IM string for relationship, or an empty string if an error occurred
     */
    virtual QString relate( const QgsGeometry& other, QString* errorMsg = nullptr ) const override;

    /** 
     * Tests whether two geometries are related by a specified Dimensional 
     * Extended 9 Intersection Model (DE-9IM) pattern.
     *
     * @param other geometry to relate to
     * @param pattern DE-9IM pattern for match
     * @param errorMsg destination storage for any error message
     * @returns true if geometry relationship matches with pattern
     */
    virtual bool relatePattern( const QgsGeometry& other, const QString& pattern, QString* errorMsg = nullptr ) const override;

    /**
     * Returns the area of this geometry.
     *
     * Areal geometries have a non-zero area.
     * Others return 0.0
     *
     * @param errorMsg destination storage for any error message
     * @returns the area of the geometry
     */
    virtual double area( QString* errorMsg = nullptr ) const override;
    
    /**
     * Returns the length of this geometry.
     *
     * Linear geometries return their length.
     * Areal geometries return their perimeter.
     * Others return 0.0
     *
     * @param errorMsg destination storage for any error message
     * @returns the length of the geometry
     */
    virtual double length( QString* errorMsg = nullptr ) const override;

    /**
     * Tests whether this geometry is topologically valid, 
     * according to the OGC SFS specification.
     *
     * @param errorMsg destination storage for any error message
     * @returns true if this geometry is valid
     */
    virtual bool isValid( QString* errorMsg = nullptr ) const override;

    /**
     * Tests whether this geometry is topologically equal to the other geometry.
     *
     * @param other the geometry with which to compare this geometry
     * @param errorMsg destination storage for any error message
     * @returns true if the two geometries are topologically equal
     */
    virtual bool isEqual( const QgsGeometry& other, QString* errorMsg = nullptr ) const override;

    /**
     * Tests whether the set of points covered by this geometry is empty.
     *
     * @param errorMsg destination storage for any error message
     * @returns true if this geometry does not cover any points
     */
    virtual bool isEmpty( QString* errorMsg = nullptr ) const override;

  private:
    const GEOSPreparedGeometry* mGeosPrepared;

    enum Overlay
    {
      INTERSECTION,
      DIFFERENCE,
      UNION,
      SYMDIFFERENCE
    };

    enum Relation
    {
      INTERSECTS,
      TOUCHES,
      CROSSES,
      WITHIN,
      OVERLAPS,
      CONTAINS,
      DISJOINT
    };

    //geos util functions
    QgsGeometry overlay( const QgsGeometry& other, Overlay op, QString* errorMsg = nullptr ) const;
    bool relation( const QgsGeometry& other, Relation r, QString* errorMsg = nullptr ) const;

  ///////////////////////////////////////// Other GEOS methods and utilities /////////////////////////////////////////

  public:
    static GEOSContextHandle_t getGEOSHandler();

    /** Returns the closest point on the geometry to the other geometry.
     * @note added in QGIS 2.14
     * @see shortestLine()
     */
    QgsGeometry closestPoint( const QgsGeometry& other, QString* errorMsg = nullptr ) const;

    /** Returns the shortest line joining this geometry to the other geometry.
     * @note added in QGIS 2.14
     * @see closestPoint()
     */
    QgsGeometry shortestLine( const QgsGeometry& other, QString* errorMsg = nullptr ) const;

  private:

    /** Ownership of geoms is transferred
     */
    static GEOSGeometry* createGeosCollection( int typeId, const QVector<GEOSGeometry*>& geoms );
};

/// @cond PRIVATE

class GEOSExceptionV2
{
  public:
    explicit GEOSExceptionV2( const QString& theMsg )
    {
      if ( theMsg == "Unknown exception thrown" && lastMsg().isNull() )
      {
        msg = theMsg;
      }
      else
      {
        msg = theMsg;
        lastMsg() = msg;
      }
    }

    // copy constructor
    GEOSExceptionV2( const GEOSExceptionV2 &rhs )
    {
      *this = rhs;
    }

    ~GEOSExceptionV2()
    {
      if ( lastMsg() == msg )
        lastMsg() = QString::null;
    }

    QString what()
    {
      return msg;
    }

  private:
    QString msg;
    static QString& lastMsg() { static QString _lastMsg; return _lastMsg; }
};

/// @endcond

#endif // QGSGEOSGEOMETRYENGINE_H
