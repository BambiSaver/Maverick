/*--------------------------------------------------------------------------*\
 |                                                                          |
 |  Copyright (C) 2016                                                      |
 |                                                                          |
 |         , __                 , __                                        |
 |        /|/  \               /|/  \                                       |
 |         | __/ _   ,_         | __/ _   ,_                                |
 |         |   \|/  /  |  |   | |   \|/  /  |  |   |                        |
 |         |(__/|__/   |_/ \_/|/|(__/|__/   |_/ \_/|/                       |
 |                           /|                   /|                        |
 |                           \|                   \|                        |
 |                                                                          |
 |      Enrico Bertolazzi                                                   |
 |      Dipartimento di Ingegneria Industriale                              |
 |      Universita` degli Studi di Trento                                   |
 |      email: enrico.bertolazzi@unitn.it                                   |
 |                                                                          |
 \*--------------------------------------------------------------------------*/

#include "Splines.hh"
#include <cmath>
#include <iomanip>

/**
 *
 */

namespace Splines {
    
    void
    SplineSurf::info( ostream & s ) const {
        s << "Bivariate spline [" << name() << "] of type = "
        << type_name()
        << '\n' ;
    }
    
#ifndef SPLINES_DO_NOT_USE_GENERIC_CONTAINER
    using GenericContainerNamespace::GC_VEC_REAL ;
    using GenericContainerNamespace::GC_VEC_INTEGER ;
    using GenericContainerNamespace::GC_MAT_REAL ;
    
    void
    SplineSurf::setup( GenericContainer const & gc ) {
        /*
         // gc["x"]
         // gc["y"]
         // gc["z"]
         //
         */
        SPLINE_ASSERT( gc.exists("x"), "[SplineSurf[" << _name << "]::setup] missing `x` field!") ;
        SPLINE_ASSERT( gc.exists("y"), "[SplineSurf[" << _name << "]::setup] missing `y` field!") ;
        SPLINE_ASSERT( gc.exists("z"), "[SplineSurf[" << _name << "]::setup] missing `z` field!") ;
        
        GenericContainer const & gc_x = gc("x") ;
        GenericContainer const & gc_y = gc("y") ;
        GenericContainer const & gc_z = gc("z") ;
        
        vec_real_type x, y ;
        gc_x.copyto_vec_real( x, "SplineSurf::setup, field `x'" ) ;
        gc_y.copyto_vec_real( y, "SplineSurf::setup, field `x'" ) ;
        
        SPLINE_ASSERT( GC_MAT_REAL == gc_z.get_type(),
                      "[SplineSurf[" << _name << "]::setup] field `z` expected to be of type `mat_real_type` found: `" <<
                      gc_z.get_type_name() << "`" ) ;
        
        bool fortran_storage = false ;
        if ( gc.exists("fortran_storage") )
            fortran_storage = gc("fortran_storage").get_bool() ;
        
        bool transposed = false ;
        if ( gc.exists("transposed") )
            transposed = gc("transposed").get_bool() ;
        
        sizeType nx = sizeType(x.size()) ;
        sizeType ny = sizeType(y.size()) ;
        
        build ( &x.front(), 1,
               &y.front(), 1,
               &gc_z.get_mat_real().front(), gc_z.get_mat_real().numRows(),
               nx, ny, fortran_storage, transposed ) ;
        
    }
#endif
    
//    sizeType
//    SplineSurf::search_x( valueType x ) const {
//        if ( lastInterval_x+1 >= X.size() || X[lastInterval_x] < x || X[lastInterval_x+1] > x ) {
//            if ( _check_range ) {
//                SPLINE_ASSERT( x >= X.front() && x <= X.back(),
//                              "method search_x( " << x << " ) out of range: [" <<
//                              X.front() << ", " << X.back() << "]" ) ;
//            }
//            lastInterval_x = sizeType(lower_bound( X.begin(), X.end(), x ) - X.begin()) ;
//            if ( lastInterval_x > 0 ) --lastInterval_x ;
//        }
//        return lastInterval_x ;
//    }
    
