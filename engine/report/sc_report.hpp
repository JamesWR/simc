// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SC_REPORT_HPP
#define SC_REPORT_HPP

#include "simulationcraft.hpp"
#include "sc_highchart.hpp"
#include <fstream>

struct player_t;
struct player_processed_report_information_t;
struct sim_report_information_t;

#define MAX_PLAYERS_PER_CHART 20

#define LOOTRANK_ENABLED 1

namespace chart
{
enum chart_e { HORIZONTAL_BAR_STACKED, HORIZONTAL_BAR, VERTICAL_BAR, PIE, LINE, XY_LINE };

std::string stat_color( stat_e type );
std::string resource_color( int type );
std::string raid_downtime ( std::vector<player_t*> &players_by_name, int print_styles = 0 );
size_t raid_aps ( std::vector<std::string>& images, sim_t*, std::vector<player_t*>&, std::string type );
size_t raid_dpet( std::vector<std::string>& images, sim_t* );
size_t raid_gear( std::vector<std::string>& images, sim_t*, int print_styles = 0 );

///std::string timeline           ( player_t*, const std::vector<double>&, const std::string&, double avg = 0, std::string color = "FDD017", size_t max_length = 0 );
std::string scale_factors      ( player_t* );
std::string scaling_dps        ( player_t* );
std::string reforge_dps        ( player_t* );
std::string normal_distribution(  double mean, double std_dev, double confidence, double tolerance_interval = 0, int print_styles = 0  );

#if LOOTRANK_ENABLED == 1
std::string gear_weights_lootrank  ( player_t* );
#endif
std::string gear_weights_wowhead   ( player_t* );
std::string gear_weights_askmrrobot( player_t* );

highchart::histogram_chart_t& generate_distribution( highchart::histogram_chart_t&, const player_t* p,
                                 const std::vector<size_t>& dist_data,
                                 const std::string& distribution_name,
                                 double avg, double min, double max );
highchart::pie_chart_t& generate_gains( highchart::pie_chart_t&, const player_t*, const resource_e );
bool generate_spent_time( highchart::pie_chart_t&, const player_t* );
highchart::pie_chart_t& generate_stats_sources( highchart::pie_chart_t&, const player_t*, const std::string title, const std::vector<stats_t*>& stats_list );
bool generate_damage_stats_sources( highchart::pie_chart_t&, const player_t* );
bool generate_heal_stats_sources( highchart::pie_chart_t&, const player_t* );
bool generate_raid_aps( highchart::bar_chart_t&, sim_t*, const std::string& type );
highchart::bar_chart_t& generate_raid_dpet( highchart::bar_chart_t&, sim_t* );
highchart::bar_chart_t& generate_player_waiting_time( highchart::bar_chart_t&, sim_t* );
bool generate_action_dpet( highchart::bar_chart_t&, const player_t* );
highchart::bar_chart_t& generate_dpet( highchart::bar_chart_t&, sim_t*, const std::vector<stats_t*>& );
highchart::time_series_t& generate_stats_timeline( highchart::time_series_t&, const stats_t* );
highchart::time_series_t& generate_actor_timeline( highchart::time_series_t&,
                                                   const player_t*      p,
                                                   const std::string&   attribute,
                                                   const std::string&   series_color,
                                                   const sc_timeline_t& data );
highchart::time_series_t& generate_actor_dps_series( highchart::time_series_t& series, const player_t* p );

} // end namespace sc_chart

namespace report
{

typedef io::ofstream sc_html_stream;


void generate_player_charts         ( player_t*, player_processed_report_information_t& );
void generate_player_buff_lists     ( player_t*, player_processed_report_information_t& );
void generate_sim_report_information( sim_t*, sim_report_information_t& );

void print_html_sample_data ( report::sc_html_stream&, const sim_t*, const extended_sample_data_t&, const std::string& name, int& td_counter, int columns = 1 );

void print_spell_query ( sim_t*, unsigned level );
void print_profiles    ( sim_t* );
void print_text        ( FILE*, sim_t*, bool detail );
void print_html        ( sim_t* );
void print_html_player ( report::sc_html_stream&, player_t*, int );
void print_xml         ( sim_t* );
void print_suite       ( sim_t* );
void print_csv_data( sim_t* );

#if SC_BETA
static const char* const beta_warnings[] =
{
  "Beta! Beta! Beta! Beta! Beta! Beta!",
  "Not All classes are yet supported.",
  "Some class models still need tweaking.",
  "Some class action lists need tweaking.",
  "Some class BiS gear setups need tweaking.",
  "Some trinkets not yet implemented.",
  "Constructive feedback regarding our output will shorten the Beta phase dramatically.",
  "Beta! Beta! Beta! Beta! Beta! Beta!",
};
#endif // SC_BETA

} // reort

std::string pretty_spell_text( const spell_data_t& default_spell, const std::string& text, const player_t& p );
inline std::string pretty_spell_text( const spell_data_t& default_spell, const char* text, const player_t& p )
{ return text ? pretty_spell_text( default_spell, std::string( text ), p ) : std::string(); }

#endif // SC_REPORT_HPP
