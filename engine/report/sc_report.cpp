// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_report.hpp"

// ==========================================================================
// Report
// ==========================================================================

namespace { // UNNAMED NAMESPACE

struct buff_is_dynamic
{
  bool operator() ( const buff_t* b ) const
  {
    if ( ! b -> quiet && b -> avg_start.sum() && ! b -> constant )
      return false;

    return true;
  }
};

struct buff_is_constant
{
  bool operator() ( const buff_t* b ) const
  {
    if ( ! b -> quiet && b -> avg_start.sum() && b -> constant )
      return false;

    return true;
  }
};

struct buff_comp
{
  bool operator()( const buff_t* i, const buff_t* j )
  {
    // Aura&Buff / Pet
    if ( ( ! i -> player || ! i -> player -> is_pet() ) && j -> player && j -> player -> is_pet() )
      return true;
    // Pet / Aura&Buff
    else if ( i -> player && i -> player -> is_pet() && ( ! j -> player || ! j -> player -> is_pet() ) )
      return false;
    // Pet / Pet
    else if ( i -> player && i -> player -> is_pet() && j -> player && j -> player -> is_pet() )
    {
      if ( i -> player -> name_str.compare( j -> player -> name_str ) == 0 )
        return ( i -> name_str.compare( j -> name_str ) < 0 );
      else
        return ( i -> player -> name_str.compare( j -> player -> name_str ) < 0 );
    }

    return ( i -> name_str.compare( j -> name_str ) < 0 );
  }
};

size_t player_chart_length( player_t* p )
{
  if ( pet_t* is_pet = dynamic_cast<pet_t*>( p ) )
    p = is_pet -> owner;

  assert( p );

  if ( ! p ) return 0; // For release builds.

  return static_cast<size_t>( p -> collected_data.fight_length.max() );
}

char stat_type_letter( stats_e type )
{
  switch ( type )
  {
    case STATS_ABSORB:
      return 'A';
    case STATS_DMG:
      return 'D';
    case STATS_HEAL:
      return 'H';
    case STATS_NEUTRAL:
    default:
      return 'X';
  }
}

class tooltip_parser_t
{
  struct error {};

  static const bool PARSE_DEBUG = true;

  const spell_data_t& default_spell;
  const dbc_t& dbc;
  const player_t* player; // For spell query tags (e.g., "$?s9999[Text if you have spell 9999][Text if you do not.]")
  const int level;

  const std::string& text;
  std::string::const_iterator pos;

  std::string result;

  unsigned parse_unsigned()
  {
    unsigned u = 0;
    while ( pos != text.end() && isdigit( *pos ) )
      u = u * 10 + *pos++ - '0';
    return u;
  }

  unsigned parse_effect_number()
  {
    if ( pos == text.end() || *pos < '1' || *pos > '9' )
      throw error();
    return *pos++ - '0';
  }

  const spell_data_t* parse_spell()
  {
    unsigned id = parse_unsigned();
    const spell_data_t* s = dbc.spell( id );
    if ( s -> id() != id )
      throw error();
    return s;
  }

  std::string parse_scaling( const spell_data_t& spell, double multiplier = 1.0 )
  {
    if ( pos == text.end() || *pos != 's' )
      throw error();
    ++pos;

    unsigned effect_number = parse_effect_number();
    if ( effect_number == 0 || effect_number > spell.effect_count() )
      throw error();

    if ( level > MAX_LEVEL )
      throw error();

    const spelleffect_data_t& effect = spell.effectN( effect_number );
    bool show_scale_factor = effect.type() != E_APPLY_AURA;
    double s_min = dbc.effect_min( effect.id(), level );
    double s_max = dbc.effect_max( effect.id(), level );
    if ( s_min < 0 && s_max == s_min )
      s_max = s_min = -s_min;
    else if ( ( player && effect.type() == E_SCHOOL_DAMAGE && ( spell.get_school_type() & SCHOOL_MAGIC_MASK ) != 0 ) ||
              ( player && effect.type() == E_HEAL ) )
    {
      double power = effect.sp_coeff() * player -> initial.stats.spell_power;
      s_min += power;
      s_max += power;
      show_scale_factor = false;
    }
    std::string result = util::to_string( util::round( multiplier * s_min ) );
    if ( s_max != s_min )
    {
      result += " to ";
      result += util::to_string( util::round( multiplier * s_max ) );
    }
    if ( show_scale_factor && effect.sp_coeff() )
    {
      result += " + ";
      result += util::to_string( 100 * multiplier * effect.sp_coeff(), 1 );
      result += '%';
    }

    return result;
  }

public:
  tooltip_parser_t( const dbc_t& d, int l, const spell_data_t& s, const std::string& t ) :
    default_spell( s ), dbc( d ), player( 0 ), level( l ), text( t ), pos( t.begin() ) {}

  tooltip_parser_t( const player_t& p, const spell_data_t& s, const std::string& t ) :
    default_spell( s ), dbc( p.dbc ), player( &p ), level( p.level ), text( t ), pos( t.begin() ) {}

