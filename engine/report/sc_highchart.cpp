#include "simulationcraft.hpp"

#include "sc_highchart.hpp"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

using namespace highchart;

// Init default (shared) json structure
chart_t::chart_t( const std::string& name ) :
  name_( name )
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

rapidjson::Value* chart_t::get_obj( const std::vector<std::string>& path )
{
  rapidjson::Value* v = 0;
  if ( path.size() < 1 )
    return v;

  if ( ! js_.HasMember( path[ 0 ].c_str() ) )
    js_.AddMember( path[ 0 ].c_str(), rapidjson::kNullType, js_.GetAllocator() );

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

      // Pad with objects, until we have enough
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
        v -> AddMember( path[ i ].c_str(), rapidjson::kNullType, js_.GetAllocator() );

      v = &( (*v)[ path[ i ].c_str() ] );
    }
  }

  return v;
}

template <typename T>
void chart_t::set_param( const std::string& path, const T& value )
{
  std::vector<std::string> path_split = util::string_split( path, "." );

  rapidjson::Value* obj = get_obj( path_split );
  if ( ! obj )
    return;

  *obj = value;
}

template<typename T>
void chart_t::set_params( const std::string& path, const std::vector<T>& values )
{
  std::vector<std::string> path_split = util::string_split( path, "." );

  rapidjson::Value* obj = get_obj( path_split );
  if ( ! obj )
    return;

  if ( obj -> GetType() != rapidjson::kArrayType )
    obj -> SetArray();

  obj -> Clear();

  for ( size_t i = 0, end = values.size(); i < end; i++ )
    obj -> PushBack( values[ i ], js_.GetAllocator() );
}

