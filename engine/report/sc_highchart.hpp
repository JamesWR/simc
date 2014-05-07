// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_HIGHCHART_HPP
#define SC_HIGHCHART_HPP

#include <string>
#include <vector>

#include "simulationcraft.hpp"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

namespace {
template <typename T>
static void do_insert( rapidjson::Value& v,
                       const std::vector<T>& values,
                       rapidjson::Document::AllocatorType& alloc )
{
  for ( size_t i = 0, end = values.size(); i < end; i++ )
    v.PushBack( values[ i ], alloc );
}
}

namespace highchart
{
struct chart_t
{
  std::string name_;
  rapidjson::Document js_;

  chart_t( const std::string& name );

  void set_title( const std::string& title )
  { this -> set( "title.text", title.c_str() ); }

  virtual std::string to_string() const;

  template <typename T>
  void set( const std::string& path, const T& value );

  template <typename T>
  void set( const std::string& path, const std::vector<T>& values );

  template <typename T>
  void add( const std::string& path, const T& value );

  template <typename T>
  void add( const std::string& path, const std::vector<T>& data );

  void add( const std::string& path, double x, double high, double low );
  void add( const std::string& path, double x, double y );

  rapidjson::Value* value( const std::string& path );
};

struct timeline_t : public chart_t
{
  timeline_t( const std::string name ) :
    chart_t( name )
  { }
};

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

template <typename T>
void chart_t::set( const std::string& path, const T& value_ )
{
  if ( rapidjson::Value* obj = value( path ) )
    *obj = value_;
}

template<typename T>
void chart_t::set( const std::string& path, const std::vector<T>& values )
{
  if ( rapidjson::Value* obj = value( path ) )
  {
    if ( obj -> GetType() != rapidjson::kArrayType )
      obj -> SetArray();

    obj -> Clear();

    do_insert( *obj, values, js_.GetAllocator() );
  }
}

template <typename T>
void chart_t::add( const std::string& path, const T& value_ )
{
  std::cout << "boop " << value_ << std::endl;
  if ( rapidjson::Value* obj = value( path ) )
  {
    if ( obj -> GetType() != rapidjson::kArrayType )
      obj -> SetArray();

    obj -> PushBack( value_, js_.GetAllocator() );
  }
}

template<typename T>
void chart_t::add( const std::string& path, const std::vector<T>& values )
{
  if ( rapidjson::Value* obj = value( path ) )
  {
    if ( obj -> GetType() != rapidjson::kArrayType )
      obj -> SetArray();

    do_insert( *obj, values, js_.GetAllocator() );
  }
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

}

#endif