    sizeType
    SplineSurf::search_x( valueType x ) const {
#ifndef MAVERICK_FAST
        if ( _check_range ) {
            SPLINE_ASSERT( x >= X.front() && x <= X.back(),
                          "method search_x( " << x << " ) out of range: [" <<
                          X.front() << ", " << X.back() << "]" ) ;
        }
#endif
        sizeType lastInterval_x = sizeType(lower_bound( X.begin(), X.end(), x ) - X.begin()) ;
        if ( lastInterval_x > 0 ) --lastInterval_x ;
        if ( lastInterval_x+1 >= X.size() ) lastInterval_x = (unsigned) X.size()-2 ;
        
        return lastInterval_x ;
    }
    
//    sizeType
//    SplineSurf::search_y( valueType y ) const {
//        if ( lastInterval_y+1 >= Y.size() || Y[lastInterval_y] < y || Y[lastInterval_y+1] > y ) {
//            if ( _check_range ) {
//                SPLINE_ASSERT( y >= Y.front() && y <= Y.back(),
//                              "method search_y( " << y << " ) out of range: [" <<
//                              Y.front() << ", " << Y.back() << "]" ) ;
//            }
//            lastInterval_y = sizeType(lower_bound( Y.begin(), Y.end(), y ) - Y.begin()) ;
//            if ( lastInterval_y > 0 ) --lastInterval_y ;
//        }
//        return lastInterval_y ;
//    }
    
    sizeType
    SplineSurf::search_y( valueType y ) const {
#ifndef MAVERICK_FAST
        if ( _check_range ) {
            SPLINE_ASSERT( y >= Y.front() && y <= Y.back(),
                          "method search_y( " << y << " ) out of range: [" <<
                          Y.front() << ", " << Y.back() << "]" ) ;
        }
#endif
        sizeType lastInterval_y = sizeType(lower_bound( Y.begin(), Y.end(), y ) - Y.begin()) ;
        if ( lastInterval_y > 0 ) --lastInterval_y ;
        if ( lastInterval_y+1 >= Y.size() ) lastInterval_y = (unsigned) Y.size()-2 ;
        
        return lastInterval_y ;
    }
    
    // ---------------------------------------------------------------------------
    void
    SplineSurf::build ( valueType const x[], sizeType incx,
                       valueType const y[], sizeType incy,
                       valueType const z[], sizeType ldZ,
                       sizeType nx, sizeType ny,
                       bool fortran_storage,
                       bool transposed ) {
        X.resize(nx) ;
        Y.resize(ny) ;
        Z.resize(nx*ny) ;
        for ( sizeType i = 0 ; i < nx ; ++i ) X[i] = x[i*incx] ;
        for ( sizeType i = 0 ; i < ny ; ++i ) Y[i] = y[i*incy] ;
        if ( fortran_storage ) {
            if ( transposed ) {
                for ( sizeType i = 0 ; i < nx ; ++i )
                    for ( sizeType j = 0 ; j < ny ; ++j )
                        Z[ipos_C(i,j)] = z[ipos_F(j,i,nx)] ;
            } else {
                for ( sizeType i = 0 ; i < nx ; ++i )
                    for ( sizeType j = 0 ; j < ny ; ++j )
                        Z[ipos_C(i,j)] = z[ipos_F(i,j,nx)] ;
            }
        } else {
            if ( transposed ) {
                for ( sizeType i = 0 ; i < nx ; ++i )
                    for ( sizeType j = 0 ; j < ny ; ++j )
                        Z[ipos_C(i,j)] = z[ipos_C(j,i,ny)] ;
            } else {
                for ( sizeType i = 0 ; i < nx ; ++i )
                    for ( sizeType j = 0 ; j < ny ; ++j )
                        Z[ipos_C(i,j)] = z[ipos_C(i,j,ny)] ;
            }
        }
        Z_max = *std::max_element(Z.begin(),Z.end()) ;
        Z_min = *std::min_element(Z.begin(),Z.end()) ;
        makeSpline() ;
    
//        X.resize(nx) ;
//        Y.resize(ny) ;
//        Z.resize(nx*ny) ;
//        for ( sizeType i = 0 ; i < nx ; ++i ) X[i] = x[i*incx] ;
//        for ( sizeType i = 0 ; i < ny ; ++i ) Y[i] = y[i*incy] ;
//        if ( (fortran_storage && transposed) || (!fortran_storage && !transposed) ) {
//            SPLINE_ASSERT( ldZ >= ny,
//                          "SplineSurf::build, ldZ = " << ldZ << " must be >= of nx = " << ny ) ;
//            for ( sizeType i = 0 ; i < nx ; ++i )
//                for ( sizeType j = 0 ; j < ny ; ++j )
//                    Z[ipos_C(i,j,ny)] = z[ipos_C(i,j,ny)] ;
//        } else {
//            SPLINE_ASSERT( ldZ >= nx,
//                          "SplineSurf::build, ldZ = " << ldZ << " must be >= of ny = " << nx ) ;
//            for ( sizeType i = 0 ; i < nx ; ++i )
//                for ( sizeType j = 0 ; j < ny ; ++j )
//                    Z[ipos_C(i,j,ny)] = z[ipos_F(i,j,nx)] ;
//        }
//        Z_max = *std::max_element(Z.begin(),Z.end()) ;
//        Z_min = *std::min_element(Z.begin(),Z.end()) ;
//        makeSpline() ;
        
    }
    
