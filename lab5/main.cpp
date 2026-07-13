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
     std::uniform_int_distribution< int > distributionBig( maxValue, 1.5 * maxValue );
     std::uniform_int_distribution< int > distributionSmall( 1, maxValue / 2 );

     for ( int i = 0; i < countElements; ++i )
     {
          matrix.emplace_back();
          for ( int j = 0; j < countElements; ++j )
          {
               if ( j == i )
               {
                    matrix[ i ].emplace_back( distributionBig( generator ) );
               }
               else if ( j == i - 1 )
               {
                    matrix[ i ].emplace_back( distributionSmall( generator ) );
               }
               else if ( j == i + 1 )
               {
                    matrix[ i ].emplace_back( distributionSmall( generator ) );
//                    int diff = std::abs( matrix[ i ][ i ] ) - std::abs( matrix[ i ][ i - 1 ] );
//                    std::uniform_int_distribution< int > localDistribution( -diff, diff );
//                    matrix[ i ].emplace_back( localDistribution( generator ) );
//                    if ( matrix[ i ][ j ] == 0 )
//                    {
//                         matrix[ i ][ j ]++;
//                    }
               }
               else
               {
                    matrix[ i ].emplace_back( 0.0 );
               }
          }
     }
}

void InitializeMatrixAndVector( std::vector< std::vector< double > >& matrix, std::vector< double >& vec )
{
     matrix = { { 3, 1, 0 }, { -1, 4, 2 }, { 0, 2, 3 } };

     vec = { 1, -3, 5 };
}

// Сходится ли метод прогонки при исходной матрице = matrix
bool IsConverge( const std::vector< std::vector< double > >& matrix )
{
     if ( matrix.empty() )
     {
          std::cout << "Передана пустая матрица при проверке сходимости алгоритма" << std::endl;
          return false;
     }

     if ( std::abs( matrix[ 0 ][ 0 ] ) < std::abs( matrix[ 0 ][ 1 ] ) )
     {
          return false;
     }

     size_t indexLastElement = matrix.size() - 1;
     if ( std::abs( matrix[ indexLastElement ][ indexLastElement ] ) <
          std::abs( matrix[ indexLastElement ][ indexLastElement - 1 ] ) )
     {
          return false;
     }

     for ( int i = 1; i < matrix.size() - 1; ++i )
     {
          if ( std::abs( matrix[ i ][ i ] ) < std::abs( matrix[ i ][ i - 1 ] ) + std::abs( matrix[ i ][ i + 1 ] ) )
          {
               return false;
          }
     }
     return true;
}

