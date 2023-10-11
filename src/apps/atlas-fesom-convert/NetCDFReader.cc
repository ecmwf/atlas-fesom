/*
 * (C) Copyright 2021- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "NetCDFReader.h"

#if ATLAS_FESOM_HAVE_NETCDF
#include <netcdf>
#endif

#include <algorithm>
#include <sstream>

#include "atlas/runtime/Exception.h"
#include "atlas/runtime/Trace.h"
#include "atlas/util/vector.h"

namespace atlas {
namespace fesom {

NetCDFReader::NetCDFReader( const util::Config& config ) {}

void NetCDFReader::read( const std::string& uri, FesomData& data ) {
#if ATLAS_FESOM_HAVE_NETCDF == 0
    ATLAS_THROW_EXCEPTION( "atlas-fesom was not compiled with NetCDF support" );
#else
    FesomDataFile file{uri};

    netCDF::NcFile ncdata( file, netCDF::NcFile::read );
    // auto read_dimensions = [&]( size_t& ni, size_t& nj ) {
    //     auto get_dim = [&]( const std::string& name ) {
    //         auto dim = ncdata.getDim( name );
    //         if ( dim.isNull() ) {
    //             ATLAS_THROW_EXCEPTION( "Dimension '" << name << "' is not present in NetCDF file" );
    //         }
    //         return dim.getSize();
    //     };
    //     ni = get_dim( "x" );
    //     nj = get_dim( "y" );
    // };

    auto read_dim = [&]( const std::string& name ) {
        auto dim = ncdata.getDim( name );
        if ( dim.isNull() ) {
            ATLAS_THROW_EXCEPTION( "Dimension '" << name << "' is not present in NetCDF file" );
        }
        return dim.getSize();
    };


    auto read_variable = [&]( const std::string& name, std::vector<double>& values ) {
        auto var = ncdata.getVar( name );
        if ( var.isNull() ) {
            ATLAS_THROW_EXCEPTION( "Variable '" << name << "' is not present in NetCDF file" );
        }
        auto dims = var.getDims();
        ATLAS_ASSERT( dims.size() == 1 );
        values.resize(dims[0].getSize());
        var.getVar( {0}, {values.size()}, values.data() );
    };

    auto read_cells = [&]( const std::string& name, std::vector<long>& values ) {
        auto var = ncdata.getVar( name );
        if ( var.isNull() ) {
            ATLAS_THROW_EXCEPTION( "Variable '" << name << "' is not present in NetCDF file" );
        }
        auto dims = var.getDims();
        ATLAS_ASSERT( dims.size() == 2 );
        ATLAS_ASSERT( dims[0].getSize() == 3 );
        values.resize(dims[1].getSize()*3);
        var.getVar( {0,0}, {3,dims[1].getSize()}, values.data() );
    };

    data.nb_nodes = read_dim( "nod2" );
    data.nb_cells = read_dim( "elem" );
    read_variable( "lon", data.lon );
    read_variable( "lat", data.lat );

    std::vector<long> connectivity_cell2node;
    read_cells("elements",connectivity_cell2node);

    // Transpose
    data.connectivity_cell2node.resize(connectivity_cell2node.size());
    for( size_t i=0; i<data.nb_cells; ++i) {
        data.connectivity_cell2node[i*3+0] = connectivity_cell2node[i+0*data.nb_cells] - 1;
        data.connectivity_cell2node[i*3+1] = connectivity_cell2node[i+2*data.nb_cells] - 1;
        data.connectivity_cell2node[i*3+2] = connectivity_cell2node[i+1*data.nb_cells] - 1;
    }
#endif
}

}  // namespace fesom
}  // namespace atlas
