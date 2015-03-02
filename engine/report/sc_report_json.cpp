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

js::sc_js_t to_json( const timespan_t& t )
{
  js::sc_js_t node;
  node.set( "seconds", t.total_seconds() );
  std::string formatted_time;
  str::format( formatted_time, "%d:%02d.%03d</td>\n", (int) t.total_minutes(), (int) t.total_seconds() % 60,
               (int) t.total_millis() % 1000 );
  node.set( "formatted", formatted_time );
  return node;
}

js::sc_js_t to_json( const simple_sample_data_t& sd )
{
  js::sc_js_t node;
  node.set( "sum", sd.sum() );
  node.set( "count", sd.count() );
  node.set( "mean", sd.mean() );
  return node;
}

js::sc_js_t to_json( const ::simple_sample_data_with_min_max_t& sd )
{
  js::sc_js_t node;
  node.set( "sum", sd.sum() );
  node.set( "count", sd.count() );
  node.set( "mean", sd.mean() );
  node.set( "min", sd.min() );
  node.set( "max", sd.max() );
  return node;
}

js::sc_js_t to_json( const ::extended_sample_data_t& sd )
{
  js::sc_js_t node;
  node.set( "name", sd.name_str );
  node.set( "sum", sd.sum() );
  node.set( "count", sd.count() );
  node.set( "mean", sd.mean() );
  node.set( "variance", sd.variance );
  node.set( "std_dev", sd.std_dev );
  node.set( "mean_variance", sd.mean_variance );
  node.set( "mean_std_dev", sd.mean_std_dev );
  node.set( "min", sd.min() );
  node.set( "max", sd.max() );
  node.set( "data", sd.data() );
  //node.set( "distribution", sd.distribution );
  return node;
}

js::sc_js_t to_json( const sc_timeline_t& tl )
{
  js::sc_js_t node;
  node.set( "mean", tl.mean() );
  node.set( "mean_std_dev", tl.mean_stddev() );
  node.set( "min", tl.min() );
  node.set( "max", tl.max() );
  node.set( "data", tl.data() );
  return node;
}

js::sc_js_t to_json( const gain_t& g )
{
  js::sc_js_t node;
  node.set( "name", g.name() );
  for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; ++r )
  {
    js::sc_js_t node2;
    node2.set( "resource", util::resource_type_string( r ) );
    node2.set( "actual", g.actual[ r ] );
    node2.set( "overflow", g.overflow[ r ] );
    node2.set( "count", g.count[ r ] );
    node.add( "data", node2 );
  }
  return node;
}

js::sc_js_t to_json( result_e i, const stats_t::stats_results_t& sr )
{
  js::sc_js_t node;
  node.set( "result", util::result_type_string( i ) );
  node.set( "actual_amount", to_json( sr.actual_amount ) );
  node.set( "avg_actual_amount", to_json( sr.avg_actual_amount ) );
  node.set( "total_amount", to_json( sr.total_amount ) );
  node.set( "fight_actual_amount", to_json( sr.fight_actual_amount ) );
  node.set( "fight_total_amount", to_json( sr.fight_total_amount ) );
  node.set( "overkill_pct", to_json( sr.overkill_pct ) );
  node.set( "count", to_json( sr.count ) );
  node.set( "pct", sr.pct );
  return node;
}

js::sc_js_t to_json( const benefit_t& b )
{
  js::sc_js_t node;
  node.set( "name", b.name() );
  node.set( "ration", to_json(b.ratio) );
  return node;
}

js::sc_js_t to_json( const proc_t& p )
{
  js::sc_js_t node;
  node.set( "name", p.name() );
  node.set( "interval_sum", to_json(p.interval_sum) );
  node.set( "count", to_json(p.count) );
  return node;
}

