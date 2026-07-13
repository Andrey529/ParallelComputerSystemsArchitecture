#include <iostream>
#include <random>
#include <sstream>
#include <vector>
#include <chrono>
#include <mpi/mpi.h>

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

void InitializeVectorRandomValues( int minValue, int maxValue, int countElements, std::vector< double >& vector )
{
     std::random_device rd;
     std::mt19937 generator( rd() );
     std::uniform_int_distribution< int > distribution( minValue, maxValue );

     for ( int i = 0; i < countElements; ++i )
     {
          vector.emplace_back( distribution( generator ) );
     }
}

void InitializeMatrixRandomValues( int minValue,
                                   int maxValue,
                                   int countElements,
                                   std::vector< std::vector< double > >& matrix )
{
     std::random_device rd;
     std::mt19937 generator( rd() );
     std::uniform_int_distribution< int > distribution( minValue, maxValue );

     for ( int i = 0; i < countElements; ++i )
     {
          matrix.emplace_back();
          for ( int j = 0; j < countElements; ++j )
          {
               matrix[ i ].emplace_back( distribution( generator ) );
          }
     }
}

void InitializeMatrixAndVector( std::vector< std::vector< double > >& matrix, std::vector< double >& vec )
{
     matrix = { { 5, 2, -1 }, { -4, 7, 3 }, { 2, -2, 4 } };

     vec = { 12, 24, 9 };
}

void Jacobi( std::vector< std::vector< double > >& matrix,
             std::vector< double >& b,
             std::vector< double >& x,
             int proccessRank,
             int countProcesses,
             double eps = 0.01,
             double norm = 1,
             int maxIterations = 100 )
{
     std::vector< double > tempX = x;
     int iteration = 0;

     // Расчет начальной и конечной позиции для расчетов для конкретного процесса
     const size_t currentProcessPositionBegin = proccessRank * x.size() / countProcesses;
     const size_t currentProcessPositionEnd = ( proccessRank + 1 ) * x.size() / countProcesses;

     while ( norm > eps && iteration < maxIterations )
     {
          // Расчет очередного значения вектора X в текущем процессе
          for ( size_t i = currentProcessPositionBegin; i < currentProcessPositionEnd; ++i )
          {
               tempX[ i ] = b[ i ];
               for ( size_t j = 0; j < x.size(); j++ )
               {
                    if ( i != j )
                    {
                         tempX[ i ] -= matrix[ i ][ j ] * x[ j ];
                    }
               }
               tempX[ i ] /= matrix[ i ][ i ];
          }

          // Подсчет максимального значения разницы текущего и следующего значений X
          norm = std::abs( x[ 0 ] - tempX[ 0 ] );
          for ( size_t i = 0; i < x.size(); i++ )
          {
               if ( std::abs( x[ i ] - tempX[ i ] ) > norm )
               {
                    norm = std::abs( x[ i ] - tempX[ i ] );
               }
               x[ i ] = tempX[ i ];
          }

          // Подготовка массива рассчитанных значений в конкретном процессе на отправку остальным процессам
          std::vector< double > sendX;
          for ( size_t i = currentProcessPositionBegin; i < currentProcessPositionEnd; ++i )
          {
               sendX.emplace_back( x[ i ] );
          }

          // Отправка массива рассчитанных значений из текущего процесса в остальные процессы
          for ( int i = 0; i < countProcesses; ++i )
          {
               if ( proccessRank != i )
               {
                    int countElements = sendX.size();
                    MPI_Send( &countElements, 1, MPI_INT, i, 0, MPI_COMM_WORLD );
                    MPI_Send( sendX.data(), countElements, MPI_DOUBLE, i, 0, MPI_COMM_WORLD );
               }
          }

          // Получение массивов рассчитанных значений из остальных процессов
          MPI_Status status;
          for ( int i = 0; i < countProcesses; ++i )
          {
               if ( proccessRank != i )
               {
                    int countElements;
                    MPI_Recv( &countElements, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status );

                    std::vector< double > recieveX;
                    recieveX.resize( countElements );
                    MPI_Recv( &( recieveX[ 0 ] ), countElements, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, &status );

                    for ( int j = i * x.size() / countProcesses, k = 0; j != ( i + 1 ) * x.size() / countProcesses; ++j, ++k )
                    {
                         x[ j ] = recieveX[ k ];
                    }
               }
          }

          ++iteration;
     }
}

int main( int argc, char** argv )
{
     int rank, size;

     MPI_Init( &argc, &argv );
     MPI_Comm_rank( MPI_COMM_WORLD, &rank );
     MPI_Comm_size( MPI_COMM_WORLD, &size );

     std::vector< std::vector< double > > matrix;
     std::vector< double > b;
     int countElements;

     // Инициализация исходной матрицы и вектора B
     if ( IsRootProcess( rank ) )
     {
          std::cout << "Всего процессов = " << size << std::endl;

//          InitializeMatrixAndVector( matrix, b );

          InitializeMatrixRandomValues( -100, 100, 10000, matrix );
          InitializeVectorRandomValues( -100, 100, 10000, b );

//          std::cout << "Исходная матрица:\n" << OutputMatrix( matrix );
//          std::cout << "Исходный вектор b:\n" << OutputVector( b );

          countElements = b.size();
     }

     // Рассылка размера матрицы и вектора B из главного процесса в дочерние процессы
     MPI_Bcast( &countElements, 1, MPI_INT, 0, MPI_COMM_WORLD );

     // Подготовка матрицы и вектора B в дочерних процессах для их получения из главного процесса
     if ( !IsRootProcess( rank ) )
     {
          for ( int i = 0; i < countElements; ++i )
          {
               matrix.emplace_back( countElements, 0.0 );
          }
          b.resize( countElements );
     }

     // Рассылка и получение исходной матрицы и вектора B из главного процесса в дочерние процессы
     for ( int i = 0; i < countElements; ++i )
     {
          MPI_Bcast( matrix[i].data(), countElements, MPI_DOUBLE, 0, MPI_COMM_WORLD );
     }
     MPI_Bcast( b.data(), countElements, MPI_DOUBLE, 0, MPI_COMM_WORLD );

     // Инициализация и задание начальных значений для вектора X
     std::vector< double > x( b.size(), 0.0 );

     auto start = std::chrono::high_resolution_clock::now();

     // Запуск итерационного метода Якоби
     Jacobi( matrix, b, x, rank, size );

     // Вывод результата
     if ( IsRootProcess( rank ) )
     {
          auto end = std::chrono::high_resolution_clock::now();
          auto durationMilli = std::chrono::duration_cast< std::chrono::milliseconds >( end - start );
          auto durationMicro = std::chrono::duration_cast< std::chrono::microseconds >( end - start );

          std::cout << "Результат решения СЛАУ x: " << OutputVector( x );
          std::cout << "Время выполнения алгоритма Якоби: " << durationMilli.count() << " мс" << std::endl;
          std::cout << "Время выполнения алгоритма Якоби: " << durationMicro.count() << " мкс" << std::endl;
     }

     MPI_Finalize();

     return 0;
}