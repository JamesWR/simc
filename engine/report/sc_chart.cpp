// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_report.hpp"
#include "sc_highchart.hpp"

#include <cmath>
#include <clocale>

namespace { // anonymous namespace ==========================================

const std::string amp = "&amp;";

// chart option overview: http://code.google.com/intl/de-DE/apis/chart/image/docs/chart_params.html


enum fill_area_e { FILL_BACKGROUND };
enum fill_e { FILL_SOLID };

std::string chart_type( chart::chart_e t )
{
  switch ( t )
  {
    case chart::HORIZONTAL_BAR_STACKED:
      return std::string( "cht=bhs" ) + amp;
    case chart::HORIZONTAL_BAR:
      return std::string( "cht=bhg" ) + amp;
    case chart::VERTICAL_BAR:
      return std::string( "cht=bvs" ) + amp;
    case chart::PIE:
      return std::string( "cht=p" ) + amp;
    case chart::LINE:
      return std::string( "cht=lc" ) + amp;
    case chart::XY_LINE:
      return std::string( "cht=lxy" ) + amp;
    default:
      assert( false );
      return std::string();
  }
}

std::string chart_size( unsigned width, unsigned height )
{
  std::ostringstream s;
  s << "chs=" << width << "x" << height << amp;
  return s.str();
}

std::string fill_chart( fill_area_e fa, fill_e ft, const std::string& color )
{
  std::string s = "chf=";

  switch ( fa )
  {
    case FILL_BACKGROUND:
      s += "bg";
      break;
    default:
      assert( false );
      break;
  }

  s += ',';

  switch ( ft )
  {
    case FILL_SOLID:
      s += 's';
      break;
    default:
      assert( false );
      break;
  }

  s += ',';
  s += color;

  s += amp;

  return s;
}

std::string chart_title( const std::string& t )
{
  std::string tmp = t;
  util::urlencode( tmp );
  util::replace_all( tmp, "%20", "+" );
  return "chtt=" + tmp + amp;
}

std::string chart_title_formatting ( const std::string& color, unsigned font_size )
{
  std::ostringstream s;

  s << "chts=" << color << ',' << font_size << amp;

  return s.str();
}

namespace color {
// http://www.wowwiki.com/Class_colors
const std::string light_blue    = "69CCF0";
const std::string pink          = "F58CBA";
const std::string purple        = "9482C9";
const std::string red           = "C41F3B";
const std::string tan           = "C79C6E";
const std::string yellow        = "FFF569";
const std::string blue          = "0070DE";
const std::string hunter_green  = "ABD473";
const std::string jade_green    = "00FF96";

// http://www.brobstsystems.com/colors1.htm
const std::string purple_dark   = "7668A1";
const std::string white         = "FFFFFF";
const std::string nearly_white  = "FCFFFF";
const std::string green         = "336600";
const std::string grey          = "C0C0C0";
const std::string olive         = "909000";
const std::string orange        = "FF7D0A";
const std::string teal          = "009090";
const std::string darker_blue   = "59ADCC";
const std::string darker_silver = "8A8A8A";
const std::string darker_yellow = "C0B84F";

/* Creates the average color of two given colors
 */
std::string mix( const std::string& color1, const std::string& color2 )
{
  assert( ( color1.length() == 6 ) && ( color2.length() == 6 ) );

  std::stringstream converter1( color1 );
  unsigned int value;
  converter1 >> std::hex >> value;
  std::stringstream converter2( color2 );
  unsigned int value2;
  converter2 >> std::hex >> value2;

  value += value2;
  value /= 2;
  std::stringstream out;
  out << std::uppercase << std::hex << value;
  return out.str();
}

/* Creates the average of all sequentially given color codes
 */
std::string mix_multiple( const std::string& color )
{
  assert( color.size() % 6 == 0 );

  unsigned i = 0, total_value = 0;
  for ( ; ( i + 1 ) * 6 < color.length(); ++i )
  {
    std::stringstream converter1( color.substr( i * 6, 6 ) );
    unsigned value;
    converter1 >> std::hex >> value;
    total_value += value;
  }
  if ( i ) total_value /= i;

  std::stringstream out;
  out << std::uppercase << std::noskipws << std::hex << total_value;
  return out.str();
}

/* Converts rgb percentage input into a hexadecimal color code
 *
 */
std::string from_pct( double r, double g, double b )
{
  assert( r >= 0 && r <= 1.0 );
  assert( g >= 0 && g <= 1.0 );
  assert( b >= 0 && b <= 1.0 );

  std::string out;
  out += util::uchar_to_hex( static_cast<unsigned char>( r * 255 ) );
  out += util::uchar_to_hex( static_cast<unsigned char>( g * 255 ) );
  out += util::uchar_to_hex( static_cast<unsigned char>( b * 255 ) );

  return out;
}

} // end namespace colour

std::string class_color( player_e type )
{
  switch ( type )
  {
    case PLAYER_NONE:  return color::grey;
    case DEATH_KNIGHT: return color::red;
    case DRUID:        return color::orange;
    case HUNTER:       return color::hunter_green;
    case MAGE:         return color::light_blue;
    case MONK:         return color::jade_green;
    case PALADIN:      return color::pink;
    case PRIEST:       return color::white;
    case ROGUE:        return color::yellow;
    case SHAMAN:       return color::blue;
    case WARLOCK:      return color::purple;
    case WARRIOR:      return color::tan;
    case ENEMY:        return color::grey;
    case ENEMY_ADD:    return color::grey;
    case HEALING_ENEMY:    return color::grey;
    case PLAYER_PET: return color::grey;
    default: assert( 0 ); return std::string();
  }
}

/* The above colors don't all work for text rendered on a light (white) background.
 * These colors work better by reducing the brightness HSV component of the above colors
 */
std::string class_color( player_e type, int print_style )
{
  if ( print_style == 1 )
  {
    switch ( type )
    {
      case MAGE:         return color::darker_blue;
      case PRIEST:       return color::darker_silver;
      case ROGUE:        return color::darker_yellow;
      default: break;
    }
  }
  return class_color( type );
}

const char* get_chart_base_url()
{
  static const char* const base_urls[] =
  {
    "http://0.chart.apis.google.com/chart?",
    "http://1.chart.apis.google.com/chart?",
    "http://2.chart.apis.google.com/chart?",
    "http://3.chart.apis.google.com/chart?",
    "http://4.chart.apis.google.com/chart?",
    "http://5.chart.apis.google.com/chart?",
    "http://6.chart.apis.google.com/chart?",
    "http://7.chart.apis.google.com/chart?",
    "http://8.chart.apis.google.com/chart?",
    "http://9.chart.apis.google.com/chart?"
  };
  static int round_robin;
  static mutex_t rr_mutex;

  auto_lock_t lock( rr_mutex );

  round_robin = ( round_robin + 1 ) % sizeof_array( base_urls );

  return base_urls[ round_robin ];
}

player_e get_player_or_owner_type( player_t* p )
{
  if ( p -> is_pet() )
    p = p -> cast_pet() -> owner;

  return p -> type;
}

/* Blizzard shool colors:
 * http://wowprogramming.com/utils/xmlbrowser/live/AddOns/Blizzard_CombatLog/Blizzard_CombatLog.lua
 * search for: SchoolStringTable
 */
// These colors are picked to sort of line up with classes, but match the "feel" of the spell class' color
std::string school_color( school_e type )
{
  switch ( type )
  {
      // -- Single Schools
      // Doesn't use the same colors as the blizzard ingame UI, as they are ugly
    case SCHOOL_NONE:         return color::white;
    case SCHOOL_PHYSICAL:     return color::tan;
    case SCHOOL_HOLY:         return color::from_pct( 1.0, 0.9, 0.5 );
    case SCHOOL_FIRE:         return color::red;
    case SCHOOL_NATURE:       return color::hunter_green;
    case SCHOOL_FROST:        return color::blue;
    case SCHOOL_SHADOW:       return color::purple;
    case SCHOOL_ARCANE:       return color::light_blue;
      // -- Physical and a Magical
    case SCHOOL_FLAMESTRIKE:  return color::mix( school_color( SCHOOL_PHYSICAL ), school_color( SCHOOL_FIRE ) );
    case SCHOOL_FROSTSTRIKE:  return color::mix( school_color( SCHOOL_PHYSICAL ), school_color( SCHOOL_FROST ) );
    case SCHOOL_SPELLSTRIKE:  return color::mix( school_color( SCHOOL_PHYSICAL ), school_color( SCHOOL_ARCANE ) );
    case SCHOOL_STORMSTRIKE:  return color::mix( school_color( SCHOOL_PHYSICAL ), school_color( SCHOOL_NATURE ) );
    case SCHOOL_SHADOWSTRIKE: return color::mix( school_color( SCHOOL_PHYSICAL ), school_color( SCHOOL_SHADOW ) );
    case SCHOOL_HOLYSTRIKE:   return color::mix( school_color( SCHOOL_PHYSICAL ), school_color( SCHOOL_HOLY ) );
      // -- Two Magical Schools
    case SCHOOL_FROSTFIRE:    return color::mix( school_color( SCHOOL_FROST ), school_color( SCHOOL_FIRE ) );
    case SCHOOL_SPELLFIRE:    return color::mix( school_color( SCHOOL_ARCANE ), school_color( SCHOOL_FIRE ) );
    case SCHOOL_FIRESTORM:    return color::mix( school_color( SCHOOL_FIRE ), school_color( SCHOOL_NATURE ) );
    case SCHOOL_SHADOWFLAME:  return color::mix( school_color( SCHOOL_SHADOW ), school_color( SCHOOL_FIRE ) );
    case SCHOOL_HOLYFIRE:     return color::mix( school_color( SCHOOL_HOLY ), school_color( SCHOOL_FIRE ) );
    case SCHOOL_SPELLFROST:   return color::mix( school_color( SCHOOL_ARCANE ), school_color( SCHOOL_FROST ) );
    case SCHOOL_FROSTSTORM:   return color::mix( school_color( SCHOOL_FROST ), school_color( SCHOOL_NATURE ) );
    case SCHOOL_SHADOWFROST:  return color::mix( school_color( SCHOOL_SHADOW ), school_color( SCHOOL_FROST ) );
    case SCHOOL_HOLYFROST:    return color::mix( school_color( SCHOOL_HOLY ), school_color( SCHOOL_FROST ) );
    case SCHOOL_SPELLSTORM:   return color::mix( school_color( SCHOOL_ARCANE ), school_color( SCHOOL_NATURE ) );
    case SCHOOL_SPELLSHADOW:  return color::mix( school_color( SCHOOL_ARCANE ), school_color( SCHOOL_SHADOW ) );
    case SCHOOL_DIVINE:       return color::mix( school_color( SCHOOL_ARCANE ), school_color( SCHOOL_HOLY ) );
    case SCHOOL_SHADOWSTORM:  return color::mix( school_color( SCHOOL_SHADOW ), school_color( SCHOOL_NATURE ) );
    case SCHOOL_HOLYSTORM:    return color::mix( school_color( SCHOOL_HOLY ), school_color( SCHOOL_NATURE ) );
    case SCHOOL_SHADOWLIGHT:  return color::mix( school_color( SCHOOL_SHADOW ), school_color( SCHOOL_HOLY ) );
      //-- Three or more schools
    case SCHOOL_ELEMENTAL:    return color::mix_multiple( school_color( SCHOOL_FIRE ) +
                                       school_color( SCHOOL_FROST ) +
                                       school_color( SCHOOL_NATURE ) );
    case SCHOOL_CHROMATIC:    return color::mix_multiple( school_color( SCHOOL_FIRE ) +
                                       school_color( SCHOOL_FROST ) +
                                       school_color( SCHOOL_ARCANE ) +
                                       school_color( SCHOOL_NATURE ) +
                                       school_color( SCHOOL_SHADOW ) );
    case SCHOOL_MAGIC:    return color::mix_multiple( school_color( SCHOOL_FIRE ) +
                                   school_color( SCHOOL_FROST ) +
                                   school_color( SCHOOL_ARCANE ) +
                                   school_color( SCHOOL_NATURE ) +
                                   school_color( SCHOOL_SHADOW ) +
                                   school_color( SCHOOL_HOLY ) );
    case SCHOOL_CHAOS:    return color::mix_multiple( school_color( SCHOOL_PHYSICAL ) +
                                   school_color( SCHOOL_FIRE ) +
                                   school_color( SCHOOL_FROST ) +
                                   school_color( SCHOOL_ARCANE ) +
                                   school_color( SCHOOL_NATURE ) +
                                   school_color( SCHOOL_SHADOW ) +
                                   school_color( SCHOOL_HOLY ) );

    default: return std::string();
  }
}

std::string get_color( player_t* p )
{
  player_e type;
  if ( p -> is_pet() )
    type = p -> cast_pet() -> owner -> type;
  else
    type = p -> type;
  return class_color( type, p -> sim -> print_styles );
}

unsigned char simple_encoding( int number )
{
  static const char encoding[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

  number = clamp( number, 0, (int)sizeof_array( encoding ) );

  return encoding[ number ];
}

std::string chart_bg_color( int print_styles )
{
  if ( print_styles == 1 )
  {
    return "666666";
  }
  else
  {
    return "dddddd";
  }
}

// ternary_coords ===========================================================

std::vector<double> ternary_coords( std::vector<plot_data_t> xyz )
{
  std::vector<double> result;
  result.resize( 2 );
  result[0] = xyz[ 2 ].value / 2.0 + xyz[ 1 ].value;
  result[1] = xyz[ 2 ].value / 2.0 * sqrt( 3.0 );
  return result;
}

// color_temperature_gradient ===============================================

std::string color_temperature_gradient( double n, double min, double range )
{
  std::string result = "";
  char buffer[ 10 ] = "";
  int red = ( int ) floor( 255.0 * ( n - min ) / range );
  int blue = 255 - red;
  snprintf( buffer, 10, "%.2X", red );
  result += buffer;
  result += "00";
  snprintf( buffer, 10, "%.2X", blue );
  result += buffer;

  return result;
}

struct compare_downtime
{
  bool operator()( player_t* l, player_t* r ) const
  {
    return l -> collected_data.waiting_time.mean() < r -> collected_data.waiting_time.mean();
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
    else if ( type == "hps" && p -> collected_data.hps.mean() <= 0 ) 
      return true; 
    else if ( type == "dtps" && p -> collected_data.dtps.mean() <= 0 ) 
      return true; 
    else if ( type == "tmi" && p -> collected_data.theck_meloree_index.mean() <= 0 ) 
      return true; 
    
    return false; 
  }
};

struct compare_dpet
{
  bool operator()( const stats_t* l, const stats_t* r ) const
  {
    return l -> apet < r -> apet;
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

struct compare_gain
{
  bool operator()( const gain_t* l, const gain_t* r ) const
  {
    return l -> actual > r -> actual;
  }
};

} // anonymous namespace ====================================================

namespace chart {

class sc_chart
{
private:
  std::string _name;
  chart::chart_e _type;
  int print_style;
  struct {unsigned width, height; } _size;
  unsigned _num_axes;
  struct {double min, max; } _text_scaling;

  std::string type()
  {
    return chart_type( _type );
  }
  std::string fill()
  {
    std::string s;
    if ( print_style == 1 )
    {
      if ( _type == chart::LINE || _type == chart::VERTICAL_BAR )
      {
        s += "chf=c,ls,0,EEEEEE,0.2,FFFFFF,0.2"; s += amp;
      }
    }
    else
    {
      s += fill_chart( FILL_BACKGROUND, FILL_SOLID, "333333" );
    }
    return s;
  }

  std::string title()
  { return chart_title( _name ); }

  std::string title_formating()
  { return chart_title_formatting( chart_bg_color( print_style ), 18 ); }

  std::string size()
  {
    if ( _size.width > 0 || _size.height > 0 )
      return chart_size( _size.width, _size.height );
    else
      return std::string();
  }

  std::string grid()
  {
    if ( _type == chart::LINE || _type == chart::VERTICAL_BAR )
      return std::string( "chg=20,20" ) + amp;

    return std::string();
  }

  std::string axis_style()
  {
    if ( _num_axes == 0 )
      return std::string();

    std::string color;
    if ( print_style == 1 )
      color = "000000";
    else
      color = "FFFFFF";

    std::string s = "chxs=";
    for ( unsigned i = 0; i < _num_axes; ++i )
    {
      if ( i > 0 ) s += "|";
      s += util::to_string( i ) + "," + color;
    }
    s += amp;
    return s;
  }

public:
  sc_chart( std::string name, chart::chart_e t, int style, int num_axes = -1 ) :
    _name( name ), _type( t ), print_style( style ), _size(), _num_axes(), _text_scaling()
  {
    _size.width = 550; _size.height = 250;
    if ( num_axes < 0 )
    {
      if ( _type == chart::LINE )
        _num_axes = 2;
      if ( _type == chart::VERTICAL_BAR )
        _num_axes = 1;
    }
    else
      _num_axes = as<unsigned>( num_axes );
  }
  /* You should not modify the width, to keep the html report properly formated!
   */
  void set_width( unsigned width )
  { _size.width = width; }

  void set_height( unsigned height )
  { _size.height = height; }

  void set_text_scaling( double min, double max )
  { _text_scaling.min = min; _text_scaling.max = max; }

  std::string create()
  {
    std::string s;

    s += get_chart_base_url();
    s += type();
    s += fill();
    s += title();
    s += title_formating();
    s += size();
    s += grid();
    s += axis_style();

    return s;
  }
};

} // end namespace chart

// ==========================================================================
// Chart
// ==========================================================================

std::string chart::raid_downtime( std::vector<player_t*>& players_by_name, int print_styles )
{
  // chart option overview: http://code.google.com/intl/de-DE/apis/chart/image/docs/chart_params.html

  if ( players_by_name.empty() )
    return std::string();

  std::vector<player_t*> waiting_list;
  for ( size_t i = 0; i < players_by_name.size(); i++ )
  {
    player_t* p = players_by_name[ i ];
    if ( ( p -> collected_data.waiting_time.mean() / p -> collected_data.fight_length.mean() ) > 0.01 )
    {
      waiting_list.push_back( p );
    }
  }

  if ( waiting_list.empty() )
    return std::string();

  range::sort( waiting_list, compare_downtime() );

  // Check if any player name contains non-ascii characters.
  // If true, add a special char ("\xE4\xB8\x80")  to the title, which fixes a weird bug with google image charts.
  // See Issue 834
  bool player_names_non_ascii = false;
  for ( size_t i = 0; i < waiting_list.size(); i++ )
  {
    if ( util::contains_non_ascii( waiting_list[ i ] -> name_str ) )
    {
      player_names_non_ascii = true;
      break;
    }
  }

  // Set up chart
  sc_chart chart( std::string( player_names_non_ascii ? "\xE4\xB8\x80" : "" ) + "Player Waiting Time", HORIZONTAL_BAR, print_styles );
  chart.set_height( as<unsigned>( waiting_list.size() ) * 30 + 30 );

  std::ostringstream s;
  s.setf( std::ios_base::fixed ); // Set fixed flag for floating point numbers

  // Create Chart
  s << chart.create();

  // Fill in data
  s << "chd=t:";
  double max_waiting = 0;
  for ( size_t i = 0; i < waiting_list.size(); i++ )
  {
    player_t* p = waiting_list[ i ];
    double waiting = 100.0 * p -> collected_data.waiting_time.mean() / p -> collected_data.fight_length.mean();
    if ( waiting > max_waiting ) max_waiting = waiting;
    s << ( i ? "|" : "" );
    s << std::setprecision( 2 ) << waiting;
  }
  s << amp;

  // Custom chart data scaling
  s << "chds=0," << ( max_waiting * 1.9 );
  s << amp;

  // Fill in color series
  s << "chco=";
  for ( size_t i = 0; i < waiting_list.size(); i++ )
  {
    if ( i ) s << ",";
    s << class_color( get_player_or_owner_type( waiting_list[ i ] ) );
  }
  s << amp;

  // Text Data
  s << "chm=";
  for ( size_t i = 0; i < waiting_list.size(); i++ )
  {
    player_t* p = waiting_list[ i ];

    std::string formatted_name = p -> name_str;
    util::urlencode( formatted_name );

    double waiting_pct = ( 100.0 * p -> collected_data.waiting_time.mean() / p -> collected_data.fight_length.mean() );

    s << ( i ? "|" : "" )  << "t++" << std::setprecision( p -> sim -> report_precision / 2 ) << waiting_pct; // Insert waiting percent

    s << "%25++" << formatted_name.c_str(); // Insert player name

    s << "," << get_color( p ); // Insert player class text color

    s << "," << i; // Series Index

    s << ",0"; // <opt_which_points> 0 == draw markers for all points

    s << ",15"; // size
  }
  s << amp;

  return s.str();
}

// chart::raid_dps ==========================================================

size_t chart::raid_aps( std::vector<std::string>& images,
                        sim_t* sim,
                        std::vector<player_t*>& players_by_aps,
                        std::string type )
{
  size_t num_players = players_by_aps.size();

  if ( num_players == 0 )
    return 0;

  double max_aps = 0;
  std::string title_str = "none";
  if ( type == "dps" )
  {
    max_aps = players_by_aps[ 0 ] -> collected_data.dps.mean();
    title_str = "DPS";
  }
  else if ( type == "hps" )
  {
    max_aps = players_by_aps[ 0 ] -> collected_data.hps.mean() + players_by_aps[ 0 ] -> collected_data.aps.mean();
    title_str = "HPS %2b APS";
  }
  else if ( type == "dtps" )
  {
    max_aps = players_by_aps[ players_by_aps.size() - 1 ] -> collected_data.dtps.mean();
    title_str = "DTPS";
  }
  else if ( type == "tmi" )
  {
    max_aps = players_by_aps[ players_by_aps.size() - 1 ] -> collected_data.theck_meloree_index.mean() / 1000.0;
    title_str = "TMI";
  }

  std::string s = std::string();
  char buffer[ 1024 ];
  bool first = true;

  std::vector<player_t*> player_list ;
  size_t max_players = MAX_PLAYERS_PER_CHART;

  // Ommit Player with 0 DPS/HPS
  range::remove_copy_if( players_by_aps, back_inserter( player_list ), filter_non_performing_players( type ) );

  num_players = player_list.size();

  if ( num_players == 0 )
    return 0;

  while ( true )
  {
    if ( num_players > max_players ) num_players = max_players;


    // Check if any player name contains non-ascii characters.
    // If true, add a special char ("\xE4\xB8\x80")  to the title, which fixes a weird bug with google image charts.
    // See Issue 834
    bool player_names_non_ascii = false;
    for ( size_t i = 0; i < num_players; i++ )
    {
      if ( util::contains_non_ascii( player_list[ i ] -> name_str ) )
      {
        player_names_non_ascii = true;
        break;
      }
    }

    std::string chart_name = first ? ( std::string( player_names_non_ascii ? "\xE4\xB8\x80" : "" ) + std::string( title_str ) + " Ranking" ) : "";
    sc_chart chart( chart_name, HORIZONTAL_BAR, sim -> print_styles );
    chart.set_height( as<unsigned>( num_players ) * 20 + ( first ? 20 : 0 ) );

    s = chart.create();
    s += "chbh=15";
    s += amp;
    s += "chd=t:";

    for ( size_t i = 0; i < num_players; i++ )
    {
      player_t* p = player_list[ i ];
      double player_mean = 0.0;
      if      ( type == "dps" )  { player_mean = p -> collected_data.dps.mean(); }
      else if ( type == "hps" )  { player_mean = p -> collected_data.hps.mean() + p -> collected_data.aps.mean(); }
      else if ( type == "dtps" ) { player_mean = p -> collected_data.dtps.mean(); }
      else if ( type == "tmi" )  { player_mean = p -> collected_data.theck_meloree_index.mean() / 1000.0; }
      snprintf( buffer, sizeof( buffer ), "%s%.0f", ( i ? "|" : "" ), player_mean ); 
      s += buffer;
    }
    s += amp;
    snprintf( buffer, sizeof( buffer ), "chds=0,%.0f", max_aps * 2.5 ); s += buffer;
    s += amp;
    s += "chco=";
    for ( size_t i = 0; i < num_players; i++ )
    {
      if ( i ) s += ",";
      s += get_color( player_list[ i ] );
    }
    s += amp;
    s += "chm=";
    for ( size_t i = 0; i < num_players; i++ )
    {
      player_t* p = player_list[ i ];
      std::string formatted_name = util::google_image_chart_encode( p -> name_str );
      util::urlencode( formatted_name );
      double player_mean = 0.0;
      if      ( type == "dps" )  { player_mean = p -> collected_data.dps.mean(); }
      else if ( type == "hps" )  { player_mean = p -> collected_data.hps.mean() + p -> collected_data.aps.mean(); }
      else if ( type == "dtps" ) { player_mean = p -> collected_data.dtps.mean(); }
      else if ( type == "tmi" )  { player_mean = p -> collected_data.theck_meloree_index.mean() / 1000.0; }
      std::string tmi_letter = ( type == "tmi" ) ? "k" : "";
      snprintf( buffer, sizeof( buffer ), "%st++%.0f%s++%s,%s,%d,0,15", ( i ? "|" : "" ), player_mean, tmi_letter.c_str(), formatted_name.c_str(), get_color( p ).c_str(), ( int )i ); s += buffer;
    }
    s += amp;



    images.push_back( s );

    first = false;
    player_list.erase( player_list.begin(), player_list.begin() + num_players );
    num_players = ( int ) player_list.size();
    if ( num_players == 0 ) break;
  }

  return images.size();
}

// chart::raid_gear =========================================================

size_t chart::raid_gear( std::vector<std::string>& images,
                         sim_t* sim,
                         int print_styles )
{
  size_t num_players = sim -> players_by_dps.size();

  if ( ! num_players )
    return 0;

  std::vector<double> data_points[ STAT_MAX ];

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    data_points[ i ].reserve( num_players );
    for ( size_t j = 0; j < num_players; j++ )
    {
      player_t* p = sim -> players_by_dps[ j ];

      data_points[ i ].push_back( ( p -> gear.   get_stat( i ) +
                                    p -> enchant.get_stat( i ) ) * gear_stats_t::stat_mod( i ) );
    }
  }

  double max_total = 0;
  for ( size_t i = 0; i < num_players; i++ )
  {
    double total = 0;
    for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
    {
      if ( stat_color( j ).empty() )
        continue;

      total += data_points[ j ][ i ];
    }
    if ( total > max_total ) max_total = total;
  }

  std::string s;
  char buffer[ 1024 ];

  std::vector<player_t*> player_list = sim -> players_by_dps;
  static const size_t max_players = MAX_PLAYERS_PER_CHART;

  while ( true )
  {
    if ( num_players > max_players ) num_players = max_players;

    unsigned height = as<unsigned>( num_players ) * 20 + 30;

    if ( num_players <= 12 ) height += 70;

    sc_chart chart( "Gear Overview", HORIZONTAL_BAR_STACKED, print_styles, 1 );
    chart.set_height( height );

    s = chart.create();

    s += "chbh=15";
    s += amp;
    s += "chd=t:";
    bool first = true;
    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      if ( stat_color( i ).empty() )
        continue;

      if ( ! first ) s += "|";
      first = false;
      for ( size_t j = 0; j < num_players; j++ )
      {
        snprintf( buffer, sizeof( buffer ), "%s%.0f", ( j ? "," : "" ), data_points[ i ][ j ] ); s += buffer;
      }
    }
    s += amp;
    snprintf( buffer, sizeof( buffer ), "chds=0,%.0f", max_total ); s += buffer;
    s += amp;
    s += "chco=";
    first = true;
    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      if ( stat_color( i ).empty() )
        continue;

      if ( ! first ) s += ",";
      first = false;
      s += stat_color( i );
    }
    s += amp;
    s += "chxt=y";
    s += amp;
    s += "chxl=0:";
    for ( size_t i = 0; i < num_players; ++i )
    {
      std::string formatted_name = player_list[ i ] -> name_str;
      util::urlencode( formatted_name );

      s += "|";
      s += formatted_name.c_str();
    }
    s += amp;
    s += "chdl=";
    first = true;
    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      if ( stat_color( i ).empty() )
        continue;

      if ( ! first ) s += "|";
      first = false;
      s += util::stat_type_abbrev( i );
    }
    s += amp;
    if ( num_players <= 12 )
    {
      s += "chdlp=t";
      s += amp;
    }
    if ( ! ( sim -> print_styles == 1 ) )
    {
      s += "chdls=dddddd,12";
      s += amp;
    }

