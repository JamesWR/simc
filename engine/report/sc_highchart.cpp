#include "sc_highchart.hpp"

using namespace highchart;

// Init default (shared) json structure
chart_t::chart_t( const std::string& name ) :
  name_( name ), height_( 250 ), width_( 550 )
{
  assert( ! name_.empty() );

  js_.SetObject();
}

std::string chart_t::to_string() const
{
    rapidjson::StringBuffer b;
    rapidjson::PrettyWriter< rapidjson::StringBuffer > writer( b );

    js_.Accept( writer );
    return b.GetString();
}
   
rapidjson::Value* chart_t::value( const std::string& path_str )
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

void chart_t::set( const std::string& path, const char* value_ )
{
  if ( rapidjson::Value* obj = value( path ) )
  {
    rapidjson::Value v( value_, js_.GetAllocator() );
    *obj = v;
  }
}

void chart_t::set( const std::string& path, const std::string& value_ )
{
  set( path, value_.c_str() );
}

void chart_t::add( const std::string& path, const char* value_ )
{
  if ( rapidjson::Value* obj = value( path ) )
  {
    if ( obj -> GetType() != rapidjson::kArrayType )
      obj -> SetArray();

    rapidjson::Value v( value_, js_.GetAllocator() );
    obj -> PushBack( v, js_.GetAllocator() );
  }
}

void chart_t::add( const std::string& path, const std::string& value_ )
{
  add( path, value_.c_str() );
}

void chart_t::add( const std::string& path, double x, double low, double high )
{
  if ( rapidjson::Value* obj = value( path ) )
  {
    if ( obj -> GetType() != rapidjson::kArrayType )
      obj -> SetArray();

    rapidjson::Value v( rapidjson::kArrayType );
    v.PushBack( x, js_.GetAllocator() ).PushBack( low, js_.GetAllocator() ).PushBack( high, js_.GetAllocator() );

    obj -> PushBack( v, js_.GetAllocator() );
  }
}

void chart_t::add( const std::string& path, double x, double y )
{
  if ( rapidjson::Value* obj = value( path ) )
  {
    if ( obj -> GetType() != rapidjson::kArrayType )
      obj -> SetArray();

    rapidjson::Value v( rapidjson::kArrayType );
    v.PushBack( x, js_.GetAllocator() ).PushBack( y, js_.GetAllocator() );

    obj -> PushBack( v, js_.GetAllocator() );
  }
}

time_series_t::time_series_t( const std::string name ) :
  chart_t( name )
{
  height_ = 200;
  set( "xAxis.title.text", "Time (seconds)" );
}

void time_series_t::add_hline( const std::string& color,
                               const std::string& label,
                               double value_ )
{
  if ( rapidjson::Value* obj = value( "yAxis.plotLines" ) )
  {
    if ( obj -> GetType() != rapidjson::kArrayType )
      obj -> SetArray();

    rapidjson::Value color_val( color.c_str(), js_.GetAllocator() );
    rapidjson::Value label_val( label.c_str(), js_.GetAllocator() );
    rapidjson::Value value_val( value_ );

    rapidjson::Value label_obj( rapidjson::kObjectType );
    label_obj.AddMember( "text", js_.GetAllocator(), label_val, js_.GetAllocator() );

    rapidjson::Value new_obj( rapidjson::kObjectType );
    new_obj.AddMember( "color", js_.GetAllocator(), color_val, js_.GetAllocator() );
    new_obj.AddMember( "label", js_.GetAllocator(), label_obj, js_.GetAllocator() );
    new_obj.AddMember( "value", js_.GetAllocator(), value_val, js_.GetAllocator() );

    obj -> PushBack( new_obj, js_.GetAllocator() );
  }
}

void time_series_t::set_series_name( size_t series_idx, const std::string& name )
{
  set( "series." + util::to_string( series_idx ) + ".name", name );
}
