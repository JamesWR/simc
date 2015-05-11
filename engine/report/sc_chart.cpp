// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_report.hpp"
#include "sc_highchart.hpp"

#include <cmath>
#include <clocale>

using namespace js;

namespace { // anonymous namespace ==========================================

const std::string amp = "&amp;";

struct compare_downtime
{
  bool operator()( const player_t* l, const player_t* r ) const
  {
    return l -> collected_data.waiting_time.mean() < r -> collected_data.waiting_time.mean();
  }
};

struct compare_apet
{
  bool operator()( const stats_t* l, const stats_t* r ) const
  {
    return l -> apet < r -> apet;
  }
};

struct compare_amount
{
  bool operator()( const stats_t* l, const stats_t* r ) const
  {
    return l -> actual_amount.mean() > r -> actual_amount.mean();
  }
};

struct compare_stats_time
{
  bool operator()( const stats_t* l, const stats_t* r ) const
  {
    return l -> total_time > r -> total_time;
  }
};

struct compare_gain
{
  bool operator()( const gain_t* l, const gain_t* r ) const
  {
    return l -> actual > r -> actual;
  }
};

struct filter_non_performing_players
{
  std::string type;
  filter_non_performing_players( std::string type_ ) : type( type_ ) {}
  bool operator()( player_t* p ) const
  {
    if ( type == "dps" && p -> collected_data.dps.mean() <= 0 )
      return true;
    else if ( type == "prioritydps" && p -> collected_data.prioritydps.mean() <= 0  )
      return true;
    else if ( type == "hps" && p -> collected_data.hps.mean() <= 0 )
      return true;
    else if ( type == "dtps" && p -> collected_data.dtps.mean() <= 0 )
      return true;
    else if ( type == "tmi" && p -> collected_data.theck_meloree_index.mean() <= 0 )
      return true;
    else if ( type == "apm" && p -> collected_data.executed_foreground_actions.mean() <= 0 )
      return true;

    return false;
  }
};

struct filter_stats_dpet
{
  bool player_is_healer;
  filter_stats_dpet( const player_t& p ) : player_is_healer( p.primary_role() == ROLE_HEAL ) {}
  bool operator()( const stats_t* st ) const
  {
    if ( st->quiet ) return true;
    if ( st->apet <= 0 ) return true;
    if ( st -> num_refreshes.mean() > 4 * st -> num_executes.mean() ) return true;
    if ( player_is_healer != ( st->type != STATS_DMG ) ) return true;

    return false;
  }
};

struct filter_waiting_stats
{
  bool operator()( const stats_t* st ) const
  {
    if ( st -> quiet ) return true;
    if ( st -> total_time <= timespan_t::zero() ) return true;
    if ( st -> background ) return true;

    return false;
  }
};

struct filter_waiting_time
{
  bool operator()( const player_t* p ) const
  {
    if ( ( p -> collected_data.waiting_time.mean() / p -> collected_data.fight_length.mean() ) > 0.01 )
    {
      return false;
    }
    return true;
  }
};

} // anonymous namespace ====================================================

bool chart::generate_raid_downtime( highchart::bar_chart_t& bc, sim_t* sim )
{
  std::vector<const player_t*> players;
  for ( size_t i = 0; i < sim -> players_by_name.size(); ++i )
  {
    const player_t* p = sim -> players_by_name[ i ];
    if ( ( p -> collected_data.waiting_time.mean() / p -> collected_data.fight_length.mean() ) > 0.01 )
    {
      players.push_back( p );
    }
  }

  if ( players.empty() )
  {
    return false;
  }

  range::sort( players, compare_downtime() );

  for ( size_t i = 0; i < players.size(); ++i )
  {
    const player_t* p = players[ i ];
    const color::rgb& c = color::class_color( p -> type );
    double waiting_pct = ( 100.0 * p -> collected_data.waiting_time.mean() / p -> collected_data.fight_length.mean() );
    sc_js_t e;
    e.set( "name", report::decorate_html_string( p -> name_str, c ) );
    e.set( "color", c.str() );
    e.set( "y", waiting_pct );

    bc.add( "series.0.data", e );
  }

  bc.set( "series.0.name", "Downtime" );
  bc.set( "series.0.tooltip.pointFormat", "<span style=\"color:{point.color}\">\u25CF</span> {series.name}: <b>{point.y}</b>%<br/>" );

  bc.set_title( "Raid downtime" );
  bc.set_yaxis_title( "Downtime% (of total fight length)" );
  bc.height_ = 92 + players.size() * 20;

  return true;
}
// ==========================================================================
// Chart
// ==========================================================================

