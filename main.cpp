#include <iostream>
#include <mpi/mpi.h>
#include <sstream>
#include <vector>

static constexpr int TAG = 0;
static constexpr int ROOT_PROCESS_RANK = 0;

bool IsRootProcess( int processRank )
{
     return processRank == ROOT_PROCESS_RANK;
}

std::string OutputMatrix( const std::vector< std::vector< double > >& matrix )
{
     std::stringstream ss;
     for ( const auto& row : matrix )
     {
          for ( const auto& elem : row )
          {
               ss << elem << ' ';
          }
          ss << std::endl;
     }

     return ss.str();
}

std::string OutputVector( const std::vector< double >& vector )
{
     std::stringstream ss;
     for ( const auto& elem : vector )
     {
          ss << elem << ' ';
     }
     ss << std::endl;
     return ss.str();
}

void InitializeMatrixAndVector( std::vector< std::vector< double > >& matrix, std::vector< double >& vec )
{
     matrix = { { 1, 2, 3, 4, 5 },
                { 6, 7, 8, 9, 10 },
                { 11, 12, 13, 14, 15 },
                { 16, 17, 18, 19, 20 },
                { 21, 22, 23, 24, 25 } };

     vec = { 1, 2, 3, 4, 5 };
}

std::vector< double > MultiplyMatrixToVector( const std::vector< std::vector< double > >& matrix,
                                              const std::vector< double >& vector )
{
     std::vector< double > result;
     for ( const auto& row : matrix )
     {
          double resultElem = 0;
          for ( int column = 0; column < row.size(); ++column )
          {
               resultElem += row[ column ] * vector[ column ];
          }
          result.emplace_back( resultElem );
     }
     return result;
}

int main( int argc, char** argv )
{
     int rank, size;

     MPI_Init( &argc, &argv );
     MPI_Comm_rank( MPI_COMM_WORLD, &rank );
     MPI_Comm_size( MPI_COMM_WORLD, &size );

     std::vector< double > result;
     MPI_Status status;

     if ( IsRootProcess( rank ) )
     {
          std::cout << "Всего процессов = " << size << std::endl;

          std::vector< std::vector< double > > matrix;
          std::vector< double > vec;

          InitializeMatrixAndVector( matrix, vec );
          std::cout << "Исходная матрица:\n" << OutputMatrix( matrix );
          std::cout << "Исходная вектор: " << OutputVector( vec );

          if ( matrix.size() != vec.size() )
          {
               std::cerr << "Количество столбцов в матрице должно совпадать с количеством элементов в векторе.\n"
                         << "Сейчас количество столбцов в матрице = " << matrix.size()
                         << ", а количество элементов в векторе = " << vec.size() << std::endl;
          }

          int elemCount = vec.size();
          int partSize = elemCount / size;
          int shift = elemCount % size;

          // отправка матрицы и вектора в дочерние процессы для расчета
          for ( int i = ROOT_PROCESS_RANK + 1; i < size; ++i )
          {
               MPI_Send( &partSize, 1, MPI_INT, i, TAG, MPI_COMM_WORLD );
               for ( int j = shift + partSize * i; j < shift + partSize * ( i + 1 ); ++j )
               {
                    MPI_Send( &( matrix[ j ][ 0 ] ), elemCount, MPI_DOUBLE, i, TAG, MPI_COMM_WORLD );
               }

               MPI_Send( vec.data(), elemCount, MPI_DOUBLE, i, TAG, MPI_COMM_WORLD );
          }

          // подсчет значения матрицы в root процессе
          std::vector< std::vector< double > > rootMatrix;
          for ( int i = 0; i < shift + partSize; ++i )
          {
               rootMatrix.emplace_back( matrix[ i ] );
          }
          std::vector< double > rootResult = MultiplyMatrixToVector( rootMatrix, vec );

          // вставка результата расчета root процесса в итоговый вектор
          for ( const auto& elem : rootResult )
          {
               result.emplace_back( elem );
          }

          // получение результирующих векторов из дочерних процессов и вставка их в итоговый вектор
          for ( int i = ROOT_PROCESS_RANK + 1; i < size; ++i )
          {
               std::vector< double > processResult;
               processResult.resize( partSize );
               MPI_Recv( &( processResult[ 0 ] ), partSize, MPI_DOUBLE, i, TAG, MPI_COMM_WORLD, &status );
               for ( const auto& elem : processResult )
               {
                    result.emplace_back( elem );
               }
          }
     }
     else
     {
          // получение количества строк в матрице из root процесса
          MPI_Probe( ROOT_PROCESS_RANK, TAG, MPI_COMM_WORLD, &status );
          int rowsCount = 0;
          MPI_Recv( &rowsCount, 1, MPI_INT, ROOT_PROCESS_RANK, TAG, MPI_COMM_WORLD, &status );

          // получение матрицы из root процесса
          std::vector< std::vector< double > > localMatrix;
          for ( int i = 0; i < rowsCount; ++i )
          {
               int elemsCount = 0;
               MPI_Probe( ROOT_PROCESS_RANK, TAG, MPI_COMM_WORLD, &status );
               MPI_Get_count( &status, MPI_DOUBLE, &elemsCount );

               std::vector< double > row;
               row.resize( elemsCount );
               MPI_Recv( &( row[ 0 ] ), elemsCount, MPI_DOUBLE, ROOT_PROCESS_RANK, TAG, MPI_COMM_WORLD, &status );
               localMatrix.emplace_back( row );
          }

          std::cout << "Матрица в " << rank << " процессе =\n" << OutputMatrix( localMatrix );

          // получение вектора из root процесса
          int elemsCount = 0;
          MPI_Probe( ROOT_PROCESS_RANK, TAG, MPI_COMM_WORLD, &status );
          MPI_Get_count( &status, MPI_DOUBLE, &elemsCount );
          std::vector< double > vec;
          vec.resize( elemsCount );
          MPI_Recv( &( vec[ 0 ] ), elemsCount, MPI_DOUBLE, ROOT_PROCESS_RANK, TAG, MPI_COMM_WORLD, &status );

          std::cout << "Вектор в " << rank << " процессе = " << OutputVector( vec );

          // расчет результата умножения матрицы на вектор
          result = MultiplyMatrixToVector( localMatrix, vec );

          // отправка результирующего вектора в root процесс
          MPI_Send( &( result[ 0 ] ), result.size(), MPI_DOUBLE, ROOT_PROCESS_RANK, TAG, MPI_COMM_WORLD );
     }

     std::cout << "Результат умножения в " << rank << " процессе = " << OutputVector( result );

     MPI_Finalize();
}