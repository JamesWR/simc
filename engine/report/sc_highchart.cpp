#include "simulationcraft.hpp"

#include "sc_highchart.hpp"

namespace
{
static const char* TEXT_OUTLINE      = "-1px -1px 0 #000000, 1px -1px 0 #000000, -1px 1px 0 #000000, 1px 1px 0 #000000";
static const char* TEXT_COLOR        = "#CACACA";
static const char* TEXT_COLOR_ALT    = "black";

static const char* TEXT_MEAN_COLOR   = "#CC8888";
static const char* TEXT_MAX_COLOR    = "#8888CC";

static const char* CHART_BGCOLOR     = "#333333";
static const char* CHART_BGCOLOR_ALT = "white";

}

using namespace highchart;

std::string highchart::build_id( const stats_t* stats )
{
  std::string s;

  s += "actor" + util::to_string( stats -> player -> index );
  s += "_" + stats -> name_str;

  return s;
}
   
std::string highchart::build_id( const player_t* actor, const std::string& suffix )
{
  std::string s = "actor" + util::to_string( actor -> index );
  s += suffix;
  return s;
}

std::string highchart::build_id( const buff_t* buff, const std::string& suffix )
{
  std::string s = "buff_" + buff -> name_str;
  if ( buff -> player )
    s += "_actor" + util::to_string( buff -> player -> index );

  s += suffix;
  return s;
}

// Init default (shared) json structure
chart_t::chart_t( const std::string& id_str, const sim_t* sim ) :
  id_str_( id_str ), height_( 250 ), width_( 550 ), sim_( sim )
{
  assert( ! id_str_.empty() );

  js_.SetObject();
}

std::string chart_t::to_string() const
{

  rapidjson::StringBuffer b;
  rapidjson::PrettyWriter< rapidjson::StringBuffer > writer( b );

  js_.Accept( writer );

  std::string str_ = "<div id=\"" + id_str_ + "\"";
  str_ += " style=\"min-width: " + util::to_string( width_ ) + "px;";
  str_ += " height: " + util::to_string( height_ ) + "px; margin: 0 auto\"></div>\n";
  str_ += "<script type=\"text/javascript\">\n";
  str_ += "jQuery( document ).ready( function( $ ) {\n$('#" + id_str_ + "').highcharts(";
  str_ += b.GetString();
  str_ += ");\n});\n";
  str_ += "</script>\n";

  return str_;
}

std::string chart_t::to_json() const
{
  rapidjson::StringBuffer b;
  rapidjson::PrettyWriter< rapidjson::StringBuffer > writer( b );

  js_.Accept( writer );

  return b.GetString();
}

std::string chart_t::to_xml() const
{
  rapidjson::StringBuffer b;
  rapidjson::PrettyWriter< rapidjson::StringBuffer > writer( b );

  js_.Accept( writer );

  std::string str = "<!CDATA[";
  str += b.GetString();
  str += "]]>";

  return str;
}

rapidjson::Value* chart_t::path_value( const std::string& path_str )
{
  std::vector<std::string> path = util::string_split( path_str, "." );
  rapidjson::Value* v = 0;
  if ( path.size() < 1 )
    return v;

  assert( ! util::is_number( path[ 0 ] ) );

  if ( ! js_.HasMember( path[ 0 ].c_str() ) )
  {
    rapidjson::Value nv;
    js_.AddMember( path[ 0 ].c_str(), js_.GetAllocator(), 
                   nv,                js_.GetAllocator() );
  }

  v = &( js_[ path[ 0 ].c_str() ] ); 

  for ( size_t i = 1, end = path.size(); i < end; i++ )
  {
    // Number is array indexing [0..size-1]
    if ( util::is_number( path[ i ] ) )
    {
      if ( v -> GetType() != rapidjson::kArrayType )
        v -> SetArray();

      unsigned idx = util::to_unsigned( path[ i ] ), missing = 0;
      if ( v -> Size() <= idx )
        missing = ( idx - v -> Size() ) + 1;

      // Pad with null objects, until we have enough
      for ( unsigned midx = 0; midx < missing; midx++ )
        v -> PushBack( rapidjson::kNullType, js_.GetAllocator() );

      v = &( (*v)[ rapidjson::SizeType( idx ) ] );
    }
    // Object traversal
    else
    {
      if ( v -> GetType() != rapidjson::kObjectType )
        v -> SetObject();

      if ( ! v -> HasMember( path[ i ].c_str() ) )
      {
        rapidjson::Value nv;
        v -> AddMember( path[ i ].c_str(), js_.GetAllocator(), 
                        nv,                js_.GetAllocator() );
      }

      v = &( (*v)[ path[ i ].c_str() ] );
    }
  }

  return v;
}

void chart_t::set_xaxis_max( double max )
{ set( "xAxis.max", max ); }