bool chart::generate_raid_gear( highchart::bar_chart_t& bc, sim_t* sim )
{
  if ( sim -> players_by_name.size() == 0 )
  {
    return false;
  }

  std::vector<double> data_points[ STAT_MAX ];
  std::vector<bool> has_stat( STAT_MAX );
  size_t n_stats = 0;
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    data_points[ i ].reserve( sim -> players_by_name.size() + 1 );
    for ( size_t j = 0; j < sim -> players_by_name.size(); j++ )
    {
      player_t* p = sim -> players_by_name[ j ];

      if ( p -> gear.get_stat( i ) + p -> enchant.get_stat( i ) > 0 )
      {
        has_stat[ i ] = true;
      }

      data_points[ i ].push_back( ( p -> gear.   get_stat( i ) +
                                    p -> enchant.get_stat( i ) ) * gear_stats_t::stat_mod( i ) );
      bc.add( "xAxis.categories", report::decorate_html_string( p -> name_str, color::class_color( p -> type ) ) );
    }

    if ( has_stat[ i ] )
    {
      n_stats++;
    }
  }

  for ( size_t i = 0; i < has_stat.size(); ++i )
  {
    if ( has_stat[ i ] )
    {
      stat_e stat = static_cast<stat_e>( i );
      bc.add( "colors", color::stat_color( stat ).str() );
    }
  }

  size_t series_idx = 0;
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    if ( ! has_stat[ i ] )
    {
      continue;
    }

    std::string series_str = "series." + util::to_string( series_idx );
    bc.add( series_str + ".name", util::stat_type_abbrev( i ) );
    for ( size_t j = 0; j < data_points[ i ].size(); j++ )
    {
      bc.add( series_str + ".data", data_points[ i ][ j ] );
    }

    series_idx++;
  }

  bc.height_ = 95 + sim -> players_by_name.size() * 24 + 55;

  bc.set_title( "Raid Gear" );
  bc.set_yaxis_title( "Cumulative stat amount" );
  bc.set( "legend.enabled", true );
  bc.set( "legend.layout", "horizontal" );
  bc.set( "plotOptions.series.stacking", "normal" );
  bc.set( "tooltip.pointFormat", "<b>{series.name}</b>: {point.y:.0f}" );

  return true;
}