    images.push_back( s );

    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      std::vector<double>& c = data_points[ i ];
      c.erase( c.begin(), c.begin() + num_players );
    }

    player_list.erase( player_list.begin(), player_list.begin() + num_players );
    num_players = ( int ) player_list.size();
    if ( num_players == 0 ) break;
  }

  return images.size();
}

// chart::scale_factors =====================================================

std::string chart::scale_factors( player_t* p )
{
  std::vector<stat_e> scaling_stats;
  // load the data from whatever the default metric is set 
  // TODO: eventually show all data
  scale_metric_e sm = p -> sim -> scaling -> scaling_metric;

  for ( std::vector<stat_e>::const_iterator it = p -> scaling_stats.begin(), end = p -> scaling_stats.end(); it != end; ++it )
  {
    if ( p -> scales_with[ *it ] )
      scaling_stats.push_back( *it );
  }

  size_t num_scaling_stats = scaling_stats.size();
  if ( num_scaling_stats == 0 )
    return std::string();

  char buffer[ 1024 ];

  std::string formatted_name = util::google_image_chart_encode( p -> scales_over().name );
  util::urlencode( formatted_name );

  sc_chart chart( "Scale Factors|" + formatted_name, HORIZONTAL_BAR, p -> sim -> print_styles );
  chart.set_height( static_cast<unsigned>( num_scaling_stats ) * 30 + 60 );

  std::string s = chart.create();

  snprintf( buffer, sizeof( buffer ), "chd=t%i:" , 1 ); s += buffer;
  for ( size_t i = 0; i < num_scaling_stats; i++ )
  {
    double factor = p -> scaling[ sm ].get_stat( scaling_stats[ i ] );
    snprintf( buffer, sizeof( buffer ), "%s%.*f", ( i ? "," : "" ), p -> sim -> report_precision, factor ); s += buffer;
  }
  s += "|";

  for ( size_t i = 0; i < num_scaling_stats; i++ )
  {
    double factor = p -> scaling[ sm ].get_stat( scaling_stats[ i ] ) - fabs( p -> scaling_error[ sm ].get_stat( scaling_stats[ i ] ) );

    snprintf( buffer, sizeof( buffer ), "%s%.*f", ( i ? "," : "" ), p -> sim -> report_precision, factor ); s += buffer;
  }
  s += "|";
  for ( size_t i = 0; i < num_scaling_stats; i++ )
  {
    double factor = p -> scaling[ sm ].get_stat( scaling_stats[ i ] ) + fabs( p -> scaling_error[ sm ].get_stat( scaling_stats[ i ] ) );

    snprintf( buffer, sizeof( buffer ), "%s%.*f", ( i ? "," : "" ), p -> sim -> report_precision, factor ); s += buffer;
  }
  s += amp;
  s += "chco=";
  s += get_color( p );
  s += amp;
  s += "chm=";
  snprintf( buffer, sizeof( buffer ), "E,FF0000,1:0,,1:20|" ); s += buffer;
  for ( size_t i = 0; i < num_scaling_stats; i++ )
  {
    double factor = p -> scaling[ sm ].get_stat( scaling_stats[ i ] );
    const char* name = util::stat_type_abbrev( scaling_stats[ i ] );
    snprintf( buffer, sizeof( buffer ), "%st++++%.*f++%s,%s,0,%d,15,0.1,%s", ( i ? "|" : "" ),
              p -> sim -> report_precision, factor, name, get_color( p ).c_str(),
              ( int )i, factor > 0 ? "e" : "s" /* If scale factor is positive, position the text right of the bar, otherwise at the base */
            ); s += buffer;
  }
  s += amp;

  // Obtain lowest and highest scale factor values + error
  double lowest_value = 0, highest_value = 0;
  for ( size_t i = 0; i < num_scaling_stats; i++ )
  {
    double value = p -> scaling[ sm ].get_stat( scaling_stats[ i ] );
    double error = fabs( p -> scaling_error[ sm ].get_stat( scaling_stats[ i ] ) );
    double high_value = std::max( value * 1.2, value + error ); // add enough space to display stat name
    if ( high_value > highest_value )
      highest_value = high_value;
    if ( value - error < lowest_value ) // it is intended that lowest_value will be <= 0
      lowest_value = value - error;
  }
  if ( lowest_value < 0 )
  { highest_value = std::max( highest_value, -lowest_value / 4 ); } // make sure we don't scre up the text
  s += "chds=" + util::to_string( lowest_value - 0.01 ) + "," + util::to_string( highest_value + 0.01 );;
  s += amp;

  return s;
}

