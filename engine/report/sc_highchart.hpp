// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_HIGHCHART_HPP
#define SC_HIGHCHART_HPP

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
  size_t height_, width_;
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
  void add( const std::string& path, const T& value_ );
  template <typename T>
  void add( const std::string& path, const std::vector<T>& data );

  void set( const std::string& path, const char* value );
  void set( const std::string& path, const std::string& value );

  void add( const std::string& path, const std::string& value_ );
  void add( const std::string& path, const char* value_ );
  void add( const std::string& path, double x, double low, double high );
  void add( const std::string& path, double x, double y );

  rapidjson::Value* value( const std::string& path );
};

struct time_series_t : public chart_t
{
  time_series_t( const std::string name );

  void set_series_name( size_t series_idx, const std::string& name );
  void add_hline( const std::string& color, const std::string& label, double value_ );
};

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

}

#endif