bool chart::generate_reforge_plot( highchart::chart_t& ac, const player_t* p )
{
  const std::vector< std::vector<plot_data_t> >& pd = p -> reforge_plot_data;
  if ( pd.empty() )
  {
    return false;
  }

  double max_dps = 0, min_dps = std::numeric_limits<double>::max(), baseline = 0;
  size_t num_stats = pd[ 0 ].size() - 1;

  for ( size_t i = 0; i < pd.size(); i++ )
  {
    assert( num_stats < pd[ i ].size() );
    if ( pd[ i ][ num_stats ].value < min_dps )
      min_dps = pd[ i ][ num_stats ].value;
    if ( pd[ i ][ num_stats ].value > max_dps )
      max_dps = pd[ i ][ num_stats ].value;

    if ( pd[ i ][ 0 ].value == 0 && pd[ i ][ 1 ].value == 0 )
    {
      baseline = pd[ i ][ num_stats ].value;
    }
  }

  double yrange = std::max( std::fabs( baseline - min_dps ), std::fabs( max_dps - baseline ) );

  ac.set_title( p -> name_str + " Reforge Plot" );
  ac.set_yaxis_title( "Damage Per Second" );
  std::string from_stat, to_stat, from_color, to_color;
  from_stat = util::stat_type_abbrev( p -> sim -> reforge_plot -> reforge_plot_stat_indices[ 0 ] );
  to_stat = util::stat_type_abbrev( p -> sim -> reforge_plot -> reforge_plot_stat_indices[ 1 ] );
  from_color = color::stat_color( p -> sim -> reforge_plot -> reforge_plot_stat_indices[ 0 ] );
  to_color = color::stat_color( p -> sim -> reforge_plot -> reforge_plot_stat_indices[ 1 ] );

  std::string span_from_stat = "<span style=\"color:" + from_color + ";font-weight:bold;\">" + from_stat + "</span>";
  std::string span_from_stat_abbrev = "<span style=\"color:" + from_color + ";font-weight:bold;\">" + from_stat.substr( 0, 2 ) + "</span>";
  std::string span_to_stat = "<span style=\"color:" + to_color + ";font-weight:bold;\">" + to_stat + "</span>";
  std::string span_to_stat_abbrev = "<span style=\"color:" + to_color + ";font-weight:bold;\">" + to_stat.substr( 0, 2 ) + "</span>";

  ac.set( "yAxis.min", baseline - yrange );
  ac.set( "yAxis.max", baseline + yrange );
  ac.set( "yaxis.minPadding", 0.01 );
  ac.set( "yaxis.maxPadding", 0.01 );
  ac.set( "xAxis.labels.overflow", "false" );
  ac.set_xaxis_title( "Reforging between " + span_from_stat + " and " + span_to_stat );

  std::string formatter_function = "function() {";
  formatter_function += "if (this.value == 0) { return 'Baseline'; } ";
  formatter_function += "else if (this.value < 0) { return Math.abs(this.value) + '<br/>" + span_from_stat_abbrev + " to " + span_to_stat_abbrev + "'; } ";
  formatter_function += "else { return Math.abs(this.value) + '<br/>" + span_to_stat_abbrev + " to " + span_from_stat_abbrev + "'; } ";
  formatter_function += "}";

  ac.set( "xAxis.labels.formatter", formatter_function );
  ac.value( "xAxis.labels.formatter" ).SetRawOutput( true );

  ac.add_yplotline( baseline, "baseline", 1.25, "#FF8866" );

  std::vector<std::pair<double, double> > mean;
  std::vector<highchart::data_triple_t> range;

  for ( size_t i = 0; i < pd.size(); i++ )
  {
    double x = pd[ i ][ 0 ].value;
    double v = pd[ i ][ 2 ].value;
    double e = pd[ i ][ 2 ].error / 2;

    mean.push_back( std::make_pair( x, v ) );
    range.push_back( highchart::data_triple_t( x, v + e, v - e ) );
  }

  ac.add_simple_series( "line", from_color, "Mean", mean );
  ac.add_simple_series( "arearange", to_color, "Range", range );

  ac.set( "series.0.zIndex", 1 );
  ac.set( "series.0.marker.radius", 0 );
  ac.set( "series.0.lineWidth", 1.5 );
  ac.set( "series.1.fillOpacity", 0.5 );
  ac.set( "series.1.lineWidth", 0 );
  ac.set( "series.1.linkedTo", ":previous" );
  ac.height_ = 500;

  return true;
}

// chart::distribution_dps ==================================================

bool chart::generate_distribution( highchart::histogram_chart_t& hc,
                                  const player_t* p,
                                  const std::vector<size_t>& dist_data,
                                  const std::string& distribution_name,
                                  double avg, double min, double max )
{
  int max_buckets = ( int ) dist_data.size();

  if ( ! max_buckets )
    return false;

  hc.set( "plotOptions.column.color", color::YELLOW.str() );
  hc.set_title( distribution_name + " Distribution" );
  if ( p )
    hc.set_toggle_id( "player" + util::to_string( p -> index ) + "toggle" );

  hc.set( "xAxis.tickInterval", 25 );
  hc.set( "xAxis.tickAtEnd", true );
  hc.set( "yAxis.title.text", "# Iterations" );
  hc.set( "tooltip.headerFormat", "Values: <b>{point.key}</b><br/>" );

  double step = ( max - min ) / dist_data.size();

  std::vector<int> tick_indices;

  for ( int i = 0; i < max_buckets; i++ )
  {
    double begin = min + i * step;
    double end = min + ( i + 1 ) * step;

    sc_js_t e;

    e.set( "y", static_cast<double>( dist_data[ i ] ) );
    if ( i == 0 )
    {
      tick_indices.push_back( i );
      e.set( "name", "min=" + util::to_string( static_cast<unsigned>( min ) ) );
      e.set( "color", color::YELLOW.dark().str() );
    }
    else if ( avg >= begin && avg <= end )
    {
      tick_indices.push_back( i );
      e.set( "name", "mean=" + util::to_string( static_cast<unsigned>( avg ) ) );
      e.set( "color", color::YELLOW.dark().str() );
    }
    else if ( i == max_buckets - 1 )
    {
      tick_indices.push_back( i );
      e.set( "name", "max=" + util::to_string( static_cast<unsigned>( max ) ) );
      e.set( "color", color::YELLOW.dark().str() );
    }
    else
    {
      e.set( "name", util::to_string( util::round( begin, 0 ) ) + " to " + util::to_string( util::round( end, 0 ) ) );
    }

    hc.add( "series.0.data", e );
  }

  hc.set( "series.0.name", "Iterations" );

  for ( size_t i = 0; i < tick_indices.size(); i++ )
    hc.add( "xAxis.tickPositions", tick_indices[ i ] );

  return true;
}