js::sc_js_t to_json( const stats_t& s )
{
  js::sc_js_t node;
  node.set( "name", s.name() );
  node.set( "school", util::school_type_string( s.school ) );
  node.set( "type", util::stats_type_string( s.type ) );
  node.set( "resource_gain", to_json( s.resource_gain ) );
  node.set( "num_executes", to_json( s.num_executes ) );
  node.set( "num_ticks", to_json( s.num_ticks ) );
  node.set( "num_refreshes", to_json( s.num_refreshes ) );
  node.set( "num_executes", to_json( s.num_direct_results ) );
  node.set( "num_tick_results", to_json( s.num_tick_results ) );
  node.set( "total_execute_time", to_json( s.total_execute_time ) );
  node.set( "total_tick_time", to_json( s.total_tick_time ) );
  node.set( "portion_amount", s.portion_amount );
  node.set( "total_intervals", to_json( s.total_intervals ) );
  node.set( "actual_amount", to_json( s.actual_amount ) );
  node.set( "total_amount", to_json( s.total_amount ) );
  node.set( "portion_aps", to_json( s.portion_aps ) );
  node.set( "portion_apse", to_json( s.portion_apse ) );
  for ( result_e i = RESULT_NONE; i < RESULT_MAX; ++i )
  {
    node.add( "direct_results", to_json( i, s.direct_results[i] ) );
    node.add( "direct_results_detail", to_json( i, s.direct_results_detail[i] ) );
    node.add( "tick_results", to_json( i, s.tick_results[i] ) );
    node.add( "tick_results_detail", to_json( i, s.tick_results_detail[i] ) );
  }
  return node;
}

js::sc_js_t to_json( const player_collected_data_t::resource_timeline_t& rtl )
{
  js::sc_js_t node;
  node.set( "resource", util::resource_type_string( rtl.type ) );
  node.set( "timeline", to_json( rtl.timeline ) );
  return node;
}

js::sc_js_t to_json( const player_collected_data_t::stat_timeline_t& stl )
{
  js::sc_js_t node;
  node.set( "stat", util::stat_type_string( stl.type ) );
  node.set( "timeline", to_json( stl.timeline ) );
  return node;
}

js::sc_js_t to_json( const player_collected_data_t::health_changes_timeline_t& hctl )
{
  js::sc_js_t node;
  if ( hctl.collect )
  {
    node.set( "timeline", to_json( hctl.merged_timeline ) );
  }
  return node;
}

js::sc_js_t to_json( const player_collected_data_t::resolve_timeline_t& rtl )
{
  js::sc_js_t node;
  node.set( "merged_timeline", to_json( rtl.merged_timeline ) );
  return node;
}

js::sc_js_t to_json( const player_collected_data_t::buffed_stats_t& bs )
{
  js::sc_js_t node;
  for ( attribute_e a = ATTRIBUTE_NONE; a < ATTRIBUTE_MAX; ++a )
  {
    js::sc_js_t anode;
    anode.set( "attribute", util::attribute_type_string( a ) );
    anode.set( "value", bs.attribute[ a ] );
    node.add( "attributes", anode );
  }
  for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; ++r )
  {
    js::sc_js_t rnode;
    rnode.set( "resource", util::resource_type_string( r ) );
    rnode.set( "value", bs.resource[r] );
    node.add( "resource_gained", rnode );
  }
  node.add( "spell_power", bs.spell_power );
  node.add( "spell_hit", bs.spell_hit );
  node.add( "spell_crit", bs.spell_crit );
  node.add( "manareg_per_second", bs.manareg_per_second );
  node.add( "attack_power", bs.attack_power );
  node.add( "attack_hit", bs.attack_hit );
  node.add( "mh_attack_expertise", bs.mh_attack_expertise );
  node.add( "oh_attack_expertise", bs.oh_attack_expertise );
  node.add( "armor", bs.armor );
  node.add( "miss", bs.miss );
  node.add( "crit", bs.crit );
  node.add( "dodge", bs.dodge );
  node.add( "parry", bs.parry );
  node.add( "block", bs.block );
  node.add( "bonus_armor", bs.bonus_armor );
  node.add( "spell_haste", bs.spell_haste );
  node.add( "spell_speed", bs.spell_speed );
  node.add( "attack_haste", bs.attack_haste );
  node.add( "attack_speed", bs.attack_speed );
  node.add( "mastery_value", bs.mastery_value );
  node.add( "multistrike", bs.multistrike );
  node.add( "readiness", bs.readiness );
  node.add( "damage_versatility", bs.damage_versatility );
  node.add( "heal_versatility", bs.heal_versatility );
  node.add( "mitigation_versatility", bs.mitigation_versatility );
  node.add( "leech", bs.leech );
  node.add( "run_speed", bs.run_speed );
  node.add( "avoidance", bs.avoidance );
  node.add( "leech", bs.leech );
  return node;
}

