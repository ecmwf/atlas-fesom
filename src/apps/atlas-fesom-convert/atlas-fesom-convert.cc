/*
 * (C) Copyright 2021- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <bitset>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>


#include "eckit/filesystem/PathName.h"
#include "eckit/log/Bytes.h"

#include "atlas/runtime/AtlasTool.h"
#include "atlas/runtime/Exception.h"

#include "NetCDFReader.h"
#include "FesomData.h"
#include "AtlasIOReader.h"
#include "AsciiReader.h"

namespace atlas {
namespace fesom {

//----------------------------------------------------------------------------------------------------------------------

struct Tool : public atlas::AtlasTool {
    bool serial() override { return true; }
    int execute( const Args& args ) override;
    std::string briefDescription() override { return "Create binary grid data files "; }
    std::string usage() override {
        return name() + " <file> --name=NAME [OPTION]... [--help,-h]";
    }
    std::string longDescription() override {
        return "Create binary grid data files \n"
               "\n"
               "       <file>: input file";
    }

    Tool( int argc, char** argv ) : AtlasTool( argc, argv ) {
        add_option( new SimpleOption<std::string>( "name", "Output grid name" ) );
        add_option( new SimpleOption<std::string>( "input-format",
                                                   "netcdf, atlas-io, ascii" ) );
        add_option( new SimpleOption<std::string>( "compression",
                                                   "Data compression: none, lz4, aec, ... (see eckit support)'" ) );
        add_option(
            new SimpleOption<std::string>( "output", "Output file path; default: <name>_<arrangement>.atlas" ) );
        add_option( new Separator( "Advanced" ) );
        add_option( new SimpleOption<bool>( "verbose", "Print verbose output" ) );
        add_option( new SimpleOption<bool>( "yaml", "Output spec instead of data" ) );
    }
};

//------------------------------------------------------------------------------------------------------


int Tool::execute( const Args& args ) {
    std::string input_format;


    // User sanity checks
    if ( args.count() == 0 ) {
        Log::error() << "No file specified." << std::endl;
        help( std::cout );
        return failed();
    }
    if ( args.count() > 1 ) {
        Log::error() << "Only one file can be specified." << std::endl;
        help( std::cout );
        return failed();
    }
    std::string input{args( 0 )};
    if ( input.find( "http" ) != 0 ) {

        eckit::PathName file( input+"/nod2d.out" );

        if ( file.exists() ) {
            Log::info() << "Autodetected ascii format" << std::endl;
            input_format = "ascii";
        }
        else {
            eckit::PathName file( input );
            if ( !file.exists() ) {
                Log::error() << "File does not exist: " << file << std::endl;
                return failed();
            }
        }
    }

    bool yaml_output = args.getBool( "yaml", false );

    std::string name       = args.getString( "name", "unnamed" );
    std::string outputfile = args.getString( "output", name + ".atlas" );

    args.get("input-format", input_format);
    if ( input_format.empty() ) {
        std::string extension = input.substr( input.find_last_of( '.' ) );
        if ( extension == ".nc" ) {
            input_format = "netcdf";
        }
        else if ( extension == ".atlas" ) {
            input_format = "atlas-io";
        }
        else {
            Log::warning() << "Could not determine input-format from extension " << extension << std::endl;
            return failed();
        }
    }

    if ( yaml_output ) {
        Log::info().reset();
    }

    FesomData data;

    if ( input_format.find( "netcdf" ) == 0 ) {
        NetCDFReader{args}.read( input, data );
    }
    else if ( input_format == "atlas-io" ) {
        AtlasIOReader{util::NoConfig()}.read( input, data );
    }
    else if ( input_format == "ascii" ) {
        AsciiReader{args}.read( input, data );
    }
    else {
        Log::warning() << "Unknown input_format \"" << input_format << "\"" << std::endl;
        Log::warning() << "Supported: atlas-io" << std::endl;
        return failed();
    }

    std::string uid = data.computeUid( args );

    if ( not yaml_output ) {
        Log::info() << "nb_nodes       : " << data.nb_nodes << std::endl;
        Log::info() << "nb_cells       : " << data.nb_cells << std::endl;
        Log::info() << "uid            : " << uid << std::endl;
    }

    if ( not yaml_output ) {
        ATLAS_TRACE( "Write data file" );
        auto length = data.write( outputfile, args );
        Log::info() << "Written " << eckit::Bytes( length ) << " to file " << outputfile << std::endl;
    }

    if ( yaml_output ) {
        std::ostream& out = std::cout;
        out << name << ": &" << name << std::endl;
        out << "    type: FESOM" << std::endl;
        out << "    name: " << name << std::endl;
        out << "    nb_nodes: " << data.nb_nodes << std::endl;
        out << "    nb_cells: " << data.nb_cells << std::endl;
        out << "    uid: " << uid << std::endl;
        out << "    data: {{location}}/" << outputfile << std::endl;
        out << uid << ": *" << name << std::endl;
    }

    return success();
}

}  // namespace fesom
}  // namespace atlas

//------------------------------------------------------------------------------------------------------

int main( int argc, char** argv ) {
    atlas::fesom::Tool tool( argc, argv );
    return tool.start();
}