bool chart::generate_gains( highchart::pie_chart_t& pc, const player_t* p, resource_e type )
{
  std::string resource_name = util::inverse_tokenize( util::resource_type_string( type ) );

  // Build gains List
  std::vector<gain_t*> gains_list;
  for ( size_t i = 0; i < p -> gain_list.size(); ++i )
  {
    gain_t* g = p -> gain_list[ i ];
    if ( g -> actual[ type ] <= 0 ) continue;
    gains_list.push_back( g );
  }
  range::sort( gains_list, compare_gain() );

  if ( gains_list.empty() )
  {
    return false;
  }

  pc.set_title( p -> name_str + " " + resource_name + " Gains" );
  pc.set( "plotOptions.pie.dataLabels.format", "<b>{point.name}</b>: {point.y:.1f}" );
  pc.set_toggle_id( "player" + util::to_string( p -> index ) + "toggle" );

  for ( size_t i = 0; i < gains_list.size(); ++i )
  {
    const gain_t* gain = gains_list[ i ];

    sc_js_t e;
    e.set( "color", color::resource_color( type ).str() );
    e.set( "y", gain -> actual[ type ] );
    e.set( "name", gain -> name_str );

    pc.add( "series.0.data", e );
  }

  return true;
}

bool chart::generate_spent_time( highchart::pie_chart_t& pc, const player_t* p )
{
  pc.set_title( p -> name_str + " Spent Time" );
  pc.set( "plotOptions.pie.dataLabels.format", "<b>{point.name}</b>: {point.y:.1f}s" );
  pc.set_toggle_id( "player" + util::to_string( p -> index ) + "toggle" );

  std::vector<stats_t*> filtered_waiting_stats;

  // Filter stats we do not want in the chart ( quiet, background, zero total_time ) and copy them to filtered_waiting_stats
  range::remove_copy_if( p -> stats_list, back_inserter( filtered_waiting_stats ), filter_waiting_stats() );

  size_t num_stats = filtered_waiting_stats.size();
  if ( num_stats == 0 && p -> collected_data.waiting_time.mean() == 0 )
    return false;

  range::sort( filtered_waiting_stats, compare_stats_time() );

  // Build Data
   if ( ! filtered_waiting_stats.empty() )
   {
     for ( size_t i = 0; i < filtered_waiting_stats.size(); ++i )
     {
       const stats_t* stats = filtered_waiting_stats[ i ];
       std::string color = color::school_color( stats -> school );
       if ( color.empty() )
       {
         p -> sim -> errorf( "chart::generate_stats_sources assertion error! School color unknown, stats %s from %s. School %s\n",
                             stats -> name_str.c_str(), p -> name(), util::school_type_string( stats -> school ) );
         assert( 0 );
       }

       sc_js_t e;
       e.set( "color", color );
       e.set( "y", stats -> total_time.total_seconds() );
       e.set( "name", stats -> name_str );

       pc.add( "series.0.data", e );
     }
   }

   if ( p -> collected_data.waiting_time.mean() > 0 )
   {
     sc_js_t e;
     e.set( "color", color::WHITE.str() );
     e.set( "y", p -> collected_data.waiting_time.mean() );
     e.set( "name", "waiting_time" );
     pc.add( "series.0.data", e );
   }

  return true;
}

bool chart::generate_stats_sources( highchart::pie_chart_t& pc, const player_t* p, const std::string title, const std::vector<stats_t*>& stats_list )
{
  if ( stats_list.empty() )
  {
    return false;
  }

  pc.set_title( title );
  pc.set( "plotOptions.pie.dataLabels.format", "<b>{point.name}</b>: {point.percentage:.1f}%" );
  pc.set_toggle_id( "player" + util::to_string( p -> index ) + "toggle" );

  for ( size_t i = 0; i < stats_list.size(); ++i )
  {
    const stats_t* stats = stats_list[ i ];
    const color::rgb& c = color::school_color( stats -> school );

    sc_js_t e;
    e.set( "color", c.str() );
    e.set( "y", util::round( 100.0 * stats -> portion_amount, 1 ) );
    e.set( "name", report::decorate_html_string( stats -> name_str, c ) );

    pc.add( "series.0.data", e );
  }

  return true;
}