  std::string parse();
};

std::string tooltip_parser_t::parse()
{
  while ( pos != text.end() )
  {
    while ( pos != text.end() && *pos != '$' )
      result += *pos++;
    if ( pos == text.end() )
      break;
    std::string::const_iterator lastpos = pos++;

    try
    {
      if ( pos == text.end() )
        throw error();

      const spell_data_t* spell = &default_spell;
      if ( isdigit( *pos ) )
      {
        spell = parse_spell();
        if ( pos == text.end() )
          throw error();
      }

      std::string replacement_text;
      switch ( *pos )
      {
        case 'd':
        {
          ++pos;
          timespan_t d = spell -> duration();
          if ( d < timespan_t::from_seconds( 1 ) )
          {
            replacement_text = util::to_string( d.total_millis() );
            replacement_text += " milliseconds";
          }
          else if ( d > timespan_t::from_seconds( 1 ) )
          {
            replacement_text = util::to_string( d.total_seconds() );
            replacement_text += " seconds";
          }
          else
            replacement_text = "1 second";
          break;
        }

        case 'h':
        {
          ++pos;
          replacement_text = util::to_string( 100 * spell -> proc_chance() );
          break;
        }

        case 'm':
        {
          ++pos;
          if ( parse_effect_number() <= spell -> effect_count() )
            replacement_text = util::to_string( spell -> effectN( parse_effect_number() ).base_value() );
          else
            replacement_text = util::to_string( 0 );
          break;
        }

        case 's':
          replacement_text = parse_scaling( *spell );
          break;

        case 't':
        {
          ++pos;
          if ( parse_effect_number() <= spell -> effect_count() )
            replacement_text = util::to_string( spell -> effectN( parse_effect_number() ).period().total_seconds() );
          else
            replacement_text = util::to_string( 0 );
          break;
        }

        case 'u':
        {
          ++pos;
          replacement_text = util::to_string( spell -> max_stacks() );
          break;
        }

        case '?':
        {
          ++pos;
          if ( pos == text.end() || *pos != 's' )
            throw error();
          ++pos;
          spell = parse_spell();
          bool has_spell = false;
          if ( player )
          {
            has_spell = player -> find_class_spell( spell -> name_cstr() ) -> ok();
            if ( ! has_spell )
              has_spell = player -> find_glyph_spell( spell -> name_cstr() ) -> ok();
          }
          replacement_text = has_spell ? "true" : "false";
          break;
        }

        case '*':
        {
          ++pos;
          unsigned m = parse_unsigned();

          if ( pos == text.end() || *pos != ';' )
            throw error();
          ++pos;

          replacement_text = parse_scaling( *spell, m );
          break;
        }

        case '/':
        {
          ++pos;
          unsigned m = parse_unsigned();

          if ( pos == text.end() || *pos != ';' )
            throw error();
          ++pos;

          replacement_text = parse_scaling( *spell, 1.0 / m );
          break;
        }

        case '@':
        {
          ++pos;
          if ( text.compare( pos - text.begin(), 9, "spelldesc" ) )
            throw error();
          pos += 9;

          spell = parse_spell();
          if ( ! spell )
            throw error();
          assert( player );
          replacement_text = pretty_spell_text( *spell, spell -> desc(), *player );
          break;
        }

        default:
          throw error();
      }

      if ( PARSE_DEBUG )
      {
        result += '{';
        result.append( lastpos, pos );
        result += '=';
      }

      result += replacement_text;

      if ( PARSE_DEBUG )
        result += '}';
    }
    catch ( error& )
    {
      result.append( lastpos, pos );
    }
  }

  return result;
}

} // UNNAMED NAMESPACE ======================================================

namespace color
{
rgb::rgb() : r_( 0 ), g_( 0 ), b_( 0 )
{ }

rgb::rgb( unsigned char r, unsigned char g, unsigned char b ) :
  r_( r ), g_( g ), b_( b )
{ }

rgb::rgb( double r, double g, double b ) :
  r_( static_cast<unsigned char>( r * 255 ) ),
  g_( static_cast<unsigned char>( g * 255 ) ),
  b_( static_cast<unsigned char>( b * 255 ) )
{ }

rgb::rgb( const std::string& color ) :
  r_( 0 ), g_( 0 ), b_( 0 )
{
  parse_color( color );
}

rgb::rgb( const char* color ) :
  r_( 0 ), g_( 0 ), b_( 0 )
{
  parse_color( color );
}

std::string rgb::rgb_str() const
{
  std::stringstream s;

  s << "rgb(" << static_cast<unsigned>( r_ ) << ", "
              << static_cast<unsigned>( g_ ) << ", "
              << static_cast<unsigned>( b_ ) << ")";

  return s.str();
}

std::string rgb::str() const
{ return *this; }

rgb& rgb::adjust( double v )
{
  if ( v < 0 || v > 1 )
  {
    return *this;
  }

  r_ *= v; g_ *= v; b_ *= v;
  return *this;
}

rgb rgb::adjust( double v ) const
{ return rgb( *this ).adjust( v ); }

rgb rgb::dark( double pct ) const
{ return rgb( *this ).adjust( 1.0 - pct ); }

rgb rgb::light( double pct ) const
{ return rgb( *this ).adjust( 1.0 + pct ); }

rgb& rgb::operator=( const std::string& color_str )
{
  parse_color( color_str );
  return *this;
}

rgb& rgb::operator+=( const rgb& other )
{
  if ( this == &( other ) )
  {
    return *this;
  }

  unsigned mix_r = ( r_ + other.r_ ) / 2;
  unsigned mix_g = ( g_ + other.g_ ) / 2;
  unsigned mix_b = ( b_ + other.b_ ) / 2;

  r_ = static_cast<unsigned char>( mix_r );
  g_ = static_cast<unsigned char>( mix_g );
  b_ = static_cast<unsigned char>( mix_b );

  return *this;
}

rgb rgb::operator+( const rgb& other ) const
{
  rgb new_color( *this );
  new_color += other;
  return new_color;
}

rgb::operator std::string() const
{
  std::stringstream s;
  operator<<( s, *this );
  return s.str();
}

bool rgb::parse_color( const std::string& color_str )
{
  std::stringstream i( color_str );

  if ( color_str.size() < 6 || color_str.size() > 7 )
  {
    return false;
  }

  if ( i.peek() == '#' )
  {
    i.get();
  }

  unsigned v = 0;
  i >> std::hex >> v;
  if ( i.fail() )
  {
    return false;
  }

  r_ = static_cast<unsigned char>( v / 0x10000 );
  g_ = static_cast<unsigned char>( ( v / 0x100 ) % 0x100 );
  b_ = static_cast<unsigned char>( v % 0x100 );

  return true;
}

std::ostream& operator<<( std::ostream& s, const rgb& r )
{
  s << '#';
  s << std::setfill('0') << std::internal << std::uppercase << std::hex;
  s << std::setw( 2 ) << static_cast<unsigned>( r.r_ )
    << std::setw( 2 ) << static_cast<unsigned>( r.g_ )
    << std::setw( 2 ) << static_cast<unsigned>( r.b_ );
  return s;
}

rgb class_color( player_e type )
{
  switch ( type )
  {
    case PLAYER_NONE:     return color::GREY;
    case PLAYER_GUARDIAN: return color::GREY;
    case DEATH_KNIGHT:    return color::COLOR_DEATH_KNIGHT;
    case DRUID:           return color::COLOR_DRUID;
    case HUNTER:          return color::COLOR_HUNTER;
    case MAGE:            return color::COLOR_MAGE;
    case MONK:            return color::COLOR_MONK;
    case PALADIN:         return color::COLOR_PALADIN;
    case PRIEST:          return color::COLOR_PRIEST;
    case ROGUE:           return color::COLOR_ROGUE;
    case SHAMAN:          return color::COLOR_SHAMAN;
    case WARLOCK:         return color::COLOR_WARLOCK;
    case WARRIOR:         return color::COLOR_WARRIOR;
    case ENEMY:           return color::GREY;
    case ENEMY_ADD:       return color::GREY;
    case HEALING_ENEMY:   return color::GREY;
    case TMI_BOSS:        return color::GREY;
    case TANK_DUMMY:      return color::GREY;
    default:              return color::GREY2;
  }
}

rgb resource_color( resource_e type )
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

