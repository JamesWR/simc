// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_report.hpp"
#include "interfaces/sc_js.hpp"
#include "util/rapidjson/filewritestream.h"

namespace
{
// Forward declarations
js::sc_js_t to_json( const player_t& p );

js::sc_js_t to_json( const extended_sample_data_t& sd )
{
  js::sc_js_t node;
  node.set( "name", sd.name_str );
  node.set( "mean", sd.mean() );
  node.set( "variance", sd.variance );
  node.set( "std_dev", sd.std_dev );
  node.set( "mean_variance", sd.mean_variance );
  node.set( "mean_std_dev", sd.mean_std_dev );
  node.set( "data", sd.data() );
  //node.set( "distribution", sd.distribution );
  return node;
}

js::sc_js_t to_json( const player_collected_data_t& cd )
{
  js::sc_js_t node;
  node.set( "dps", to_json( cd.dps ) );
  node.set( "dmg", to_json( cd.dmg ) );
  return node;
}

js::sc_js_t to_json( const pet_t& p )
{
  return to_json( static_cast<const player_t&>( p ) );
}

js::sc_js_t to_json( const player_t& p )
{
  js::sc_js_t node;
  node.set( "name", p.name() );
  node.set( "collected_data", to_json( p.collected_data ) );
  for ( size_t i = 0; i < p.pet_list.size(); ++i )
  {
    node.add( "pets", to_json( *p.pet_list[i] ) );
  }
  return node;
}

js::sc_js_t to_json( const sim_t& sim )
{
  js::sc_js_t node;
  node.set( "version", SC_VERSION );
  for ( size_t i = 0; i < sim.player_no_pet_list.size(); ++i )
  {
    node.add( "players", to_json( *sim.player_no_pet_list[i] ) );
  }
  return node;
}

void print_json_( FILE* o, const sim_t& sim )
{
  js::sc_js_t root = to_json( sim );
  std::array<char, 1024> buffer;
  rapidjson::FileWriteStream b( o, buffer.data(), buffer.size() );
  rapidjson::PrettyWriter< rapidjson::FileWriteStream > writer( b );

  root.js_.Accept( writer );
}

} // unnamed namespace

namespace report
{

void print_json( sim_t& sim )
{
  if ( sim.json_file_str.empty() )
    return;

// Setup file stream and open file
  io::cfile s( sim.json_file_str, "w" );
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