// chart::scaling_dps =======================================================

std::string chart::scaling_dps( player_t* p )
{
  double max_dps = 0, min_dps = std::numeric_limits<double>::max();

  for ( size_t i = 0; i < p -> dps_plot_data.size(); ++i )
  {
    std::vector<plot_data_t>& pd = p -> dps_plot_data[ i ];
    size_t size = pd.size();
    for ( size_t j = 0; j < size; j++ )
    {
      if ( pd[ j ].value > max_dps ) max_dps = pd[ j ].value;
      if ( pd[ j ].value < min_dps ) min_dps = pd[ j ].value;
    }
  }
  if ( max_dps <= 0 )
    return std::string();

  double step = p -> sim -> plot -> dps_plot_step;
  int range = p -> sim -> plot -> dps_plot_points / 2;
  const int end = 2 * range;
  size_t num_points = 1 + 2 * range;

  char buffer[ 1024 ];

  std::string formatted_name = util::google_image_chart_encode( p -> scales_over().name );
  util::urlencode( formatted_name );

  sc_chart chart( "Stat Scaling|" + formatted_name, LINE, p -> sim -> print_styles );
  chart.set_height( 300 );

  std::string s = chart.create();

  s += "chd=t:";
  bool first = true;
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    if ( stat_color( i ).empty() ) continue;
    std::vector<plot_data_t>& pd = p -> dps_plot_data[ i ];
    size_t size = pd.size();
    if ( size != num_points ) continue;
    if ( ! first ) s += "|";
    for ( size_t j = 0; j < size; j++ )
    {
      snprintf( buffer, sizeof( buffer ), "%s%.0f", ( j ? "," : "" ), pd[ j ].value ); s += buffer;
    }
    first = false;
  }
  s += amp;
  snprintf( buffer, sizeof( buffer ), "chds=%.0f,%.0f", min_dps, max_dps ); s += buffer;
  s += amp;
  s += "chxt=x,y";
  s += amp;
  if ( p -> sim -> plot -> dps_plot_positive )
  {
    const int start = 0;  // start and end only used for dps_plot_positive
    snprintf( buffer, sizeof( buffer ), "chxl=0:|0|%%2b%.0f|%%2b%.0f|%%2b%.0f|%%2b%.0f|1:|%.0f|%.0f", ( start + ( 1.0 / 4 )*end )*step, ( start + ( 2.0 / 4 )*end )*step, ( start + ( 3.0 / 4 )*end )*step, ( start + end )*step, min_dps, max_dps ); s += buffer;
  }
  else if ( p -> sim -> plot -> dps_plot_negative )
  {
    const int start = (int) - ( p -> sim -> plot -> dps_plot_points * p -> sim -> plot -> dps_plot_step );
    snprintf( buffer, sizeof( buffer ), "chxl=0:|%d|%.0f|%.0f|%.0f|0|1:|%.0f|%.0f", start, start * 0.75, start * 0.5, start * 0.25, min_dps, max_dps ); s += buffer;
  }
  else
  {
    snprintf( buffer, sizeof( buffer ), "chxl=0:|%.0f|%.0f|0|%%2b%.0f|%%2b%.0f|1:|%.0f|%.0f|%.0f", ( -range * step ), ( -range * step ) / 2, ( +range * step ) / 2, ( +range * step ), min_dps, p -> collected_data.dps.mean(), max_dps ); s += buffer;
    s += amp;
    snprintf( buffer, sizeof( buffer ), "chxp=1,1,%.0f,100", 100.0 * ( p -> collected_data.dps.mean() - min_dps ) / ( max_dps - min_dps ) ); s += buffer;
  }
  s += amp;
  s += "chdl=";
  first = true;
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    if ( stat_color( i ).empty() ) continue;
    size_t size = p -> dps_plot_data[ i ].size();
    if ( size != num_points ) continue;
    if ( ! first ) s += "|";
    s += util::stat_type_abbrev( i );
    first = false;
  }
  s += amp;
  if ( ! ( p -> sim -> print_styles == 1 ) )
  {
    s += "chdls=dddddd,12";
    s += amp;
  }
  s += "chco=";
  first = true;
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    if ( stat_color( i ).empty() ) continue;
    size_t size = p -> dps_plot_data[ i ].size();
    if ( size != num_points ) continue;
    if ( ! first ) s += ",";
    first = false;
    s += stat_color( i );
  }
  s += amp;
  snprintf( buffer, sizeof( buffer ), "chg=%.4f,10,1,3", floor( 10000.0 * 100.0 / ( num_points - 1 ) ) / 10000.0 ); s += buffer;
  s += amp;

  return s;
}