    case RESOURCE_ECLIPSE:       return class_color( DRUID );

    case RESOURCE_CHI:           return class_color( MONK );

    case RESOURCE_NONE:
    default:                     return GREY2;
  }
}

rgb stat_color( stat_e type )
{
  switch ( type )
  {
    case STAT_STRENGTH:                 return COLOR_WARRIOR;
    case STAT_AGILITY:                  return COLOR_HUNTER;
    case STAT_INTELLECT:                return COLOR_MAGE;
    case STAT_SPIRIT:                   return GREY3;
    case STAT_ATTACK_POWER:             return COLOR_ROGUE;
    case STAT_SPELL_POWER:              return COLOR_WARLOCK;
    case STAT_READINESS_RATING:         return COLOR_DEATH_KNIGHT;
    case STAT_CRIT_RATING:              return COLOR_PALADIN;
    case STAT_HASTE_RATING:             return COLOR_SHAMAN;
    case STAT_MASTERY_RATING:           return COLOR_ROGUE.dark();
    case STAT_MULTISTRIKE_RATING:       return COLOR_DEATH_KNIGHT + COLOR_WARRIOR;
    case STAT_DODGE_RATING:             return COLOR_MONK;
    case STAT_PARRY_RATING:             return TEAL;
    case STAT_ARMOR:                    return COLOR_PRIEST;
    case STAT_BONUS_ARMOR:              return COLOR_PRIEST;
    case STAT_VERSATILITY_RATING:       return PURPLE.dark();
    default:                            return GREY2;
  }
}

/* Blizzard shool colors:
 * http://wowprogramming.com/utils/xmlbrowser/live/AddOns/Blizzard_CombatLog/Blizzard_CombatLog.lua
 * search for: SchoolStringTable
 */
// These colors are picked to sort of line up with classes, but match the "feel" of the spell class' color
rgb school_color( school_e type )
{
  switch ( type )
  {
    // -- Single Schools
    case SCHOOL_NONE:         return color::COLOR_NONE;
    case SCHOOL_PHYSICAL:     return color::PHYSICAL;
    case SCHOOL_HOLY:         return color::HOLY;
    case SCHOOL_FIRE:         return color::FIRE;
    case SCHOOL_NATURE:       return color::NATURE;
    case SCHOOL_FROST:        return color::FROST;
    case SCHOOL_SHADOW:       return color::SHADOW;
    case SCHOOL_ARCANE:       return color::ARCANE;
    // -- Physical and a Magical
    case SCHOOL_FLAMESTRIKE:  return school_color( SCHOOL_PHYSICAL ) + school_color( SCHOOL_FIRE );
    case SCHOOL_FROSTSTRIKE:  return school_color( SCHOOL_PHYSICAL ) + school_color( SCHOOL_FROST );
    case SCHOOL_SPELLSTRIKE:  return school_color( SCHOOL_PHYSICAL ) + school_color( SCHOOL_ARCANE );
    case SCHOOL_STORMSTRIKE:  return school_color( SCHOOL_PHYSICAL ) + school_color( SCHOOL_NATURE );
    case SCHOOL_SHADOWSTRIKE: return school_color( SCHOOL_PHYSICAL ) + school_color( SCHOOL_SHADOW );
    case SCHOOL_HOLYSTRIKE:   return school_color( SCHOOL_PHYSICAL ) + school_color( SCHOOL_HOLY );
      // -- Two Magical Schools
    case SCHOOL_FROSTFIRE:    return color::FROSTFIRE;
    case SCHOOL_SPELLFIRE:    return school_color( SCHOOL_ARCANE ) + school_color( SCHOOL_FIRE );
    case SCHOOL_FIRESTORM:    return school_color( SCHOOL_FIRE ) + school_color( SCHOOL_NATURE );
    case SCHOOL_SHADOWFLAME:  return school_color( SCHOOL_SHADOW ) + school_color( SCHOOL_FIRE );
    case SCHOOL_HOLYFIRE:     return school_color( SCHOOL_HOLY ) + school_color( SCHOOL_FIRE );
    case SCHOOL_SPELLFROST:   return school_color( SCHOOL_ARCANE ) + school_color( SCHOOL_FROST );
    case SCHOOL_FROSTSTORM:   return school_color( SCHOOL_FROST ) + school_color( SCHOOL_NATURE );
    case SCHOOL_SHADOWFROST:  return school_color( SCHOOL_SHADOW ) + school_color( SCHOOL_FROST );
    case SCHOOL_HOLYFROST:    return school_color( SCHOOL_HOLY ) + school_color( SCHOOL_FROST );
    case SCHOOL_SPELLSTORM:   return school_color( SCHOOL_ARCANE ) + school_color( SCHOOL_NATURE );
    case SCHOOL_SPELLSHADOW:  return school_color( SCHOOL_ARCANE ) + school_color( SCHOOL_SHADOW );
    case SCHOOL_DIVINE:       return school_color( SCHOOL_ARCANE ) + school_color( SCHOOL_HOLY );
    case SCHOOL_SHADOWSTORM:  return school_color( SCHOOL_SHADOW ) + school_color( SCHOOL_NATURE );
    case SCHOOL_HOLYSTORM:    return school_color( SCHOOL_HOLY ) + school_color( SCHOOL_NATURE );
    case SCHOOL_SHADOWLIGHT:  return school_color( SCHOOL_SHADOW ) + school_color( SCHOOL_HOLY );
      //-- Three or more schools
    case SCHOOL_ELEMENTAL:    return color::ELEMENTAL;
    case SCHOOL_CHROMATIC:    return school_color( SCHOOL_FIRE ) +
                                       school_color( SCHOOL_FROST ) +
                                       school_color( SCHOOL_ARCANE ) +
                                       school_color( SCHOOL_NATURE ) +
                                       school_color( SCHOOL_SHADOW );
    case SCHOOL_MAGIC:    return school_color( SCHOOL_FIRE ) +
                                   school_color( SCHOOL_FROST ) +
                                   school_color( SCHOOL_ARCANE ) +
                                   school_color( SCHOOL_NATURE ) +
                                   school_color( SCHOOL_SHADOW ) +
                                   school_color( SCHOOL_HOLY );
    case SCHOOL_CHAOS:    return school_color( SCHOOL_PHYSICAL ) +
                                   school_color( SCHOOL_FIRE ) +
                                   school_color( SCHOOL_FROST ) +
                                   school_color( SCHOOL_ARCANE ) +
                                   school_color( SCHOOL_NATURE ) +
                                   school_color( SCHOOL_SHADOW ) +
                                   school_color( SCHOOL_HOLY );

    default:
                          return GREY2;
  }
}
} /* namespace color */