std::vector< double > TridiagonalMatrixAlgorithm( std::vector< double >& a, std::vector< double >& b,
                                                  std::vector< double >& c, std::vector< double >& d,
                                                  int proccessRank, int countProcesses )
{
     int n = b.size();

     if ( a.size() != n - 1 || c.size() != n - 1 ) {
          throw std::invalid_argument( "Размеры массивов не соответствуют требованиям для трехдиагональной матрицы." );
     }

     // Расчет начальной и конечной позиции для расчетов для конкретного процесса
     const size_t currentProcessPositionBegin = proccessRank * d.size() / countProcesses;
     const size_t currentProcessPositionEnd = ( proccessRank + 1 ) * d.size() / countProcesses;

     std::vector< double > alpha( n, 0.0 );
     std::vector< double > beta( n, 0.0 );
     std::vector< double > x( n, 0.0 );

     // Прямой ход
     alpha[ 0 ] = -c[ 0 ] / b[ 0 ];
     beta[ 0 ] = d[ 0 ] / b[ 0 ];

     for ( int i = 1; i < n - 1; ++i )
     {
          alpha[ i ] = -c[ i ] / ( b[ i ] + a[ i - 1 ] * alpha[ i - 1 ] );
          beta[ i ] = ( d[ i ] - a[ i - 1 ] * beta[ i - 1 ] ) / ( b[ i ] + a[ i - 1 ] * alpha[ i - 1 ] );
     }

     beta[ n - 1 ] = ( d[ n - 1 ] - a[ n - 2 ] * beta[ n - 2 ] ) / ( b[ n - 1 ] + a[ n - 2 ] * alpha[ n - 2 ] );

     // Подготовка массивов рассчитанных значений в конкретном процессе на отправку остальным процессам
     std::vector< double > sendAlpha, sendBeta;
     for ( size_t i = currentProcessPositionBegin; i < currentProcessPositionEnd; ++i )
     {
          sendAlpha.emplace_back( alpha[ i ] );
          sendBeta.emplace_back( beta[ i ] );
     }

     // Отправка массивов рассчитанных значений из текущего процесса в остальные процессы
     for ( int i = 0; i < countProcesses; ++i )
     {
          if ( proccessRank != i )
          {
               int countElements = sendAlpha.size();
               MPI_Send( &countElements, 1, MPI_INT, i, 0, MPI_COMM_WORLD );
               MPI_Send( sendAlpha.data(), countElements, MPI_DOUBLE, i, 0, MPI_COMM_WORLD );

               countElements = sendBeta.size();
               MPI_Send( &countElements, 1, MPI_INT, i, 0, MPI_COMM_WORLD );
               MPI_Send( sendBeta.data(), countElements, MPI_DOUBLE, i, 0, MPI_COMM_WORLD );
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
               std::vector< double > recieveAlpha;
               recieveAlpha.resize( countElements );
               MPI_Recv( &( recieveAlpha[ 0 ] ), countElements, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, &status );

               MPI_Recv( &countElements, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status );
               std::vector< double > recieveBeta;
               recieveBeta.resize( countElements );
               MPI_Recv( &( recieveBeta[ 0 ] ), countElements, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, &status );
          }
     }

     // Обратный ход
     x[ n - 1 ] = beta[ n - 1 ];
     for ( int i = n - 2; i >= 0; --i )
     {
          x[ i ] = alpha[ i ] * x[ i + 1 ] + beta[ i ];
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
     for ( int i = 0; i < countProcesses; ++i )
     {
          if ( proccessRank != i )
          {
               int countElements;
               MPI_Recv( &countElements, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status );

               std::vector< double > recieveX;
               std::vector< double > X( countElements, 0.0 );
               recieveX.resize( countElements );
               MPI_Recv( &( recieveX[ 0 ] ), countElements, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, &status );
          }
     }

     return x;
}

int main( int argc, char** argv )
{
     int rank, size;

     MPI_Init( &argc, &argv );
     MPI_Comm_rank( MPI_COMM_WORLD, &rank );
     MPI_Comm_size( MPI_COMM_WORLD, &size );

     std::vector< std::vector< double > > matrix;
     std::vector< double > a;
     std::vector< double > b;
     std::vector< double > c;
     std::vector< double > d;
     int countElements;

     // Инициализация исходной матрицы и вектора D
     if ( IsRootProcess( rank ) )
     {
          std::cout << "Всего процессов = " << size << std::endl;

          InitializeVectorRandomValues( -100, 100, 10000, d );

          bool flag = false;
          for ( int i = 0; i < 10; ++i )
          {
               matrix.clear();
               InitializeMatrixRandomValues( -100, 100, 10000, matrix );
               if ( IsConverge( matrix ) )
               {
                    flag = true;
                    break;
               }

          }
          if ( !flag )
          {
               std::cout << "За 10 попыток не удалось сгенерировать удовлетворяющую условиям сходимости матрицу" << std::endl;
          }

//          d.clear();
//          matrix.clear();
//          InitializeMatrixAndVector( matrix, d );

//          std::cout << "Исходная матрица:\n" << OutputMatrix( matrix );
//          std::cout << "Исходный вектор d:\n" << OutputVector( d );

          countElements = d.size();

          b.emplace_back( matrix[ 0 ][ 0 ] );
          c.emplace_back( matrix[ 0 ][ 1 ] );

          for ( int i = 1; i < matrix.size() - 1; ++i )
          {
               a.emplace_back( matrix[ i ][ i - 1 ] );
               b.emplace_back( matrix[ i ][ i ] );
               c.emplace_back( matrix[ i ][ i + 1 ] );
          }

          size_t indexLastElement = matrix.size() - 1;
          a.emplace_back( matrix[ indexLastElement ][ indexLastElement - 1 ] );
          b.emplace_back( matrix[ indexLastElement ][ indexLastElement ] );

//          std::cout << "Исходный вектор a:\n" << OutputVector( a );
//          std::cout << "Исходный вектор b:\n" << OutputVector( b );
//          std::cout << "Исходный вектор c:\n" << OutputVector( c );
     }

     // Рассылка размера матрицы и вектора D из главного процесса в дочерние процессы
     MPI_Bcast( &countElements, 1, MPI_INT, 0, MPI_COMM_WORLD );

     // Подготовка матрицы и вектора D в дочерних процессах для их получения из главного процесса
     if ( !IsRootProcess( rank ) )
     {
          a.resize( countElements - 1 );
          b.resize( countElements );
          c.resize( countElements - 1 );
          d.resize( countElements );
     }

     // Рассылка и получение исходной матрицы и вектора D из главного процесса в дочерние процессы
     MPI_Bcast( a.data(), a.size(), MPI_DOUBLE, 0, MPI_COMM_WORLD );
     MPI_Bcast( b.data(), b.size(), MPI_DOUBLE, 0, MPI_COMM_WORLD );
     MPI_Bcast( c.data(), c.size(), MPI_DOUBLE, 0, MPI_COMM_WORLD );
     MPI_Bcast( d.data(), d.size(), MPI_DOUBLE, 0, MPI_COMM_WORLD );

     // Инициализация и задание начальных значений для вектора X
     std::vector< double > x( d.size(), 0.0 );

     auto start = std::chrono::high_resolution_clock::now();
     // Запуск алгоритма
     x = TridiagonalMatrixAlgorithm( a, b, c, d, rank, size );
     auto end = std::chrono::high_resolution_clock::now();

     // Вывод результата
     if ( IsRootProcess( rank ) )
     {
          auto durationMilli = std::chrono::duration_cast< std::chrono::milliseconds >( end - start );
          auto durationMicro = std::chrono::duration_cast< std::chrono::microseconds >( end - start );

//          std::cout << "Результат решения СЛАУ x: " << OutputVector( x );
          std::cout << "Время выполнения метода Прогонки: " << durationMilli.count() << " мс" << std::endl;
          std::cout << "Время выполнения метода Прогонки: " << durationMicro.count() << " мкс" << std::endl;
     }

     MPI_Finalize();

     return 0;
}