// chart::reforge_dps =======================================================

std::string chart::reforge_dps( player_t* p )
{
  if ( ! p )
    return std::string();

  std::vector< std::vector<plot_data_t> >& pd = p -> reforge_plot_data;
  if ( pd.empty() )
    return std::string();

  double dps_range = 0.0, min_dps = std::numeric_limits<double>::max(), max_dps = 0.0;

  size_t num_stats = pd[ 0 ].size() - 1;
  if ( num_stats != 3 && num_stats != 2 )
  {
    p -> sim -> errorf( "You must choose 2 or 3 stats to generate a reforge plot.\n" );
    return std::string();
  }

  for ( size_t i = 0; i < pd.size(); i++ )
  {
    assert( num_stats < pd[ i ].size() );
    if ( pd[ i ][ num_stats ].value < min_dps )
      min_dps = pd[ i ][ num_stats ].value;
    if ( pd[ i ][ num_stats ].value > max_dps )
      max_dps = pd[ i ][ num_stats ].value;
  }

  dps_range = max_dps - min_dps;

  std::ostringstream s;
  s.setf( std::ios_base::fixed ); // Set fixed flag for floating point numbers
  if ( num_stats == 2 )
  {
    int num_points = ( int ) pd.size();
    std::vector<stat_e> stat_indices = p -> sim -> reforge_plot -> reforge_plot_stat_indices;
    plot_data_t& baseline = pd[ num_points / 2 ][ 2 ];
    double min_delta = baseline.value - ( min_dps - baseline.error / 2 );
    double max_delta = ( max_dps + baseline.error / 2 ) - baseline.value;
    double max_ydelta = std::max( min_delta, max_delta );
    int ysteps = 5;
    double ystep_amount = max_ydelta / ysteps;

    int negative_steps = 0, positive_steps = 0, positive_offset = -1;

    for ( int i = 0; i < num_points; i++ )
    {
      if ( pd[ i ][ 0 ].value < 0 )
        negative_steps++;
      else if ( pd[ i ][ 0 ].value > 0 )
      {
        if ( positive_offset == -1 )
          positive_offset = i;
        positive_steps++;
      }
    }

    // We want to fit about 4 labels per side, but if there's many plot points, have some sane 
    int negative_mod = static_cast<int>( std::max( std::ceil( negative_steps / 4 ), 4.0 ) );
    int positive_mod = static_cast<int>( std::max( std::ceil( positive_steps / 4 ), 4.0 ) );

    std::string formatted_name = util::google_image_chart_encode( p -> scales_over().name );
    util::urlencode( formatted_name );

    sc_chart chart( "Reforge Scaling|" + formatted_name, XY_LINE, p -> sim -> print_styles );
    chart.set_height( 400 );

    s << chart.create();

    // Generate reasonable X-axis labels in conjunction with the X data series.
    std::string xl1, xl2;

    // X series
    s << "chd=t2:";
    for ( int i = 0; i < num_points; i++ )
    {
      s << static_cast< int >( pd[ i ][ 0 ].value );
      if ( i < num_points - 1 )
        s << ",";

      bool label = false;
      // Label start
      if ( i == 0 )
        label = true;
      // Label end
      else if ( i == num_points - 1 )
        label = true;
      // Label baseline
      else if ( pd[ i ][ 0 ].value == 0 )
        label = true;
      // Label every negative_modth value (left side of baseline)
      else if ( pd[ i ][ 0 ].value < 0 && i % negative_mod == 0 )
        label = true;
      // Label every positive_modth value (right side of baseline), if there is
      // enough room until the end of the graph
      else if ( pd[ i ][ 0 ].value > 0 && i <= num_points - positive_mod && ( i - positive_offset ) > 0 && ( i - positive_offset + 1 ) % positive_mod == 0 )
        label = true;

      if ( label )
      {
        xl1 += util::to_string( pd[ i ][ 0 ].value );
        if ( i == 0 || i == num_points - 1 )
        {
          xl1 += "+";
          xl1 += util::stat_type_abbrev( stat_indices[ 0 ] );
        }

        xl2 += util::to_string( pd[ i ][ 1 ].value );
        if ( i == 0 || i == num_points - 1 )
        {
          xl2 += "+";
          xl2 += util::stat_type_abbrev( stat_indices[ 1 ] );
        }

      }
      // Otherwise, "fake" a label by adding simply a space. This is required
      // so that we can get asymmetric reforge ranges to correctly display the
      // baseline position on the X axis
      else
      {
        xl1 += "+";
        xl2 += "+";
      }

      if ( i < num_points - 1 )
        xl1 += "|";
      if ( i < num_points - 1 )
        xl2 += "|";
    }

    // Y series
    s << "|";
    for ( int i = 0; i < num_points; i++ )
    {
      s << static_cast< int >( pd[ i ][ 2 ].value );
      if ( i < num_points - 1 )
        s << ",";
    }

    // Min Y series
    s << "|-1|";
    for ( int i = 0; i < num_points; i++ )
    {
      s << static_cast< int >( pd[ i ][ 2 ].value - pd[ i ][ 2 ].error / 2 );
      if ( i < num_points - 1 )
        s << ",";
    }

    // Max Y series
    s << "|-1|";
    for ( int i = 0; i < num_points; i++ )
    {
      s << static_cast< int >( pd[ i ][ 2 ].value + pd[ i ][ 2 ].error / 2 );
      if ( i < num_points - 1 )
        s << ",";
    }

    s << amp;

    // Axis dimensions
    s << "chds=" << ( int ) pd[ 0 ][ 0 ].value << "," << ( int ) pd[ num_points - 1 ][ 0 ].value << "," << static_cast< int >( floor( baseline.value - max_ydelta ) ) << "," << static_cast< int >( ceil( baseline.value + max_ydelta ) );
    s << amp;

    s << "chxt=x,y,x";
    s << amp;

    // X Axis labels (generated above)
    s << "chxl=0:|" << xl1 << "|2:|" << xl2 << "|";

    // Y Axis labels
    s << "1:|";
    for ( int i = ysteps; i >= 1; i -= 1 )
    {
      s << ( int ) util::round( baseline.value - i * ystep_amount ) << " (" << - ( int ) util::round( i * ystep_amount ) << ")|";
    }
    s << static_cast< int >( baseline.value ) << "|";
    for ( int i = 1; i <= ysteps; i += 1 )
    {
      s << ( int ) util::round( baseline.value + i * ystep_amount ) << " (%2b" << ( int ) util::round( i * ystep_amount ) << ")";
      if ( i < ysteps )
        s << "|";
    }
    s << amp;

    // Chart legend
    if ( ! ( p -> sim -> print_styles == 1 ) )
    {
      s << "chdls=dddddd,12";
      s << amp;
    }

    // Chart color
    s << "chco=";
    s << stat_color( stat_indices[ 0 ] );
    s << amp;

    // Grid lines
    s << "chg=" << util::to_string( 100 / ( 1.0 * num_points ) ) << ",";
    s << util::to_string( 100 / ( ysteps * 2 ) );
    s << ",3,3,0,0";
    s << amp;

    // Chart markers (Errorbars and Center-line)
    s << "chm=E,FF2222,1,-1,1:5|h,888888,1,0.5,1,-1.0";
  }
  else if ( num_stats == 3 )
  {
    if ( max_dps == 0 ) return 0;

    std::vector<std::vector<double> > triangle_points;
    std::vector< std::string > colors;
    for ( int i = 0; i < ( int ) pd.size(); i++ )
    {
      std::vector<plot_data_t> scaled_dps = pd[ i ];
      int ref_plot_amount = p -> sim -> reforge_plot -> reforge_plot_amount;
      for ( int j = 0; j < 3; j++ )
        scaled_dps[ j ].value = ( scaled_dps[ j ].value + ref_plot_amount ) / ( 3. * ref_plot_amount );
      triangle_points.push_back( ternary_coords( scaled_dps ) );
      colors.push_back( color_temperature_gradient( pd[ i ][ 3 ].value, min_dps, dps_range ) );
    }

    s << "<form action='";
    s << get_chart_base_url();
    s << "' method='POST'>";
    s << "<input type='hidden' name='chs' value='525x425' />";
    s << "\n";
    s << "<input type='hidden' name='cht' value='s' />";
    s << "\n";
    if ( ! ( p -> sim -> print_styles == 1 ) )
    {
      s << "<input type='hidden' name='chf' value='bg,s,333333' />";
      s << "\n";
    }

    s << "<input type='hidden' name='chd' value='t:";
    for ( size_t j = 0; j < 2; j++ )
    {
      for ( size_t i = 0; i < triangle_points.size(); i++ )
      {
        s << triangle_points[ i ][ j ];
        if ( i < triangle_points.size() - 1 )
          s << ",";
      }
      if ( j == 0 )
        s << "|";
    }
    s << "' />";
    s << "\n";
    s << "<input type='hidden' name='chco' value='";
    for ( int i = 0; i < ( int ) colors.size(); i++ )
    {
      s << colors[ i ];
      if ( i < ( int ) colors.size() - 1 )
        s << "|";
    }
    s << "' />\n";
    s << "<input type='hidden' name='chds' value='-0.1,1.1,-0.1,0.95' />";
    s << "\n";

    if ( ! ( p -> sim -> print_styles == 1 ) )
    {
      s << "<input type='hidden' name='chdls' value='dddddd,12' />";
      s << "\n";
    }
    s << "\n";
    s << "<input type='hidden' name='chg' value='5,10,1,3'";
    s << "\n";
    std::string formatted_name = p -> name_str;
    util::urlencode( formatted_name );
    s << "<input type='hidden' name='chtt' value='";
    s << formatted_name;
    s << "+Reforge+Scaling' />";
    s << "\n";
    if ( p -> sim -> print_styles == 1 )
    {
      s << "<input type='hidden' name='chts' value='666666,18' />";
    }
    else
    {
      s << "<input type='hidden' name='chts' value='dddddd,18' />";
    }
    s << "\n";
    s << "<input type='hidden' name='chem' value='";
    std::vector<stat_e> stat_indices = p -> sim -> reforge_plot -> reforge_plot_stat_indices;
    s << "y;s=text_outline;d=FF9473,18,l,000000,_,";
    s << util::stat_type_string( stat_indices[ 0 ] );
    s << ";py=1.0;po=0.0,0.01;";
    s << "|y;s=text_outline;d=FF9473,18,r,000000,_,";
    s << util::stat_type_string( stat_indices[ 1 ] );
    s << ";py=1.0;po=1.0,0.01;";
    s << "|y;s=text_outline;d=FF9473,18,h,000000,_,";
    s << util::stat_type_string( stat_indices[ 2 ] );
    s << ";py=1.0;po=0.5,0.9' />";
    s << "\n";
    s << "<input type='submit' value='Get Reforge Plot Chart'>";
    s << "\n";
    s << "</form>";
    s << "\n";
  }

  return s.str();
}