std::string pretty_spell_text( const spell_data_t& default_spell, const std::string& text, const player_t& p )
{ return tooltip_parser_t( p, default_spell, text ).parse(); }

// report::print_profiles ===================================================

void report::print_profiles( sim_t* sim )
{
  int k = 0;
  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    if ( p -> is_pet() ) continue;

    k++;

    if ( !p -> report_information.save_gear_str.empty() ) // Save gear
    {
      io::cfile file( p -> report_information.save_gear_str, "w" );
      if ( ! file )
      {
        sim -> errorf( "Unable to save gear profile %s for player %s\n", p -> report_information.save_gear_str.c_str(), p -> name() );
      }
      else
      {
        std::string profile_str = "";
        p -> create_profile( profile_str, SAVE_GEAR );
        fprintf( file, "%s", profile_str.c_str() );
      }
    }

    if ( !p -> report_information.save_talents_str.empty() ) // Save talents
    {
      io::cfile file( p -> report_information.save_talents_str, "w" );
      if ( ! file )
      {
        sim -> errorf( "Unable to save talents profile %s for player %s\n", p -> report_information.save_talents_str.c_str(), p -> name() );
      }
      else
      {
        std::string profile_str = "";
        p -> create_profile( profile_str, SAVE_TALENTS );
        fprintf( file, "%s", profile_str.c_str() );
      }
    }

    if ( !p -> report_information.save_actions_str.empty() ) // Save actions
    {
      io::cfile file( p -> report_information.save_actions_str, "w" );
      if ( ! file )
      {
        sim -> errorf( "Unable to save actions profile %s for player %s\n", p -> report_information.save_actions_str.c_str(), p -> name() );
      }
      else
      {
        std::string profile_str = "";
        p -> create_profile( profile_str, SAVE_ACTIONS );
        fprintf( file, "%s", profile_str.c_str() );
      }
    }

    std::string file_name = p -> report_information.save_str;

    if ( file_name.empty() && sim -> save_profiles )
    {
      file_name  = sim -> save_prefix_str;
      file_name += p -> name_str;
      if ( sim -> save_talent_str != 0 )
      {
        file_name += "_";
        file_name += p -> primary_tree_name();
      }
      file_name += sim -> save_suffix_str;
      file_name += ".simc";
    }

    if ( file_name.empty() ) continue;

    io::cfile file( file_name, "w" );
    if ( ! file )
    {
      sim -> errorf( "Unable to save profile %s for player %s\n", file_name.c_str(), p -> name() );
      continue;
    }

    std::string profile_str = "";
    p -> create_profile( profile_str );
    fprintf( file, "%s", profile_str.c_str() );
  }

  // Save overview file for Guild downloads
  //if ( /* guild parse */ )
  if ( sim -> save_raid_summary )
  {
    static const char* const filename = "Raid_Summary.simc";
    io::cfile file( filename, "w" );
    if ( ! file )
    {
      sim -> errorf( "Unable to save overview profile %s\n", filename );
    }
    else
    {
      fprintf( file, "#Raid Summary\n"
                     "# Contains %d Players.\n\n", k );

      for ( unsigned int i = 0; i < sim -> actor_list.size(); ++i )
      {
        player_t* p = sim -> actor_list[ i ];
        if ( p -> is_pet() ) continue;

        if ( ! p -> report_information.save_str.empty() )
          fprintf( file, "%s\n", p -> report_information.save_str.c_str() );
        else if ( sim -> save_profiles )
        {
          fprintf( file,
                   "# Player: %s Spec: %s Role: %s\n"
                   "%s%s",
                   p -> name(), p -> primary_tree_name(),
                   util::role_type_string( p -> primary_role() ),
                   sim -> save_prefix_str.c_str(), p -> name() );

          if ( sim -> save_talent_str != 0 )
            fprintf( file, "-%s", p -> primary_tree_name() );

          fprintf( file, "%s.simc\n\n", sim -> save_suffix_str.c_str() );
        }
      }
    }
  }
}

// report::print_spell_query ================================================

void report::print_spell_query( std::ostream& out, dbc_t& dbc, const spell_data_expr_t& sq, unsigned level )
{

  expr_data_e data_type = sq.data_type;
  for ( std::vector<uint32_t>::const_iterator i = sq.result_spell_list.begin(); i != sq.result_spell_list.end(); ++i )
  {
    switch ( data_type )
    {
    case DATA_TALENT:
      out << spell_info::talent_to_str( dbc, dbc.talent( *i ) );
      break;
    case DATA_EFFECT:
      {
        std::ostringstream sqs;
        const spell_data_t* spell = dbc.spell( dbc.effect( *i ) -> spell_id() );
        if ( spell )
        {
          spell_info::effect_to_str( dbc, spell, dbc.effect( *i ), sqs );
          out << sqs.str();
        }
      }
      break;
    case DATA_SET_BONUS:
      out << spell_info::set_bonus_to_str( dbc, (dbc::set_bonus( dbc.ptr ) + *i) );
      break;
    default:
      {
        const spell_data_t* spell = dbc.spell( *i );
        out << spell_info::to_str( dbc, spell, level );
      }
    }
  }
}