    void
    SplineSurf::build( valueType const z[], sizeType ldZ,
                      sizeType nx, sizeType ny,
                      bool fortran_storage,
                      bool transposed ) {
        vector<valueType> XX(nx), YY(ny) ; // temporary vector
        for ( sizeType i = 0 ; i < nx ; ++i ) XX[i] = i ;
        for ( sizeType i = 0 ; i < ny ; ++i ) YY[i] = i ;
        build ( &XX.front(), 1, &YY.front(), 1, z, ldZ, nx, ny, fortran_storage, transposed ) ;
    }
    
    // ---------------------------------------------------------------------------
    // ---------------------------------------------------------------------------
    // ---------------------------------------------------------------------------
    void
    BiCubicSplineBase::load( sizeType i, sizeType j, valueType bili3[4][4] ) const {
        
        //
        //  1    3
        //
        //  0    2
        //
        
        sizeType ny = sizeType(Y.size()) ;
        sizeType i0 = ipos_C(i,j,ny) ;
        sizeType i1 = ipos_C(i,j+1,ny) ;
        sizeType i2 = ipos_C(i+1,j,ny) ;
        sizeType i3 = ipos_C(i+1,j+1,ny) ;
        
        bili3[0][0] = Z[i0];   bili3[0][1] = Z[i1];
        bili3[0][2] = DY[i0];  bili3[0][3] = DY[i1];
        
        bili3[1][0] = Z[i2];   bili3[1][1] = Z[i3];
        bili3[1][2] = DY[i2];  bili3[1][3] = DY[i3];
        
        bili3[2][0] = DX[i0];  bili3[2][1] = DX[i1];
        bili3[2][2] = DXY[i0]; bili3[2][3] = DXY[i1];
        
        bili3[3][0] = DX[i2];  bili3[3][1] = DX[i3];
        bili3[3][2] = DXY[i2]; bili3[3][3] = DXY[i3];
        
    }
    
    valueType
    BiCubicSplineBase::operator () ( valueType x, valueType y ) const {
        sizeType i = search_x( x ) ;
        sizeType j = search_y( y ) ;
        valueType u[4];
        valueType v[4];
        valueType bili3[4][4];
        Hermite3( x - X[i], X[i+1] - X[i], u ) ;
        Hermite3( y - Y[j], Y[j+1] - Y[j], v ) ;
        load(i,j,bili3) ;
        return bilinear3( u, bili3, v ) ;
    }
    
    valueType
    BiCubicSplineBase::Dx( valueType x, valueType y ) const {
        sizeType i = search_x( x ) ;
        sizeType j = search_y( y ) ;
        valueType u_D[4];
        valueType v[4];
        valueType bili3[4][4];
        Hermite3_D( x - X[i], X[i+1] - X[i], u_D ) ;
        Hermite3  ( y - Y[j], Y[j+1] - Y[j], v   ) ;
        load(i,j,bili3) ;
        return bilinear3( u_D, bili3, v ) ;
    }
    