// chart::timeline ==========================================================
/*
std::string chart::timeline(  player_t* p,
                              const std::vector<double>& timeline_data,
                              const std::string& timeline_name,
                              double avg,
                              std::string color,
                              size_t max_length )
{
  if ( timeline_data.empty() )
    return std::string();

  if ( max_length == 0 || max_length > timeline_data.size() )
    max_length = timeline_data.size();

  static const size_t max_points = 600;
  static const double encoding_range  = 60.0;

  size_t max_buckets = max_length;
  int increment = ( ( max_buckets > max_points ) ?
                    ( ( int ) floor( ( double ) max_buckets / max_points ) + 1 ) :
                    1 );

  double timeline_min = std::min( ( max_buckets ?
                                    *std::min_element( timeline_data.begin(), timeline_data.begin() + max_length ) :
                                    0.0 ), 0.0 );

  double timeline_max = ( max_buckets ?
                          *std::max_element( timeline_data.begin(), timeline_data.begin() + max_length ) :
                          0 );

  double timeline_range = timeline_max - timeline_min;

  double encoding_adjust = encoding_range / ( timeline_max - timeline_min );


  sc_chart chart( timeline_name + " Timeline", LINE, p -> sim -> print_styles );
  chart.set_height( 200 );

  std::ostringstream s;
  s << chart.create();
  char * old_locale = setlocale( LC_ALL, "C" );
  s << "chd=s:";
  for ( size_t i = 0; i < max_buckets; i += increment )
    s << simple_encoding( ( int ) ( ( timeline_data[ i ] - timeline_min ) * encoding_adjust ) );
  s << amp;

  if ( ! ( p -> sim -> print_styles == 1 ) )
  {
    s << "chco=" << color;
    s << amp;
  }

  s << "chds=0," << util::to_string( encoding_range, 0 );
  s << amp;

  if ( avg || timeline_min < 0.0 )
  {
    s << "chm=h," << color::yellow << ",0," << ( avg - timeline_min ) / timeline_range << ",0.4";
    s << "|h," << color::red << ",0," << ( 0 - timeline_min ) / timeline_range << ",0.4";
    s << amp;
  }

  s << "chxt=x,y";
  s << amp;

  std::ostringstream f; f.setf( std::ios::fixed ); f.precision( 0 );
  f << "chxl=0:|0|sec=" << util::to_string( max_buckets ) << "|1:|" << ( timeline_min < 0.0 ? "min=" : "" ) << timeline_min;
  if ( timeline_min < 0.0 )
    f << "|0";
  if ( avg )
    f << "|avg=" << util::to_string( avg, 0 );
  else f << "|";
  if ( timeline_max )
    f << "|max=" << util::to_string( timeline_max, 0 );
  s << f.str();
  s << amp;

  s << "chxp=1,1,";
  if ( timeline_min < 0.0 )
  {
    s << util::to_string( 100.0 * ( 0 - timeline_min ) / timeline_range, 0 );
    s << ",";
  }
  s << util::to_string( 100.0 * ( avg - timeline_min ) / timeline_range, 0 );
  s << ",100";

  setlocale( LC_ALL, old_locale );
  return s.str();
}
*/

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

  hc.set_title( distribution_name + " Distribution" );
  if ( p )
    hc.set_toggle_id( "player" + util::to_string( p -> index ) + "toggle" );

  std::vector<highchart::data_entry_t> data;
  for ( int i = 0; i < max_buckets; i++ )
  {
    highchart::data_entry_t e;
    e.value = dist_data[ i ];
    data.push_back( e );
  }
  hc.add_data_series( "column", "", data );

  //snprintf( buffer, sizeof( buffer ), "chxl=0:|min=%.0f|avg=%.0f|max=%.0f", min, avg, max ); s += buffer;


  return true;
}

#if LOOTRANK_ENABLED == 1
// chart::gear_weights_lootrank =============================================