void report::print_spell_query( xml_node_t* root, FILE* file, dbc_t& dbc, const spell_data_expr_t& sq, unsigned level )
{

  expr_data_e data_type = sq.data_type;
  for ( std::vector<uint32_t>::const_iterator i = sq.result_spell_list.begin(); i != sq.result_spell_list.end(); ++i )
  {
    switch ( data_type )
    {
    case DATA_TALENT:
      spell_info::talent_to_xml( dbc, dbc.talent( *i ), root );
      break;
    case DATA_EFFECT:
      {
        std::ostringstream sqs;
        const spell_data_t* spell = dbc.spell( dbc.effect( *i ) -> spell_id() );
        if ( spell )
        {
          spell_info::effect_to_xml( dbc, spell, dbc.effect( *i ), root );
        }
      }
      break;
    case DATA_SET_BONUS:
      spell_info::talent_to_xml( dbc, dbc.talent( *i ), root );
      break;
    default:
      {
        const spell_data_t* spell = dbc.spell( *i );
        spell_info::to_xml( dbc, spell, root, level );
      }
    }
  }

  util::fprintf( file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" );
  root -> print_xml( file );
}
// report::print_suite ======================================================

void report::print_suite( sim_t* sim )
{
  std::cout << "\nGenerating reports...";

  report::print_text( sim, sim -> report_details != 0 );

  report::print_html( sim );
  report::print_xml( sim );
  report::print_profiles( sim );
  report::print_csv_data( sim );
}

void report::print_html_sample_data( report::sc_html_stream& os, const player_t* p, const extended_sample_data_t& data, const std::string& name, int& td_counter, int columns )
{
  // Print Statistics of a Sample Data Container
  os << "\t\t\t\t\t\t\t<tr";
  if ( td_counter & 1 )
  {
    os << " class=\"odd\"";
  }
  td_counter++;
  os << ">\n";
  os << "\t\t\t\t\t\t\t\t<td class=\"left small\" colspan=\"" << columns << "\">";
  os.format( "<a id=\"actor%d_%s_stats_toggle\" class=\"toggle-details\">%s</a></td>\n",
             p -> index, name.c_str(), name.c_str() );

  os << "\t\t\t\t\t\t\t\t</tr>\n";

  os << "\t\t\t\t\t\t\t<tr class=\"details hide\">\n";

  os << "\t\t\t\t\t\t\t\t<td class=\"filler\" colspan=\"" << columns << "\">\n";
  int i = 0;

  os << "\t\t\t\t\t\t\t<table class=\"details\">\n";

  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  os << "\t\t\t\t\t\t\t\t\t<th class=\"left\"><b>Sample Data</b></th>\n"
     << "\t\t\t\t\t\t\t\t\t<th class=\"right\">" << data.name_str << "</th>\n"
     << "\t\t\t\t\t\t\t\t</tr>\n";

  ++i;
  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  os.format(
    "\t\t\t\t\t\t\t\t\t<td class=\"left\">Count</td>\n"
    "\t\t\t\t\t\t\t\t\t<td class=\"right\">%d</td>\n"
    "\t\t\t\t\t\t\t\t</tr>\n",
    data.size() );

  ++i;
  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  os.format(
    "\t\t\t\t\t\t\t\t\t<td class=\"left\">Mean</td>\n"
    "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
    "\t\t\t\t\t\t\t\t</tr>\n",
    data.mean() );


  ++i;
  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";

  os.format(
    "\t\t\t\t\t\t\t\t\t<td class=\"left\">Minimum</td>\n"
    "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
    "\t\t\t\t\t\t\t\t</tr>\n",
    data.min() );

  ++i;
  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  os.format(
    "\t\t\t\t\t\t\t\t\t<td class=\"left\">Maximum</td>\n"
    "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
    "\t\t\t\t\t\t\t\t</tr>\n",
    data.max() );

  ++i;
  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  os.format(
    "\t\t\t\t\t\t\t\t\t<td class=\"left\">Spread ( max - min )</td>\n"
    "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
    "\t\t\t\t\t\t\t\t</tr>\n",
    data.max() - data.min() );

  ++i;
  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  os.format(
    "\t\t\t\t\t\t\t\t\t<td class=\"left\">Range [ ( max - min ) / 2 * 100%% ]</td>\n"
    "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
    "\t\t\t\t\t\t\t\t</tr>\n",
    data.mean() ? ( ( data.max() - data.min() ) / 2 ) * 100 / data.mean() : 0 );

  if ( !data.simple )
  {
    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">Standard Deviation</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.4f</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      data.std_dev );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">5th Percentile</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      data.percentile( 0.05 ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">95th Percentile</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      data.percentile( 0.95 ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">( 95th Percentile - 5th Percentile )</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      data.percentile( 0.95 ) - data.percentile( 0.05 ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\"><b>Mean Distribution</b></td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\"></td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n" );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">Standard Deviation</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.4f</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      data.mean_std_dev );

    ++i;
    double mean_error = data.mean_std_dev * p -> sim -> confidence_estimator;
    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">%.2f%% Confidence Intervall</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">( %.2f - %.2f )</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      p -> sim -> confidence * 100.0,
      data.mean() - mean_error,
      data.mean() + mean_error );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">Normalized %.2f%% Confidence Intervall</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">( %.2f%% - %.2f%% )</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      p -> sim -> confidence * 100.0,
      data.mean() ? 100 - mean_error * 100 / data.mean() : 0,
      data.mean() ? 100 + mean_error * 100 / data.mean() : 0 );



    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"left\"><b>Approx. Iterations needed for ( always use n>=50 )</b></td>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"right\"></td>\n"
       << "\t\t\t\t\t\t\t\t</tr>\n";

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">1%% Error</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      ( int ) ( data.mean() ? ( ( mean_error * mean_error * ( ( float ) data.size() ) / ( 0.01 * data.mean() * 0.01 * data.mean() ) ) ) : 0 ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.1%% Error</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      ( int ) ( data.mean() ? ( ( mean_error * mean_error * ( ( float ) data.size() ) / ( 0.001 * data.mean() * 0.001 * data.mean() ) ) ) : 0 ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.1 Scale Factor Error with Delta=300</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      ( int ) ( 2.0 * mean_error * mean_error * ( ( float ) data.size() ) / ( 30 * 30 ) ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.05 Scale Factor Error with Delta=300</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      ( int ) ( 2.0 * mean_error * mean_error * ( ( float ) data.size() ) / ( 15 * 15 ) ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.01 Scale Factor Error with Delta=300</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      ( int ) (  2.0 * mean_error * mean_error * ( ( float ) data.size() ) / ( 3 * 3 ) ) );

  }

  os << "\t\t\t\t\t\t\t\t</table>\n";

  if ( ! data.simple )
  {
    std::string tokenized_div_name = data.name_str + "_dist";
    util::tokenize( tokenized_div_name );

    highchart::histogram_chart_t chart( tokenized_div_name, p -> sim );
    chart.set_toggle_id( "actor" + util::to_string( p -> index ) + "_" + name + "_stats_toggle" );
    if ( chart::generate_distribution( chart, 0, data.distribution, name, data.mean(), data.min(), data.max() ) )
    {
      os << chart.to_target_div();
      p -> sim -> highcharts_str += chart.to_aggregate_string();
      //os << chart.to_string();
    }
  }


  os << "\t\t\t\t\t\t\t\t</td>\n"
     << "\t\t\t\t\t\t\t</tr>\n";
}

