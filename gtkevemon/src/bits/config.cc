#include <stdint.h>
#include <iostream>
#include <string>

#include "util/os.h"

#include "argumentsettings.h"
#include "defines.h"
#include "config.h"

/* Default configuration is always loaded. The application
 * can rely on these keys to be there all the time even if
 * the values are not in the configuration file. */
char const* default_config =
    "[accounts]\n"
    "[characters]\n"
    "[evetime]\n"
    "  valid = false\n"
    "  difference = 0\n"
    "  time_format = %Y-%m-%d %H:%M:%S\n"
    "  time_short_format = %m-%d %H:%M\n"
    "[network]\n"
    "  use_proxy = false\n"
    "  proxy_address = \n"
    "  proxy_port = 80\n"
    "  api_ssl = true\n"
    "[notifications]\n"
    "  show_popup_dialog = true\n"
    "  show_tray_icon = false\n"
    "  show_info_bar = true\n"
    "  exec_handler = false\n"
    "  exec_command = mail your@mail.org -s \"$CHAR finished $SKILL $LEVEL\"\n"
    "  exec_data = Congratulations, $CHAR finished $SKILL $LEVEL$NL$NLStatistics about the skill:$NLStart SP: $STARTSP$NLDestination SP: $DESTSP$NLAcquired SP: $NEWSP$NLSP/h: $SPPH$NLTotal training time: $TRAINTIME$NL$NLThis message was generated by GtkEveMon, $LOCALTIME\n"
    "  minimum_sp = 16000\n"
    "[planner]\n"
    "  columns_format = +0 +1 +2 -3 -4 -5 -6 -7 -8 -9\n"
    "  current_plan = \n"
    "  gui_dimension = 800x550\n"
    "  pane_position = 250\n"
    "[servermonitor]\n"
    "[settings]\n"
    "  auto_update_sheets = true\n"
    "  detailed_tray_tooltip = true\n"
    "  eve_command = eve\n"
    "  eve_command_2 = \n"
    "  eve_command_3 = \n"
    "  eve_command_4 = \n"
    "  eve_command_5 = \n"
    "  minimize_on_close = false\n"
    "  startup_servercheck = true\n"
    "  tray_usage = never\n"
    "  trunc_corpname = false\n"
    "  verbose_wintitle = true\n"
    "[skillqueue]\n"
    "  columns_format = +0 +1 -2 -3 -4 +5 -6 +7\n"
    "[versionchecker]\n"
    "  enabled = true\n"
    "  raise_updater = true\n"
    "[versionchecker.datafiles]\n"
    "  CertificateTree.xml.gz = \n"
    "  SkillTree.xml.gz = \n"
    "[versionchecker.last_seen]\n";

/* The initial configuration is loaded once if the configuration
 * file is created for the first time. Thus it initializes the
 * configuration but the values can be deleted from the config. */
char const* initial_config =
    "[servermonitor]\n"
    "  0_Tranquility = 87.237.38.200\n"
    "  1_Serenity = 61.129.54.100\n"
    "  2_Singularity = 87.237.38.50\n";

/* ---------------------------------------------------------------- */

Conf Config::conf;
std::string Config::conf_dir;
std::string Config::filename;

/* ---------------------------------------------------------------- */

void
Config::init_defaults (void)
{
  Config::conf.add_from_string(default_config);
}

/* ---------------------------------------------------------------- */

void
Config::init_config_path (void)
{
  /* The first part here determines the user's config directory.
   * If there is no home directory or any other error occures,
   * the current directory or even worse, /tmp is used. */
  std::string user_conf_dir;
  if (!ArgumentSettings::config_dir.empty())
  {
    user_conf_dir = ArgumentSettings::config_dir;
  }
  else
  {
    char const* homedir = OS::get_default_home_path();
    if (homedir != 0)
    {
      user_conf_dir = homedir;
      user_conf_dir += "/" CONF_HOME_DIR;
    }
    else
    {
      std::cout << "Warning: Couldn't determine home directry!" << std::endl;
      char buffer[512];
      char const* ret = OS::getcwd(buffer, 512);
      if (ret != 0)
        user_conf_dir = buffer;
      else
      {
        std::cout << "Warning: Couldn't even determine CWD!" << std::endl;
        user_conf_dir = OS_TEMP_DIR;
      }
    }
  }

  //std::cout << "Resolved config directory to: " << user_conf_dir << std::endl;

  /* Check if the GtkEveMon directory exists in the home directry. */
  //int dir_exists = ::access(user_conf_dir.c_str(), F_OK);
  bool dir_exists = OS::dir_exists(user_conf_dir.c_str());
  if (!dir_exists)
  {
    /* Directory does not exists. Create it. */
    std::cout << "Creating config directory: " << user_conf_dir << std::endl;
    int ret = OS::mkdir(user_conf_dir.c_str());
    if (ret < 0)
    {
      std::cout << "Error: Couldn't create the config directory!" << std::endl;
      std::cout << "Error: Falling back to " OS_TEMP_DIR << std::endl;
      user_conf_dir = OS_TEMP_DIR;
    }
  }

  /* Set the filename, make it official. */
  Config::conf_dir = user_conf_dir;
  Config::filename = user_conf_dir + "/gtkevemon.conf";
}

/* ---------------------------------------------------------------- */

void
Config::init_user_config (void)
{
  if (Config::conf_dir.empty() || Config::filename.empty())
    throw Exception("Error: Location of user configuration not available");

  /* Check if the config file is in-place. If not, dump the current
   * default configuration to file. */
  //int conf_exists = ::access(Config::filename.c_str(), F_OK);
  bool conf_exists = OS::file_exists(Config::filename.c_str());
  if (!conf_exists)
  {
    std::cout << "Creating initial config file: gtkevemon.conf" << std::endl;
    Config::conf.add_from_string(initial_config);
    Config::conf.to_file(Config::filename);
  }
  else
  {
    /* Now read the config file. If it's for some reason not there,
     * it will be created when GtkEveMon exits. */
    Config::conf.add_from_file(Config::filename);
  }
}

/* ---------------------------------------------------------------- */

void
Config::setup_http (AsyncHttp* fetcher, bool is_api_call)
{
  fetcher->set_agent("GtkEveMon");

  ConfSectionPtr section = Config::conf.get_section("network");
  bool use_proxy = section->get_value("use_proxy")->get_bool();
  bool use_ssl = section->get_value("api_ssl")->get_bool();

  if (use_proxy)
  {
    std::string proxy_host = section->get_value("proxy_address")->get_string();
    int proxy_port = section->get_value("proxy_port")->get_int();
    fetcher->set_proxy(proxy_host, (uint16_t)proxy_port);
  }

  if (is_api_call && use_ssl)
  {
    fetcher->set_use_ssl(true);
    fetcher->set_port(443);
  }
}
