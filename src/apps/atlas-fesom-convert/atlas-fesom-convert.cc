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
#include "eckit/filesystem/LocalPathName.h"
#include "eckit/log/Bytes.h"

#include "atlas/runtime/AtlasTool.h"
#include "atlas/runtime/Exception.h"

#include "NetCDFReader.h"
#include "FesomData.h"
#include "AtlasIOReader.h"
#include "AsciiReader.h"

namespace atlas::fesom {

//----------------------------------------------------------------------------------------------------------------------

struct Tool : public atlas::AtlasTool {
    bool serial() override { return true; }
    int execute( const Args& args ) override;
    std::string briefDescription() override { return "Create atlas-fesom grid data files "; }
    std::string usage() override {
        return name() + " <file-or-dir> --name=NAME [OPTION]... [--help,-h]";
    }
    std::string longDescription() override {
        return "Create binary grid data files \n"
               "\n"
               "       <file-or-dir>: input file";
    }

    Tool( int argc, char** argv ) : AtlasTool( argc, argv ) {
        add_option( new SimpleOption<std::string>( "name", "Output grid name" ) );
        add_option( new SimpleOption<std::string>( "input-format",
                                                   "netcdf, atlas-io, ascii" ) );
        add_option( new SimpleOption<std::string>( "compression",
                                                   "Data compression: none, lz4, aec, ... (see eckit support)'" ) );
        add_option( new SimpleOption<std::string>( "output.data",  "Output data file; default: <name>.atlas" ) );
        add_option( new SimpleOption<std::string>( "output.grids", "Output grid-spec file; default: grids.yaml" ) );
        add_option( new Separator( "Advanced" ) );
        add_option( new SimpleOption<bool>( "verbose", "Print verbose output" ) );
        add_option( new SimpleOption<bool>( "skip-uid", "Skip computation of uid") );
    }
};

//------------------------------------------------------------------------------------------------------

auto is_absolute_path = [] (std::string path) -> bool {
    return path[0] == '/' || path[0] == '~';
};

auto make_absolute_path = [] (std::string reference_path, std::string path) -> std::string {
    eckit::PathName ref_path{reference_path};
    eckit::PathName absolute_path{path};
    if (reference_path.size() && not is_absolute_path(path)) {
        absolute_path = reference_path / absolute_path;
    }
    return absolute_path.fullName().asString();
};

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

    bool verbose = args.getBool("verbose", false);

    eckit::PathName cwd = eckit::LocalPathName::cwd();

    std::string name       = args.getString( "name", "fesom" );
    std::string outputfile = args.getString( "output.data", name + ".atlas" );

    args.get("input-format", input_format);
    if ( input_format.empty() ) {
        std::string extension = input.substr( input.find_last_of( '.' ) );
        if (extension == ".nc") {
            if (verbose) {
                Log::info() << "Autodetected NetCDF format" << std::endl;
            }
            input_format = "netcdf";
        }
        else if (extension == ".atlas") {
            if (verbose) {
                Log::info() << "Autodetected atlas-io format" << std::endl;
            }
            input_format = "atlas-io";
        }
        else {
            Log::warning() << "Could not determine input-format from extension " << extension << std::endl;
            return failed();
        }
    }

    FesomData data;

    if ( input_format.find( "netcdf" ) == 0 ) {
        Log::warning() << "Warning: NetCDF files may have coordinates that are different to original ascii-encoded"
                       << " coordinates, to machine precision, leading to differently computed uid."
                       << std::endl;
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

    std::string uid_N = name + "_N";
    std::string uid_C = name + "_C";
    bool skip_uid = args.getBool("skip-uid", false);
    if ( not skip_uid ) {
        uid_N = data.computeUid( "N" );
        uid_C = data.computeUid( "C" );
    }

    {
        ATLAS_TRACE( "Write data file" );
        std::string outputfile_fullpath = make_absolute_path( cwd, outputfile );
        auto length = data.write( outputfile_fullpath, args );
        Log::info() << "Written " << eckit::Bytes( length ) << " to file " << outputfile_fullpath << std::endl;
    }


    {
        ATLAS_TRACE( "Write gridspec file" );
        std::stringstream out;
        std::string name_N = name + "_N";
        std::string name_C = name + "_C";

        out << name_N << ": &" << name_N << '\n';
        out << "    type: FESOM\n";
        out << "    name: " << name_N << '\n';
        out << "    base_name: " << name << '\n';
        out << "    arrangement: N\n";
        out << "    size: " << data.nb_nodes << '\n';
        if (not skip_uid) {
            out << "    uid: " << uid_N << '\n';
        }
        out << "    data: file://" << outputfile << '\n';
        out << name << ": *" << name_N << '\n';
        if (not skip_uid) {
            out << uid_N << ": *" << name_N << '\n';
        }
        out << '\n';
        out << name_C << ": &" << name_C << '\n';
        out << "    type: FESOM\n";
        out << "    name: " << name_C << '\n';
        out << "    base_name: " << name << '\n';
        out << "    arrangement: C\n";
        out << "    size: " << data.nb_cells << '\n';
        if (not skip_uid) {
            out << "    uid: " << uid_C << '\n';
        }
        out << "    data: file://" << outputfile << '\n';
        if (not skip_uid) {
            out << uid_C << ": *" << name_C << '\n';
        }

        std::string grid_spec_outputfile = args.getString( "output.grids", "atlas-fesom-grids.yaml" );
        std::ofstream ofs( grid_spec_outputfile );
        ofs << out.str();
        ofs.close();
        Log::info() << "Written gridspec to file " << grid_spec_outputfile << std::endl;
    }


    return success();
}

}  // namespace atlas::fesom

//------------------------------------------------------------------------------------------------------

int main( int argc, char** argv ) {
    atlas::fesom::Tool tool( argc, argv );
    return tool.start();
}