void report::print_html_sample_data( report::sc_html_stream& os, const sim_t* sim, const extended_sample_data_t& data, const std::string& name, int& td_counter, int columns )
{
  // Print Statistics of a Sample Data Container
  os << "\t\t\t\t\t\t\t<tr";
  if ( td_counter & 1 )
  {
    os << " class=\"odd\"";
  }
  td_counter++;
  os << ">\n";
  os << "\t\t\t\t\t\t\t\t<td class=\"left small\" colspan=\"" << columns << "\">";
  os.format( "<a class=\"toggle-details\">%s</a></td>\n",
             name.c_str() );

  os << "\t\t\t\t\t\t\t\t</tr>\n";

  os << "\t\t\t\t\t\t\t<tr class=\"details hide\">\n";

  os << "\t\t\t\t\t\t\t\t<td class=\"filler\" colspan=\"" << columns << "\">\n";
  int i = 0;

  os << "\t\t\t\t\t\t\t<table class=\"details\">\n";

  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  os << "\t\t\t\t\t\t\t\t\t<th class=\"left\"><b>Sample Data</b></th>\n"
     << "\t\t\t\t\t\t\t\t\t<th class=\"right\">" << data.name_str << "</th>\n"
     << "\t\t\t\t\t\t\t\t</tr>\n";

  ++i;
  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  os.format(
    "\t\t\t\t\t\t\t\t\t<td class=\"left\">Count</td>\n"
    "\t\t\t\t\t\t\t\t\t<td class=\"right\">%d</td>\n"
    "\t\t\t\t\t\t\t\t</tr>\n",
    (int) data.size() );

  ++i;
  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  os.format(
    "\t\t\t\t\t\t\t\t\t<td class=\"left\">Mean</td>\n"
    "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
    "\t\t\t\t\t\t\t\t</tr>\n",
    data.mean() );


  ++i;
  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";

  os.format(
    "\t\t\t\t\t\t\t\t\t<td class=\"left\">Minimum</td>\n"
    "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
    "\t\t\t\t\t\t\t\t</tr>\n",
    data.min() );

  ++i;
  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  os.format(
    "\t\t\t\t\t\t\t\t\t<td class=\"left\">Maximum</td>\n"
    "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
    "\t\t\t\t\t\t\t\t</tr>\n",
    data.max() );

  ++i;
  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  os.format(
    "\t\t\t\t\t\t\t\t\t<td class=\"left\">Spread ( max - min )</td>\n"
    "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
    "\t\t\t\t\t\t\t\t</tr>\n",
    data.max() - data.min() );

  ++i;
  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  os.format(
    "\t\t\t\t\t\t\t\t\t<td class=\"left\">Range [ ( max - min ) / 2 * 100%% ]</td>\n"
    "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
    "\t\t\t\t\t\t\t\t</tr>\n",
    data.mean() ? ( ( data.max() - data.min() ) / 2 ) * 100 / data.mean() : 0 );

  if ( !data.simple )
  {
    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">Standard Deviation</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.4f</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      data.std_dev );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">5th Percentile</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      data.percentile( 0.05 ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">95th Percentile</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      data.percentile( 0.95 ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">( 95th Percentile - 5th Percentile )</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      data.percentile( 0.95 ) - data.percentile( 0.05 ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\"><b>Mean Distribution</b></td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\"></td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n" );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">Standard Deviation</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.4f</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      data.mean_std_dev );

    ++i;
    double mean_error = data.mean_std_dev * sim -> confidence_estimator;
    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">%.2f%% Confidence Intervall</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">( %.2f - %.2f )</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      sim -> confidence * 100.0,
      data.mean() - mean_error,
      data.mean() + mean_error );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">Normalized %.2f%% Confidence Intervall</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">( %.2f%% - %.2f%% )</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      sim -> confidence * 100.0,
      data.mean() ? 100 - mean_error * 100 / data.mean() : 0,
      data.mean() ? 100 + mean_error * 100 / data.mean() : 0 );



    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"left\"><b>Approx. Iterations needed for ( always use n>=50 )</b></td>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"right\"></td>\n"
       << "\t\t\t\t\t\t\t\t</tr>\n";

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">1%% Error</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      ( int ) ( data.mean() ? ( ( mean_error * mean_error * ( ( float ) data.size() ) / ( 0.01 * data.mean() * 0.01 * data.mean() ) ) ) : 0 ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.1%% Error</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      ( int ) ( data.mean() ? ( ( mean_error * mean_error * ( ( float ) data.size() ) / ( 0.001 * data.mean() * 0.001 * data.mean() ) ) ) : 0 ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.1 Scale Factor Error with Delta=300</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      ( int ) ( 2.0 * mean_error * mean_error * ( ( float ) data.size() ) / ( 30 * 30 ) ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.05 Scale Factor Error with Delta=300</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      ( int ) ( 2.0 * mean_error * mean_error * ( ( float ) data.size() ) / ( 15 * 15 ) ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.01 Scale Factor Error with Delta=300</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      ( int ) (  2.0 * mean_error * mean_error * ( ( float ) data.size() ) / ( 3 * 3 ) ) );

  }

  os << "\t\t\t\t\t\t\t\t</table>\n";

  if ( ! data.simple )
  {
    highchart::histogram_chart_t chart( data.name_str + "_dist", sim );
    chart::generate_distribution( chart, 0, data.distribution, name, data.mean(), data.min(), data.max() );
    os << chart.to_string();
  }


  os << "\t\t\t\t\t\t\t\t</td>\n"
     << "\t\t\t\t\t\t\t</tr>\n";
}

