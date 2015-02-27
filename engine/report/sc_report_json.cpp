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
#if __cplusplus >= 201103L
// Forward declarations
  js::sc_js_t to_json(const player_t& p);



  js::sc_js_t to_json(const simple_sample_data_t& sd)
  {
    js::sc_js_t node;
    node.set("sum", sd.sum());
    node.set("count", sd.count());
    node.set("mean", sd.mean());
    return node;
  }

  js::sc_js_t to_json(const simple_sample_data_with_min_max_t& sd)
  {
    js::sc_js_t node;
    node.set("sum", sd.sum());
    node.set("count", sd.count());
    node.set("mean", sd.mean());
    node.set("min", sd.min());
    node.set("max", sd.max());
    return node;
  }

  js::sc_js_t to_json(const extended_sample_data_t& sd)
  {
    js::sc_js_t node;
    node.set("name", sd.name_str);
    node.set("sum", sd.sum());
    node.set("count", sd.count());
    node.set("mean", sd.mean());
    node.set("variance", sd.variance);
    node.set("std_dev", sd.std_dev);
    node.set("mean_variance", sd.mean_variance);
    node.set("mean_std_dev", sd.mean_std_dev);
    node.set("min", sd.min());
    node.set("max", sd.max());
    node.set("data", sd.data());
    //node.set( "distribution", sd.distribution );
    return node;
  }

  js::sc_js_t to_json(const sc_timeline_t& tl)
  {
    js::sc_js_t node;
    node.set("mean", tl.mean());
    node.set("mean_std_dev", tl.mean_stddev());
    node.set("min", tl.min());
    node.set("max", tl.max());
    node.set("data", tl.data());
    return node;
  }

  js::sc_js_t to_json(const player_collected_data_t::resource_timeline_t& rtl)
  {
    js::sc_js_t node;
    node.set("resource", util::resource_type_string( rtl.type ) );
    node.set("timeline", to_json( rtl.timeline ) );
    return node;
  }

  js::sc_js_t to_json(const player_collected_data_t::stat_timeline_t& stl)
  {
    js::sc_js_t node;
    node.set("stat", util::stat_type_string( stl.type ) );
    node.set("timeline", to_json( stl.timeline ) );
    return node;
  }

  js::sc_js_t to_json(const player_collected_data_t& cd)
  {
    js::sc_js_t node;
    node.set("fight_length", to_json(cd.fight_length));
    node.set("waiting_time", to_json(cd.waiting_time));
    node.set("executed_foreground_actions", to_json(cd.executed_foreground_actions));
    node.set("dmg", to_json(cd.dmg));
    node.set("compound_dmg", to_json(cd.compound_dmg));
    node.set("prioritydps", to_json(cd.prioritydps));
    node.set("dps", to_json(cd.dps));
    node.set("dpse", to_json(cd.dpse));
    node.set("dtps", to_json(cd.dtps));
    node.set("dmg_taken", to_json(cd.dmg_taken));
    node.set("timeline_dmg", to_json(cd.timeline_dmg));
    node.set("timeline_dmg_taken", to_json(cd.timeline_dmg_taken));

    node.set("heal", to_json(cd.heal));
    node.set("compound_heal", to_json(cd.compound_heal));
    node.set("hps", to_json(cd.hps));
    node.set("hpse", to_json(cd.hpse));
    node.set("htps", to_json(cd.htps));
    node.set("heal_taken", to_json(cd.heal_taken));
    node.set("timeline_healing_taken", to_json(cd.timeline_healing_taken));

    node.set("absorb", to_json(cd.absorb));
    node.set("compound_absorb", to_json(cd.compound_absorb));
    node.set("aps", to_json(cd.aps));
    node.set("atps", to_json(cd.atps));
    node.set("absorb_taken", to_json(cd.absorb_taken));

    node.set("deaths", to_json(cd.deaths));
    node.set("theck_meloree_index", to_json(cd.theck_meloree_index));
    node.set("effective_theck_meloree_index", to_json(cd.effective_theck_meloree_index));
    node.set("max_spike_amount", to_json(cd.max_spike_amount));

    node.set("target_metric", to_json(cd.target_metric));
    for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; ++r ) {
      js::sc_js_t rnode;
      rnode.set( "resource", util::resource_type_string( r ) );
      rnode.set( "data", to_json( cd.resource_lost[ r ] ) );
      node.add( "resource_lost", rnode );
    }
    for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; ++r ) {
      js::sc_js_t rnode;
      rnode.set( "resource", util::resource_type_string( r ) );
      rnode.set( "data", to_json( cd.resource_gained[ r ] ) );
      node.add( "resource_gained", rnode );
    }
    for ( const auto& rtl : cd.resource_timelines ) {
      node.add( "resource_timelines", to_json( rtl ) );
    }
    // std::vector<simple_sample_data_with_min_max_t > combat_end_resource;
    for ( const auto& stl : cd.stat_timelines ) {
      node.add( "stat_timelines", to_json( stl ) );
    }
    // health_changes_timeline_t health_changes;     //records all health changes
    // health_changes_timeline_t health_changes_tmi; //records only health changes due to damage and self-healng/self-absorb
    // resolve_timeline_t resolve_timeline;
    //auto_dispose< std::vector<action_sequence_data_t*> > action_sequence;
    //auto_dispose< std::vector<action_sequence_data_t*> > action_sequence_precombat;
    // buffed_stats_t buffed_stats_snapshot



    return node;
  }

  js::sc_js_t to_json(const pet_t& p)
  {
    return to_json(static_cast<const player_t&>(p));
  }

  js::sc_js_t to_json(const player_t& p)
  {
    js::sc_js_t node;
    node.set("name", p.name());
    node.set("collected_data", to_json(p.collected_data));
    for ( const auto& pet : p.pet_list )
    {
      node.add("pets", to_json(*pet));
    }
    return node;
  }

  js::sc_js_t to_json(const sim_t& sim)
  {
    js::sc_js_t node;
    node.set("version", SC_VERSION);
    for ( const auto& player : sim.player_no_pet_list.data() )
    {
      node.add("players", to_json(*player));
    }
    return node;
  }
#else
  js::sc_js_t to_json(const sim_t&)
  {
    js::sc_js_t() node;
    node.set( "info", "not available without c++11" );
    return node;
  }
#endif
  void print_json_(FILE* o, const sim_t& sim)
  {
    js::sc_js_t root = to_json(sim);
    std::array<char, 1024> buffer;
    rapidjson::FileWriteStream b(o, buffer.data(), buffer.size());
    rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(b);

    root.js_.Accept(writer);
  }

} // unnamed namespace

namespace report
{

  void print_json(sim_t& sim)
  {
    if (sim.json_file_str.empty())
      return;

// Setup file stream and open file
    io::cfile s(sim.json_file_str, "w");
    if (!s)
    {
      sim.errorf("Failed to open JSON output file '%s'.", sim.json_file_str.c_str());
      return;
    }

// Print JSON report
    try
    {
      print_json_(s, sim);
    } catch (const std::exception& e)
    {
      sim.errorf("Failed to print JSON output! %s", e.what());
    }
  }

} // report
