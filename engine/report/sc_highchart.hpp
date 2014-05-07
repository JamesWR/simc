// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_HIGHCHART_HPP
#define SC_HIGHCHART_HPP

#include <string>
#include <vector>

namespace highchart
{
  struct chart_t
  {
    std::string name_;
    rapidjson::Document js_;

    chart_t( const std::string& name );

    virtual std::string to_string() const
    { return std::string(); }

    rapidjson::Value* get_obj( const std::vector<std::string>& path );

    template <typename T>
    void set_param( const std::string& path, const T& value );

    template <typename T>
    void set_params( const std::string& path, const std::vector<T>& values );
  };

  struct time_series_t : public chart_t
  {

    time_series_t( const std::string& name ) :
      chart_t( name )
    { }

    std::string to_string() const;
  };
}

#endif
