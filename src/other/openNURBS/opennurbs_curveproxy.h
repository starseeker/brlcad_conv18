/* $Header$ */
/* $NoKeywords: $ */
/*
//
// Copyright (c) 1993-2001 Robert McNeel & Associates. All rights reserved.
// Rhinoceros is a registered trademark of Robert McNeel & Assoicates.
//
// THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
// ALL IMPLIED WARRANTIES OF FITNESS FOR ANY PARTICULAR PURPOSE AND OF
// MERCHANTABILITY ARE HEREBY DISCLAIMED.
//
// For complete openNURBS copyright information see <http://www.opennurbs.org>.
//
////////////////////////////////////////////////////////////////
*/

////////////////////////////////////////////////////////////////
//
//   Definition of curve proxy object
//
////////////////////////////////////////////////////////////////

#if !defined(OPENNURBS_CURVEPROXY_INC_)
#define OPENNURBS_CURVEPROXY_INC_

/*
Description:
  An ON_CurveProxy is a reference to an ON_Curve.
  One may specify a subdomain of the referenced curve
	and apply a affine reparameterization, possibly  reversing
	the orientation.  The underlying curve cannot be modified through
	the curve proxy.
Details:
	The reference to the "real_curve" is const, so most functions
	which modify an ON_Curve will fail when passed an ON_CurveProxy.
*/
class ON_CurveProxy;
class ON_CLASS ON_CurveProxy : public ON_Curve
{
  ON_OBJECT_DECLARE(ON_CurveProxy);

public:
  // virtual ON_Object::DestroyRuntimeCache override
  void DestroyRuntimeCache( bool bDelete = true );

public:
  ON_CurveProxy();
  ON_CurveProxy( const ON_CurveProxy& );
  ON_CurveProxy( const ON_Curve* );
  ON_CurveProxy( const ON_Curve*, ON_Interval );

  ON_CurveProxy& operator=(const ON_CurveProxy&);

  virtual ~ON_CurveProxy();

  // virtual ON_Object::SizeOf override
  unsigned int SizeOf() const;

  // virtual ON_Object::DataCRC override
  ON__UINT32 DataCRC(ON__UINT32 current_remainder) const;

  /*
  Description:
    Sets the curve geometry that "this" is a proxy for.
    Sets proxy domain to proxy_curve->Domain().
  Parameters:
    real_curve - [in]
  */
  void SetProxyCurve( const ON_Curve* real_curve );

  /*
  Description:
    Sets the curve geometry that "this" is a proxy for.
    Sets proxy domain to proxy_curve->Domain().
  Parameters:
    real_curve - [in]
    real_curve_subdomain - [in] increasing sub interval of
            real_curve->Domain().  This interval defines the
            portion the "real" curve geometry that "this" proxy
            uses.
    bReversed - [in] true if the parameterization of "this" proxy
            as a curve is reversed from the underlying "real" curve
            geometry.
  */
  void SetProxyCurve( const ON_Curve* real_curve,
                      ON_Interval real_curve_subdomain
                      );

  /*
  Returns:
    "Real" curve geometry that "this" is a proxy for.
  */
  const ON_Curve* ProxyCurve() const;

  /*
  Description:
    Sets portion of the "real" curve that this proxy represents.
    Does NOT change the domain of "this" curve.
  Parameters:
    proxy_curve_subdomain - [in] increasing sub interval of
            ProxyCurve()->Domain().  This interval defines the
            portion the curve geometry that "this" proxy uses.
  Remarks:
    This function is poorly named.  It does NOT set the proxy
    curve's domain.  It does set the interval of the "real"
    curve for which "this" is a proxy.
  */
  bool SetProxyCurveDomain( ON_Interval proxy_curve_subdomain );


  /*
  Returns:
    Sub interval of the "real" curve's domain that "this" uses.
    This interval is not necessarily the same as "this" curve's
    domain.
  Remarks:
    This function is poorly named.  It does NOT get the proxy
    curve's domain.  It does get the evaluation interval
    of the "real" curve for which "this" is a proxy.
  */
  ON_Interval ProxyCurveDomain() const;

  /*
  Returns:
    True if "this" as a curve is reversed from the "real" curve
    geometry.
  */
  bool ProxyCurveIsReversed() const;

  /*
  Parameters:
    t - [in] parameter for "this" curve
  Returns:
    Corresponding parameter in m_real_curve's domain.
  */
  double RealCurveParameter( double t ) const;

  /*
  Parameters:
    real_curve_parameter - [in] m_real_curve parameter
  Returns:
    Corresponding parameter for "this" curve
  */
  double ThisCurveParameter( double real_curve_parameter ) const;

private:
  // "real" curve geometry that "this" is a proxy for.
  const ON_Curve* m_real_curve;

