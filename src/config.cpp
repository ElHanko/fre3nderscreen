#include "config.h"

#include <sys/stat.h>
#include <fstream>
#include <iomanip>

Config *Config::instance{NULL};

Config::Config() {
}

Config *Config::get_instance() {
  if (instance == NULL) {
    instance = new Config();
  }
  return instance;
}

void Config::init(std::string config_path, const std::string thumbdir) {
  path = config_path;
  struct stat buffer;

  json fans_conf = {
    {
      {"id", "output_pin fan0"},
      {"display_name", "Toolhead Fan"}
    },
    {
      {"id", "output_pin fan1"},
      {"display_name", "Back Fan"}
    },
    {
      {"id", "output_pin fan2"},
      {"display_name", "Side Fan"}
    }
  };

  json sensors_conf = {
    {
      {"id", "extruder"},
      {"display_name", "Extruder"},
      {"controllable", true},
      {"color", "red"}
    },
    {
      {"id", "heater_bed"},
      {"display_name", "Bed"},
      {"controllable", true},
      {"color", "purple"}
    },
    {
      {"id", "temperature_sensor chamber_temp"},
      {"display_name", "Chamber"},
      {"controllable", false},
      {"color", "blue"}
    }
  };

  json cooldown_conf = {{
    "cooldown",
    "SET_HEATER_TEMPERATURE HEATER=extruder TARGET=0\n"
    "SET_HEATER_TEMPERATURE HEATER=heater_bed TARGET=0"
  }};
  json default_macros_conf = {
    {"load_filament", "_GUPPY_LOAD_MATERIAL"},
    {"unload_filament", "_GUPPY_QUIT_MATERIAL"}
  };

  if (stat(config_path.c_str(), &buffer) == 0) {
    data = json::parse(std::fstream(config_path));
  } else {
    data = {
      {"log_path", "/usr/data/printer_data/logs/fre3nderscreen.log"},
      {"thumbnail_path", thumbdir},
      {"display_sleep_sec", 600},
      {"log_level", "debug"},
      {"moonraker_api_key", false},
      {"moonraker_host", "127.0.0.1"},
      {"moonraker_port", 17126},
      {"monitored_sensors", sensors_conf},
      {"fans", fans_conf},
      {"default_macros", default_macros_conf},
    };
  }

  data["config_path"] = config_path;

  auto &monitored_sensors = data["/monitored_sensors"_json_pointer];
  if (monitored_sensors.is_null()) {
    monitored_sensors = sensors_conf;
  }

  auto &fans = data["/fans"_json_pointer];
  if (fans.is_null()) {
    fans = fans_conf;
  }

  auto &default_macros = data["/default_macros"_json_pointer];
  if (default_macros.is_null()) {
    default_macros_conf.merge_patch(cooldown_conf);
    default_macros = default_macros_conf;
  } else if (!default_macros.contains("cooldown")) {
    default_macros.merge_patch(cooldown_conf);
  }

  auto &log_level = data["/log_level"_json_pointer];
  if (log_level.is_null()) {
    log_level = "debug";
  }

  auto &touch_calibrated = data["/touch_calibrated"_json_pointer];
  if (touch_calibrated.is_null()) {
    touch_calibrated = true;
  }

  auto &estop = data["/prompt_emergency_stop"_json_pointer];
  if (estop.is_null()) {
    estop = true;
  }

  auto &display_sleep = data["/display_sleep_sec"_json_pointer];
  if (display_sleep.is_null()) {
    display_sleep = 600;
  }

  std::ofstream o(config_path);
  o << std::setw(2) << data << std::endl;
}
std::string Config::get_thumbnail_path() {
  return get<std::string>("/thumbnail_path");
}

std::string Config::get_path() {
    return path;
}

json &Config::get_json(const std::string &json_path) {
  return data[json::json_pointer(json_path)];
}

void Config::save() {
  std::ofstream o(path);
  o << std::setw(2) << data << std::endl;
}
