#include <iostream>
#include <random>
#include <sstream>
#include <vector>
#include <chrono>

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


void Gauss( std::vector< std::vector< double > >& matrix, std::vector< double >& x, int threadsCount )
{
     const int N = matrix.size();
     // Прямой ход, приведение к верхнетреугольному виду
     for ( int i = 0; i < N; i++)
     {
          double tmp = matrix[ i ][ i ];
#pragma omp parallel for num_threads( threadsCount ) shared( N, matrix, i, tmp ) default( none )
          for ( int j = N; j >= i; j-- )
          {
               matrix[ i ][ j ] /= tmp;
          }

          for ( int j = i + 1; j < N; j++ )
          {
               tmp = matrix[ j ][ i ];
#pragma omp parallel for num_threads( threadsCount ) shared( N, matrix, tmp, i, j ) default( none )
               for ( int k = N; k >= i; k-- )
               {
                    matrix[ j ][ k ] -= tmp * matrix[ i ][ k ];
               }
          }
     }

     // Обратный ход
     x[ N - 1 ] = matrix[ N - 1 ][ N ];
     for ( int i = N - 2; i >= 0; --i )
     {
          x[ i ] = matrix[ i ][ N ];

          std::vector< double > substract( N, 0.0 );
#pragma omp parallel for num_threads( threadsCount ) shared( i, substract, matrix, N, x ) default( none )
          for ( int j = i + 1; j < N; ++j )
          {
               substract[ j ] = matrix[ i ][ j ] * x[ j ];
          }

          for ( int j = i + 1; j < N; ++j )
          {
               x[ i ] -= substract[ j ];
          }
     }
}

int main()
{
     std::vector< std::vector< double > > matrix;
     std::vector< double > b;
     int N{};
     int threadsCount = 8;

     std::cout << "Введите количество потоков исполнения программы:" << std::endl;
     std::cin >> threadsCount;

//     std::cout << "Введите размерность матрицы для генерации ее элементов:" << std::endl;
//     std::cin >> N;

     std::vector< double > result;
     long long microsecondsForAllTests = 0;
     long long millisecondsForAllTests = 0;
     int countIterations = 10;
     for ( int i = 0; i < countIterations; ++i )
     {
          matrix.clear();
          b.clear();
          result.clear();

          InitializeMatrixAndVector( matrix, b );

//          InitializeMatrixRandomValues( -100, 100, N, matrix );
//          InitializeVectorRandomValues( -100, 100, N, b );
          result.resize( b.size(), 0.0 );

          std::cout << "Исходная матрица:\n" << OutputMatrix( matrix );
          std::cout << "Исходный вектор b:\n" << OutputVector( b );

          for ( int j = 0; j < b.size(); ++j )
          {
               matrix[ j ].emplace_back( b[ j ] );
          }

          auto start = std::chrono::high_resolution_clock::now();
          // Метод Гаусса
          Gauss( matrix, result, threadsCount );
          auto end = std::chrono::high_resolution_clock::now();

          microsecondsForAllTests += std::chrono::duration_cast< std::chrono::microseconds >( end - start ).count();
          millisecondsForAllTests += std::chrono::duration_cast< std::chrono::milliseconds >( end - start ).count();
     }

     // Вывод результата
     std::cout << "Потоков выполнения программы: " << threadsCount << std::endl;
     std::cout << "Результат решения СЛАУ x: " << OutputVector( result );
     std::cout << "Время выполнения алгоритма методом Гаусса: " << microsecondsForAllTests / countIterations << " мкс" << std::endl;
     std::cout << "Время выполнения алгоритма методом Гаусса: " << millisecondsForAllTests / countIterations << " мс" << std::endl;

     return 0;
}