    valueType
    BiCubicSplineBase::Dy( valueType x, valueType y ) const {
        sizeType i = search_x( x ) ;
        sizeType j = search_y( y ) ;
        valueType u[4];
        valueType v_D[4];
        valueType bili3[4][4];
        Hermite3  ( x - X[i], X[i+1] - X[i], u   ) ;
        Hermite3_D( y - Y[j], Y[j+1] - Y[j], v_D ) ;
        load(i,j,bili3) ;
        return bilinear3( u, bili3, v_D ) ;
    }
    
    valueType
    BiCubicSplineBase::Dxy( valueType x, valueType y ) const {
        sizeType i = search_x( x ) ;
        sizeType j = search_y( y ) ;
        valueType u_D[4];
        valueType v_D[4];
        valueType bili3[4][4];
        Hermite3_D( x - X[i], X[i+1] - X[i], u_D ) ;
        Hermite3_D( y - Y[j], Y[j+1] - Y[j], v_D ) ;
        load(i,j,bili3) ;
        return bilinear3( u_D, bili3, v_D ) ;
    }
    
    valueType
    BiCubicSplineBase::Dxx( valueType x, valueType y ) const {
        sizeType i = search_x( x ) ;
        sizeType j = search_y( y ) ;
        valueType u_DD[4];
        valueType v[4];
        valueType bili3[4][4];
        Hermite3_DD( x - X[i], X[i+1] - X[i], u_DD ) ;
        Hermite3   ( y - Y[j], Y[j+1] - Y[j], v    ) ;
        load(i,j,bili3) ;
        return bilinear3( u_DD, bili3, v ) ;
    }
    
    valueType
    BiCubicSplineBase::Dyy( valueType x, valueType y ) const {
        sizeType i = search_x( x ) ;
        sizeType j = search_y( y ) ;
        valueType u[4];
        valueType v_DD[4];
        valueType bili3[4][4];
        Hermite3   ( x - X[i], X[i+1] - X[i], u    ) ;
        Hermite3_DD( y - Y[j], Y[j+1] - Y[j], v_DD ) ;
        load(i,j,bili3) ;
        return bilinear3( u, bili3, v_DD ) ;
    }
    
    void
    BiCubicSplineBase::D( valueType x, valueType y, valueType d[3] ) const {
        sizeType i = search_x( x ) ;
        sizeType j = search_y( y ) ;
        valueType u[4];
        valueType v[4];
        valueType u_D[4];
        valueType v_D[4];
        valueType bili3[4][4];
        Hermite3   ( x - X[i], X[i+1] - X[i], u    ) ;
        Hermite3_D ( x - X[i], X[i+1] - X[i], u_D  ) ;
        Hermite3   ( y - Y[j], Y[j+1] - Y[j], v    ) ;
        Hermite3_D ( y - Y[j], Y[j+1] - Y[j], v_D  ) ;
        load(i,j,bili3) ;
        d[0] = bilinear3( u, bili3, v ) ;
        d[1] = bilinear3( u_D, bili3, v ) ;
        d[2] = bilinear3( u, bili3, v_D ) ;
    }
    
    void
    BiCubicSplineBase::DD( valueType x, valueType y, valueType d[6] ) const {
        sizeType i = search_x( x ) ;
        sizeType j = search_y( y ) ;
        valueType u[4];
        valueType v[4];
        valueType u_D[4];
        valueType v_D[4];
        valueType u_DD[4];
        valueType v_DD[4];
        valueType bili3[4][4];
        Hermite3   ( x - X[i], X[i+1] - X[i], u    ) ;
        Hermite3_D ( x - X[i], X[i+1] - X[i], u_D  ) ;
        Hermite3_DD( x - X[i], X[i+1] - X[i], u_DD ) ;
        Hermite3   ( y - Y[j], Y[j+1] - Y[j], v    ) ;
        Hermite3_D ( y - Y[j], Y[j+1] - Y[j], v_D  ) ;
        Hermite3_DD( y - Y[j], Y[j+1] - Y[j], v_DD ) ;
        load(i,j,bili3) ;
        d[0] = bilinear3( u, bili3, v ) ;
        d[1] = bilinear3( u_D, bili3, v ) ;
        d[2] = bilinear3( u, bili3, v_D ) ;
        d[3] = bilinear3( u_DD, bili3, v ) ;
        d[4] = bilinear3( u_D, bili3, v_D ) ;
        d[5] = bilinear3( u, bili3, v_DD ) ;
    }
    
