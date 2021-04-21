#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <boost/program_options.hpp>
#include <condition_variable>
#include <libdevcrypto/Common.h>

#include "taraxad_version.hpp"
#include "common/static_init.hpp"
#include "node/full_node.hpp"

using namespace taraxa;
using namespace std;

struct GenericOptions {
    bool debug_;
};

struct LsCommand : public GenericOptions {
    bool hidden_;
    std::string path_;
};

struct ChmodCommand : public GenericOptions {
    bool recurse_;
    std::string perms_;
    std::string path_;
};

typedef boost::variant<LsCommand, ChmodCommand> Command;

Command ParseOptions(int argc, const char *argv[])
{
    namespace po = boost::program_options;

    po::options_description global("Global options");
    global.add_options()
        ("debug", "Turn on debug output")
        ("command", po::value<std::string>(), "command to execute")
        ("subargs", po::value<std::vector<std::string> >(), "Arguments for command");

    po::positional_options_description pos;
    pos.add("command", 1).
        add("subargs", -1);

    po::variables_map vm;

    po::parsed_options parsed = po::command_line_parser(argc, argv).
        options(global).
        positional(pos).
        allow_unregistered().
        run();

    po::store(parsed, vm);
    
    std::string cmd = vm["command"].as<std::string>();

    if (cmd == "ls")
    {
        // ls command has the following options:
        po::options_description ls_desc("ls options");
        ls_desc.add_options()
            ("hidden", "Show hidden files")
            ("path", po::value<std::string>(), "Path to list");

        // Collect all the unrecognized options from the first pass. This will include the
        // (positional) command name, so we need to erase that.
        std::vector<std::string> opts = po::collect_unrecognized(parsed.options, po::include_positional);
        opts.erase(opts.begin());

        // Parse again...
        po::store(po::command_line_parser(opts).options(ls_desc).run(), vm);

        LsCommand ls;
        ls.debug_ = vm.count("debug");
        ls.hidden_ = vm.count("hidden");
        ls.path_ = vm["path"].as<std::string>();

        return ls;
    }
    else if (cmd == "chmod")
    {
        // Something similar
    }

    // unrecognised command
    throw po::invalid_option_value(cmd);
}

namespace bpo = boost::program_options;

void printInfo() {
  cout << "NAME:\n" << "\ttaraxad - Taraxa blockchain full node implementation" << endl;
  cout << "USAGE:\n" << "\ttaraxad command [command options]" << endl;
  cout << "VERSION:\n" << "\t" << TARAXAD_VERSION << endl;
}

string getHomeDir() {
  struct passwd *pw = getpwuid(getuid());
  const char *homedir = pw->pw_dir;
  
  return string(homedir);
}

string getTaraxaDefaultDir() {
  string home_dir = getHomeDir();
  auto taraxa_dir = home_dir + "/.taraxa";
  fs::create_directory(taraxa_dir);
  
  return taraxa_dir;
}

string getAccountFile() {
  return getTaraxaDefaultDir() + "/account.json";
}

string getVrfFile() {
  return getTaraxaDefaultDir() + "/vrf.json";
}

bool fileExist(string file) {
  ifstream ifile;
  ifile.open(file);
  if (ifile) {
    ifile.close();
    return true;
  } else {
    return false;
  }  
}

bool writeFile(string file_name, Json::Value json) {
  ofstream ofile(file_name);
  if (ofile.is_open()) {
    ofile << json;
    return true;
  } else {
    cerr << "Cannot open file " << file_name << endl;
    return false;
  }  
}

Json::Value getAccountJson(KeyPair account) {
  Json::Value json(Json::objectValue);
  json["node_secret"] = toHex(account.secret().ref());
  json["node_public"] = account.pub().toString();
  json["node_address"] = account.address().toString();

  return json;
}

Json::Value getVrfJson(taraxa::vrf_wrapper::vrf_sk_t sk, taraxa::vrf_wrapper::vrf_pk_t pk) {
  Json::Value json(Json::objectValue);
  json["vrf_secret"] = sk.toString();
  json["vrf_public"] = pk.toString();

  return json;
}