void report::generate_player_buff_lists( player_t*  p, player_processed_report_information_t& ri )
{
  if ( ri.buff_lists_generated )
    return;

  // Append p -> buff_list to ri.buff_list
  ri.buff_list.insert( ri.buff_list.end(), p -> buff_list.begin(), p -> buff_list.end() );

  for ( size_t i = 0; i < p -> pet_list.size(); ++i )
  {
    pet_t* pet = p -> pet_list[ i ];
    // Append pet -> buff_list to ri.buff_list
    ri.buff_list.insert( ri.buff_list.end(), pet -> buff_list.begin(), pet -> buff_list.end() );
  }

  // Append p -> sim -> buff_list to ri.buff_list
  ri.buff_list.insert( ri.buff_list.end(), p -> sim -> buff_list.begin(), p -> sim -> buff_list.end() );

  // Filter out non-dynamic buffs, copy them into ri.dynamic_buffs and sort
  //range::remove_copy_if( ri.buff_list, back_inserter( ri.dynamic_buffs ), buff_is_dynamic );
  range::remove_copy_if( ri.buff_list, back_inserter( ri.dynamic_buffs ), buff_is_dynamic() );
  range::sort( ri.dynamic_buffs, buff_comp() );

  // Filter out non-constant buffs, copy them into ri.constant_buffs and sort
  range::remove_copy_if( ri.buff_list, back_inserter( ri.constant_buffs ), buff_is_constant() );
  range::sort( ri.constant_buffs, buff_comp() );

  ri.buff_lists_generated = true;
}

void report::generate_player_charts( player_t* p, player_processed_report_information_t& ri )
{
  if ( ri.charts_generated )
    return;

  // Scaling charts
  if ( ! ( ( p -> sim -> scaling -> num_scaling_stats <= 0 ) || p -> quiet || p -> is_pet() || p -> is_enemy() || p -> is_add() || p -> type == HEALING_ENEMY ) )
  {
    ri.gear_weights_lootrank_link        = gear_weights_lootrank   ( p );
    ri.gear_weights_wowhead_std_link     = gear_weights_wowhead    ( p );
    ri.gear_weights_askmrrobot_link      = gear_weights_askmrrobot ( p );
  }

  // Create html profile str
  p -> create_profile( ri.html_profile_str, SAVE_ALL, true );

  ri.charts_generated = true;
}

/* Lootrank generators */

// chart::gear_weights_lootrank =============================================

std::array<std::string, SCALE_METRIC_MAX> report::gear_weights_lootrank( player_t* p )
{
  std::array<std::string, SCALE_METRIC_MAX> sa;

  for ( scale_metric_e sm = SCALE_METRIC_NONE; sm < SCALE_METRIC_MAX; sm++ )
  {
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
	str::format( s, "&%s=%.*f", name, p -> sim -> report_precision, value );
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
    str::format( s, "&maxlv=%d", p -> level );

    if ( p -> items[ 0 ].parsed.data.id ) s += "&t1=" + util::to_string( p -> items[ 0 ].parsed.data.id );
    if ( p -> items[ 1 ].parsed.data.id ) s += "&t2=" + util::to_string( p -> items[ 1 ].parsed.data.id );
    if ( p -> items[ 2 ].parsed.data.id ) s += "&t3=" + util::to_string( p -> items[ 2 ].parsed.data.id );
    if ( p -> items[ 4 ].parsed.data.id ) s += "&t5=" + util::to_string( p -> items[ 4 ].parsed.data.id );
    if ( p -> items[ 5 ].parsed.data.id ) s += "&t8=" + util::to_string( p -> items[ 5 ].parsed.data.id );
    if ( p -> items[ 6 ].parsed.data.id ) s += "&t9=" + util::to_string( p -> items[ 6 ].parsed.data.id );
    if ( p -> items[ 7 ].parsed.data.id ) s += "&t10=" + util::to_string( p -> items[ 7 ].parsed.data.id );
    if ( p -> items[ 8 ].parsed.data.id ) s += "&t6=" + util::to_string( p -> items[ 8 ].parsed.data.id );
    if ( p -> items[ 9 ].parsed.data.id ) s += "&t7=" + util::to_string( p -> items[ 9 ].parsed.data.id );
    if ( p -> items[ 10 ].parsed.data.id ) s += "&t11=" + util::to_string( p -> items[ 10 ].parsed.data.id );
    if ( p -> items[ 11 ].parsed.data.id ) s += "&t31=" + util::to_string( p -> items[ 11 ].parsed.data.id );
    if ( p -> items[ 12 ].parsed.data.id ) s += "&t12=" + util::to_string( p -> items[ 12 ].parsed.data.id );
    if ( p -> items[ 13 ].parsed.data.id ) s += "&t32=" + util::to_string( p -> items[ 13 ].parsed.data.id );
    if ( p -> items[ 14 ].parsed.data.id ) s += "&t4=" + util::to_string( p -> items[ 14 ].parsed.data.id );
    if ( p -> items[ 15 ].parsed.data.id ) s += "&t14=" + util::to_string( p -> items[ 15 ].parsed.data.id );
    if ( p -> items[ 16 ].parsed.data.id ) s += "&t15=" + util::to_string( p -> items[ 16 ].parsed.data.id );

    util::urlencode( s );

    sa[ sm ] = s;
  }
  return sa;
}

// chart::gear_weights_wowhead ==============================================

std::array<std::string, SCALE_METRIC_MAX> report::gear_weights_wowhead( player_t* p )
{
  std::array<std::string, SCALE_METRIC_MAX> sa;

  for ( scale_metric_e sm = SCALE_METRIC_NONE; sm < SCALE_METRIC_MAX; sm++ )
  {
    bool first = true;

    std::string s = "http://www.wowhead.com/?items&amp;filter=";

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

    // Min ilvl of 600 (sensible for current raid tier).
    s += "minle=600;";

    std::string    id_string = "";
    std::string value_string = "";

    bool positive_normalizing_value = p -> scaling[ sm ].get_stat( p -> normalize_by() ) >= 0;

    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      double value = positive_normalizing_value ? p -> scaling[ sm ].get_stat( i ) : -p -> scaling[ sm ].get_stat( i );
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

        str::format( id_string, "%d", id );
        str::format( value_string, "%.*f", p -> sim -> report_precision, value );
      }
    }

    s += "wt=" + id_string + ";";
    s += "wtv=" + value_string + ";";

    sa[ sm ] = s;
  }
  return sa;
}

// chart::gear_weights_askmrrobot ===========================================