bool chart::generate_damage_stats_sources( highchart::pie_chart_t& chart, const player_t* p )
{
  std::vector<stats_t*> stats_list;

   for ( size_t i = 0; i < p -> stats_list.size(); ++i )
   {
     stats_t* st = p -> stats_list[ i ];
     if ( st -> quiet ) continue;
     if ( st -> actual_amount.mean() <= 0 ) continue;
     if ( st -> type != STATS_DMG ) continue;
     stats_list.push_back( st );
   }

   for ( size_t i = 0; i < p -> pet_list.size(); ++i )
   {
     pet_t* pet = p -> pet_list[ i ];
     for ( size_t j = 0; j < pet -> stats_list.size(); ++j )
     {
       stats_t* st = pet -> stats_list[ j ];
       if ( st -> quiet ) continue;
       if ( st -> actual_amount.mean() <= 0 ) continue;
       if ( st -> type != STATS_DMG ) continue;
       stats_list.push_back( st );
     }
   }

  range::sort( stats_list, compare_amount() );

  if ( stats_list.size() == 0 )
    return false;

  generate_stats_sources( chart, p, p -> name_str + " Damage Sources", stats_list );
  chart.set( "series.0.name", "Damage" );
  chart.set( "plotOptions.pie.tooltip.pointFormat", "<span style=\"color:{point.color}\">\u25CF</span> {series.name}: <b>{point.y}</b>%<br/>" );
  return true;
}

bool chart::generate_heal_stats_sources( highchart::pie_chart_t& chart, const player_t* p )
{
  std::vector<stats_t*> stats_list;

  for ( size_t i = 0; i < p -> stats_list.size(); ++i )
  {
    stats_t* st = p -> stats_list[ i ];
    if ( st -> quiet ) continue;
    if ( st -> actual_amount.mean() <= 0 ) continue;
    if ( st -> type == STATS_DMG ) continue;
    stats_list.push_back( st );
  }

  for ( size_t i = 0; i < p -> pet_list.size(); ++i )
  {
    pet_t* pet = p -> pet_list[ i ];
    for ( size_t j = 0; j < pet -> stats_list.size(); ++j )
    {
      stats_t* st = pet -> stats_list[ j ];
      if ( st -> quiet ) continue;
      if ( st -> actual_amount.mean() <= 0 ) continue;
      if ( st -> type == STATS_DMG ) continue;
      stats_list.push_back( st );
    }
  }

  if ( stats_list.size() == 0 )
    return false;

  range::sort( stats_list, compare_amount() );

  generate_stats_sources( chart, p, p -> name_str + " Healing Sources", stats_list );

  return true;
}

