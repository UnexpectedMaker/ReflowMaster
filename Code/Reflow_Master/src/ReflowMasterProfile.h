/*
---------------------------------------------------------------------------
Reflow Master Control - v1.0.0 - 01/07/2018

AUTHOR/LICENSE:
Created by Seon Rozenblum - seon@unexpectedmaker.com
Copyright 2016 License: GNU GPL v3 http://www.gnu.org/licenses/gpl-3.0.html

LINKS:
Project home: github.com/unexpectedmaker/reflowmaster
Blog: unexpectedmaker.com

DISCLAIMER:
This software is furnished "as is", without technical support, and with no 
warranty, express or implied, as to its usefulness for any purpose.

PURPOSE:
This is the profile structure used by the Reflow Master toaster oven controller made by Unexpected Maker

HISTORY:
01/08/2018 v1.0 - Initial release.
20/05/2019 v1.04 - Increased max curve to support profiles up to 8mins

---------------------------------------------------------------------------
  Each profile is initialised with the follow data:

      Paste name ( String )
      Paste type ( String )
      Paste Reflow Temperature ( int )
      Profile graph X values - time
      Profile graph Y values - temperature
      Length of the graph  ( int, how long if the graph array )
      Fan kick in time if using a fan ( int, seconds )
      Open door message time ( int, seconds )
 */
#define ELEMENTS(x)   (sizeof(x) / sizeof(x[0]))

 class ReflowGraph
 {

    public:
      String n;
      String t;
      int tempDeg;
      float reflowGraphX[10];
      float reflowGraphY[10];
      float reflowTangents[10] { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
      float wantedCurve[480];
      int len = -1;
      int fanTime = -1;
      int offTime = -1;
      int completeTime = -1;

      float maxWantedDelta = 0;

      ReflowGraph()
      {
      }
      
      ReflowGraph(String nm, String tp, int temp, float flowX[], float flowY[], int leng, int fanT, int offT )
      {
       n = nm;
       t = tp;
       tempDeg = temp;
       offTime = offT;
       fanTime = fanT;

       int minLength = min( 10, leng );

       len = minLength;

       completeTime = flowX[ len -1 ];

       for ( int i = 0; i < minLength; i++ )
       {
        reflowGraphX[i] = flowX[i];
        reflowGraphY[i] = flowY[i];

        if ( i == 0 )
          reflowTangents[i] = 1;
        else if ( i >= fanT )
          reflowTangents[i] = 1;
        else
          reflowTangents[i] = 0;
       }

       for ( int i = 0; i < ELEMENTS(wantedCurve); i++ )
          wantedCurve[i] = -1; 
     }
     
     ~ReflowGraph()
     {
     }

     float MaxValue()
     {
      float maxV = -1;
      for( int i = 0; i < len; i++ )
          maxV = max( maxV, reflowGraphY[i] );

      return maxV;
     }

     float MinValue()
     {
      float minV = 1000;
      for( int i = 0; i < len; i++ )
      {
        if ( reflowGraphY[i] > 0 )
          minV = min( minV, reflowGraphY[i] );
      }

      return minV;
     }

     float MaxTime()
     {
      float maxV = -1;
      for( int i = 0; i < len; i++ )
          maxV = max( maxV, reflowGraphX[i] );

      return maxV;
     }
 };


/*
 * End ReflowGraphs
 */