Json::Value getAccountSecretKey() {
  auto account_file = getAccountFile();
  ifstream ifile(account_file);
  if (ifile.is_open()) {
    Json::Value account_json;
    ifile >> account_json;
    ifile.close();
    return account_json["node_secret"];
  } else {
    cerr << "Cannot open file " << account_file << endl;
    exit(1);
  }
}

Json::Value getVrfSecretKey() {
  auto vrf_file = getVrfFile();
  ifstream ifile(vrf_file);
  if (ifile.is_open()) {
    Json::Value vrf_json;
    ifile >> vrf_json;
    ifile.close();
    return vrf_json["vrf_secret"];
  } else {
    cerr << "Cannot open file " << vrf_file << endl;
    exit(1);
  }
}

Json::Value getNetworkBootNodes(int network_identifier) {
  Json::Value network_boot_nodes(Json::arrayValue);

  switch (network_identifier) {
    case 1: {
      cout << "Mainnet has not supported yet" << endl;
      exit(1);
    }
    case 2: {
      Json::Value boot_node1(Json::objectValue);
      boot_node1["id"] = "1105725ff38294f42197a3851c87631610956404678a25f90da5f989bb590413a62665cadde1b820de3e9098367909110bba6d62d248444ce45547a703bd8d10";
      boot_node1["ip"] = "104.154.116.107";
      boot_node1["tcp_port"] = 10002;
      boot_node1["udp_port"] = 10002;
      network_boot_nodes.append(boot_node1);
      Json::Value boot_node2(Json::objectValue);
      boot_node2["id"] = "9869d8c263ce97d2e32c7ce5e746a8bab53227ccf279f850bbf9def1ddb7fbc08dfb29e8f1ceb93c5b83d08e06618e0993c6db7e55caa05a44605bdb74fbb0da";
      boot_node2["ip"] = "35.238.25.204";
      boot_node2["tcp_port"] = 10002;
      boot_node2["udp_port"] = 10002;
      network_boot_nodes.append(boot_node2);
      Json::Value boot_node3(Json::objectValue);
      boot_node3["id"] = "d7c8b4a8cc43549cdbb0f40c090559ae5d2584ba25aed95913f10fae9bceb17421f3250ee95126f33a77f679f4c7e806a0a9e643aa5aaec723f9e3155e85a820";
      boot_node3["ip"] = "34.66.134.211";
      boot_node3["tcp_port"] = 10002;
      boot_node3["udp_port"] = 10002;
      network_boot_nodes.append(boot_node3);      
      break;
    }
    case 3: {
      cout << "Devnet has not supported yet" << endl;
      exit(1);
    }
    default: {
      cerr << "Wrong network identifier " << network_identifier << " (1=Mainnet, 2=Testnet, 3=Devnet)" << endl;
      exit(1);
    }
  }

  return network_boot_nodes;
}

Json::Value generateLogging() {
  Json::Value logging(Json::objectValue);
  auto &configurations = logging["configurations"] = Json::Value(Json::arrayValue);

  Json::Value standard(Json::objectValue);
  standard["name"] = "standard";
  standard["on"] = true;
  standard["verbosity"] = "INFO";

  auto &channels = standard["channels"] = Json::Value(Json::arrayValue);
  Json::Value full_node(Json::objectValue);
  full_node["name"] = "FULLND";
  full_node["verbosity"] = "INFO";
  channels.append(full_node);
  Json::Value pbft_mgr(Json::objectValue);
  pbft_mgr["name"] = "PBFT_MGR";
  pbft_mgr["verbosity"] = "INFO";
  channels.append(pbft_mgr);

  auto &outputs = standard["outputs"] = Json::Value(Json::arrayValue);
  Json::Value console(Json::objectValue);
  console["type"] = "console";
  console["format"] = "%NodeId% %Channel% [%TimeStamp%] %SeverityStr%: %Message%";
  outputs.append(console);
  Json::Value file(Json::objectValue);
  file["type"] = "file";
  file["file_name"] = "Taraxa_N1_%m%d%Y_%H%M%S_%5N.log";
  file["rotation_size"] = 10000000;
  file["time_based_rotation"] = "0,0,0";
  file["format"] = "%NodeId% %Channel% [%TimeStamp%] %SeverityStr%: %Message%";
  file["max_size"] = 1000000000;
  outputs.append(file);

  configurations.append(standard);

  return logging;
}

