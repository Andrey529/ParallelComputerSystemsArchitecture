#include <mpi/mpi.h>
#include <iostream>
#include <vector>

const int Tag = 0;
const int root = 0;

double sum_array( std::vector< double > array, int n )
{
     double sum = 0;
     for (int i = 0; i < n; ++i)
     {
          sum += array[ i ];
     }
     return sum;
}

void InitializeMatrixAndVector( std::vector< std::vector< double > >& matrix, std::vector< double > vec )
{
     matrix =
}

std::vector< double > MultiplyMatrixToVector( const std::vector< std::vector< double > >& matrix, const std::vector< double >& vector )
{
     std::vector< double > result;
     for ( int row = 0; row < matrix.size(); ++row )
     {
          double resultElem = 0;
          for( int column = 0; column < matrix[ row ].size(); ++column )
          {
               resultElem += matrix[ row ][ column ] * vector[ column ];
          }
          result.emplace_back( resultElem );
     }
     return result;
}

int main( int argc, char** argv )
{
     int rank, commSize;

     MPI_Init( &argc, &argv );
     MPI_Comm_rank( MPI_COMM_WORLD, &rank );
     MPI_Comm_size( MPI_COMM_WORLD, &commSize );

     double sum = 0;
     std::vector< double > arr;
     int n = 10;
     MPI_Status status;

     if (root == rank)
     {
          std::vector< std::vector< double > > matrix;
          std::vector< double > vec;

          for (int i = 0; i < n; ++i)
          {
               std::cout << "input vector " << i << " element = " << arr[ i ] << std::endl;
          }

          int partSize = n / commSize;

          int shift = n % commSize;
          for (int i = root + 1; i < commSize; ++i)
          {
               MPI_Send( arr.data() + shift + partSize * i, partSize, MPI_DOUBLE, i, Tag, MPI_COMM_WORLD );
          }
          sum = sum_array(arr, shift + partSize);

          for (int i = root + 1; i < commSize; ++i)
          {
               double buffer;
               MPI_Recv( &buffer, 1, MPI_DOUBLE, i, Tag, MPI_COMM_WORLD, &status );
               sum += buffer;
          }
     }
     else
     {
          MPI_Probe( root, Tag, MPI_COMM_WORLD, &status );
          MPI_Get_count( &status, MPI_DOUBLE, &n );

          std::vector< double > localArr;
          arr.resize( n );
          MPI_Recv( arr.data(), n, MPI_DOUBLE, root, Tag, MPI_COMM_WORLD, &status );

          sum = sum_array( arr, n );

          MPI_Send( &sum, 1, MPI_DOUBLE, root, Tag, MPI_COMM_WORLD );
     }

     std::cout << rank << " : " << sum << std::endl;

     MPI_Finalize();
}