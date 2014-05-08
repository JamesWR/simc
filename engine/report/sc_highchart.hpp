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

namespace highchart
{

struct chart_t
{
  std::string id_str_;
  size_t height_, width_;
  rapidjson::Document js_;

  chart_t( const std::string& id_str );

  void set_title( const std::string& title )
  { this -> set( "title.text", title.c_str() ); }

  virtual std::string to_string() const;

  template <typename T>
  void set( const std::string& path, const T& value );
  template <typename T>
  void set( const std::string& path, const std::vector<T>& values );
  template<typename T>
  void set( rapidjson::Value& obj, const std::string& name, const T& value_ );
  template<typename T>
  void set( rapidjson::Value& obj, const std::string& name, const std::vector<T>& value_ );

  template <typename T>
  void add( const std::string& path, const T& value_ );
  template <typename T>
  void add( const std::string& path, const std::vector<T>& data );

  void set( const std::string& path, const char* value );
  void set( const std::string& path, const std::string& value );
  void set( rapidjson::Value& obj, const std::string& name, const char* value_ );
  void set( rapidjson::Value& obj, const std::string& name, const std::string& value_ );

  void add( const std::string& path, const std::string& value_ );
  void add( const std::string& path, const char* value_ );
  void add( const std::string& path, double x, double low, double high );
  void add( const std::string& path, double x, double y );

protected:
  rapidjson::Value* value( const std::string& path );
  rapidjson::Value& do_set( rapidjson::Value& obj, const char* name_, rapidjson::Value& value_ )
  {
    assert( obj.GetType() == rapidjson::kObjectType );

    return obj.AddMember( name_, js_.GetAllocator(), value_, js_.GetAllocator() );
  }

  template <typename T>
  rapidjson::Value& do_insert( rapidjson::Value& obj, const std::vector<T>& values )
  {
    assert( obj.GetType() == rapidjson::kArrayType );

    for ( size_t i = 0, end = values.size(); i < end; i++ )
      obj.PushBack( values[ i ], js_.GetAllocator() );

    return obj;
  }
};

struct time_series_t : public chart_t
{
  const stats_t* stats_;

  time_series_t( const stats_t* stats );

  void add_series( const std::string& color, const std::string& name, const std::vector<double>& series );

  void set_mean( double value_ );
  std::string build_id( const stats_t* stats );
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

    do_insert( *obj, values );
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

    do_insert( *obj, values );
  }
}

template<typename T>
void chart_t::set( rapidjson::Value& obj, const std::string& name_, const T& value_ )
{
  assert( obj.GetType() == rapidjson::kObjectType );

  rapidjson::Value value_obj( value_ );

  do_set( obj, name_.c_str(), value_obj );
}

template<typename T>
void chart_t::set( rapidjson::Value& obj, const std::string& name_, const std::vector<T>& value_ )
{
  assert( obj.GetType() == rapidjson::kObjectType );

  rapidjson::Value value_obj( rapidjson::kArrayType );

  do_set( obj, name_.c_str(), do_insert( value_obj, value_ ) );
}
}

#endif
