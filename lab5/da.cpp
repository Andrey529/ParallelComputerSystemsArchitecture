std::vector< double > ParallelTridiagonalMatrixAlgorithm( std::vector< double >& a,
                                                          std::vector< double >& b,
                                                          std::vector< double >& c,
                                                          std::vector< double >& d,
                                                          int rank, int size )
{
     int n = d.size();

     for ( int i = 1; i < n; ++i )
     {
          double coef = a[ i ] / b[ i - 1 ];
          a[ i ] = -coef * a[ i - 1 ];
          b[ i ] = b[ i ] - coef * c[ i - 1 ];
          d[ i ] = d[ i ] - coef * d[ i - 1 ];
     }

     for ( int i = n - 3; i >= 0; --i )
     {
          double coef = c[ i ] / b[ i + 1 ];
          c[ i ] = -coef * c[ i + 1 ];
          a[ i ] = a[ i ] - coef * a[ i + 1 ];
          d[ i ] = d[ i ] - coef * d[ i + 1];
     }


}