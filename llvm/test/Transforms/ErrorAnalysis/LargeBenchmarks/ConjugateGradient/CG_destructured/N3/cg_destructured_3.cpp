#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>
using namespace std;

const double NEARZERO = 1.0e-10;       // interpretation of "zero"

using vec    = vector<double>;         // vector
using matrix = vector<vec>;            // matrix (=collection of (row) vectors)

// Prototypes
void print( string title, const vec &V );
void print( string title, const matrix &A );
vec matrixTimesVector( const matrix &A, const vec &V );
vec vectorCombination( double a, const vec &U, double b, const vec &V );
double innerProduct( const vec &U, const vec &V );
double vectorNorm( const vec &V );
vec conjugateGradientSolver( const matrix &A, const vec &B );


//======================================================================


int main()
{
  matrix A = {{ 0.8204, -0.4552, -0.3366},
              {-0.4552,  1.0242, -0.2784},
              {-0.3366, -0.2784,  0.4965}};
  vec B = { 1, 2, 3 };

  vec X = conjugateGradientSolver( A, B );

  cout << "Solves AX = B\n";
  print( "\nA:", A );
  print( "\nB:", B );
  print( "\nX:", X );
  print( "\nCheck AX:", matrixTimesVector( A, X ) );
}


//======================================================================

// Prints the vector V
void print( string title, const vec &V )
{
  cout << title << '\n';

  int n = V.size();
  for ( int i = 0; i < n; i++ )
  {
    double x = V[i];   if ( abs( x ) < NEARZERO ) x = 0.0;
    cout << x << '\t';
  }
  cout << '\n';
}


//======================================================================

// Prints the matrix A
void print( string title, const matrix &A )
{
  cout << title << '\n';

  int m = A.size(), n = A[0].size();                      // A is an m x n matrix
  for ( int i = 0; i < m; i++ )
  {
    for ( int j = 0; j < n; j++ )
    {
      double x = A[i][j];   if ( abs( x ) < NEARZERO ) x = 0.0;
      cout << x << '\t';
    }
    cout << '\n';
  }
}


//======================================================================

// Inner product of the matrix A with vector V returned as C (a vector)
vec matrixTimesVector( const matrix &A, const vec &V )     // Matrix times vector
{
  int n = A.size();
  vec C( n );
  for ( int i = 0; i < n; i++ ) C[i] = innerProduct( A[i], V );
  return C;
}


//======================================================================

// Returns the Linear combination of aU+bV as a vector W.
vec vectorCombination( double a, const vec &U, double b, const vec &V )        // Linear combination of vectors
{
  int n = U.size();
  vec W( n );
  for ( int j = 0; j < n; j++ ) W[j] = a * U[j] + b * V[j];
  return W;
}


//======================================================================

// Returns the inner product of vector U with V.
double innerProduct( const vec &U, const vec &V )          // Inner product of U and V
{
  return inner_product( U.begin(), U.end(), V.begin(), 0.0 );
}


//======================================================================

// Computes and returns the Euclidean/2-norm of the vector V.
double vectorNorm( const vec &V )                          // Vector norm
{
  return sqrt( innerProduct( V, V ) );
}


//======================================================================

// The conjugate gradient solving algorithm.
vec conjugateGradientSolver( const matrix &A_, const vec &B_ )
{
  // Setting a tolerance level which will be used as a termination condition for this algorithm
  double TOLERANCE = 1.0e-10;

  // Number of vectors/rows in the matrix A.
  int n = A_.size();
  int iterations = 4;

  // Matrix Initialization
  double A_0_0 = A_[0][0], A_0_1 = A_[0][1], A_0_2 = A_[0][2];
  double A_1_0 = A_[1][0], A_1_1 = A_[1][1], A_1_2 = A_[1][2];
  double A_2_0 = A_[2][0], A_2_1 = A_[2][1], A_2_2 = A_[2][2];
  
  // Initializing vector X which will be set to the solution by this algorithm.
  double X_0 = 0.0, X_1 = 0.0, X_2 = 0.0;

  // Vector Initializations
  double R_0 = B_[0], R_1 = B_[1], R_2 = B_[2];
  double P_0 = B_[0], P_1 = B_[1], P_2 = B_[2];

  int k = 0;

  while ( k < iterations )
  {
    double Rold_0 = R_0, Rold_1 = R_1, Rold_2 = R_2;
    double AP_0, AP_1, AP_2;


//    for ( int i = 0; i < n; i++ ) {
//      AP[i] = 0;
//      for (int j = 0; j < int(A[0].size()); ++j)
//        AP[i] += A[i][j]*P[j];
//    }

    AP_0 = AP_1 = AP_2 = 0;

    AP_0 += A_0_0*P_0 + A_0_1*P_1 + A_0_2*P_2;
    AP_1 += A_1_0*P_0 + A_1_1*P_1 + A_1_2*P_2;
    AP_2 += A_2_0*P_0 + A_2_1*P_1 + A_2_2*P_2;

    double NormOfR = 0;
//    for (int j = 0; j < n; ++j)
//      NormOfR += R[j]*R[j];

    NormOfR += R_0*R_0 + R_1*R_1 + R_2*R_2;


    double P_AP_Product = 0;
//    for (int j = 0; j < n; ++j)
//      P_AP_Product += P[j]*AP[j];

    P_AP_Product += P_0*AP_0 + P_1*AP_1 + P_2*AP_2;

    double alpha = NormOfR / max( P_AP_Product, NEARZERO );


//    for ( int j = 0; j < n; j++ )
//      X[j] = X[j] + alpha * P[j];                             // Next estimate of solution
//      R[j] = R[j] - alpha * AP[j];                            // Residual
//    if ( vectorNorm( R ) < TOLERANCE ) break;             // Convergence test

    X_0 += alpha*P_0;
    X_1 += alpha*P_1;
    X_2 += alpha*P_2;

    R_0 -= alpha*AP_0;
    R_1 -= alpha*AP_1;
    R_2 -= alpha*AP_2;

    NormOfR = 0;
    NormOfR += R_0*R_0 + R_1*R_1 + R_2*R_2;
    if (NormOfR < TOLERANCE) break;


    double NormOfRold = 0;
//    for (int j = 0; j < n; ++j)
//      NormOfRold += Rold[j] * Rold[j];

    NormOfRold += Rold_0*Rold_0 + Rold_1*Rold_1 + Rold_2*Rold_2;

//    double beta = NormOfR / max( NormOfRold, NEARZERO );

    double beta = NormOfR / max( NormOfRold, NEARZERO );

//    for ( int j = 0; j < n; j++ )
//      P[j] = R[j] + beta * P[j];                            // Next gradient

    P_0 = R_0 + beta * P_0;
    P_1 = R_1 + beta * P_1;
    P_2 = R_2 + beta * P_2;

    k++;
  }

  vec X( n, 0.0);
  X[0] = X_0;
  X[1] = X_1;
  X[2] = X_2;

  return X;
}