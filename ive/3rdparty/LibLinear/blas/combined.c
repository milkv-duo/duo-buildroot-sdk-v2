#include <math.h>  /* Needed for fabs() and sqrt() */
#include "blas.h"

#ifdef __cplusplus
extern "C" {
#endif

double dnrm2_(int *n, double *x, int *incx)
{
  long int ix, nn, iincx;
  double norm, scale, absxi, ssq, temp;

/*  DNRM2 returns the euclidean norm of a vector via the function
    name, so that

       DNRM2 := sqrt( x'*x )
    -- This version written on 25-October-1982.
       Modified on 14-October-1993 to inline the call to SLASSQ.
       Sven Hammarling, Nag Ltd.   */

  /* Dereference inputs */
  nn = *n;
  iincx = *incx;

  if( nn > 0 && iincx > 0 )
  {
    if (nn == 1)
    {
      norm = fabs(x[0]);
    }
    else
    {
      scale = 0.0;
      ssq = 1.0;

      /* The following loop is equivalent to this call to the LAPACK
         auxiliary routine:   CALL SLASSQ( N, X, INCX, SCALE, SSQ )*/

      for (ix=(nn-1)*iincx; ix>=0; ix-=iincx)
      {
        if (x[ix] != 0.0)
        {
          absxi = fabs(x[ix]);
          if (scale < absxi)
          {
            temp = scale / absxi;
            ssq = ssq * (temp * temp) + 1.0;
            scale = absxi;
          }
          else
          {
            temp = absxi / scale;
            ssq += temp * temp;
          }
        }
      }
      norm = scale * sqrt(ssq);
    }
  }
  else
    norm = 0.0;

  return norm;

} /* dnrm2_ */


double ddot_(int *n, double *sx, int *incx, double *sy, int *incy)
{
  long int i, m, nn, iincx, iincy;
  double stemp;
  long int ix, iy;

  /* forms the dot product of two vectors.
     uses unrolled loops for increments equal to one.
     jack dongarra, linpack, 3/11/78.
     modified 12/3/93, array(1) declarations changed to array(*) */

  /* Dereference inputs */
  nn = *n;
  iincx = *incx;
  iincy = *incy;

  stemp = 0.0;
  if (nn > 0)
  {
    if (iincx == 1 && iincy == 1) /* code for both increments equal to 1 */
    {
      m = nn-4;
      for (i = 0; i < m; i += 5)
        stemp += sx[i] * sy[i] + sx[i+1] * sy[i+1] + sx[i+2] * sy[i+2] +
                 sx[i+3] * sy[i+3] + sx[i+4] * sy[i+4];

      for ( ; i < nn; i++)        /* clean-up loop */
        stemp += sx[i] * sy[i];
    }
    else /* code for unequal increments or equal increments not equal to 1 */
    {
      ix = 0;
      iy = 0;
      if (iincx < 0)
        ix = (1 - nn) * iincx;
      if (iincy < 0)
        iy = (1 - nn) * iincy;
      for (i = 0; i < nn; i++)
      {
        stemp += sx[ix] * sy[iy];
        ix += iincx;
        iy += iincy;
      }
    }
  }

  return stemp;
} /* ddot_ */


int dscal_(int *n, double *sa, double *sx, int *incx)
{
  long int i, m, nincx, nn, iincx;
  double ssa;

  /* scales a vector by a constant.
     uses unrolled loops for increment equal to 1.
     jack dongarra, linpack, 3/11/78.
     modified 3/93 to return if incx .le. 0.
     modified 12/3/93, array(1) declarations changed to array(*) */

  /* Dereference inputs */
  nn = *n;
  iincx = *incx;
  ssa = *sa;

  if (nn > 0 && iincx > 0)
  {
    if (iincx == 1) /* code for increment equal to 1 */
    {
      m = nn-4;
      for (i = 0; i < m; i += 5)
      {
        sx[i] = ssa * sx[i];
        sx[i+1] = ssa * sx[i+1];
        sx[i+2] = ssa * sx[i+2];
        sx[i+3] = ssa * sx[i+3];
        sx[i+4] = ssa * sx[i+4];
      }
      for ( ; i < nn; ++i) /* clean-up loop */
        sx[i] = ssa * sx[i];
    }
    else /* code for increment not equal to 1 */
    {
      nincx = nn * iincx;
      for (i = 0; i < nincx; i += iincx)
        sx[i] = ssa * sx[i];
    }
  }

  return 0;
} /* dscal_ */


int daxpy_(int *n, double *sa, double *sx, int *incx, double *sy,
           int *incy)
{
  long int i, m, ix, iy, nn, iincx, iincy;
  register double ssa;

  /* constant times a vector plus a vector.
     uses unrolled loop for increments equal to one.
     jack dongarra, linpack, 3/11/78.
     modified 12/3/93, array(1) declarations changed to array(*) */

  /* Dereference inputs */
  nn = *n;
  ssa = *sa;
  iincx = *incx;
  iincy = *incy;

  if( nn > 0 && ssa != 0.0 )
  {
    if (iincx == 1 && iincy == 1) /* code for both increments equal to 1 */
    {
      m = nn-3;
      for (i = 0; i < m; i += 4)
      {
        sy[i] += ssa * sx[i];
        sy[i+1] += ssa * sx[i+1];
        sy[i+2] += ssa * sx[i+2];
        sy[i+3] += ssa * sx[i+3];
      }
      for ( ; i < nn; ++i) /* clean-up loop */
        sy[i] += ssa * sx[i];
    }
    else /* code for unequal increments or equal increments not equal to 1 */
    {
      ix = iincx >= 0 ? 0 : (1 - nn) * iincx;
      iy = iincy >= 0 ? 0 : (1 - nn) * iincy;
      for (i = 0; i < nn; i++)
      {
        sy[iy] += ssa * sx[ix];
        ix += iincx;
        iy += iincy;
      }
    }
  }

  return 0;
} /* daxpy_ */


#ifdef __cplusplus
}
#endif