js::sc_js_t to_json( const player_collected_data_t::action_sequence_data_t& asd )
{
  js::sc_js_t node;
  node.set( "time", to_json( asd.time ) );
  if ( asd.action )
  {
    node.set( "action_name", asd.action->name() );
    node.set( "target_name", asd.target->name() );
  } else
  {
    node.set( "wait_time", to_json( asd.wait_time ) );

  }
  // TODO print resources
  // TODO print buffs

  return node;
}

js::sc_js_t to_json( const player_collected_data_t& cd )
{
  js::sc_js_t node;
  node.set( "fight_length", to_json( cd.fight_length ) );
  node.set( "waiting_time", to_json( cd.waiting_time ) );
  node.set( "executed_foreground_actions", to_json( cd.executed_foreground_actions ) );
  node.set( "dmg", to_json( cd.dmg ) );
  node.set( "compound_dmg", to_json( cd.compound_dmg ) );
  node.set( "prioritydps", to_json( cd.prioritydps ) );
  node.set( "dps", to_json( cd.dps ) );
  node.set( "dpse", to_json( cd.dpse ) );
  node.set( "dtps", to_json( cd.dtps ) );
  node.set( "dmg_taken", to_json( cd.dmg_taken ) );
  node.set( "timeline_dmg", to_json( cd.timeline_dmg ) );
  node.set( "timeline_dmg_taken", to_json( cd.timeline_dmg_taken ) );

  node.set( "heal", to_json( cd.heal ) );
  node.set( "compound_heal", to_json( cd.compound_heal ) );
  node.set( "hps", to_json( cd.hps ) );
  node.set( "hpse", to_json( cd.hpse ) );
  node.set( "htps", to_json( cd.htps ) );
  node.set( "heal_taken", to_json( cd.heal_taken ) );
  node.set( "timeline_healing_taken", to_json( cd.timeline_healing_taken ) );

  node.set( "absorb", to_json( cd.absorb ) );
  node.set( "compound_absorb", to_json( cd.compound_absorb ) );
  node.set( "aps", to_json( cd.aps ) );
  node.set( "atps", to_json( cd.atps ) );
  node.set( "absorb_taken", to_json( cd.absorb_taken ) );

  node.set( "deaths", to_json( cd.deaths ) );
  node.set( "theck_meloree_index", to_json( cd.theck_meloree_index ) );
  node.set( "effective_theck_meloree_index", to_json( cd.effective_theck_meloree_index ) );
  node.set( "max_spike_amount", to_json( cd.max_spike_amount ) );

  node.set( "target_metric", to_json( cd.target_metric ) );
  for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; ++r )
  {
    js::sc_js_t rnode;
    rnode.set( "resource", util::resource_type_string( r ) );
    rnode.set( "data", to_json( cd.resource_lost[r] ) );
    node.add( "resource_lost", rnode );
  }
  for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; ++r )
  {
    js::sc_js_t rnode;
    rnode.set( "resource", util::resource_type_string( r ) );
    rnode.set( "data", to_json( cd.resource_gained[r] ) );
    node.add( "resource_gained", rnode );
  }
  for ( const auto& rtl : cd.resource_timelines )
  {
    node.add( "resource_timelines", to_json( rtl ) );
  }
  // std::vector<simple_sample_data_with_min_max_t > combat_end_resource;
  for ( const auto& stl : cd.stat_timelines )
  {
    node.add( "stat_timelines", to_json( stl ) );
  }
  node.set( "health_changes", to_json( cd.health_changes ) );
  node.set( "health_changes", to_json( cd.health_changes_tmi ) );
  node.set( "resolve_timeline", to_json( cd.resolve_timeline ) );
  for ( const auto& asd : cd.action_sequence )
  {
    node.add( "action_sequence", to_json( *asd ) );
  }
  for ( const auto& asd : cd.action_sequence_precombat )
  {
    node.add( "action_sequence_precombat", to_json( *asd ) );
  }
  node.set( "buffed_stats_snapshot", to_json( cd.buffed_stats_snapshot ) );

  return node;
}