    // ---------------------------------------------------------------------------
    // ---------------------------------------------------------------------------
    // ---------------------------------------------------------------------------
    void
    BiQuinticSplineBase::load( sizeType i, sizeType j, valueType bili5[6][6] ) const {
        
        sizeType ny  = sizeType(Y.size()) ;
        sizeType i00 = ipos_C(i,j,ny) ;
        sizeType i01 = ipos_C(i,j+1,ny) ;
        sizeType i10 = ipos_C(i+1,j,ny) ;
        sizeType i11 = ipos_C(i+1,j+1,ny) ;
        
        //
        //  1    3
        //
        //  0    2
        //
        // H0, H1, dH0, dH1, ddH0, ddH1
        
        // + + + + + +
        // + + + + + +
        // + + + + . .
        // + + + + . .
        // + + . . . .
        // + + . . . .
        
        // P
        bili5[0][0] = Z[i00]; bili5[0][1] = Z[i01];
        bili5[1][0] = Z[i10]; bili5[1][1] = Z[i11];
        
        // DX
        bili5[2][0] = DX[i00]; bili5[2][1] = DX[i01];
        bili5[3][0] = DX[i10]; bili5[3][1] = DX[i11];
        
        // DXX
        bili5[4][0] = DXX[i00]; bili5[4][1] = DXX[i01];
        bili5[5][0] = DXX[i10]; bili5[5][1] = DXX[i11];
        
        // DY
        bili5[0][2] = DY[i00]; bili5[0][3] = DY[i01];
        bili5[1][2] = DY[i10]; bili5[1][3] = DY[i11];
        
        // DYY
        bili5[0][4] = DYY[i00]; bili5[0][5] = DYY[i01];
        bili5[1][4] = DYY[i10]; bili5[1][5] = DYY[i11];
        
        // DXY
        bili5[2][2] = DXY[i00]; bili5[2][3] = DXY[i01];
        bili5[3][2] = DXY[i10]; bili5[3][3] = DXY[i11];
        
        // DXXY
        bili5[4][2] = DXXY[i00]; bili5[4][3] = DXXY[i01];
        bili5[5][2] = DXXY[i10]; bili5[5][3] = DXXY[i11];
        
        // DXYY
        bili5[2][4] = DXYY[i00]; bili5[2][5] = DXYY[i01];
        bili5[3][4] = DXYY[i10]; bili5[3][5] = DXYY[i11];
        
        // DXXYY
        bili5[4][4] = DXXYY[i00]; bili5[4][5] = DXXYY[i01];
        bili5[5][4] = DXXYY[i10]; bili5[5][5] = DXXYY[i11];
    }
    
    valueType
    BiQuinticSplineBase::operator () ( valueType x, valueType y ) const {
        sizeType i = search_x( x ) ;
        sizeType j = search_y( y ) ;
        valueType u[6];
        valueType v[6];
        valueType bili5[6][6];
        Hermite5( x - X[i], X[i+1] - X[i], u ) ;
        Hermite5( y - Y[j], Y[j+1] - Y[j], v ) ;
        load(i,j,bili5) ;
        return bilinear5( u, bili5, v ) ;
    }
    
    valueType
    BiQuinticSplineBase::Dx( valueType x, valueType y ) const {
        sizeType i = search_x( x ) ;
        sizeType j = search_y( y ) ;
        valueType u_D[6];
        valueType v[6];
        valueType bili5[6][6];
        Hermite5_D( x - X[i], X[i+1] - X[i], u_D ) ;
        Hermite5  ( y - Y[j], Y[j+1] - Y[j], v   ) ;
        load(i,j,bili5) ;
        return bilinear5( u_D, bili5, v ) ;
    }
    
    valueType
    BiQuinticSplineBase::Dy( valueType x, valueType y ) const {
        sizeType i = search_x( x ) ;
        sizeType j = search_y( y ) ;
        valueType u[6];
        valueType v_D[6];
        valueType bili5[6][6];
        Hermite5  ( x - X[i], X[i+1] - X[i], u   ) ;
        Hermite5_D( y - Y[j], Y[j+1] - Y[j], v_D ) ;
        load(i,j,bili5) ;
        return bilinear5( u, bili5, v_D ) ;
    }
    
