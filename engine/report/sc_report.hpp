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
struct spell_data_expr_t;

#define MAX_PLAYERS_PER_CHART 20

#define LOOTRANK_ENABLED 0 // The website works, but the link we send out is not usable. If anyone ever fixes it, just set this to 1.

namespace chart
{
enum chart_e { HORIZONTAL_BAR_STACKED, HORIZONTAL_BAR, VERTICAL_BAR, PIE, LINE, XY_LINE };

std::string raid_downtime ( std::vector<player_t*> &players_by_name, int print_styles = 0 );
size_t raid_aps ( std::vector<std::string>& images, sim_t*, std::vector<player_t*>&, std::string type );
size_t raid_dpet( std::vector<std::string>& images, sim_t* );
size_t raid_gear( std::vector<std::string>& images, sim_t*, int print_styles = 0 );

///std::string timeline           ( player_t*, const std::vector<double>&, const std::string&, double avg = 0, std::string color = "FDD017", size_t max_length = 0 );
std::string scale_factors      ( player_t* );
std::string scaling_dps        ( player_t* );
std::string reforge_dps        ( player_t* );
std::string timeline_dps_error       ( player_t* );
std::string action_dpet (player_t* );
std::string aps_portion ( player_t* );
std::string time_spent( player_t* );
std::string gains( player_t*, resource_e );
std::string normal_distribution(  double mean, double std_dev, double confidence, double tolerance_interval = 0, int print_styles = 0  );

bool generate_raid_downtime( highchart::bar_chart_t&, sim_t* );
bool generate_raid_aps( highchart::bar_chart_t&, sim_t*, const std::string& type );
bool generate_distribution( highchart::histogram_chart_t&, const player_t* p,
                                 const std::vector<size_t>& dist_data,
                                 const std::string& distribution_name,
                                 double avg, double min, double max );
highchart::pie_chart_t& generate_gains( highchart::pie_chart_t&, const player_t*, const resource_e );
bool generate_spent_time( highchart::pie_chart_t&, const player_t* );
bool generate_stats_sources( highchart::pie_chart_t&, const player_t*, const std::string title, const std::vector<stats_t*>& stats_list );
bool generate_damage_stats_sources( highchart::pie_chart_t&, const player_t* );
bool generate_heal_stats_sources( highchart::pie_chart_t&, const player_t* );
highchart::bar_chart_t& generate_raid_dpet( highchart::bar_chart_t&, sim_t* );
bool generate_action_dpet( highchart::bar_chart_t&, const player_t* );
bool generate_apet( highchart::bar_chart_t&, const std::vector<stats_t*>& );
highchart::time_series_t& generate_stats_timeline( highchart::time_series_t&, const stats_t* );
highchart::time_series_t& generate_actor_timeline( highchart::time_series_t&,
                                                   const player_t*      p,
                                                   const std::string&   attribute,
                                                   const std::string&   series_color,
                                                   const sc_timeline_t& data );
bool generate_actor_dps_series( highchart::time_series_t& series, const player_t* p );
bool generate_scale_factors( highchart::bar_chart_t& bc, const player_t* p, scale_metric_e metric );
bool generate_scaling_plot( highchart::chart_t& bc, const player_t* p, scale_metric_e metric );
bool generate_reforge_plot( highchart::chart_t& bc, const player_t* p );

} // end namespace sc_chart

namespace color
{
struct rgb
{
  unsigned char r_, g_, b_;

  rgb();

  rgb( unsigned char r, unsigned char g, unsigned char b );
  rgb( double r, double g, double b );
  rgb( const std::string& color );
  rgb( const char* color );

  std::string rgb_str() const;
  std::string str() const;

  rgb& adjust( double v );
  rgb adjust( double v ) const;
  rgb dark( double pct = 0.25 ) const;
  rgb light( double pct = 0.25 ) const;

  rgb& operator=( const std::string& color_str );
  rgb& operator+=( const rgb& other );
  rgb operator+( const rgb& other ) const;
  operator std::string() const;

private:
  bool parse_color( const std::string& color_str );
};

std::ostream& operator<<( std::ostream& s, const rgb& r );

rgb mix( const rgb& c0, const rgb& c1 );

rgb class_color( player_e type );
rgb resource_color( resource_e type );
rgb stat_color( stat_e type );
rgb school_color( school_e school );

// Class colors
const rgb COLOR_DEATH_KNIGHT = "C41F3B";
const rgb COLOR_DRUID        = "FF7D0A";
const rgb COLOR_HUNTER       = "ABD473";
const rgb COLOR_MAGE         = "69CCF0";
const rgb COLOR_MONK         = "00FF96";
const rgb COLOR_PALADIN      = "F58CBA";
const rgb COLOR_PRIEST       = "FFFFFF";
const rgb COLOR_ROGUE        = "FFF569";
const rgb COLOR_SHAMAN       = "0070DE";
const rgb COLOR_WARLOCK      = "9482C9";
const rgb COLOR_WARRIOR      = "C79C6E";

const rgb WHITE              = "FFFFFF";
const rgb GREY               = "333333";
const rgb GREY2              = "666666";
const rgb GREY3              = "8A8A8A";
const rgb YELLOW             = COLOR_ROGUE;
const rgb PURPLE             = "9482C9";
const rgb RED                = COLOR_DEATH_KNIGHT;
const rgb TEAL               = "009090";
const rgb BLACK              = "000000";

// School colors
const rgb COLOR_NONE         = WHITE;
const rgb PHYSICAL           = COLOR_WARRIOR;
const rgb HOLY               = "FFCC00";
const rgb FIRE               = COLOR_DEATH_KNIGHT;
const rgb NATURE             = COLOR_HUNTER;
const rgb FROST              = COLOR_SHAMAN;
const rgb SHADOW             = PURPLE;
const rgb ARCANE             = COLOR_MAGE;
const rgb ELEMENTAL          = COLOR_MONK;
const rgb FROSTFIRE          = "9900CC";
} /* namespace color */

namespace report
{

typedef io::ofstream sc_html_stream;

void generate_player_charts         ( player_t*, player_processed_report_information_t& );
void generate_player_buff_lists     ( player_t*, player_processed_report_information_t& );
void generate_sim_report_information( sim_t*, sim_report_information_t& );

void print_html_sample_data ( report::sc_html_stream&, const sim_t*, const extended_sample_data_t&, const std::string& name, int& td_counter, int columns = 1 );
void print_html_sample_data ( report::sc_html_stream&, const player_t*, const extended_sample_data_t&, const std::string& name, int& td_counter, int columns = 1 );

void print_spell_query ( std::ostream& out, dbc_t& dbc, const spell_data_expr_t&, unsigned level );
void print_spell_query ( xml_node_t* out, FILE* file, dbc_t& dbc, const spell_data_expr_t&, unsigned level );
void print_profiles    ( sim_t* );
void print_text        ( sim_t*, bool detail );
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

std::array<std::string, SCALE_METRIC_MAX> gear_weights_lootrank  ( player_t* );
std::array<std::string, SCALE_METRIC_MAX> gear_weights_wowhead   ( player_t* );
std::array<std::string, SCALE_METRIC_MAX> gear_weights_askmrrobot( player_t* );

std::string decorate_html_string( const std::string& value, const color::rgb& color );

} // reort

std::string pretty_spell_text( const spell_data_t& default_spell, const std::string& text, const player_t& p );
inline std::string pretty_spell_text( const spell_data_t& default_spell, const char* text, const player_t& p )
{ return text ? pretty_spell_text( default_spell, std::string( text ), p ) : std::string(); }

#endif // SC_REPORT_HPP