std::string chart::gear_weights_lootrank( player_t* p )
{
  char buffer[ 1024 ];

  std::string s = "http://www.guildox.com/go/wr.asp?";

  switch ( p -> type )
  {
    case DEATH_KNIGHT: s += "Cla=2048"; break;
    case DRUID:        s += "Cla=1024"; break;
    case HUNTER:       s += "Cla=4";    break;
    case MAGE:         s += "Cla=128";  break;
    case MONK:         s += "Cla=4096"; break;
    case PALADIN:      s += "Cla=2";    break;
    case PRIEST:       s += "Cla=16";   break;
    case ROGUE:        s += "Cla=8";    break;
    case SHAMAN:       s += "Cla=64";   break;
    case WARLOCK:      s += "Cla=256";  break;
    case WARRIOR:      s += "Cla=1";    break;
    default: p -> sim -> errorf( "%s", util::player_type_string( p -> type ) ); assert( 0 ); break;
  }

  switch ( p -> type )
  {
    case WARRIOR:
    case PALADIN:
    case DEATH_KNIGHT:
      s += "&Art=1";
      break;
    case HUNTER:
    case SHAMAN:
      s += "&Art=2";
      break;
    case DRUID:
    case ROGUE:
    case MONK:
      s += "&Art=4";
      break;
    case MAGE:
    case PRIEST:
    case WARLOCK:
      s += "&Art=8";
      break;
    default:
      break;
  }

  /* FIXME: Commenting this out since this won't currently work the way we handle pandaren, and we don't currently care what faction people are anyway
  switch ( p -> race )
  {
  case RACE_PANDAREN_ALLIANCE:
  case RACE_NIGHT_ELF:
  case RACE_HUMAN:
  case RACE_GNOME:
  case RACE_DWARF:
  case RACE_WORGEN:
  case RACE_DRAENEI: s += "&F=A"; break;

  case RACE_PANDAREN_HORDE:
  case RACE_ORC:
  case RACE_TROLL:
  case RACE_UNDEAD:
  case RACE_BLOOD_ELF:
  case RACE_GOBLIN:
  case RACE_TAUREN: s += "&F=H"; break;

  case RACE_PANDAREN:
  default: break;
  }
  */
  scale_metric_e sm = p -> sim -> scaling -> scaling_metric;
  bool positive_normalizing_value = p -> scaling[ sm ].get_stat( p -> normalize_by() ) >= 0;
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    double value = positive_normalizing_value ? p -> scaling[ sm ].get_stat( i ) : -p -> scaling[ sm ].get_stat( i );
    if ( value == 0 ) continue;

    const char* name;
    switch ( i )
    {
      case STAT_STRENGTH:                 name = "Str";  break;
      case STAT_AGILITY:                  name = "Agi";  break;
      case STAT_STAMINA:                  name = "Sta";  break;
      case STAT_INTELLECT:                name = "Int";  break;
      case STAT_SPIRIT:                   name = "Spi";  break;
      case STAT_SPELL_POWER:              name = "spd";  break;
      case STAT_ATTACK_POWER:             name = "map";  break;
      case STAT_EXPERTISE_RATING:         name = "Exp";  break;
      case STAT_HIT_RATING:               name = "mhit"; break;
      case STAT_CRIT_RATING:              name = "mcr";  break;
      case STAT_HASTE_RATING:             name = "mh";   break;
      case STAT_MASTERY_RATING:           name = "Mr";   break;
      case STAT_ARMOR:                    name = "Arm";  break;
      case STAT_BONUS_ARMOR:              name = "bar";  break;
      case STAT_WEAPON_DPS:
        if ( HUNTER == p -> type ) name = "rdps"; else name = "dps";  break;
      case STAT_WEAPON_OFFHAND_DPS:       name = "odps"; break;
      default: name = 0; break;
    }

    if ( name )
    {
      snprintf( buffer, sizeof( buffer ), "&%s=%.*f", name, p -> sim -> report_precision, value );
      s += buffer;
    }
  }

  // Set the trinket style choice
  switch ( p -> specialization() )
  {
    case DEATH_KNIGHT_BLOOD:
    case DRUID_GUARDIAN:
    case MONK_BREWMASTER:
    case PALADIN_PROTECTION:
    case WARRIOR_PROTECTION:
      // Tank
      s += "&TF=1";
      break;

    case DEATH_KNIGHT_FROST:
    case DEATH_KNIGHT_UNHOLY:
    case DRUID_FERAL:
    case MONK_WINDWALKER:
    case PALADIN_RETRIBUTION:
    case ROGUE_ASSASSINATION:
    case ROGUE_COMBAT:
    case ROGUE_SUBTLETY:
    case SHAMAN_ENHANCEMENT:
    case WARRIOR_ARMS:
    case WARRIOR_FURY:
      // Melee DPS
      s += "&TF=2";
      break;

    case HUNTER_BEAST_MASTERY:
    case HUNTER_MARKSMANSHIP:
    case HUNTER_SURVIVAL:
      // Ranged DPS
      s += "&TF=4";
      break;

    case DRUID_BALANCE:
    case MAGE_ARCANE:
    case MAGE_FIRE:
    case MAGE_FROST:
    case PRIEST_SHADOW:
    case SHAMAN_ELEMENTAL:
    case WARLOCK_AFFLICTION:
    case WARLOCK_DEMONOLOGY:
    case WARLOCK_DESTRUCTION:
      // Caster DPS
      s += "&TF=8";
      break;

      // Healer
    case DRUID_RESTORATION:
    case MONK_MISTWEAVER:
    case PALADIN_HOLY:
    case PRIEST_DISCIPLINE:
    case PRIEST_HOLY:
    case SHAMAN_RESTORATION:
      s += "&TF=16";
      break;

    default: break;
  }

  s += "&Gem=3"; // FIXME: Remove this when epic gems become available
  s += "&Ver=7";
  snprintf( buffer, sizeof( buffer ), "&maxlv=%d", p -> level );
  s += buffer;

  if ( p -> items[  0 ].parsed.data.id ) s += "&t1="  + util::to_string( p -> items[  0 ].parsed.data.id );
  if ( p -> items[  1 ].parsed.data.id ) s += "&t2="  + util::to_string( p -> items[  1 ].parsed.data.id );
  if ( p -> items[  2 ].parsed.data.id ) s += "&t3="  + util::to_string( p -> items[  2 ].parsed.data.id );
  if ( p -> items[  4 ].parsed.data.id ) s += "&t5="  + util::to_string( p -> items[  4 ].parsed.data.id );
  if ( p -> items[  5 ].parsed.data.id ) s += "&t8="  + util::to_string( p -> items[  5 ].parsed.data.id );
  if ( p -> items[  6 ].parsed.data.id ) s += "&t9="  + util::to_string( p -> items[  6 ].parsed.data.id );
  if ( p -> items[  7 ].parsed.data.id ) s += "&t10=" + util::to_string( p -> items[  7 ].parsed.data.id );
  if ( p -> items[  8 ].parsed.data.id ) s += "&t6="  + util::to_string( p -> items[  8 ].parsed.data.id );
  if ( p -> items[  9 ].parsed.data.id ) s += "&t7="  + util::to_string( p -> items[  9 ].parsed.data.id );
  if ( p -> items[ 10 ].parsed.data.id ) s += "&t11=" + util::to_string( p -> items[ 10 ].parsed.data.id );
  if ( p -> items[ 11 ].parsed.data.id ) s += "&t31=" + util::to_string( p -> items[ 11 ].parsed.data.id );
  if ( p -> items[ 12 ].parsed.data.id ) s += "&t12=" + util::to_string( p -> items[ 12 ].parsed.data.id );
  if ( p -> items[ 13 ].parsed.data.id ) s += "&t32=" + util::to_string( p -> items[ 13 ].parsed.data.id );
  if ( p -> items[ 14 ].parsed.data.id ) s += "&t4="  + util::to_string( p -> items[ 14 ].parsed.data.id );
  if ( p -> items[ 15 ].parsed.data.id ) s += "&t14=" + util::to_string( p -> items[ 15 ].parsed.data.id );
  if ( p -> items[ 16 ].parsed.data.id ) s += "&t15=" + util::to_string( p -> items[ 16 ].parsed.data.id );

  util::urlencode( s );

  return s;
}
#endif

// chart::gear_weights_wowhead ==============================================

std::string chart::gear_weights_wowhead( player_t* p )
{
  char buffer[1024];
  bool first = true;

  // FIXME: switch back to www.wowhead.com once WoD goes live
  std::string s = "http://wod.wowhead.com/?items&amp;filter=";

  switch ( p -> type )
  {
  case DEATH_KNIGHT: s += "ub=6;";  break;
  case DRUID:        s += "ub=11;"; break;
  case HUNTER:       s += "ub=3;";  break;
  case MAGE:         s += "ub=8;";  break;
  case PALADIN:      s += "ub=2;";  break;
  case PRIEST:       s += "ub=5;";  break;
  case ROGUE:        s += "ub=4;";  break;
  case SHAMAN:       s += "ub=7;";  break;
  case WARLOCK:      s += "ub=9;";  break;
  case WARRIOR:      s += "ub=1;";  break;
  case MONK:         s += "ub=10;"; break;
  default: assert( 0 ); break;
  }

  // Restrict wowhead to rare gems. When epic gems become available:"gm=4;gb=1;"
  s += "gm=3;gb=1;";

  // Min ilvl of 463 (sensible for current raid tier).
  s += "minle=463;";

  std::string    id_string = "";
  std::string value_string = "";

  scale_metric_e sm = p -> sim -> scaling -> scaling_metric;
  bool positive_normalizing_value = p -> scaling[sm].get_stat( p -> normalize_by() ) >= 0;

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    double value = positive_normalizing_value ? p -> scaling[sm].get_stat( i ) : -p -> scaling[sm].get_stat( i );
    if ( value == 0 ) continue;

    int id = 0;
    switch ( i )
    {
    case STAT_STRENGTH:                 id = 20;  break;
    case STAT_AGILITY:                  id = 21;  break;
    case STAT_STAMINA:                  id = 22;  break;
    case STAT_INTELLECT:                id = 23;  break;
    case STAT_SPIRIT:                   id = 24;  break;
    case STAT_SPELL_POWER:              id = 123; break;
    case STAT_ATTACK_POWER:             id = 77;  break;
    case STAT_CRIT_RATING:              id = 96;  break;
    case STAT_HASTE_RATING:             id = 103; break;
    case STAT_ARMOR:                    id = 41;  break;
    case STAT_BONUS_ARMOR:              id = 109; break;
    case STAT_MASTERY_RATING:           id = 170; break;
    case STAT_VERSATILITY_RATING:       id = 215; break;
    case STAT_MULTISTRIKE_RATING:       id = 200; break;
    case STAT_WEAPON_DPS:
      if ( HUNTER == p -> type ) id = 138; else id = 32;  break;
    default: break;
    }

    if ( id )
    {
      if ( !first )
      {
        id_string += ":";
        value_string += ":";
      }
      first = false;

      snprintf( buffer, sizeof( buffer ), "%d", id );
      id_string += buffer;

      snprintf( buffer, sizeof( buffer ), "%.*f", p -> sim -> report_precision, value );
      value_string += buffer;
    }
  }

  s += "wt=" + id_string + ";";
  s += "wtv=" + value_string + ";";

  return s;
}