bool chart::generate_raid_aps( highchart::bar_chart_t& bc,
                                                sim_t* s,
                                    const std::string& type )
{
  // Prepare list, based on the selected metric
  std::vector<player_t*> player_list;
  std::string long_type;

  if ( util::str_compare_ci( type, "dps" ) )
  {
    long_type = "Damage per Second";
    range::remove_copy_if( s -> players_by_dps, back_inserter( player_list ), filter_non_performing_players( type ) );
  }
  else  if ( util::str_compare_ci( type, "prioritydps" ) )
  {
    long_type = "Priority Target/Boss Damage ";
    range::remove_copy_if( s -> players_by_priority_dps, back_inserter( player_list ), filter_non_performing_players( type ) );
  }
  else if ( util::str_compare_ci( type, "hps" ) )
  {
    long_type = "Heal & Absorb per Second";
    range::remove_copy_if( s -> players_by_hps, back_inserter( player_list ), filter_non_performing_players( type ) );
  }
  else if ( util::str_compare_ci( type, "dtps" ) )
  {
    long_type = "Damage Taken per Second";
    range::remove_copy_if( s -> players_by_dtps, back_inserter( player_list ), filter_non_performing_players( type ) );
  }
  else if ( util::str_compare_ci( type, "tmi" ) )
  {
    long_type = "Theck-Meloree Index";
    range::remove_copy_if( s -> players_by_tmi, back_inserter( player_list ), filter_non_performing_players( type ) );
  }
  else if ( util::str_compare_ci( type, "apm" ) )
  {
    long_type = "Actions Per Minute";
    range::remove_copy_if( s -> players_by_apm, back_inserter( player_list ), filter_non_performing_players( type ) );
  }

  // Nothing to visualize
  if ( player_list.size() == 0 )
    return false;

  for ( size_t i = 0; i < player_list.size(); ++i )
  {
    const player_t* p = player_list[ i ];
    const color::rgb& c = color::class_color( p -> type );

    sc_js_t e;
    e.set( "color", c.str() );
    e.set( "name", report::decorate_html_string( p -> name_str, c ) );
    double value = 0;
    if ( util::str_compare_ci( type, "dps" ) )
      value = p -> collected_data.dps.mean();
    else if ( util::str_compare_ci( type, "prioritydps" ) )
      value = p -> collected_data.prioritydps.mean();
    else if ( util::str_compare_ci( type, "hps" ) )
      value = p -> collected_data.hps.mean() + p -> collected_data.aps.mean();
    else if ( util::str_compare_ci( type, "dtps" ) )
      value = p -> collected_data.dtps.mean();
    else if ( util::str_compare_ci( type, "tmi" ) )
      value = p -> collected_data.theck_meloree_index.mean();
    else if ( util::str_compare_ci( type, "apm" ) )
    {
      double fight_length = p -> collected_data.fight_length.mean();
      double foreground_actions = p -> collected_data.executed_foreground_actions.mean();
      if ( fight_length > 0 )
      {
        value = 60 * foreground_actions / fight_length;
      }
    }

    std::string aid = "#player";
    aid += util::to_string( p -> index );
    aid += "toggle";

    e.set( "y", util::round( value, 1 ) );
    e.set( "id", aid );
    bc.add( "series.0.data", e );
  }

  bc.height_ = 95 + player_list.size() * 24;
  bc.set_title( long_type + " Ranking" );
  bc.set( "yAxis.title.text", long_type.c_str() );
  // Make the Y-axis a bit longer, so we can put in all numbers on the right
  // side of the bar charts
  bc.set( "yAxis.maxPadding", 0.15 );
  bc.set( "plotOptions.bar.dataLabels.crop", false );
  bc.set( "plotOptions.bar.dataLabels.overflow", "none" );
  bc.set( "plotOptions.bar.dataLabels.y", -1 );
  bc.set( "plotOptions.bar.dataLabels.enabled", true );
  bc.set( "plotOptions.bar.dataLabels.verticalAlign", "middle" );
  bc.set( "plotOptions.bar.dataLabels.format", "{y:.1f}" );
  bc.set( "tooltip.enabled", false );

  std::string js = "function(event) {";
  js += "var anchor = jQuery(event.point.id);";
  js += "anchor.click();";
  js += "jQuery('html, body').animate({ scrollTop: anchor.offset().top }, 'slow');";
  js += "}";
  bc.set( "plotOptions.bar.events.click", js );
  bc.value( "plotOptions.bar.events.click" ).SetRawOutput( true );

  return true;
}

highchart::bar_chart_t& chart::generate_raid_dpet( highchart::bar_chart_t& bc, sim_t* s )
{
  bc.set_title( "Raid Damage Per Execute Time" );

  // Prepare stats list
  std::vector<stats_t*> stats_list;
  for ( size_t i = 0; i < s -> players_by_dps.size(); i++ )
  {
    player_t* p = s -> players_by_dps[ i ];

    // Copy all stats* from p -> stats_list to stats_list, which satisfy the filter
    range::remove_copy_if( p -> stats_list, back_inserter( stats_list ), filter_stats_dpet( *p ) );
  }
  range::sort( stats_list, compare_apet() );

  generate_apet( bc, stats_list );

  return bc;

}

bool chart::generate_apet( highchart::bar_chart_t& bc, const std::vector<stats_t*>& stats_list )
{
  if ( stats_list.empty() )
  {
    return false;
  }

  size_t num_stats = stats_list.size();
  std::vector<player_t*> players;

  for ( size_t i = 0; i < num_stats; ++i )
  {
    const stats_t* stats = stats_list[ i ];
    if ( stats -> player -> is_pet() || stats -> player -> is_enemy() )
    {
      continue;
    }

    if ( std::find( players.begin(), players.end(), stats -> player ) ==
         players.end() )
    {
      players.push_back( stats -> player );
    }
  }

  bc.height_ = 92 + num_stats * 20;

  for ( size_t i = 0; i < num_stats; ++i )
  {
    const stats_t* stats = stats_list[ i ];
    const color::rgb& c = color::school_color( stats_list[ i ] -> school );

    sc_js_t e;
    e.set( "color", c.str() );
    std::string name_str = report::decorate_html_string( stats -> name_str, c );
    if ( players.size() > 1 )
    {
      name_str += "<br/>(";
      name_str += stats -> player -> name();
      name_str += ")";
    }
    e.set( "name", name_str );
    e.set( "y", util::round( stats -> apet, 1 ) );

    bc.add( "series.0.data", e );
  }

  bc.set( "series.0.name", "Damage per Execute Time" );

  return true;
}

