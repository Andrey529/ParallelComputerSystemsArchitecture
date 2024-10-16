#include <iostream>
#include <random>
#include <sstream>
#include <chrono>
#include <vector>
#include <omp.h>

std::vector< int > GenerateVectorRandomValues( size_t countElements, int minValue, int maxValue )
{
     std::random_device rd;
     std::mt19937 generator( rd() );
     std::uniform_int_distribution< int > distribution( minValue, maxValue );

     std::vector< int > vec;
     for ( int i = 0; i < countElements; ++i )
     {
          vec.emplace_back( distribution( generator ) );
     }
     return vec;
}

std::string OutputVec( const std::vector< int >& vec )
{
     std::stringstream ss;
     for ( const auto& elem : vec )
     {
          ss << elem << ' ';
     }
     ss << std::endl;
     return ss.str();
}

void ShellSortConsequently( std::vector< int >& vec )
{
     size_t vecSize = vec.size();

     for ( size_t gap = vecSize / 2; gap > 0; gap /= 2 )
     {
          for ( size_t i = gap; i < vecSize; i++ )
          {
               int temp = vec[ i ];
               size_t j;

               for ( j = i; j >= gap && vec[ j - gap ] > temp; j -= gap )
               {
                    vec[ j ] = vec[ j - gap ];
               }

               vec[ j ] = temp;
          }
     }
}

void InsertionSort( std::vector< int >& vec, size_t i, size_t gap )
{
     size_t vecSize = vec.size();
     int temp = 0;
     size_t j = 0;

     for ( size_t f = gap + i; f < vecSize; f = f + gap )
     {
          j = f;
          while ( j > i && vec[ j - gap ] > vec[ j ] )
          {
               temp = vec[ j ];
               vec[ j ] = vec[ j - gap ];
               vec[ j - gap ] = temp;
               j = j - gap;
          }
     }
}

void ShellSortParallel( std::vector< int >& vec )
{
     size_t vecSize = vec.size();
     size_t gap;
     size_t i = 0;
     for ( gap = vecSize / 2; gap > 0; gap /= 2 )
     {
#pragma omp parallel for shared( vec, vecSize, gap, i ) default( none )
          for ( i = 0; i < gap; i++ )
          {
               InsertionSort( vec, i, gap );
          }
     }
}

int main()
{
     int numThreads{};
     std::cout << "Введите количество потоков:" << std::endl;
     std::cin >> numThreads;
     omp_set_num_threads( numThreads );

     size_t countElements{};
     std::cout << "Введите количество элементов массива:" << std::endl;
     std::cin >> countElements;

     long long microsecondsForAllTests = 0;

     for ( int i = 0; i < 100; ++i )
     {
          std::vector< int > vecConsequently = GenerateVectorRandomValues( countElements, -100, 100 );
          std::vector< int > vecParallel = vecConsequently;

          std::cout << "Исходный массив до сортировки:\n" << OutputVec( vecConsequently );

          ShellSortConsequently( vecConsequently );

          std::cout << "Массив после сортировки последовательно:\n" << OutputVec( vecConsequently );

          auto start = std::chrono::high_resolution_clock::now();

          ShellSortParallel( vecParallel );

          auto end = std::chrono::high_resolution_clock::now();
          microsecondsForAllTests += std::chrono::duration_cast< std::chrono::microseconds >( end - start ).count();

          std::cout << "Массив после сортировки параллельно:\n" << OutputVec( vecParallel );
     }

     std::cout << "Среднее время выполнения: " << microsecondsForAllTests / 100 << " микросекунд" << std::endl;

     return 0;
}