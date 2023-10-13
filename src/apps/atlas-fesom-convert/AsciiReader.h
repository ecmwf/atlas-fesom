/*
 * (C) Copyright 2021- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#pragma once

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "FesomData.h"
#include "atlas-fesom/util/FesomDataFile.h"
#include "atlas/runtime/Exception.h"
#include "eckit/filesystem/PathName.h"
#include "eckit/log/ProgressTimer.h"

namespace atlas {
namespace fesom {

struct ReadNode {
    bool verbose = false;
    int version  = 1;
    int index;
    double lat;
    double lon;
    int dummy;
    friend std::istream& operator>>( std::istream& in, ReadNode& r ) {
        in >> r.index;
        in >> r.lon;
        in >> r.lat;
        return in;
    }
};

struct ReadCell {
    bool verbose = false;
    int version  = 1;
    std::array<int, 3> nodes;
    friend std::istream& operator>>( std::istream& in, ReadCell& r ) {
        in >> r.nodes[0];
        in >> r.nodes[1];
        in >> r.nodes[2];
        return in;
    }
};


class AsciiReader {
private:
    bool verbose_{false};
    int version_{1};

public:
    AsciiReader( const util::Config& config ) {
        config.get( "verbose", verbose_ );
        config.get( "version", version_ );
    }

    void read( const std::string& uri, FesomData& data ) {

        auto trace = atlas::Trace( Here(), "read" );

        std::unique_ptr<atlas::Trace> trace_nodes;
        std::unique_ptr<atlas::Trace> trace_cells;

        data.nb_nodes = 0;
        data.nb_cells = 0;

        FesomDataFile file_nodes{uri+"/nod2d.out"};
        FesomDataFile file_cells{uri+"/elem2d.out"};

        // Read nodes
        {
            trace_nodes = std::make_unique<atlas::Trace>( Here(), "read nodes");

            std::ifstream ifstrm(file_nodes.c_str(), std::ifstream::in);
            if (!ifstrm.is_open()) {
                ATLAS_THROW_EXCEPTION("Could not open file " << file_nodes);
            }

            // Reading nb_nodes
            {
                std::string line;
                std::getline(ifstrm, line);
                std::istringstream iss(line);
                iss >> data.nb_nodes;
            }

            data.lon.resize( data.nb_nodes );
            data.lat.resize( data.nb_nodes );

            int ncol = 0;
            // count columns
            {
                std::string line, word;

                std::ofstream::pos_type p = ifstrm.tellg();
                std::getline(ifstrm, line);
                ifstrm.seekg(p); // rewind to beginning of line

                std::istringstream iss(line);
                while (iss >> word) {
                ++ncol;
                }
            }
            eckit::Channel blackhole;
            eckit::ProgressTimer progress( "Reading file " + file_nodes.path(), data.nb_nodes, " point", double( 1 ),
                                        data.nb_nodes > 5.e6 && verbose_ ? Log::info() : blackhole );

            if (ncol == 3) {
                int dummy;
                for ( std::uint64_t n = 0; n < data.nb_nodes; ++n, ++progress ) {
                    ifstrm >> dummy >> data.lon[n] >> data.lat[n];
                }
            }
            else if (ncol == 4) {
                int dummy;
                for ( std::uint64_t n = 0; n < data.nb_nodes; ++n, ++progress ) {
                    ifstrm >> dummy >> data.lon[n] >> data.lat[n] >> dummy;
                }
            }
            else {
                ATLAS_THROW_EXCEPTION("Unexpected file format (ncol = " << ncol << ")");
            }
        
            trace_nodes->stop();
        }

        // Reading cells
        {
            trace_cells = std::make_unique<atlas::Trace>( Here(), "read cells");

            std::ifstream ifstrm{file_cells.c_str()};

            // Reading nb_cels
            {
                std::string line;
                std::getline( ifstrm, line );
                std::istringstream iss{line};
                iss >> data.nb_cells;
            }

            data.connectivity_cell2node.resize( data.nb_cells*3 );

            eckit::Channel blackhole;
            eckit::ProgressTimer progress( "Reading file " + file_cells.path(), data.nb_cells, " cell", double( 1 ),
                                           data.nb_nodes > 5.e6 && verbose_ ? Log::info() : blackhole );

            std::array<std::uint64_t,3> nodes;
            for ( size_t n = 0; n < data.nb_cells; ++n, ++progress ) {
                ifstrm >> nodes[0] >> nodes[1] >> nodes[2];
                data.connectivity_cell2node[n*3+0] = nodes[0] - 1;
                data.connectivity_cell2node[n*3+1] = nodes[1] - 1;
                data.connectivity_cell2node[n*3+2] = nodes[2] - 1;
            }
            trace_cells->stop();
        }

        std::unique_ptr<atlas::Trace> trace_ensure_outward;
        {
            trace_ensure_outward = std::make_unique<atlas::Trace>(Here(),"Ensure outward normals");
            data.ensureOutwardNormals();
            trace_ensure_outward->stop();
        }

        trace.stop();

        if ( trace_nodes->elapsed() > 1. && verbose_ ) {
            Log::info() << "Reading file " << file_nodes << " took " <<  trace_nodes->elapsed() << " seconds." << std::endl;
        }
        if ( trace_cells->elapsed() > 1. && verbose_ ) {
            Log::info() << "Reading file " << file_cells << " took " <<  trace_cells->elapsed() << " seconds." << std::endl;
        }
        if ( trace_ensure_outward->elapsed() > 1. && verbose_ ) {
            Log::info() << "Ensuring outward normals took " <<  trace_ensure_outward->elapsed() << " seconds." << std::endl;
        }
        if ( trace.elapsed() > 1. && verbose_ ) {
            Log::info() << "Reading files took " << trace.elapsed() << " seconds." << std::endl;
        }
    }
};



}  // namespace Fesom
}  // namespace atlas

//------------------------------------------------------------------------------------------------------