void chart_t::set_xaxis_title( const std::string& label )
{ set( "xAxis.title.text", label ); }

void chart_t::set_yaxis_title( const std::string& label )
{ set( "yAxis.title.text", label ); }

void chart_t::set_title( const std::string& title )
{ set( "title.text", title ); }

void chart_t::add_series( const std::string& color,
                          const std::string& name,
                          const std::vector<double>& series )
{
  rapidjson::Value obj( rapidjson::kObjectType );

  set( obj, "data", series );
  set( obj, "name", name );

  std::string color_hex = color;
  if ( color_hex[ 0 ] != '#' )
    color_hex = '#' + color_hex;
  
  add( "series", obj );
  add( "colors", color_hex );
}

chart_t& chart_t::set( const std::string& path, const char* value_ )
{
  if ( rapidjson::Value* obj = path_value( path ) )
  {
    rapidjson::Value v( value_, js_.GetAllocator() );
    *obj = v;
  }
  return *this;
}

chart_t& chart_t::set( const std::string& path, const std::string& value_ )
{
  return set( path, value_.c_str() );
}

chart_t& chart_t::add( const std::string& path, const char* value_ )
{
  if ( rapidjson::Value* obj = path_value( path ) )
  {
    if ( obj -> GetType() != rapidjson::kArrayType )
      obj -> SetArray();

    rapidjson::Value v( value_, js_.GetAllocator() );
    obj -> PushBack( v, js_.GetAllocator() );
  }
  return *this;
}

chart_t& chart_t::add( const std::string& path, const std::string& value_ )
{
  return add( path, value_.c_str() );
}

chart_t& chart_t::add( const std::string& path, rapidjson::Value& value_ )
{
  if ( rapidjson::Value* obj = path_value( path ) )
  {
    if ( obj -> GetType() != rapidjson::kArrayType )
      obj -> SetArray();

    obj -> PushBack( value_, js_.GetAllocator() );
  }
  return *this;
}

chart_t& chart_t::add( const std::string& path, double x, double low, double high )
{
  if ( rapidjson::Value* obj = path_value( path ) )
  {
    if ( obj -> GetType() != rapidjson::kArrayType )
      obj -> SetArray();

    rapidjson::Value v( rapidjson::kArrayType );
    v.PushBack( x, js_.GetAllocator() ).PushBack( low, js_.GetAllocator() ).PushBack( high, js_.GetAllocator() );

    obj -> PushBack( v, js_.GetAllocator() );
  }
  return *this;
}

chart_t& chart_t::add( const std::string& path, double x, double y )
{
  if ( rapidjson::Value* obj = path_value( path ) )
  {
    if ( obj -> GetType() != rapidjson::kArrayType )
      obj -> SetArray();

    rapidjson::Value v( rapidjson::kArrayType );
    v.PushBack( x, js_.GetAllocator() ).PushBack( y, js_.GetAllocator() );

    obj -> PushBack( v, js_.GetAllocator() );
  }
  return *this;
}

chart_t& chart_t::set( rapidjson::Value& obj, const std::string& name_, const char* value_ )
{
  assert( obj.GetType() == rapidjson::kObjectType );

  rapidjson::Value value_obj( value_, js_.GetAllocator() );

  do_set( obj, name_.c_str(), value_obj );
  return *this;
}

chart_t& chart_t::set( rapidjson::Value& obj, const std::string& name, const std::string& value_ )
{
  return set( obj, name, value_.c_str() );
}

time_series_t::time_series_t( const std::string& id_str, const sim_t* sim ) :
  chart_t( id_str, sim )
{
  set( "legend.enabled", false );

  set( "chart.type", "area" );
  set( "chart.style.fontSize", "11px" );

  add( "chart.spacing", 5 ).add( "chart.spacing", 5 ).add( "chart.spacing", 5 ).add( "chart.spacing", 5 );

  // Setup background color
  if ( sim_ -> print_styles == 1 )
    set( "chart.backgroundColor", CHART_BGCOLOR_ALT );
  else
    set( "chart.backgroundColor", CHART_BGCOLOR );

  set( "plotOptions.series.shadow", true );
  set( "plotOptions.area.lineWidth", 1.25 );
  set( "plotOptions.area.states.hover.lineWidth", 1 );
  set( "plotOptions.area.fillOpacity", 0.2 );

  // Setup axes
  std::string color = TEXT_COLOR;
  if ( sim_ -> print_styles == 1 )
    color = TEXT_COLOR_ALT;

  set( "xAxis.lineColor", color );
  set( "xAxis.tickColor", color );
  set( "xAxis.title.style.color", color );
  set( "xAxis.labels.style.color", color );
  set_xaxis_title( "Time (seconds)" );

  set( "yAxis.lineColor", color );
  set( "yAxis.tickColor", color );
  set( "yAxis.title.style.color", color );
  set( "yAxis.labels.style.color", color );

  set( "title.style.fontSize", "13px" );
  set( "title.style.color", color );

  if ( sim_ -> print_styles != 1 )
  {
    set( "title.style.textShadow", TEXT_OUTLINE );
    set( "xAxis.title.style.textShadow", TEXT_OUTLINE );
    set( "yAxis.title.style.textShadow", TEXT_OUTLINE );
    set( "xAxis.labels.style.textShadow", TEXT_OUTLINE );
    set( "yAxis.labels.style.textShadow", TEXT_OUTLINE );
  }
}

