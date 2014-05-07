#include "rapidjson/document.h"

#include "simulationcraft.hpp"

#include "sc_highchart.hpp"

using namespace highchart;

// Init default (shared) json structure
chart_t::chart_t( const std::string& name ) :
  name_( name )
{
  assert( ! name_.empty() );

  js_.SetObject();
}

rapidjson::Value* chart_t::get_obj( const std::vector<std::string>& path )
{
  rapidjson::Value* v = 0;
  if ( path.size() < 1 )
    return v;

  if ( ! js_.HasMember( path[ 0 ].c_str() ) )
    v = &( js_.AddMember( path[ 0 ].c_str(), rapidjson::kObjectType, js_.GetAllocator() ) );
  else
    v = &( js_[ path[ 0 ].c_str() ] ); 

  for ( size_t i = 1; i < path.size() - 1; i++ )
  {
    if ( ! v -> HasMember( path[ i ].c_str() ) )
      v = &( v -> AddMember( path[ i ].c_str(), rapidjson::kObjectType, js_.GetAllocator() ) );
    else
      v = &( (*v)[ path[ i ].c_str() ] );
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

  for ( size_t i = 0, end = values.size(); i < end; i++ )
    obj -> PushBack( values[ i ], js_.GetAllocator() );
}