  // If TRUE, the parameterization of "this" proxy is
  // the reverse of the m_curve parameterization.
  bool m_bReversed;

  // The m_domain interval is always increasing and included in
  // m_curve->Domain().  The m_domain interval defines the portion
  // of m_curve that "this" proxy uses and it can be a proper
  // sub-interval of m_curve->Domain().
  ON_Interval m_real_curve_domain;

  // The evaluation domain of this curve.  If "t" is a parameter for
  // "this" and "r" is a parameter for m_curve, then when m_bReversed==false
	// we have
  // t = m_this_domain.ParameterAt(m_real_curve_domain.NormalizedParameterAt(r))
  // r = m_real_curve_domain.ParameterAt(m_this_domain.NormalizedParameterAt(t))
	// and when m_bReversed==true we have
  // t = m_this_domain.ParameterAt(1 - m_real_curve_domain.NormalizedParameterAt(r))
  // r = m_real_curve_domain.ParameterAt(1 - m_this_domain.NormalizedParameterAt(t))
  ON_Interval m_this_domain;

  ON_Interval RealCurveInterval( const ON_Interval* sub_domain ) const;


public:
  /*
  Description:
    Get a duplicate of the curve.
  Returns:
    A duplicate of the curve.
  Remarks:
    The caller must delete the returned curve.
    For non-ON_CurveProxy objects, this simply duplicates the curve using
    ON_Object::Duplicate.
    For ON_CurveProxy objects, this duplicates the actual proxy curve
    geometry and, if necessary, trims and reverse the result to that
    the returned curve's parameterization and locus match the proxy curve's.
  */
  ON_Curve* DuplicateCurve() const;

  /////////////////////////////////////////////////////////////////
  // ON_Object overrides

  /*
  Description:
    Tests an object to see if its data members are correctly
    initialized.
  Parameters:
    text_log - [in] if the object is not valid and text_log
        is not NULL, then a brief englis description of the
        reason the object is not valid is appened to the log.
        The information appended to text_log is suitable for
        low-level debugging purposes by programmers and is
        not intended to be useful as a high level user
        interface tool.
  Returns:
    @untitled table
    TRUE     object is valid
    FALSE    object is invalid, uninitialized, etc.
  Remarks:
    Overrides virtual ON_Object::IsValid
  */
  BOOL IsValid( ON_TextLog* text_log = NULL ) const;

  void Dump( ON_TextLog& ) const; // for debugging

  BOOL Write( // returns FALSE - nothing serialized
         ON_BinaryArchive&  // open binary file
       ) const;

  BOOL Read( // returns FALSE - nothing serialized
         ON_BinaryArchive&  // open binary file
       );

  /////////////////////////////////////////////////////////////////
  // ON_Geometry overrides

  int Dimension() const;

  BOOL GetBBox( // returns TRUE if successful
         double*,    // minimum
         double*,    // maximum
         BOOL = FALSE  // TRUE means grow box
         ) const;

  BOOL Transform(
         const ON_Xform&
         );

  /////////////////////////////////////////////////////////////////
  // ON_Curve overrides

  // Returns:
  //   domain of the curve.
  // Remarks:
  //   If m_bReverse is TRUE, this returns the reverse
  //   of m_domain.
  ON_Interval Domain() const;

  /* virtual ON_Curve::SetDomain() override */
  BOOL SetDomain(
        double t0,
        double t1
        );

  bool SetDomain( ON_Interval domain );

  int SpanCount() const; // number of smooth spans in curve

  BOOL GetSpanVector(
    double*
    ) const;

  int Degree( // returns maximum algebraic degree of any span
                  // ( or a good estimate if curve spans are not algebraic )
    ) const;

  // (optional - override if curve is piecewise smooth)
  BOOL GetParameterTolerance( // returns tminus < tplus: parameters tminus <= s <= tplus
         double,  // t = parameter in domain
         double*, // tminus
         double*  // tplus
         ) const;

  BOOL IsLinear( // TRUE if curve locus is a line segment between
                 // between specified points
        double = ON_ZERO_TOLERANCE // tolerance to use when checking linearity
        ) const;

  // virtual override of ON_Curve::IsPolyline
  int IsPolyline(
        ON_SimpleArray<ON_3dPoint>* pline_points = NULL,
        ON_SimpleArray<double>* pline_t = NULL
        ) const;

  BOOL IsArc( // ON_Arc.m_angle > 0 if curve locus is an arc between
              // specified points
        const ON_Plane* = NULL, // if not NULL, test is performed in this plane
        ON_Arc* = NULL, // if not NULL and TRUE is returned, then arc parameters
                         // are filled in
        double = ON_ZERO_TOLERANCE    // tolerance to use when checking
        ) const;

