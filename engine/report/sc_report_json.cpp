// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_report.hpp"
#include "interfaces/sc_js.hpp"

namespace
{
void to_json( js::sc_js_t& node, const extended_sample_data_t& sd )
{
  node.set( "name", sd.name_str );
  node.set( "mean", sd.mean() );
}
void to_json( js::sc_js_t& node, const player_collected_data_t& cd )
{
  {
    js::sc_js_t dps;
    to_json( dps, cd.dps );
    node.set( "dps", dps );
  }
}
void to_json( js::sc_js_t& node, const player_t& p )
{
  node.set( "name", p.name() );
  {
    js::sc_js_t cd;
    to_json( cd, p.collected_data );
    node.set( "collected_data", cd );
  }
}

void to_json( js::sc_js_t& node, const sim_t& sim )
{
  for ( size_t i = 0; i < sim.player_list.size(); ++i )
  {
    js::sc_js_t player_node;
    to_json( player_node, *sim.player_list[i] );
    node.add( "players", player_node );
  }
}

void print_json_( io::ofstream& o, const sim_t& sim )
{
  js::sc_js_t root;
  to_json( root, sim );

  o << root.to_json();
}

} // unnamed namespace

namespace report
{

void print_json( sim_t& sim )
{
  if ( sim.json_file_str.empty() )
    return;

  // Setup file stream and open file
  io::ofstream s;
  s.open( sim.json_file_str );
  if ( !s )
  {
    sim.errorf( "Failed to open JSON output file '%s'.", sim.json_file_str.c_str() );
    return;
  }

  // Print JSON report
  try
  {
    print_json_( s, sim );
  } catch ( const std::exception& e )
  {
    sim.errorf( "Failed to print JSON output! %s", e.what() );
  }
}

} // report