// Forward declaration
js::sc_js_t to_json( const player_t& p );

js::sc_js_t to_json( const pet_t::owner_coefficients_t& oc )
{
  js::sc_js_t node;
  node.set( "armor", oc.armor );
  node.set( "health", oc.health );
  node.set( "ap_from_ap", oc.ap_from_ap );
  node.set( "ap_from_sp", oc.ap_from_sp );
  node.set( "sp_from_ap", oc.sp_from_ap );
  node.set( "sp_from_sp", oc.sp_from_sp );
  return node;
}

js::sc_js_t to_json( const pet_t& p )
{
  js::sc_js_t node;
  node.set( "stamina_per_owner", p.stamina_per_owner );
  node.set( "intellect_per_owner", p.intellect_per_owner );
  node.set( "pet_type", util::pet_type_string( p.pet_type ) );
  node.set( "stamina_per_owner", p.stamina_per_owner );
  node.set( "stamina_per_owner", p.stamina_per_owner );
  node.set( "owner_coefficients", to_json( p.owner_coeff ) );
  node.set( "player_t", to_json( static_cast<const player_t&>( p ) ) );
  return node;
}

js::sc_js_t to_json( const player_t& p )
{
  js::sc_js_t node;
  node.set( "name", p.name() );
  node.set( "race", util::race_type_string( p.race ) );
  node.set( "role", util::role_type_string( p.role ) );
  node.set( "level", p.level );
  node.set( "party", p.party );
  node.set( "ready_type", p.ready_type );
  node.set( "specialization", util::specialization_string( p.specialization() ) );
  node.set( "bugs", p.bugs );
  node.set( "wod_hotfix", p.wod_hotfix );
  node.set( "scale_player", p.scale_player );
  node.set( "death_pct", p.death_pct );
  node.set( "size", p.size );
  node.set( "potion_used", p.potion_used );
  // TODO
  node.set( "collected_data", to_json( p.collected_data ) );
  // TODO
  for ( const auto& pet : p.pet_list )
  {
    node.add( "pets", to_json( *pet ) );
  }
  for ( const auto& stat : p.stats_list )
  {
    node.add( "stats", to_json( *stat ) );
  }
  return node;
}