// chart::gear_weights_askmrrobot ===========================================

std::string chart::gear_weights_askmrrobot( player_t* p )
{
  std::stringstream ss;
  // AMR update week of 8/15/2013 guarantees that the origin_str provided from their SimC export is
  // a valid base for appending stat weights.  If the origin has askmrrobot in it, just use that
  if ( util::str_in_str_ci( p -> origin_str, "askmrrobot" ) )
    ss << p -> origin_str;
  // otherwise, we need to construct it from whatever information we have available
  else
  {
    ss << "http://www.askmrrobot.com/wow/gear/";

    // Use valid names if we are provided those
    if ( ! p -> region_str.empty() && ! p -> server_str.empty() && ! p -> name_str.empty() )
    {
      if ( p -> region_str == "us" )
        ss << p -> region_str << "a";
      else
        ss << p -> region_str;

      ss << '/' << p -> server_str << '/' << p -> name_str;
    }
    // otherwise try to reconstruct it from the origin string
    else
    {
      std::string region_str, server_str, name_str;
      if ( util::parse_origin( region_str, server_str, name_str, p -> origin_str ) )
      {
        if ( region_str == "us" )
          ss << region_str << "a";
        else
          ss << region_str;

        ss << '/' << server_str << '/' << name_str;
      }
      else
      {
        if ( p -> sim -> debug )
          p -> sim -> errorf( "Unable to construct AMR link - invalid/unknown region, server, or name string" );
        return "";        
      }
    }
    ss << "?spec=";

    // This next section is sort of unwieldly, I may move this to external functions

    // Player type
    switch ( p -> type )
    {
    case DEATH_KNIGHT: ss << "DeathKnight";  break;
    case DRUID:        ss << "Druid"; break;
    case HUNTER:       ss << "Hunter";  break;
    case MAGE:         ss << "Mage";  break;
    case PALADIN:      ss << "Paladin";  break;
    case PRIEST:       ss << "Priest";  break;
    case ROGUE:        ss << "Rogue";  break;
    case SHAMAN:       ss << "Shaman";  break;
    case WARLOCK:      ss << "Warlock";  break;
    case WARRIOR:      ss << "Warrior";  break;
    case MONK:         ss << "Monk"; break;
      // if this isn't a player, the AMR link is useless
    default: assert( 0 ); break;
    }
    // Player spec
    switch ( p -> specialization() )
    {
    case DEATH_KNIGHT_FROST:
      {
        if ( p -> main_hand_weapon.type == WEAPON_2H ) { ss << "Frost2H"; break; }
        else {ss << "FrostDW"; break;}
      }
    case DEATH_KNIGHT_UNHOLY:   ss << "Unholy2H"; break;
    case DEATH_KNIGHT_BLOOD:    ss << "Blood2H"; break;
    case DRUID_BALANCE:         ss << "Moonkin"; break;
    case DRUID_FERAL:           ss << "FeralCat"; break;
    case DRUID_GUARDIAN:        ss << "FeralBear"; break;
    case DRUID_RESTORATION:     ss << "Restoration"; break;
    case HUNTER_BEAST_MASTERY:  ss << "BeastMastery"; break;
    case HUNTER_MARKSMANSHIP:   ss << "Marksmanship"; break;
    case HUNTER_SURVIVAL:       ss << "Survival"; break;
    case MAGE_ARCANE:           ss << "Arcane"; break;
    case MAGE_FIRE:             ss << "Fire"; break;
    case MAGE_FROST:            ss << "Frost"; break;
    case PALADIN_HOLY:          ss << "Holy"; break;
    case PALADIN_PROTECTION:    ss << "Protection"; break;
    case PALADIN_RETRIBUTION:   ss << "Retribution"; break;
    case PRIEST_DISCIPLINE:     ss << "Discipline"; break;
    case PRIEST_HOLY:           ss << "Holy"; break;
    case PRIEST_SHADOW:         ss << "Shadow"; break;
    case ROGUE_ASSASSINATION:   ss << "Assassination"; break;
    case ROGUE_COMBAT:          ss << "Combat"; break;
    case ROGUE_SUBTLETY:        ss << "Subtlety"; break;
    case SHAMAN_ELEMENTAL:      ss << "Elemental"; break;
    case SHAMAN_ENHANCEMENT:    ss << "Enhancement"; break;
    case SHAMAN_RESTORATION:    ss << "Restoration"; break;
    case WARLOCK_AFFLICTION:    ss << "Affliction"; break;
    case WARLOCK_DEMONOLOGY:    ss << "Demonology"; break;
    case WARLOCK_DESTRUCTION:   ss << "Destruction"; break;
    case WARRIOR_ARMS:          ss << "Arms"; break;
    case WARRIOR_FURY:
      {
        if ( p -> main_hand_weapon.type == WEAPON_SWORD_2H || p -> main_hand_weapon.type == WEAPON_AXE_2H || p -> main_hand_weapon.type == WEAPON_MACE_2H || p -> main_hand_weapon.type == WEAPON_POLEARM )
        { ss << "Fury2H"; break; }
        else { ss << "Fury"; break; }
      }
    case WARRIOR_PROTECTION:    ss << "Protection"; break;
    case MONK_BREWMASTER:
      {
        if ( p -> main_hand_weapon.type == WEAPON_STAFF || p -> main_hand_weapon.type == WEAPON_POLEARM ) { ss << "Brewmaster2h"; break; }
        else { ss << "BrewmasterDw"; break; }
      }
    case MONK_MISTWEAVER:       ss << "Mistweaver"; break;
    case MONK_WINDWALKER:
      {
        if ( p -> main_hand_weapon.type == WEAPON_STAFF || p -> main_hand_weapon.type == WEAPON_POLEARM ) { ss << "Windwalker2h"; break; }
        else { ss << "WindwalkerDw"; break; }
      }
      // if this is a pet or an unknown spec, the AMR link is pointless anyway
    default: assert( 0 ); break;
    }
  }
  // add weights
  ss << "&weights=";

  // check for negative normalizer
  scale_metric_e sm = p -> sim -> scaling -> scaling_metric;
  bool positive_normalizing_value = p -> scaling_normalized[ sm ].get_stat( p -> normalize_by() ) >= 0;

  // AMR accepts a max precision of 2 decimal places
  ss.precision( std::min( p -> sim -> report_precision + 1, 2 ) );

  // flag for skipping the first comma
  bool skipFirstComma = false;

  // loop through stats and append the relevant ones to the URL
  for ( stat_e i = STAT_NONE; i < STAT_MAX; ++i )
  {
    // get stat weight value
    double value = positive_normalizing_value ? p -> scaling_normalized[ sm ].get_stat( i ) : -p -> scaling_normalized[ sm ].get_stat( i );

    // if the weight is negative or AMR won't recognize the stat type string, skip this stat
    if ( value <= 0 || util::str_compare_ci( util::stat_type_askmrrobot( i ), "unknown" ) ) continue;

    // skip the first comma
    if ( skipFirstComma )
      ss << ',';
    skipFirstComma = true;

    // AMR enforces certain bounds on stats, cap at 9.99 for regular and 99.99 for weapon DPS
    if ( ( i == STAT_WEAPON_DPS || i == STAT_WEAPON_OFFHAND_DPS ) && value > 99.99 )
      value = 99.99;
    else if ( value > 9.99 )
      value = 9.99;

    // append the stat weight to the URL
    ss << util::stat_type_askmrrobot( i ) << ':' << std::fixed << value;
  }

  // softweights, softcaps, hardcaps would go here if we supported them

  return util::encode_html( ss.str() );
}

/* Generates a nice looking normal distribution chart,
 * with the area of +- ( tolerance_interval * std_dev ) highlighted.
 *
 * If tolerance interval is 0, it will be calculated from the given confidence level
 */
std::string chart::normal_distribution( double mean, double std_dev, double confidence, double tolerance_interval, int print_styles )
{
  std::ostringstream s;

  assert( confidence >= 0 && confidence <= 1.0 && "confidence must be between 0 and 1" );

  if ( tolerance_interval == 0.0 && confidence > 0 )
    tolerance_interval =  rng::stdnormal_inv( 1.0 - ( 1.0 - confidence ) / 2.0 );

  sc_chart chart( util::to_string( confidence * 100.0, 2 ) + "%25" + " Confidence Interval", LINE, print_styles, 4 );
  chart.set_height( 185 );

  s <<  chart.create();
  s << "chco=FF0000";
  s << amp;

  // set axis range
  s << "chxr=0," << mean - std_dev * 4 << "," << mean + std_dev * 4 << "|2,0," << 1 / ( std_dev * sqrt ( 0.5 * M_PI ) );
  s << amp;

  s << "chxt=x,x,y,y";
  s << amp;

  s << "chxl=1:|DPS|3:|p";
  s << amp;

  s << chart_title_formatting( chart_bg_color( print_styles ), 18 );

  // create the normal distribution function
  s << "chfd=0,x," << mean - std_dev * 4 << "," << mean + std_dev * 4 << "," << std_dev / 100.0 << ",100*exp(-(x-" << mean << ")^2/(2*" << std_dev << "^2))";
  s << amp;

  s << "chd=t:-1";
  s << amp;

  // create tolerance interval limiters
  s << "chm=B,C6D9FD,0," << std::max( 4 * std_dev - std_dev * tolerance_interval, 0.0 ) * 100.0 / std_dev << ":" << floor( std::min( 4 * std_dev + std_dev * tolerance_interval, 8 * std_dev ) * 100.0 / std_dev ) << ",0";

  return s.str();
}

// chart::resource_color ====================================================

std::string chart::resource_color( int type )
{
  switch ( type )
  {
    case RESOURCE_HEALTH:
    case RESOURCE_RUNE_UNHOLY:   return class_color( HUNTER );

    case RESOURCE_RUNE_FROST:
    case RESOURCE_MANA:          return class_color( SHAMAN );

    case RESOURCE_ENERGY:
    case RESOURCE_FOCUS:         
    case RESOURCE_COMBO_POINT:   return class_color( ROGUE );

    case RESOURCE_RAGE:
    case RESOURCE_RUNIC_POWER:
    case RESOURCE_RUNE:
    case RESOURCE_RUNE_BLOOD:    return class_color( DEATH_KNIGHT );

    case RESOURCE_HOLY_POWER:    return class_color( PALADIN );

    case RESOURCE_SOUL_SHARD:
    case RESOURCE_BURNING_EMBER:
    case RESOURCE_DEMONIC_FURY:  return class_color( WARLOCK );
    case RESOURCE_ECLIPSE: return class_color( DRUID );

    case RESOURCE_CHI:           return class_color( MONK );

    case RESOURCE_NONE:
    default:                   return "000000";
  }
}

