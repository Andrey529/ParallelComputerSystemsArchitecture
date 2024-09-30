#include <chrono>

#include <mpi/mpi.h>

#include "utility.h"

int main( int argc, char** argv )
{
     int rank, size;

     auto start = std::chrono::high_resolution_clock::now();

     MPI_Init( &argc, &argv );
     MPI_Comm_rank( MPI_COMM_WORLD, &rank );
     MPI_Comm_size( MPI_COMM_WORLD, &size );

     std::vector< double > result;
     MPI_Status status;

     if ( utility::IsRootProcess( rank ) )
     {
          std::cout << "Всего процессов = " << size << std::endl;

          std::vector< std::vector< double > > matrix;
          std::vector< double > vec;

          utility::InitializeMatrixRandomValues( 1, 1000, 10000, matrix );
          utility::InitializeVectorRandomValues( 1, 1000, 10000, vec );

          std::cout << "Исходная матрица:\n" << utility::OutputMatrix( matrix );
          std::cout << "Исходная вектор: " << utility::OutputVector( vec );

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
          for ( int i = utility::ROOT_PROCESS_RANK + 1; i < size; ++i )
          {
               MPI_Send( &partSize, 1, MPI_INT, i, utility::TAG, MPI_COMM_WORLD );
               for ( int j = shift + partSize * i; j < shift + partSize * ( i + 1 ); ++j )
               {
                    MPI_Send( &( matrix[ j ][ 0 ] ), elemCount, MPI_DOUBLE, i, utility::TAG, MPI_COMM_WORLD );
               }

               MPI_Send( vec.data(), elemCount, MPI_DOUBLE, i, utility::TAG, MPI_COMM_WORLD );
          }

          // подсчет значения матрицы в root процессе
          std::vector< std::vector< double > > rootMatrix;
          for ( int i = 0; i < shift + partSize; ++i )
          {
               rootMatrix.emplace_back( matrix[ i ] );
          }
          std::vector< double > rootResult = utility::MultiplyMatrixToVector( rootMatrix, vec );

          // вставка результата расчета root процесса в итоговый вектор
          for ( const auto& elem : rootResult )
          {
               result.emplace_back( elem );
          }

          // получение результирующих векторов из дочерних процессов и вставка их в итоговый вектор
          for ( int i = utility::ROOT_PROCESS_RANK + 1; i < size; ++i )
          {
               std::vector< double > processResult;
               processResult.resize( partSize );
               MPI_Recv( &( processResult[ 0 ] ), partSize, MPI_DOUBLE, i, utility::TAG, MPI_COMM_WORLD, &status );
               for ( const auto& elem : processResult )
               {
                    result.emplace_back( elem );
               }
          }
     }
     else
     {
          // получение количества строк в матрице из root процесса
          MPI_Probe( utility::ROOT_PROCESS_RANK, utility::TAG, MPI_COMM_WORLD, &status );
          int rowsCount = 0;
          MPI_Recv( &rowsCount, 1, MPI_INT, utility::ROOT_PROCESS_RANK, utility::TAG, MPI_COMM_WORLD, &status );

          // получение матрицы из root процесса
          std::vector< std::vector< double > > localMatrix;
          for ( int i = 0; i < rowsCount; ++i )
          {
               int elemsCount = 0;
               MPI_Probe( utility::ROOT_PROCESS_RANK, utility::TAG, MPI_COMM_WORLD, &status );
               MPI_Get_count( &status, MPI_DOUBLE, &elemsCount );

               std::vector< double > row;
               row.resize( elemsCount );
               MPI_Recv( &( row[ 0 ] ), elemsCount, MPI_DOUBLE, utility::ROOT_PROCESS_RANK, utility::TAG,
                         MPI_COMM_WORLD, &status );
               localMatrix.emplace_back( row );
          }

          // получение вектора из root процесса
          int elemsCount = 0;
          MPI_Probe( utility::ROOT_PROCESS_RANK, utility::TAG, MPI_COMM_WORLD, &status );
          MPI_Get_count( &status, MPI_DOUBLE, &elemsCount );
          std::vector< double > vec;
          vec.resize( elemsCount );
          MPI_Recv( &( vec[ 0 ] ), elemsCount, MPI_DOUBLE, utility::ROOT_PROCESS_RANK, utility::TAG, MPI_COMM_WORLD,
                    &status );

          // расчет результата умножения матрицы на вектор
          result = utility::MultiplyMatrixToVector( localMatrix, vec );

          // отправка результирующего вектора в root процесс
          MPI_Send( &( result[ 0 ] ), result.size(), MPI_DOUBLE, utility::ROOT_PROCESS_RANK, utility::TAG,
                    MPI_COMM_WORLD );
     }

     if ( utility::IsRootProcess( rank ) )
     {
          std::cout << "Результат умножения в " << rank << " процессе = " << utility::OutputVector( result );

          auto end = std::chrono::high_resolution_clock::now();
          auto duration = std::chrono::duration_cast< std::chrono::milliseconds >( end - start );
          std::cout << "Время выполнения: " << duration.count() << " мс" << std::endl;
     }

     MPI_Finalize();

     return 0;
}