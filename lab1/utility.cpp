#include "utility.h"

namespace utility
{

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

void InitializeMatrixFromFile( const std::string& filename, std::vector< std::vector< double > >& matrix )
{
     std::ifstream file( filename );
     if ( !file.is_open() )
     {
          std::cerr << "Не удалось считать матрицу из файла. Ошибка открытия файла: " << filename << std::endl;
     }

     int countElements;
     file >> countElements;

     // Чтение данных из файла
     for ( int i = 0; i < countElements; ++i )
     {
          matrix.emplace_back();
          for ( int j = 0; j < countElements; ++j )
          {
               double elem;
               file >> elem;
               matrix[ i ].emplace_back( elem );
          }
     }

     file.close();
}

void InitializeVectorFromFile( const std::string& filename, std::vector< double >& vector )
{
     std::ifstream file( filename );
     if ( !file.is_open() )
     {
          std::cerr << "Не удалось считать вектор из файла. Ошибка открытия файла: " << filename << std::endl;
     }

     int countElements;
     file >> countElements;

     // Чтение данных из файла
     for ( int j = 0; j < countElements; ++j )
     {
          double elem;
          file >> elem;
          vector.emplace_back( elem );
     }

     file.close();
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

} // namespace utility