  BOOL IsPlanar(
        ON_Plane* = NULL, // if not NULL and TRUE is returned, then plane parameters
                           // are filled in
        double = ON_ZERO_TOLERANCE    // tolerance to use when checking
        ) const;

  BOOL IsInPlane(
        const ON_Plane&, // plane to test
        double = ON_ZERO_TOLERANCE    // tolerance to use when checking
        ) const;

  BOOL IsClosed(  // TRUE if curve is closed (either curve has
        void      // clamped end knots and euclidean location of start
        ) const;  // CV = euclidean location of end CV, or curve is
                  // periodic.)

  BOOL IsPeriodic(  // TRUE if curve is a single periodic segment
        void
        ) const;

  /*
  Description:
    Search for a derivatitive, tangent, or curvature discontinuity.
  Parameters:
    c - [in] type of continity to test for.  If ON::C1_continuous
    t0 - [in] search begins at t0
    t1 - [in] (t0 < t1) search ends at t1
    t - [out] if a discontinuity is found, the *t reports the
          parameter at the discontinuity.
    hint - [in/out] if GetNextDiscontinuity will be called repeatedly,
       passing a "hint" with initial value *hint=0 will increase the speed
       of the search.
    dtype - [out] if not NULL, *dtype reports the kind of discontinuity
        found at *t.  A value of 1 means the first derivative or unit tangent
        was discontinuous.  A value of 2 means the second derivative or
        curvature was discontinuous.
    cos_angle_tolerance - [in] default = cos(1 degree) Used only when
        c is ON::G1_continuous or ON::G2_continuous.  If the cosine
        of the angle between two tangent vectors
        is <= cos_angle_tolerance, then a G1 discontinuity is reported.
    curvature_tolerance - [in] (default = ON_SQRT_EPSILON) Used only when
        c is ON::G2_continuous.  If K0 and K1 are curvatures evaluated
        from above and below and |K0 - K1| > curvature_tolerance,
        then a curvature discontinuity is reported.
  Returns:
    TRUE if a discontinuity was found on the interior of the interval (t0,t1).
  Remarks:
    Overrides ON_Curve::GetNextDiscontinuity.
  */
  bool GetNextDiscontinuity(
                  ON::continuity c,
                  double t0,
                  double t1,
                  double* t,
                  int* hint=NULL,
                  int* dtype=NULL,
                  double cos_angle_tolerance=0.99984769515639123915701155881391,
                  double curvature_tolerance=ON_SQRT_EPSILON
                  ) const;

  /*
  Description:
    Test continuity at a curve parameter value.
  Parameters:
    c - [in] continuity to test for
    t - [in] parameter to test
    hint - [in] evaluation hint
    point_tolerance - [in] if the distance between two points is
        greater than point_tolerance, then the curve is not C0.
    d1_tolerance - [in] if the difference between two first derivatives is
        greater than d1_tolerance, then the curve is not C1.
    d2_tolerance - [in] if the difference between two second derivatives is
        greater than d2_tolerance, then the curve is not C2.
    cos_angle_tolerance - [in] default = cos(1 degree) Used only when
        c is ON::G1_continuous or ON::G2_continuous.  If the cosine
        of the angle between two tangent vectors
        is <= cos_angle_tolerance, then a G1 discontinuity is reported.
    curvature_tolerance - [in] (default = ON_SQRT_EPSILON) Used only when
        c is ON::G2_continuous.  If K0 and K1 are curvatures evaluated
        from above and below and |K0 - K1| > curvature_tolerance,
        then a curvature discontinuity is reported.
  Returns:
    TRUE if the curve has at least the c type continuity at the parameter t.
  Remarks:
    Overrides ON_Curve::IsContinuous.
  */
  bool IsContinuous(
    ON::continuity c,
    double t,
    int* hint = NULL,
    double point_tolerance=ON_ZERO_TOLERANCE,
    double d1_tolerance=ON_ZERO_TOLERANCE,
    double d2_tolerance=ON_ZERO_TOLERANCE,
    double cos_angle_tolerance=0.99984769515639123915701155881391,
    double curvature_tolerance=ON_SQRT_EPSILON
    ) const;

  BOOL Reverse();       // reverse parameterizatrion
                        // Domain changes from [a,b] to [-b,-a]

  BOOL Evaluate( // returns FALSE if unable to evaluate
         double,         // evaluation parameter
         int,            // number of derivatives (>=0)
         int,            // array stride (>=Dimension())
         double*,        // array of length stride*(ndir+1)
         int = 0,        // optional - determines which side to evaluate from
                         //         0 = default
                         //      <  0 to evaluate from below,
                         //      >  0 to evaluate from above
         int* = 0        // optional - evaluation hint (int) used to speed
                         //            repeated evaluations
         ) const;