std::string chart::stat_color( stat_e type )
{
  switch ( type )
  {
    case STAT_STRENGTH:                 return class_color( WARRIOR );
    case STAT_AGILITY:                  return class_color( HUNTER );
    case STAT_INTELLECT:                return class_color( MAGE );
    case STAT_SPIRIT:                   return color::darker_silver;
    case STAT_ATTACK_POWER:             return class_color( ROGUE );
    case STAT_SPELL_POWER:              return class_color( WARLOCK );
    case STAT_READINESS_RATING:         return class_color( DEATH_KNIGHT );
    case STAT_CRIT_RATING:              return class_color( PALADIN );
    case STAT_HASTE_RATING:             return class_color( SHAMAN );
    case STAT_MASTERY_RATING:           return class_color( ROGUE );
    case STAT_MULTISTRIKE_RATING:       return color::mix( color::red, color::tan );
    case STAT_DODGE_RATING:             return class_color( MONK );
    case STAT_PARRY_RATING:             return color::teal;
    case STAT_ARMOR:                    return class_color( PRIEST );
    case STAT_BONUS_ARMOR:              return class_color( PRIEST );
    default:                            return std::string();
  }
}

highchart::pie_chart_t& chart::generate_gains( highchart::pie_chart_t& pc, const player_t* p, const resource_e type )
{
  std::string resource_name = util::inverse_tokenize( util::resource_type_string( type ) );
  pc.set_title( p -> name_str + " " + resource_name + " Gains" );
  pc.set( "plotOptions.pie.dataLabels.format", "<b>{point.name}</b>: {point.y:.1f}" );
  pc.set_toggle_id( "player" + util::to_string( p -> index ) + "toggle" );

  // Build gains List
  std::vector<gain_t*> gains_list;
  for ( size_t i = 0; i < p -> gain_list.size(); ++i )
  {
    gain_t* g = p -> gain_list[ i ];
    if ( g -> actual[ type ] <= 0 ) continue;
    gains_list.push_back( g );
  }
  range::sort( gains_list, compare_gain() );


  // Build Data
  std::vector<highchart::data_entry_t> data;
   if ( ! gains_list.empty() )
   {
     for ( size_t i = 0; i < gains_list.size(); ++i )
     {
       const gain_t* gain = gains_list[ i ];
       std::string color_hex = "#" + resource_color( type );;

       highchart::data_entry_t e;
       e.color = color_hex;
       e.value = gain -> actual[ type ];
       e.name = gain -> name_str;
       data.push_back( e );
     }
   }

   // Add Data to Chart
   pc.add_data_series( data );

  return pc;
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
  std::vector<highchart::data_entry_t> data;
   if ( ! filtered_waiting_stats.empty() )
   {
     for ( size_t i = 0; i < filtered_waiting_stats.size(); ++i )
     {
       const stats_t* stats = filtered_waiting_stats[ i ];
       std::string color = school_color( stats -> school );
       if ( color.empty() )
       {
         p -> sim -> errorf( "chart::generate_stats_sources assertion error! School color unknown, stats %s from %s. School %s\n",
                             stats -> name_str.c_str(), p -> name(), util::school_type_string( stats -> school ) );
         assert( 0 );
       }

       std::string color_hex = "#" + color;

       highchart::data_entry_t e;
       e.color = color_hex;
       e.value = stats -> total_time.total_seconds();
       e.name = stats -> name_str;
       data.push_back( e );
     }
   }
   if ( p -> collected_data.waiting_time.mean() > 0 )
   {
     highchart::data_entry_t e;
     e.color = "#000000";
     e.value = p -> collected_data.waiting_time.mean();
     e.name = "player waiting time";
     data.push_back( e );
   }

   // Add Data to Chart
   pc.add_data_series( data );

  return true;
}

highchart::pie_chart_t& chart::generate_stats_sources( highchart::pie_chart_t& pc, const player_t* p, const std::string title, const std::vector<stats_t*>& stats_list )
{
  pc.set_title( title );
  pc.set( "plotOptions.pie.dataLabels.format", "<b>{point.name}</b>: {point.percentage:.1f} %" );
  pc.set_toggle_id( "player" + util::to_string( p -> index ) + "toggle" );

  // Build Data
  std::vector<highchart::data_entry_t> data;
   if ( ! stats_list.empty() )
   {
     for ( size_t i = 0; i < stats_list.size(); ++i )
     {
       const stats_t* stats = stats_list[ i ];
       std::string color = school_color( stats -> school );
       if ( color.empty() )
       {
         p -> sim -> errorf( "chart::generate_stats_sources assertion error! School color unknown, stats %s from %s. School %s\n",
                             stats -> name_str.c_str(), p -> name(), util::school_type_string( stats -> school ) );
         assert( 0 );
       }

       std::string color_hex = "#" + color;

       highchart::data_entry_t e;
       e.color = color_hex;
       e.value = stats -> portion_amount;
       e.name = stats -> name_str;
       data.push_back( e );
     }
   }

   // Add Data to Chart
   pc.add_data_series( data );

  return pc;
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

highchart::bar_chart_t& chart::generate_player_waiting_time( highchart::bar_chart_t& bc, sim_t* s )
{
  bc.set_title( "Player Waiting Time" );

  // Build List
  std::vector<player_t*> waiting_list;
  range::remove_copy_if( s -> players_by_name, back_inserter( waiting_list ), filter_waiting_time() );
  range::sort( waiting_list, compare_downtime() );

  // Create Data
  std::vector<highchart::data_entry_t> data;
  for ( size_t i = 0; i < waiting_list.size(); ++i )
  {
    const player_t* p = waiting_list[ i ];
    std::string color = class_color( p -> type );
    if ( color.empty() )
    {
      s -> errorf( "%s Player class color unknown. Type %s\n",
          p -> name(), util::player_type_string( p -> type ) );
      assert( 0 );
    }
    highchart::data_entry_t e;
    e.color = "#" + color;
    e.name = p -> name_str;
    e.value = p -> collected_data.waiting_time.mean();
    data.push_back( e );
  }

  bc.add_data_series( data );

  return bc;

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

  // Nothing to visualize
  if ( player_list.size() == 0 )
    return false;

  std::vector<highchart::data_entry_t> data;

  for ( size_t i = 0; i < player_list.size(); ++i )
  {
    const player_t* p = player_list[ i ];
    std::string color = class_color( p -> type );
    if ( color.empty() )
    {
      s -> errorf( "%s Player class color unknown. Type %s\n",
          p -> name(), util::player_type_string( p -> type ) );
      assert( 0 );
    }

    highchart::data_entry_t e;
    e.color = "#" + color;
    e.name = p -> name_str;
    if ( util::str_compare_ci( type, "dps" ) )
      e.value = p -> collected_data.dps.mean();
    else if ( util::str_compare_ci( type, "hps" ) )
      e.value = p -> collected_data.hps.mean() + p -> collected_data.aps.mean();
    else if ( util::str_compare_ci( type, "dtps" ) )
      e.value = p -> collected_data.dtps.mean();
    else if ( util::str_compare_ci( type, "tmi" ) )
      e.value = p -> collected_data.theck_meloree_index.mean();

    data.push_back( e );
  }

  bc.height_ = 92 + player_list.size() * 16;
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

  bc.add_data_series( data );

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
  range::sort( stats_list, compare_dpet() );

  generate_dpet( bc, s, stats_list );

  return bc;

}

highchart::bar_chart_t& chart::generate_dpet( highchart::bar_chart_t& bc , sim_t* s, const std::vector<stats_t*>& stats_list )
{
  if ( ! stats_list.empty() )
  {
    size_t num_stats = stats_list.size();

    //bc.height_ = num_stats * 30 + 30;
    bc.height_ = 0;

    std::vector<highchart::data_entry_t> data;
    for ( size_t i = 0; i < num_stats; ++i )
    {
      const stats_t* stats = stats_list[ i ];
      std::string color = school_color( stats_list[ i ] -> school );
      if ( color.empty() )
      {
        s -> errorf( "chart_t::action_dpet assertion error! School color unknown, stats %s from %s. School %s\n",
            stats -> name_str.c_str(), stats -> player-> name(), util::school_type_string( stats -> school ) );
        assert( 0 );
      }
      highchart::data_entry_t e;
      e.color = "#" + color;
      e.name = stats -> name_str;
      e.value = stats -> apet;
      data.push_back( e );
    }

    bc.add_data_series( data );
  }
  return bc;
}

bool chart::generate_action_dpet( highchart::bar_chart_t& bc, const player_t* p )
{
  bc.set_title( p -> name_str + " Damage per Execute Time" );
  bc.set_yaxis_title( "Damage per Execute Time" );
  bc.set_toggle_id( "player" + util::to_string( p -> index ) + "toggle" );

  std::vector<stats_t*> stats_list;

  // Copy all stats* from p -> stats_list to stats_list, which satisfy the filter
  range::remove_copy_if( p -> stats_list, back_inserter( stats_list ), filter_stats_dpet( *p ) );
  range::sort( stats_list, compare_dpet() );

  if ( stats_list.size() == 0 )
    return false;

  generate_dpet( bc, p -> sim, stats_list );

  return true;

}

// Generate a "standard" timeline highcharts object as a string based on a stats_t object
highchart::time_series_t& chart::generate_stats_timeline( highchart::time_series_t& ts, const stats_t* s )
{
  sc_timeline_t timeline_aps;
  s -> timeline_amount.build_derivative_timeline( timeline_aps );
  ts.set_toggle_id( "actor" + util::to_string( s -> player -> index ) + "_" + s -> name_str + "_toggle" );

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

  std::string area_color = color::yellow;
  if ( s -> action_list.size() > 0 )
    area_color = school_color( s -> action_list[ 0 ] -> school );

  ts.add_simple_series( "area", area_color, s -> type == STATS_DMG ? "DPS" : "HPS", timeline_aps.data() );
  ts.set_mean( s -> portion_aps.mean() );

  return ts;
}

highchart::time_series_t& chart::generate_actor_dps_series( highchart::time_series_t& ts, const player_t* p )
{
  sc_timeline_t timeline_dps;
  p -> collected_data.timeline_dmg.build_derivative_timeline( timeline_dps );
  ts.set_toggle_id( "player" + util::to_string( p -> index ) + "toggle" );

  ts.set( "yAxis.min", 0 );
  ts.set_yaxis_title( "Damage per second" );
  ts.set_title( p -> name_str + " Damage per second" );
  ts.add_simple_series( "area", class_color( p -> type ), "DPS", timeline_dps.data() );
  ts.set_mean( p -> collected_data.dps.mean() );

  return ts;
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