Json::Value generateConfigFile(string &db_path, int network_identifier) {
  Json::Value config_file(Json::objectValue);

  // Account
  auto account_file = getAccountFile();
  if (!fileExist(account_file)) {
    auto account = dev::KeyPair::create();
    auto account_json = getAccountJson(account);

    // Create account file
    auto account_file = getAccountFile();
    if (!writeFile(account_file, account_json)) {
      exit(1);
    }
    // config_file["node_secret"] = toHex(account.secret().ref());
  }
  // config_file["node_secret"] = getAccountSecretKey();
  
  // VRF
  auto vrf_file = getVrfFile();
  if (!fileExist(vrf_file)) {
    auto [pk, sk] = taraxa::vrf_wrapper::getVrfKeyPair();
    auto json = getVrfJson(sk, pk);

    // Create VRF file
    if (!writeFile(vrf_file, json)) {
      exit(1);
    }
    // config_file["vrf_secret"] = sk.toString();
  }
  // config_file["vrf_secret"] = getVrfSecretKey();

  // DB
  if (db_path.empty()) {
    db_path = getTaraxaDefaultDir();
  }
  config_file["db_path"] = db_path;

  config_file["network_is_boot_node"] = true;
  config_file["network_address"] = "0.0.0.0";
  config_file["network_tcp_port"] = 10002;
  config_file["network_udp_port"] = 10002;
  config_file["network_simulated_delay"] = 0;
  config_file["network_transaction_interval"] = 100;
  config_file["network_encrypted"] = 1;
  config_file["network_bandwidth"] = 40;
  config_file["network_ideal_peer_count"] = 10;
  config_file["network_max_peer_count"] = 50;
  config_file["network_sync_level_size"] = 4;
  config_file["network_boot_nodes"] = getNetworkBootNodes(network_identifier);

  auto &rpc = config_file["rpc"] = Json::Value(Json::objectValue);
  rpc["http_port"] = 7777;
  rpc["ws_port"] = 8777;
  rpc["threads_num"] = 10;

  auto &test_params = config_file["test_params"] = Json::Value(Json::objectValue);
  test_params["max_transaction_queue_warn"] = 0;
  test_params["max_transaction_queue_drop"] = 0;
  test_params["max_block_queue_warn"] = 0;
  test_params["db_snapshot_each_n_pbft_block"] = 100;
  test_params["db_max_snapshots"] = 5;
  auto &block_proposer = test_params["block_proposer"] = Json::Value(Json::objectValue);
  block_proposer["shard"] = 1;
  block_proposer["transaction_limit"] = 0;

  config_file["logging"] = generateLogging();

  return config_file;
}