  //////////
  // Find parameter of the point on a curve that is closest to test_point.
  // If the maximum_distance parameter is > 0, then only points whose distance
  // to the given point is <= maximum_distance will be returned.  Using a
  // positive value of maximum_distance can substantially speed up the search.
  // If the sub_domain parameter is not NULL, then the search is restricted
  // to the specified portion of the curve.
  //
  // TRUE if returned if the search is successful.  FALSE is returned if
  // the search fails.
  bool GetClosestPoint( const ON_3dPoint&, // test_point
          double*,       // parameter of local closest point returned here
          double = 0.0,  // maximum_distance
          const ON_Interval* = NULL // sub_domain
          ) const;

  //////////
  // Find parameter of the point on a curve that is locally closest to
  // the test_point.  The search for a local close point starts at
  // seed_parameter. If the sub_domain parameter is not NULL, then
  // the search is restricted to the specified portion of the curve.
  //
  // TRUE if returned if the search is successful.  FALSE is returned if
  // the search fails.
  BOOL GetLocalClosestPoint( const ON_3dPoint&, // test_point
          double,    // seed_parameter
          double*,   // parameter of local closest point returned here
          const ON_Interval* = NULL // sub_domain
          ) const;

  //////////
  // Length of curve.
  // TRUE if returned if the length calculation is successful.
  // FALSE is returned if the length is not calculated.
  //
  // The arc length will be computed so that
  // (returned length - real length)/(real length) <= fractional_tolerance
  // More simply, if you want N significant figures in the answer, set the
  // fractional_tolerance to 1.0e-N.  For "nice" curves, 1.0e-8 works
  // fine.  For very high degree nurbs and nurbs with bad parametrizations,
  // use larger values of fractional_tolerance.
  BOOL GetLength( // returns TRUE if length successfully computed
          double*,                   // length returned here
          double = 1.0e-8,           // fractional_tolerance
          const ON_Interval* = NULL  // (optional) sub_domain
          ) const;

  /*
  Description:
    Used to quickly find short curves.
  Parameters:
    tolerance - [in] (>=0)
    sub_domain - [in] If not NULL, the test is performed
      on the interval that is the intersection of
      sub_domain with Domain().
  Returns:
    True if the length of the curve is <= tolerance.
  Remarks:
    Faster than calling Length() and testing the
    result.
  */
  bool IsShort(
    double tolerance,
    const ON_Interval* sub_domain = NULL
    ) const;

  // override of virtual ON_Curve::GetNormalizedArcLengthPoint(...)
  virtual
  BOOL GetNormalizedArcLengthPoint(
          double s,
          double* t,
          double fractional_tolerance = 1.0e-8,
          const ON_Interval* sub_domain = NULL
          ) const;

  // override of virtual ON_Curve::GetNormalizedArcLengthPoints(...)
  virtual
  BOOL GetNormalizedArcLengthPoints(
          int count,
          const double* s,
          double* t,
          double absolute_tolerance = 0.0,
          double fractional_tolerance = 1.0e-8,
          const ON_Interval* sub_domain = NULL
          ) const;

  // override of virtual ON_Curve::Trim
  BOOL Trim(
    const ON_Interval& domain
    );

  // override of virtual ON_Curve::Split
  BOOL Split(
      double t,
      ON_Curve*& left_side,
      ON_Curve*& right_side
    ) const;

  int GetNurbForm( // returns 0: unable to create NURBS representation
                   //            with desired accuracy.
                   //         1: success - returned NURBS parameterization
                   //            matches the curve's to wthe desired accuracy
                   //         2: success - returned NURBS point locus matches
                   //            the curve's to the desired accuracy but, on
                   //            the interior of the curve's domain, the
                   //            curve's parameterization and the NURBS
                   //            parameterization may not match to the
                   //            desired accuracy.
        ON_NurbsCurve&,
        double = 0.0,
        const ON_Interval* = NULL // OPTIONAL subdomain of ON_CurveProxy::Domain()
        ) const;

  int HasNurbForm( // returns 0: unable to create NURBS representation
                   //            with desired accuracy.
                   //         1: success - returned NURBS parameterization
                   //            matches the curve's to wthe desired accuracy
                   //         2: success - returned NURBS point locus matches
                   //            the curve's to the desired accuracy but, on
                   //            the interior of the curve's domain, the
                   //            curve's parameterization and the NURBS
                   //            parameterization may not match to the
                   //            desired accuracy.
        ) const;

  // virtual ON_Curve::GetCurveParameterFromNurbFormParameter override
  BOOL GetCurveParameterFromNurbFormParameter(
        double, // nurbs_t
        double* // curve_t
        ) const;

  // virtual ON_Curve::GetNurbFormParameterFromCurveParameter override
  BOOL GetNurbFormParameterFromCurveParameter(
        double, // curve_t
        double* // nurbs_t
        ) const;
};


#endif
