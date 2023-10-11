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
    auto nb_nodes = [&]( const std::string& uid_or_name ) {
        int v;
        grid::SpecRegistry::get( uid_or_name ).get( "nb_nodes", v );
        return v;
    };
    auto uid = [&]( const std::string& uid_or_name ) {
        return grid::SpecRegistry::get( uid_or_name ).getString( "uid" );
    };


    std::vector<std::string> uids;
    std::map<std::string, std::string> check_name;

    std::vector<std::string> grids{"FESOM1"};
    for ( const auto& grid : grids ) {
        std::string name = grid;
        EXPECT( registered( name ) );
        EXPECT( grid_name( name ) == grid );
        EXPECT_NO_THROW( uid( name ) );
        EXPECT_NO_THROW( nb_nodes( name ) );
        Log::info() << std::setw( 11 ) << std::left << name << "    " << uid( name ) << "    " << nb_nodes( name )
                    << std::endl;
        uids.push_back( uid( name ) );
        check_name[uids.back()] = name;
    }

    for ( const auto& grid : uids ) {
        EXPECT( registered( grid ) );
        EXPECT( grid == uid( grid ) );
        EXPECT( grid_name( grid ) == check_name[grid] );
    }
}

//-----------------------------------------------------------------------------

}  // namespace test
}  // namespace atlas

int main( int argc, char** argv ) {
    return atlas::test::run( argc, argv );
}