    valueType
    BiQuinticSplineBase::Dxy( valueType x, valueType y ) const {
        sizeType i = search_x( x ) ;
        sizeType j = search_y( y ) ;
        valueType u_D[6];
        valueType v_D[6];
        valueType bili5[6][6];
        Hermite5_D( x - X[i], X[i+1] - X[i], u_D ) ;
        Hermite5_D( y - Y[j], Y[j+1] - Y[j], v_D ) ;
        load(i,j,bili5) ;
        return bilinear5( u_D, bili5, v_D ) ;
    }
    
    valueType
    BiQuinticSplineBase::Dxx( valueType x, valueType y ) const {
        sizeType i = search_x( x ) ;
        sizeType j = search_y( y ) ;
        valueType v[6];
        valueType u_DD[6];
        valueType bili5[6][6];
        Hermite5_DD( x - X[i], X[i+1] - X[i], u_DD ) ;
        Hermite5   ( y - Y[j], Y[j+1] - Y[j], v    ) ;
        load(i,j,bili5) ;
        return bilinear5( u_DD, bili5, v ) ;
    }
    
    valueType
    BiQuinticSplineBase::Dyy( valueType x, valueType y ) const {
        sizeType i = search_x( x ) ;
        sizeType j = search_y( y ) ;
        valueType u[6];
        valueType v_DD[6];
        valueType bili5[6][6];
        Hermite5   ( x - X[i], X[i+1] - X[i], u    ) ;
        Hermite5_DD( y - Y[j], Y[j+1] - Y[j], v_DD ) ;
        load(i,j,bili5) ;
        return bilinear5( u, bili5, v_DD ) ;
    }
    
    void
    BiQuinticSplineBase::D( valueType x, valueType y, valueType d[3] ) const {
        sizeType i = search_x( x ) ;
        sizeType j = search_y( y ) ;
        valueType u[6];
        valueType v[6];
        valueType u_D[6];
        valueType v_D[6];
        valueType bili5[6][6];
        Hermite5   ( x - X[i], X[i+1] - X[i], u    ) ;
        Hermite5_D ( x - X[i], X[i+1] - X[i], u_D  ) ;
        Hermite5   ( y - Y[j], Y[j+1] - Y[j], v    ) ;
        Hermite5_D ( y - Y[j], Y[j+1] - Y[j], v_D  ) ;
        load(i,j,bili5) ;
        d[0] = bilinear5( u, bili5, v ) ;
        d[1] = bilinear5( u_D, bili5, v ) ;
        d[2] = bilinear5( u, bili5, v_D ) ;
    }
    
    void
    BiQuinticSplineBase::DD( valueType x, valueType y, valueType d[6] ) const {
        sizeType i = search_x( x ) ;
        sizeType j = search_y( y ) ;
        valueType u[6];
        valueType v[6];
        valueType u_D[6];
        valueType v_D[6];
        valueType u_DD[6];
        valueType v_DD[6];
        valueType bili5[6][6];
        Hermite5   ( x - X[i], X[i+1] - X[i], u    ) ;
        Hermite5_D ( x - X[i], X[i+1] - X[i], u_D  ) ;
        Hermite5_DD( x - X[i], X[i+1] - X[i], u_DD ) ;
        Hermite5   ( y - Y[j], Y[j+1] - Y[j], v    ) ;
        Hermite5_D ( y - Y[j], Y[j+1] - Y[j], v_D  ) ;
        Hermite5_DD( y - Y[j], Y[j+1] - Y[j], v_DD ) ;
        load(i,j,bili5) ;
        d[0] = bilinear5( u, bili5, v ) ;
        d[1] = bilinear5( u_D, bili5, v ) ;
        d[2] = bilinear5( u, bili5, v_D ) ;
        d[3] = bilinear5( u_DD, bili5, v ) ;
        d[4] = bilinear5( u_D, bili5, v_D ) ;
        d[5] = bilinear5( u, bili5, v_DD ) ;
    }
    
}