bool chart::generate_action_dpet( highchart::bar_chart_t& bc, const player_t* p )
{
  std::vector<stats_t*> stats_list;

  // Copy all stats* from p -> stats_list to stats_list, which satisfy the filter
  range::remove_copy_if( p -> stats_list, back_inserter( stats_list ), filter_stats_dpet( *p ) );
  range::sort( stats_list, compare_apet() );

  if ( stats_list.size() == 0 )
    return false;

  bc.set_title( p -> name_str + " Damage per Execute Time" );
  bc.set_yaxis_title( "Damage per Execute Time" );
  bc.set_toggle_id( "player" + util::to_string( p -> index ) + "toggle" );

  generate_apet( bc, stats_list );

  return true;
}

bool chart::generate_scaling_plot( highchart::chart_t& ac, const player_t* p, scale_metric_e metric )
{
  double max_dps = 0, min_dps = std::numeric_limits<double>::max();

  for ( size_t i = 0; i < p -> dps_plot_data.size(); ++i )
  {
    const std::vector<plot_data_t>& pd = p -> dps_plot_data[ i ];
    size_t size = pd.size();
    for ( size_t j = 0; j < size; j++ )
    {
      if ( pd[ j ].value > max_dps ) max_dps = pd[ j ].value;
      if ( pd[ j ].value < min_dps ) min_dps = pd[ j ].value;
    }
  }

  if ( max_dps <= 0 )
  {
    return false;
  }

  scaling_metric_data_t scaling_data = p -> scaling_for_metric( metric );

  ac.set_title( scaling_data.name + " Scaling Plot" );
  ac.set_yaxis_title( util::scale_metric_type_string( metric ) );
  ac.set_xaxis_title( "\u0394 Stat" );
  ac.set( "chart.type", "line" );
  ac.set( "legend.enabled", true );
  ac.set( "legend.margin", 5 );
  ac.set( "legend.padding", 0 );
  ac.set( "legend.itemMarginBottom", 5 );

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    const std::vector<plot_data_t>& pd = p -> dps_plot_data[ i ];
    // Odds of metric value being 0 is pretty far, so lets just use that to
    // determine if we need to plot the stat or not
    if ( pd.size() == 0 )
    {
      continue;
    }

    std::string color = color::stat_color( i );

    if ( color.empty() )
    {
      continue;
    }

    std::vector<std::pair<double, double> > data;

    ac.add( "colors", "#" + color );

    for ( size_t j = 0; j < pd.size(); j++ )
    {
      data.push_back( std::pair<double, double>( pd[ j ].plot_step, pd[ j ].value ) );
    }

    ac.add_simple_series( "", "", util::stat_type_abbrev( i ), data );
  }

  return true;
}

// chart::generate_scale_factors ===========================================