std::array<std::string, SCALE_METRIC_MAX> report::gear_weights_askmrrobot( player_t* p )
{
  std::array<std::string, SCALE_METRIC_MAX> sa;

  for ( scale_metric_e sm = SCALE_METRIC_NONE; sm < SCALE_METRIC_MAX; sm++ )
  {
    bool use_generic = false;
    std::stringstream ss;
    // AMR's origin_str provided from their SimC export is a valid base for appending stat weights.
    // If the origin has askmrrobot in it, just use that, but replace wow/player/ with wow/optimize/
    if ( util::str_in_str_ci( p -> origin_str, "askmrrobot" ) )
    {
      std::string origin = p -> origin_str;
      util::replace_all( origin, "wow/player", "wow/optimize" );
      ss << origin;
    }
    // otherwise, we need to construct it from whatever information we have available
    else
    {
      // format is base (below) /region/server/name#spec=[spec];[weightsHash]
      ss << "http://www.askmrrobot.com/wow";

      // Use valid names if we are provided those
      if ( ! p -> region_str.empty() && ! p -> server_str.empty() && ! p -> name_str.empty() )
        ss << "/optimize/" << p -> region_str << '/' << p -> server_str << '/' << p -> name_str;

      // otherwise try to reconstruct it from the origin string
      else
      {
        std::string region_str, server_str, name_str;
        if ( util::parse_origin( region_str, server_str, name_str, p -> origin_str ) )
          ss << "/optimize/" << region_str << '/' << server_str << '/' << name_str;
        // if we can't reconstruct, default to a generic character
        // this uses the base followed by /[spec]#[weightsHash]
        else
        {
          use_generic = true;
          ss << "/best-in-slot/generic/";
        }
      }
    }

    // This next section is sort of unwieldly, I may move this to external functions
    std::string spec;

    // Player type
    switch ( p -> type )
    {
    case DEATH_KNIGHT: spec += "DeathKnight";  break;
    case DRUID:        spec += "Druid"; break;
    case HUNTER:       spec += "Hunter";  break;
    case MAGE:         spec += "Mage";  break;
    case PALADIN:      spec += "Paladin";  break;
    case PRIEST:       spec += "Priest";  break;
    case ROGUE:        spec += "Rogue";  break;
    case SHAMAN:       spec += "Shaman";  break;
    case WARLOCK:      spec += "Warlock";  break;
    case WARRIOR:      spec += "Warrior";  break;
    case MONK:         spec += "Monk"; break;
      // if this isn't a player, the AMR link is useless
    default: assert( 0 ); break;
    }
    // Player spec
    switch ( p -> specialization() )
    {
    case DEATH_KNIGHT_BLOOD:    spec += "Blood"; break;
    case DEATH_KNIGHT_FROST:
    {
      if ( p -> main_hand_weapon.type == WEAPON_2H ) { spec += "Frost2H"; break; }
      else { spec += "FrostDw"; break; }
    }
    case DEATH_KNIGHT_UNHOLY:   spec += "Unholy"; break;
    case DRUID_BALANCE:         spec += "Balance"; break;
    case DRUID_FERAL:           spec += "Feral"; break;
    case DRUID_GUARDIAN:        spec += "Guardian"; break;
    case DRUID_RESTORATION:     spec += "Restoration"; break;
    case HUNTER_BEAST_MASTERY:  spec += "BeastMastery"; break;
    case HUNTER_MARKSMANSHIP:   spec += "Marksmanship"; break;
    case HUNTER_SURVIVAL:       spec += "Survival"; break;
    case MAGE_ARCANE:           spec += "Arcane"; break;
    case MAGE_FIRE:             spec += "Fire"; break;
    case MAGE_FROST:            spec += "Frost"; break;
    case MONK_BREWMASTER:
    {
      if ( p -> main_hand_weapon.type == WEAPON_STAFF || p -> main_hand_weapon.type == WEAPON_POLEARM ) { spec += "Brewmaster2h"; break; }
      else { spec += "BrewmasterDw"; break; }
    }
    case MONK_MISTWEAVER:       spec += "Mistweaver"; break;
    case MONK_WINDWALKER:
    {
      if ( p -> main_hand_weapon.type == WEAPON_STAFF || p -> main_hand_weapon.type == WEAPON_POLEARM ) { spec += "Windwalker2h"; break; }
      else { spec += "WindwalkerDw"; break; }
    }
    case PALADIN_HOLY:          spec += "Holy"; break;
    case PALADIN_PROTECTION:    spec += "Protection"; break;
    case PALADIN_RETRIBUTION:   spec += "Retribution"; break;
    case PRIEST_DISCIPLINE:     spec += "Discipline"; break;
    case PRIEST_HOLY:           spec += "Holy"; break;
    case PRIEST_SHADOW:         spec += "Shadow"; break;
    case ROGUE_ASSASSINATION:   spec += "Assassination"; break;
    case ROGUE_COMBAT:          spec += "Combat"; break;
    case ROGUE_SUBTLETY:        spec += "Subtlety"; break;
    case SHAMAN_ELEMENTAL:      spec += "Elemental"; break;
    case SHAMAN_ENHANCEMENT:    spec += "Enhancement"; break;
    case SHAMAN_RESTORATION:    spec += "Restoration"; break;
    case WARLOCK_AFFLICTION:    spec += "Affliction"; break;
    case WARLOCK_DEMONOLOGY:    spec += "Demonology"; break;
    case WARLOCK_DESTRUCTION:   spec += "Destruction"; break;
    case WARRIOR_ARMS:          spec += "Arms"; break;
    case WARRIOR_FURY:
    {
      if ( p -> main_hand_weapon.type == WEAPON_SWORD_2H || p -> main_hand_weapon.type == WEAPON_AXE_2H || p -> main_hand_weapon.type == WEAPON_MACE_2H || p -> main_hand_weapon.type == WEAPON_POLEARM )
      {
        spec += "Fury2H"; break;
      }
      else { spec += "Fury"; break; }
    }
    case WARRIOR_PROTECTION:    spec += "Protection"; break;

      // if this is a pet or an unknown spec, the AMR link is pointless anyway
    default: assert( 0 ); break;
    }

    // if we're using a generic character, need spec
    if ( use_generic )
      ss << util::tolower( spec );

    ss << "#spec=" << spec << ";";

    // add weights
    ss << "weights=";

    // check for negative normalizer
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

    sa[ sm ] = util::encode_html( ss.str() );
  }
  return sa;
}

std::string report::decorate_html_string( const std::string& value, const color::rgb& color )
{
  std::stringstream s;

  s << "<span style=\"color:" << color;
  s << "\">" << value << "</span>";

  return s.str();
}
