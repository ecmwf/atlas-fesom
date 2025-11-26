/*
 * (C) Copyright 2021- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "atlas/grid/SpecRegistry.h"

#include "tests/AtlasTestEnvironment.h"

namespace atlas {
namespace test {

//-----------------------------------------------------------------------------

CASE( "test spec" ) {
    auto registered = [&]( const std::string& uid_or_name ) { return grid::SpecRegistry::has( uid_or_name ); };
    auto grid_name  = [&]( const std::string& uid_or_name ) {
        return grid::SpecRegistry::get( uid_or_name ).getString( "name" );
    };
    auto grid_base_name  = [&]( const std::string& uid_or_name ) {
        return grid::SpecRegistry::get( uid_or_name ).getString( "base_name" );
    };
    auto size = [&]( const std::string& uid_or_name ) {
        return grid::SpecRegistry::get( uid_or_name ).getInt( "size" );
    };
    auto uid = [&]( const std::string& uid_or_name ) {
        return grid::SpecRegistry::get( uid_or_name ).getString( "uid" );
    };


    std::vector<std::string> uids;
    std::map<std::string, std::string> check_name;

    std::vector<std::string> grids{"fesom-pi", "CORE2", "tORCA025", "NG5", "DART"};
    for ( const auto& grid : grids ) {
        for (auto arrangement: {"N", "C"}) {
            std::string name = grid+"_"+arrangement;
            EXPECT( registered( name ) );
            EXPECT_EQ( grid_name( name ), name );
            EXPECT_NO_THROW( uid( name ) );
            EXPECT_NO_THROW( size( name ) );
            EXPECT_EQ( grid_base_name( name ), grid );
            Log::info() << std::setw( 11 ) << std::left << name << "    " << std::setw( 11 ) << std::left << grid_base_name( name ) << "    " << uid( name ) << "    " << size( name )
                        << std::endl;
            uids.push_back( uid( name ) );
            check_name[uids.back()] = name;
        }
    }

    for ( const auto& grid : uids ) {
        EXPECT( registered( grid ) );
        EXPECT_EQ( grid, uid( grid ) );
        EXPECT_EQ( grid_name( grid ), check_name[grid] );
        Log::info() << grid << "    " << std::setw( 11 ) << std::left << grid_name( grid ) << "    " << size( grid )
                    << std::endl;
    }
}

//-----------------------------------------------------------------------------

}  // namespace test
}  // namespace atlas

int main( int argc, char** argv ) {
    return atlas::test::run( argc, argv );
}