/**
 * Add y-axis plotline to the chart at value_ height. If name is given, a
 * subtitle will be added to the chart, stating name_=value_. Both the line and
 * the text use color_. line_width_ sets the line width of the plotline. If
 * there are multiple plotlines defined, the subtitle text will be concatenated
 * at the end.
 */
time_series_t& time_series_t::add_yplotline( double value_,
                                             const std::string& name_,
                                             double line_width_,
                                             const std::string& color_ )
{
  if ( rapidjson::Value* obj = path_value( "yAxis.plotLines" ) )
  {
    if ( obj -> GetType() != rapidjson::kArrayType )
      obj -> SetArray();

    set( "subtitle.style.fontsize", "10px" );
    set( "subtitle.style.textShadow", TEXT_OUTLINE );

    std::string color = color_;
    if ( color.empty() )
      color = "#FFFFFF";

    if ( rapidjson::Value* text_v = path_value( "subtitle.text" ) )
    {
      std::string mean_str = "<span style=\"color: ";
      mean_str += color;
      mean_str += ";\">";
      if ( ! name_.empty() )
        mean_str += name_ + "=";
      mean_str += util::to_string( value_ );
      mean_str += "</span>";

      if ( text_v -> GetType() == rapidjson::kStringType )
        mean_str = std::string( text_v -> GetString() ) + ", " + mean_str;

      text_v -> SetString( mean_str.c_str(), mean_str.size(), js_.GetAllocator() );
    }

    rapidjson::Value new_obj( rapidjson::kObjectType );
    set( new_obj, "color", color );
    set( new_obj, "value", value_ );
    set( new_obj, "width", line_width_ );
    set( new_obj, "zIndex", 5 );

    obj -> PushBack( new_obj, js_.GetAllocator() );
  }

  return *this;
}

time_series_t& time_series_t::set_mean( double value_, const std::string& color )
{
  std::string mean_color = color;
  if ( mean_color.empty() )
    mean_color = TEXT_MEAN_COLOR;

  return add_yplotline( value_, "mean", 1.25, mean_color );
}

time_series_t& time_series_t::set_max( double value_, const std::string& color )
{
  std::string max_color = color;
  if ( max_color.empty() )
    max_color = TEXT_MAX_COLOR;

  return add_yplotline( value_, "max", 1.25, max_color );
}

bar_chart_t::bar_chart_t( const std::string& id_str, const sim_t* sim ) :
    chart_t( id_str, sim )
{
  set( "legend.enabled", false );

  set( "chart.type", "bar" );
  set( "chart.style.fontSize", "11px" );

  //add( "chart.spacing", 5 ).add( "chart.spacing", 5 ).add( "chart.spacing", 5 ).add( "chart.spacing", 5 );

  // Setup background color
  if ( sim_ -> print_styles == 1 )
    set( "chart.backgroundColor", CHART_BGCOLOR_ALT );
  else
    set( "chart.backgroundColor", CHART_BGCOLOR );

  set( "plotOptions.series.shadow", true );
  //set( "plotOptions.bar.states.hover.lineWidth", 1 );
  set( "plotOptions.bar.fillOpacity", 0.2 );
  set( "plotOptions.bar.dataLabels.enabled", true );
  set( "plotOptions.bar.dataLabels.format", "{point.series.name}: {y}" );

  // Setup axes
  std::string color = TEXT_COLOR;
  if ( sim_ -> print_styles == 1 )
    color = TEXT_COLOR_ALT;

  set( "xAxis.lineColor", color );
  set( "xAxis.tickColor", color );
  set( "xAxis.title.style.color", color );
  set( "xAxis.labels.style.color", color );

  set( "yAxis.lineColor", color );
  set( "yAxis.tickColor", color );
  set( "yAxis.title.style.color", color );
  set( "yAxis.labels.style.color", color );
  set_yaxis_title( "Damage per Execute Time" );

  set( "title.style.fontSize", "13px" );
  set( "title.style.color", color );

  if ( sim_ -> print_styles != 1 )
  {
    set( "title.style.textShadow", TEXT_OUTLINE );
    set( "xAxis.title.style.textShadow", TEXT_OUTLINE );
    set( "yAxis.title.style.textShadow", TEXT_OUTLINE );
    set( "xAxis.labels.style.textShadow", TEXT_OUTLINE );
  }
}

