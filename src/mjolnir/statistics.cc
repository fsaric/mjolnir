
#include "mjolnir/statistics.h"
#include "mjolnir/graphvalidator.h"
#include "mjolnir/graphtilebuilder.h"

#include <ostream>
#include <set>
#include <boost/format.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/property_tree/ptree.hpp>
#include <sqlite3.h>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <valhalla/midgard/logging.h>
#include <valhalla/baldr/tilehierarchy.h>
#include <valhalla/baldr/graphid.h>
#include <valhalla/baldr/graphconstants.h>

using namespace valhalla::midgard;
using namespace valhalla::baldr;
using namespace valhalla::mjolnir;

namespace valhalla {
namespace mjolnir {

validator_stats::validator_stats ()
  : tile_maps(), tile_areas(), country_maps(), iso_codes(), tile_ids(), dupcounts(3), densities(3) { }

void validator_stats::add_tile_road (const uint32_t& tile_id, const RoadClass& rclass, float length) {
  tile_ids.insert(tile_id);
  tile_maps[tile_id][rclass] += length;
}

void validator_stats::add_tile_area (const uint32_t& tile_id, const float area) {
  tile_areas[tile_id] = area;
}

void validator_stats::add_country_road (const std::string& ctry_code, const RoadClass& rclass, float length) {
  iso_codes.insert(ctry_code);
  country_maps[ctry_code][rclass] += length;
}

void validator_stats::add_density (float density, int level) {
  densities[level].push_back(density);
}

void validator_stats::add_dup (uint32_t newdup, int level) {
  dupcounts[level].push_back(newdup);
}

const std::set<uint32_t>& validator_stats::get_ids () const { return tile_ids; }

const std::set<std::string>& validator_stats::get_isos () const { return iso_codes; }

const std::map<uint32_t, std::map<RoadClass, float> >& validator_stats::get_tile_maps () const { return tile_maps; }

const std::map<uint32_t, float>& validator_stats::get_tile_areas() const { return tile_areas; }

const std::map<std::string, std::map<RoadClass, float> >& validator_stats::get_country_maps () const { return country_maps; }

const std::vector<uint32_t> validator_stats::get_dups(int level) const { return dupcounts[level]; }

const std::vector<float> validator_stats::get_densities(int level) const { return densities[level]; }

const std::vector<std::vector<uint32_t> > validator_stats::get_dups() const { return dupcounts; }

const std::vector<std::vector<float> > validator_stats::get_densities() const { return densities; }

void validator_stats::add (const validator_stats& stats) {
  auto newTileMaps = stats.get_tile_maps();
  auto newTileAreas = stats.get_tile_areas();
  auto newCountryMaps = stats.get_country_maps();
  auto ids = stats.get_ids();
  auto isos = stats.get_isos();
  for (auto& id : ids) {
    for (auto& rclass : rclasses) {
      add_tile_road(id, rclass, newTileMaps[id][rclass]);
      add_tile_area(id, newTileAreas[id]);
    }
  }
  for (auto& iso : isos) {
    for (auto& rclass : rclasses) {
      add_country_road(iso, rclass, newCountryMaps[iso][rclass]);
    }
  }
  uint32_t level = 0;
  for (auto& dupvec : stats.get_dups()) {
    for (auto& dup : dupvec) {
      add_dup(dup, level);
    }
    level++;
  }
  level = 0;
  for (auto& densityvec : stats.get_densities()) {
    for (auto& density : densityvec) {
      add_density(density, level);
    }
    level++;
  }
}

void validator_stats::log_country_stats() {
  // Print the Country statistics
  for (auto country : iso_codes) {
    LOG_DEBUG("Country: " + country);
    for (auto rclass : rclasses) {
      std::string roadStr = roadClassToString[rclass];
      LOG_DEBUG((boost::format("   %1%: %2% Km")
        % roadStr % country_maps[country][rclass]).str());
    }
  }
}

void validator_stats::log_tile_stats() {
  // Print the tile statistics
  for (auto tileid : tile_ids) {
    LOG_DEBUG("Tile: " + std::to_string(tileid));
    LOG_DEBUG((boost::format("    Tile Area: %1% sq. Km") % tile_areas[tileid]).str());
    for (auto rclass : rclasses) {
      std::string roadStr = roadClassToString[rclass];
      LOG_DEBUG((boost::format("    %1%: %2% Km")
        % roadStr % tile_maps[tileid][rclass]).str());
    }
  }
}

void validator_stats::build_db(const boost::property_tree::ptree& pt) {
  // Get the location of the db file to write
  std::string dir = pt.get<std::string>("mjolnir.statistics.statistics_dir");
  std::string db_name = pt.get<std::string>("mjolnir.statistics.db_name");
  std::string database = dir + "/" + db_name;

  if (boost::filesystem::exists(database)) {
    boost::filesystem::remove(database);
  }

  sqlite3 *db_handle;
  sqlite3_stmt *stmt;
  uint32_t ret;
  char *err_msg = NULL;
  std::string sql;

  ret = sqlite3_open_v2(database.c_str(), &db_handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("cannot open " + database);
    sqlite3_close(db_handle);
    db_handle = NULL;
    return;
  }

  // Create table for tiles
  sql = "CREATE TABLE tiledata (";
  sql += "tileid INTEGER,";
  sql += "tilearea REAL,";
  sql += "motorway REAL,";
  sql += "trunk REAL,";
  sql += "pmary REAL,";
  sql += "secondary REAL,";
  sql += "tertiary REAL,";
  sql += "unclassified REAL,";
  sql += "residential REAL,";
  sql += "serviceother REAL";
  sql += ")";
  ret = sqlite3_exec(db_handle, sql.c_str(), NULL, NULL, &err_msg);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Error: " + std::string(err_msg));
    sqlite3_free(err_msg);
    sqlite3_close(db_handle);
    return;
  }
  // Create table for countries
  sql = "CREATE TABLE countrydata (";
  sql += "isocode TEXT,";
  sql += "motorway REAL,";
  sql += "trunk REAL,";
  sql += "pmary REAL,";
  sql += "secondary REAL,";
  sql += "tertiary REAL,";
  sql += "unclassified REAL,";
  sql += "residential REAL,";
  sql += "serviceother REAL";
  sql += ")";
  ret = sqlite3_exec(db_handle, sql.c_str(), NULL, NULL, &err_msg);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Error: " + std::string(err_msg));
    sqlite3_free(err_msg);
    sqlite3_close(db_handle);
    return;
  }
  // Begin the prepared statements for tiledata
  ret = sqlite3_exec(db_handle, "BEGIN", NULL, NULL, &err_msg);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Error: " + std::string(err_msg));
    sqlite3_free(err_msg);
    sqlite3_close(db_handle);
    return;
  }
  sql = "INSERT INTO tiledata (tileid, tilearea, motorway, trunk, pmary, secondary, tertiary, unclassified, residential, serviceother) ";
  sql += "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
  ret = sqlite3_prepare_v2(db_handle, sql.c_str(), strlen (sql.c_str()), &stmt, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("SQL error: " + sql);
    LOG_ERROR(std::string(sqlite3_errmsg(db_handle)));
    sqlite3_close (db_handle);
    return;
  }

  // Fill DB with the tile statistics
  for (auto tileid : tile_ids) {
    uint8_t index = 1;
    sqlite3_reset(stmt);
    sqlite3_clear_bindings;
    sqlite3_bind_int(stmt, index, tileid);
    ++index;
    sqlite3_bind_double(stmt, index, tile_areas[tileid]);
    ++index;
    for (auto rclass : rclasses) {
      std::string roadStr = roadClassToString[rclass];
      sqlite3_bind_double(stmt, index, tile_maps[tileid][rclass]);
      ++index;
    }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW) {
      continue;
    }
    LOG_ERROR("sqlite3_step() error: " + std::string(sqlite3_errmsg(db_handle)));
  }
  sqlite3_finalize (stmt);
  ret = sqlite3_exec (db_handle, "COMMIT", NULL, NULL, &err_msg);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Error: " + std::string(err_msg));
    sqlite3_free (err_msg);
    sqlite3_close (db_handle);
    return;
  }

  // Begin the prepared statements for country data
  ret = sqlite3_exec(db_handle, "BEGIN", NULL, NULL, &err_msg);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Error: " + std::string(err_msg));
    sqlite3_free(err_msg);
    sqlite3_close(db_handle);
    return;
  }
  sql = "INSERT INTO countrydata (isocode, motorway, trunk, pmary, secondary, tertiary, unclassified, residential, serviceother) ";
  sql += "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";
  ret = sqlite3_prepare_v2(db_handle, sql.c_str(), strlen (sql.c_str()), &stmt, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("SQL error: " + sql);
    LOG_ERROR(std::string(sqlite3_errmsg(db_handle)));
    sqlite3_close (db_handle);
    return;
  }

  // Fill DB with the country statistics
  for (auto country : iso_codes) {
    uint8_t index = 1;
    sqlite3_reset(stmt);
    sqlite3_clear_bindings;
    sqlite3_bind_text(stmt, index, country.c_str(), country.length(), SQLITE_STATIC);
    ++index;
    for (auto rclass : rclasses) {
      std::string roadStr = roadClassToString[rclass];
      sqlite3_bind_double(stmt, index, country_maps[country][rclass]);
      ++index;
    }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW) {
      continue;
    }
    LOG_ERROR("sqlite3_step() error: " + std::string(sqlite3_errmsg(db_handle)));
  }
  sqlite3_finalize (stmt);
  ret = sqlite3_exec (db_handle, "COMMIT", NULL, NULL, &err_msg);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Error: " + std::string(err_msg));
    sqlite3_free (err_msg);
    sqlite3_close (db_handle);
    return;
  }
  sqlite3_close(db_handle);
}
}
}