int main(int argc, const char* argv[]) {
  // ParseOptions(argc, argv);
  // return 1;

  static_init();

  try {
    string account_sk;
    string vrf_sk;
    string config_file;
    string db_path;
    int network_identifier;

    string conf_taraxa;
    string conf_chain;
    bool destroy_db = 0;
    bool rebuild_network = 0;
    bool rebuild_db = 0;
    uint64_t rebuild_db_period = 0;
    uint64_t revert_to_period = 0;

    bpo::options_description command("COMMANDS");
    command.add_options()
      ("node", "Runs the actual node (default)")
      ("account", "Generate new account")
      ("account-secret-key", bpo::value<string>(&account_sk), "Generate account by secret key")
      ("vrf", "Generate new VRF keys")
      ("vrf-secret-key", bpo::value<string>(&vrf_sk), "Generate VRF public key")
      ("version", "Print version of taraxd");
    bpo::options_description user_options("USER OPTIONS");
    user_options.add_options()
        ("help", "Print this help message and exit")
        ("config", bpo::value<string>(&config_file), "JSON configuration file - default at ~/.taraxa/config.json")
        ("datadir", bpo::value<string>(&db_path), "Database directory - default at ~/.taraxa/db")
        ("networkId", bpo::value<int>(&network_identifier)->default_value(2), "Network identifier (integer, 1=Mainnet, 2=Testnet, 3=Devnet) (default: 2)");
        // ("version", "Print version of taraxd")
        // ("conf_taraxa", bpo::value<string>(&conf_taraxa), "Config for taraxa node (either json file path or inline json) [required]")
    bpo::options_description dev_options("DEVELOPER OPTIONS");
    dev_options.add_options()
        ("destroy_db", bpo::bool_switch(&destroy_db), "Destroys all the existing data in the database")
        ("rebuild_db", bpo::bool_switch(&rebuild_db), 
            "Reads the raw dag/pbft blocks from the db and executes all the blocks from scratch rebuilding all the other "
            "database tables - this could take a long time")
        ("rebuild_db_period", bpo::value<uint64_t>(&rebuild_db_period),
            "Use with rebuild_db - Rebuild db up to a specified period")
        ("rebuild_network", bpo::bool_switch(&rebuild_network), "Delete all saved network/nodes information and rebuild network from boot nodes")
        ("revert_to_period", bpo::value<uint64_t>(&revert_to_period),
            "Revert db/state to specified period (specify period) ");
    bpo::options_description allowed_options;
    allowed_options.add(command);
    allowed_options.add(user_options);
    allowed_options.add(dev_options);
    bpo::variables_map option_vars;
    auto parsed_line = bpo::parse_command_line(argc, argv, allowed_options);
    bpo::store(parsed_line, option_vars);
    bpo::notify(option_vars);
    // TODO: remove
    // cout << "argc " << argc << endl;
    // cout << "argv " << argv << endl;
    // for (const auto& it : option_vars) {
    //   std::cout << "key: " << it.first.c_str() << " ";
    //   auto& value = it.second.value();
    //   if (auto v = boost::any_cast<uint32_t>(&value))
    //     std::cout << "value: " << *v << endl;
    //   else if (auto v = boost::any_cast<std::string>(&value))
    //     std::cout << "value " << *v << endl;
    //   else
    //     std::cout << "error" << endl;
    // }

    if (option_vars.count("help")) {
      printInfo();
      cout << allowed_options << endl;
      return 1;
    }

    if (option_vars.count("version")) {
      cout << TARAXAD_VERSION << endl;
      return 1;
    }

    if (option_vars.count("account")) {
      auto account_file = getAccountFile();
      if (fileExist(account_file)) {
        cout << "Account file exist at " << account_file << endl;
        return 1;
      }

      auto account = dev::KeyPair::create();
      auto json = getAccountJson(account);
      if (!writeFile(account_file, json)) {
        return 1;
      }
      // account_sk = toHex(account.secret().ref());

      cout << "Generate account file at " << account_file << endl;
      cout << "Please save the secret key at a safe place. Account secret key: " << toHex(account.secret().ref()) << endl;
      cout << "Account public key: " << account.pub() << endl;
      cout << "Account address: " << account.address() << endl;
      return 1;
    }
    if (option_vars.count("account-secret-key")) {
      auto account_file = getAccountFile();
      if (fileExist(account_file)) {
        cout << "Account file exist at " << account_file << endl;
        return 1;
      }

      auto sk = dev::Secret(account_sk, dev::Secret::ConstructFromStringType::FromHex);
      auto account = dev::KeyPair(sk);
      auto json = getAccountJson(account);
      if (!writeFile(account_file, json)) {
        return 1;
      }

      cout << "Generate account file at " << account_file << endl;
      cout << "Please save the secret key at a safe place. Account secret key: " << toHex(account.secret().ref()) << endl;
      cout << "Account public key: " << account.pub() << endl;
      cout << "Account address: " << account.address() << endl;
      return 1;
    }

    if (option_vars.count("vrf")) {
      auto vrf_file = getVrfFile();
      if (fileExist(vrf_file)) {
        cout << "VRF file exist at " << vrf_file << endl;
        return 1;
      }

      auto [pk, sk] = taraxa::vrf_wrapper::getVrfKeyPair();
      auto json = getVrfJson(sk, pk);
      if (!writeFile(vrf_file, json)) {
        return 1;
      }
      // vrf_sk = sk.toString();

      cout << "Generate VRF file at " << vrf_file << endl;
      cout << "Please save the secret key at a safe place. VRF secret key: " << sk << endl;
      cout << "VRF public key: " << pk << endl;
      return 1;
    }
    if (option_vars.count("vrf-secret-key")) {
      auto vrf_file = getVrfFile();
      if (fileExist(vrf_file)) {
        cout << "VRF file exist at " << vrf_file << endl;
        return 1;
      }

      auto sk = taraxa::vrf_wrapper::vrf_sk_t(vrf_sk);
      auto pk = taraxa::vrf_wrapper::getVrfPublicKey(sk);
      auto json = getVrfJson(sk, pk);
      if (!writeFile(vrf_file, json)) {
        return 1;
      }

      cout << "Generate VRF file at " << vrf_file << endl;
      cout << "Please save the secret key at a safe place. VRF secret key: " << sk << endl;
      cout << "VRF public key: " << pk << endl;
      return 1;
    }

    // if (option_vars.count("node")) {
    if (config_file.empty()) {
      config_file = getTaraxaDefaultDir() + "/config.json";
    }
    if (fileExist(config_file)) {
      cout << "Config file is exist at " << config_file << endl;
    } else {
      ofstream ofile(config_file);
      if (ofile.is_open()) {
        auto config = generateConfigFile(db_path, network_identifier);
        ofile << config;
        ofile.close();
      } else {
        cerr << "Cannot open config file " << config_file << endl;
        return 1;
      }
    }
      // return 1;
    // }

    // if (!option_vars.count("conf_taraxa")) {
    //   cerr << "Please specify full node configuration file "
    //           "[--conf_taraxa]..."
    //        << endl;
    //   return 1;
    // }

    // Loads config
    // FullNodeConfig cfg(conf_taraxa, conf_chain);
    FullNodeConfig cfg(config_file, conf_chain);

    // Validates config values
    if (!cfg.validate()) {
      cerr << "Invalid configration. Please make sure config values are valid";
      return 1;
    }

    cfg.node_secret = getAccountSecretKey().asString();
    cfg.vrf_secret = vrf_wrapper::vrf_sk_t(getVrfSecretKey().asString());
    cout << "Account secret key: " << cfg.node_secret << endl;
    cout << "VRF secret key: " << cfg.vrf_secret.toString() << endl;

    if (destroy_db) {
      fs::remove_all(cfg.db_path);
    }
    if (rebuild_network) {
      fs::remove_all(cfg.net_file_path());
    }
    cfg.test_params.db_revert_to_period = revert_to_period;
    cfg.test_params.rebuild_db = rebuild_db;
    cfg.test_params.rebuild_db_period = rebuild_db_period;
    FullNode::Handle node(cfg, true);
    if (node->isStarted()) {
      cout << "Taraxa node started" << endl;
      // TODO graceful shutdown
      mutex mu;
      unique_lock l(mu);
      condition_variable().wait(l);
    }
    cout << "Taraxa Node exited ..." << endl;
    return 0;
  } catch (taraxa::ConfigException const& e) {
    cerr << "Configuration exception: " << e.what() << endl;
  } catch (...) {
    cerr << boost::current_exception_diagnostic_information() << endl;
  }
  return 1;
}
