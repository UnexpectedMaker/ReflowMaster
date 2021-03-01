/*
  Library for 1-d splines
  Copyright Ryan Michael
  Licensed under the LGPLv3
*/
#ifndef spline_h
#define spline_h
#define Hermite 10
#define Catmull 11

class Spline
{
  public:
    Spline( void );
    Spline( float x[], float y[], int numPoints, int degree = 1 );
    Spline( float x[], float y[], float m[], int numPoints );
    float value( float x );
    void setPoints( float x[], float y[], int numPoints );
    void setPoints( float x[], float y[], float m[], int numPoints );
    void setDegree( int degree );

  private:
    float calc( float, int);
    float* _x;
    float* _y;
    float* _m;
    int _degree;
    int _length;
    int _prev_point;

    float hermite( float t, float p0, float p1, float m0, float m1, float x0, float x1 );
    float hermite_00( float t );
    float hermite_10( float t );
    float hermite_01( float t );
    float hermite_11( float t );
    float catmull_tangent( int i );
};

#endif
