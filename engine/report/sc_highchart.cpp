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
  {
    rapidjson::Type obj_type = rapidjson::kObjectType;
    if ( path.size() > 2 && util::is_number( path[ 1 ] ) )
      obj_type = rapidjson::kArrayType;

    js_.AddMember( path[ 0 ].c_str(), obj_type, js_.GetAllocator() );
  }

  v = &( js_[ path[ 0 ].c_str() ] ); 

  for ( size_t i = 1, end = path.size() - 1; i < end; i++ )
  {
    // Number is array indexing [0..size-1]
    if ( util::is_number( path[ i ] ) )
    {
      assert( v -> GetType() == rapidjson::kArrayType );

      unsigned idx = util::to_unsigned( path[ i ] ), missing = 0;
      if ( v -> Size() <= idx )
        missing = ( idx - v -> Size() ) + 1;

      // Pad with objects, until we have enough
      for ( unsigned midx = 0; midx < missing; midx++ )
      {
        rapidjson::Value mvalue( rapidjson::kObjectType );
        v -> PushBack( mvalue, js_.GetAllocator() );
      }

      v = &( (*v)[ rapidjson::SizeType( idx ) ] );
    }
    // Object traversal
    else
    {
      assert( v -> GetType() == rapidjson::kObjectType );

      if ( ! v -> HasMember( path[ i ].c_str() ) )
      {
        rapidjson::Type obj_type = rapidjson::kObjectType;
        if ( i < end - 1 && util::is_number( path[ i + 1 ] ) )
          obj_type = rapidjson::kArrayType;

        v -> AddMember( path[ i ].c_str(), obj_type, js_.GetAllocator() );
      }

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

  if ( ! obj -> HasMember( path_split.back().c_str() ) )
    obj -> AddMember( path_split.back().c_str(), value, js_.GetAllocator() );
  else
    (*obj)[ path_split.back().c_str() ] = value;
}

template<typename T>
void chart_t::set_params( const std::string& path, const std::vector<T>& values )
{
  std::vector<std::string> path_split = util::string_split( path, "." );

  rapidjson::Value* obj = get_obj( path_split );
  if ( ! obj )
    return;

  if ( ! obj -> HasMember( path_split.back().c_str() ) )
    obj = &( obj -> AddMember( path_split.back().c_str(), rapidjson::kArrayType, js_.GetAllocator() ) );
  else
    obj -> Clear();

  for ( size_t i = 0, end = values.size(); i < end; i++ )
    obj -> PushBack( values[ i ], js_.GetAllocator() );
}

