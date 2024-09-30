#ifndef PARALLELCOMPUTERSYSTEMSARCHITECTURE_UTILITY_H
#define PARALLELCOMPUTERSYSTEMSARCHITECTURE_UTILITY_H

#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

namespace utility
{

static constexpr int TAG = 0;
static constexpr int ROOT_PROCESS_RANK = 0;

bool IsRootProcess( int processRank );

std::string OutputMatrix( const std::vector< std::vector< double > >& matrix );

std::string OutputVector( const std::vector< double >& vector );

void InitializeMatrixAndVector( std::vector< std::vector< double > >& matrix, std::vector< double >& vec );

void InitializeMatrixFromFile( const std::string& filename, std::vector< std::vector< double > >& matrix );

void InitializeVectorFromFile( const std::string& filename, std::vector< double >& vector );

void InitializeMatrixRandomValues( int minValue,
                                   int maxValue,
                                   int countElements,
                                   std::vector< std::vector< double > >& matrix );

void InitializeVectorRandomValues( int minValue, int maxValue, int countElements, std::vector< double >& vector );

std::vector< double > MultiplyMatrixToVector( const std::vector< std::vector< double > >& matrix,
                                              const std::vector< double >& vector );

} // namespace utility

#endif // PARALLELCOMPUTERSYSTEMSARCHITECTURE_UTILITY_H