js::sc_js_t to_json( const sim_t& sim )
{
  js::sc_js_t node;
  node.set( "debug", sim.debug );
  node.set( "max_time", to_json( sim.max_time ) );
  node.set( "expected_iteration_time", to_json( sim.expected_iteration_time ) );
  node.set( "vary_combat_length", sim.vary_combat_length );
  node.set( "iterations", sim.iterations );
  node.set( "target_error", sim.target_error );
  node.set( "queue_lag", to_json( sim.queue_lag ) );
  node.set( "queue_lag_stddev", to_json( sim.queue_lag_stddev ) );
  node.set( "gcd_lag", to_json( sim.gcd_lag ) );
  node.set( "gcd_lag_stddev", to_json( sim.gcd_lag_stddev ) );
  node.set( "channel_lag", to_json( sim.channel_lag ) );
  node.set( "channel_lag_stddev", to_json( sim.channel_lag_stddev ) );
  node.set( "queue_gcd_reduction", to_json( sim.queue_gcd_reduction ) );
  node.set( "strict_gcd_queue", sim.strict_gcd_queue );
  node.set( "confidence", sim.confidence );
  node.set( "confidence_estimator", sim.confidence_estimator );
  node.set( "world_lag", to_json( sim.world_lag ) );
  node.set( "world_lag_stddev", to_json( sim.world_lag_stddev ) );
  node.set( "travel_variance", sim.travel_variance );
  node.set( "default_skill", sim.default_skill );
  node.set( "reaction_time", to_json( sim.reaction_time ) );
  node.set( "regen_periodicity", to_json( sim.regen_periodicity ) );
  node.set( "ignite_sampling_delta", to_json( sim.ignite_sampling_delta ) );
  node.set( "fixed_time", sim.fixed_time );
  node.set( "optimize_expressions", sim.optimize_expressions );
  node.set( "optimal_raid", sim.optimal_raid );
  node.set( "log", sim.log );
  node.set( "debug_each", sim.debug_each );
  // TODO
  node.set( "auto_ready_trigger", sim.auto_ready_trigger );
  node.set( "stat_cache", sim.stat_cache );
  node.set( "max_aoe_enemies", sim.max_aoe_enemies );
  node.set( "show_etmi", sim.show_etmi );
  node.set( "tmi_window_global", sim.tmi_window_global );
  node.set( "tmi_bin_size", sim.tmi_bin_size );
  node.set( "requires_regen_event", sim.requires_regen_event );
  node.set( "enemy_death_pct", sim.enemy_death_pct );
  node.set( "rel_target_level", sim.rel_target_level );
  // TODO
  for ( const auto& player : sim.player_no_pet_list.data() )
  {
    node.add( "players", to_json( *player ) );
  }
  for ( const auto& healing : sim.healing_no_pet_list.data() )
  {
    node.add( "healing_players", to_json( *healing ) );
  }
  for ( const auto& target : sim.target_list.data() )
  {
    node.add( "target", to_json( *target ) );
  }
  return node;
}
#else
js::sc_js_t to_json( const sim_t& )
{
  js::sc_js_t node;
  node.set( "info", "not available without c++11" );
  return node;
}
#endif

js::sc_js_t get_root( const sim_t& sim )
{
  js::sc_js_t root;
  root.set( "version", SC_VERSION );
  root.set( "ptr_enabled", SC_USE_PTR );
  root.set( "beta_enabled", SC_BETA );
  root.set( "build_date", __DATE__ );
  root.set( "build_time", __TIME__ );
  root.set( "sim", to_json( sim ) );
  return root;
}

void print_json_raw( FILE* o, const sim_t& sim )
{
  js::sc_js_t root = get_root( sim );
  std::array<char, 1024> buffer;
  rapidjson::FileWriteStream b( o, buffer.data(), buffer.size() );
  rapidjson::Writer<rapidjson::FileWriteStream> writer( b );
  root.js_.Accept( writer );
}

void print_json_pretty( FILE* o, const sim_t& sim )
{
  js::sc_js_t root = get_root( sim );
  std::array<char, 1024> buffer;
  rapidjson::FileWriteStream b( o, buffer.data(), buffer.size() );
  rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer( b );
  root.js_.Accept( writer );
}

std::string print_json_string_raw( const sim_t& sim )
{
  js::sc_js_t root = get_root( sim );
  rapidjson::StringBuffer b;
  rapidjson::Writer<rapidjson::StringBuffer> writer( b );
  root.js_.Accept( writer );
  return b.GetString();
}

std::string print_json_string_pretty( const sim_t& sim )
{
  js::sc_js_t root = get_root( sim );
  rapidjson::StringBuffer b;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer( b );
  root.js_.Accept( writer );
  return b.GetString();
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
    print_json_pretty( s, sim );
  } catch ( const std::exception& e )
  {
    sim.errorf( "Failed to print JSON output! %s", e.what() );
  }
}

} // report