bool chart::generate_scale_factors( highchart::bar_chart_t& bc, const player_t* p, scale_metric_e metric )
{
  std::vector<stat_e> scaling_stats;

  for ( size_t i = 0, end = p -> scaling_stats[ metric ].size(); i < end; ++i )
  {
    if ( ! p -> scales_with[ p -> scaling_stats[ metric ][ i ] ] )
    {
      continue;
    }

    scaling_stats.push_back( p -> scaling_stats[ metric ][ i ] );
  }

  if ( scaling_stats.size() == 0 )
  {
    return false;
  }

  scaling_metric_data_t scaling_data = p -> scaling_for_metric( metric );

  bc.set_title( scaling_data.name + " Scale Factors" );
  bc.height_ = 92 + scaling_stats.size() * 24;

  bc.set_yaxis_title( util::scale_metric_type_string( metric ) + std::string( " per point" ) );

  //bc.set( "plotOptions.bar.dataLabels.align", "center" );
  bc.set( "plotOptions.errorbar.stemColor", "#FF0000" );
  bc.set( "plotOptions.errorbar.whiskerColor", "#FF0000" );
  bc.set( "plotOptions.errorbar.whiskerLength", "75%" );
  bc.set( "plotOptions.errorbar.whiskerWidth", 1.5 );

  std::vector<double> data;
  std::vector<std::pair<double, double> > error;
  for ( size_t i = 0; i < scaling_stats.size(); ++i )
  {

    double value = p -> scaling[ metric ].get_stat( scaling_stats[ i ] );
    double error_value = p -> scaling_error[ metric ].get_stat( scaling_stats[ i ] );
    data.push_back( value );
    error.push_back( std::pair<double, double>( value - fabs( error_value ), value + fabs( error_value ) ) );

    std::string category_str = util::stat_type_abbrev( scaling_stats[ i ] );
    category_str += " (" + util::to_string( util::round( value, p -> sim -> report_precision ), p -> sim -> report_precision ) + ")";

    bc.add( "xAxis.categories", category_str );
  }

  bc.add_simple_series( "bar", color::class_color( p -> type ), util::scale_metric_type_abbrev( metric ) + std::string( " per point" ), data );
  bc.add_simple_series( "errorbar", "", "Error", error );


  //bc.set( "series.0.dataLabels.enabled", false );
  //bc.set( "series.0.dataLabels.format", "{point.y:." + util::to_string( p -> sim -> report_precision ) + "f}" );
  //bc.set( "series.0.dataLabels.y", -2 );
  return true;
}

// Generate a "standard" timeline highcharts object as a string based on a stats_t object
highchart::time_series_t& chart::generate_stats_timeline( highchart::time_series_t& ts, const stats_t* s )
{
  sc_timeline_t timeline_aps;
  s -> timeline_amount.build_derivative_timeline( timeline_aps );
  std::string stats_type = util::stats_type_string( s -> type );
  ts.set_toggle_id( "actor" + util::to_string( s -> player -> index ) + "_" + s -> name_str + "_" + stats_type + "_toggle" );

  ts.height_ = 200;
  ts.set( "yAxis.min", 0 );
  if ( s -> type == STATS_DMG )
  {
    ts.set_yaxis_title( "Damage per second" );
    if ( s -> action_list.size() > 0 && s -> action_list[ 0 ] -> s_data -> id() != 0 )
      ts.set_title( std::string( s -> action_list[ 0 ] -> s_data -> name_cstr() ) + " Damage per second" );
    else
      ts.set_title( s -> name_str + " Damage per second" );
  }
  else
    ts.set_yaxis_title( "Healing per second" );

  std::string area_color = color::YELLOW;
  if ( s -> action_list.size() > 0 )
    area_color = color::school_color( s -> action_list[ 0 ] -> school );

  ts.add_simple_series( "area", area_color, s -> type == STATS_DMG ? "DPS" : "HPS", timeline_aps.data() );
  ts.set_mean( s -> portion_aps.mean() );

  return ts;
}

bool chart::generate_actor_dps_series( highchart::time_series_t& ts, const player_t* p )
{
  if ( p -> collected_data.dps.mean() <= 0 )
  {
    return false;
  }

  sc_timeline_t timeline_dps;
  p -> collected_data.timeline_dmg.build_derivative_timeline( timeline_dps );
  ts.set_toggle_id( "player" + util::to_string( p -> index ) + "toggle" );

  ts.set( "yAxis.min", 0 );
  ts.set_yaxis_title( "Damage per second" );
  ts.set_title( p -> name_str + " Damage per second" );
  ts.add_simple_series( "area", color::class_color( p -> type ), "DPS", timeline_dps.data() );
  ts.set_mean( p -> collected_data.dps.mean() );

  return false;
}

highchart::time_series_t& chart::generate_actor_timeline( highchart::time_series_t& ts,
                                                          const player_t*      p,
                                                          const std::string&   attribute,
                                                          const std::string&   series_color,
                                                          const sc_timeline_t& data )
{
  std::string attr_str = util::inverse_tokenize( attribute );
  ts.set_toggle_id( "player" + util::to_string( p -> index ) + "toggle" );
  ts.set_title( p -> name_str + " " + attr_str );
  ts.set_yaxis_title( "Average " + attr_str );
  ts.add_simple_series( "area", series_color, attr_str, data.data() );
  ts.set_xaxis_max( p -> sim -> simulation_length.max() );

  return ts;
}

