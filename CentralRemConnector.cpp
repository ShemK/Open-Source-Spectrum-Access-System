#include "CentralRemConnector.hpp"

#define DEBUG 0
#if DEBUG == 1 || DEBUG > 2
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...) /*__VA_ARGS__*/
#endif

CentralRemConnector::CentralRemConnector(std::string dbname, std::string user, std::string password, std::string addr)
{
  this->dbname = dbname;
  this->user = user;
  this->password = password;
  this->addr = addr;
  db_connection = false;
}

CentralRemConnector::~CentralRemConnector()
{
  if (db_connection)
  {
    db_handler->disconnect();
    dprintf("Disconnected from database\n");
  }
  delete db_handler;
  dprintf("Connector Destroyed\n");
}

bool CentralRemConnector::connect()
{
  std::string dbstring = "dbname = " + dbname + " user = " + user + " password = " + password + " hostaddr = " + addr + " port = 5432";
  try
  {
    db_handler = new pqxx::connection(dbstring);
    if (db_handler->is_open())
    {
      dprintf("Connected to database\n");
      db_connection = true;
      prepareKnownAttributes();
      return true;
    }
    else
    {
      return false;
    }
  }
  catch (const std::exception &e)
  {
    dprintf("Connection to database failed\n");
    return false;
  }
}

void CentralRemConnector::prepareKnownAttributes()
{
  pqxx::work worker(*db_handler);
  std::string query = "SELECT nodeID from NodeInfo;";
  pqxx::result r = worker.exec(query);

  if (r.size() > 0)
  {
    for (auto row : r)
    {
      int tempID = row[0].as<int>();
      known_nodes.push_back(tempID);
      node_info temp;
      temp.nodeID = tempID;
      node_info_vector.push_back(temp);
    }
  }
  dprintf("Known Nodes: \n");
  for (int i = 0; i < known_nodes.size(); i++)
  {
    dprintf("NodeID: %d\n", known_nodes.at(i));
  }
  worker.commit();
}

void CentralRemConnector::analyze(const char *recv_buffer, int recv_len)
{
  pqxx::work worker(*db_handler);
  std::string received_string;
  pmt::pmt_t nodeParam;
  float occ, center_freq;
  if (recv_len > 0)
  {
    for (int i = 0; i < recv_len; i++)
    {
      received_string.push_back(recv_buffer[i]);
    }

    pmt_t received_dict = pmt::deserialize_str(received_string);
    // std::cout << received_dict << "\n";
    pmt_t key_list = pmt::dict_keys(received_dict);
    pmt::pmt_t not_found = pmt::mp(0);

    nodeParam = dict_ref(received_dict, pmt::string_to_symbol("NodeParam"), not_found);

    if (nodeParam != not_found)
    {
      pmt_t attributes = pmt::dict_ref(nodeParam, pmt::string_to_symbol("attributes"), not_found);

      pmt::pmt_t nodeID = pmt::dict_ref(attributes, pmt::string_to_symbol("nodeID"), not_found);

      if (nodeID != not_found)
      {
        if (!nodeKnown(pmt::to_long(nodeID)))
        {
          std::string query = insert(attributes, "NodeInfo");
          worker.exec(query);
          //  worker.commit();
        }
      }
    }


    for (size_t i = 0; i < pmt::length(key_list); i++)
    {
      std::string query;
      pmt_t individual_dict = pmt::dict_ref(received_dict, pmt::nth(i, key_list), not_found);
      if (pmt::symbol_to_string(pmt::nth(i, key_list)) == "INSERT")
      {
        pmt_t attributes = pmt::dict_ref(individual_dict, pmt::string_to_symbol("attributes"), not_found);
        std::string table = pmt::symbol_to_string(pmt::dict_ref(individual_dict, pmt::string_to_symbol("table"), not_found));
        query = insert(attributes, table);
        center_freq = pmt::to_double(pmt::dict_ref(attributes, pmt::intern("center_freq"), not_found));
        float bandwidth = pmt::to_double(pmt::dict_ref(attributes, pmt::intern("bandwidth"), not_found));
        int nodeID = pmt::to_long(pmt::dict_ref(attributes, pmt::intern("nodeid"), not_found));
        pmt::pmt_t noise_pmt = pmt::dict_ref(attributes, pmt::intern("noise_floor"), not_found);
        double noise_floor;
        pmt::pmt_t occ_pmt = pmt::dict_ref(attributes, pmt::intern("occ"), not_found);
        if(pmt::is_number(occ_pmt)){
          //printf("Number\n" );
          occ = pmt::to_double(occ_pmt);

          //    pushData(occ,center_freq,nodeID,bandwidth,&worker);
        }
        else if (pmt::is_f32vector(occ_pmt))
        {
          //printf("vector\n" );

          std::vector<float> occ_vector = pmt::f32vector_elements(occ_pmt);
          pushData(occ_vector,center_freq,nodeID,bandwidth,&worker);
          /*
          for(unsigned int i = 0; i < occ_vector.size();i++){
          occ = occ_vector[i];
          pushData(occ,center_freq,nodeID,bandwidth,&worker);
        }
        */
      }


      if(pmt::is_number(noise_pmt)){
        //printf("Number\n" );
        noise_floor = pmt::to_double(noise_pmt);
      }
      else if (pmt::is_f32vector(noise_pmt))
      {
        //printf("vector\n" );

        std::vector<float> noise_vector = pmt::f32vector_elements(noise_pmt);
      }
      //std::cout << "Noise Floor: " << noise_floor<< std::endl;

    }
    else if (pmt::symbol_to_string(pmt::nth(i, key_list)) == "UPDATE")
    {
      pmt_t attributes = pmt::dict_ref(individual_dict, pmt::string_to_symbol("attributes"), not_found);
      pmt_t conditions = pmt::dict_ref(individual_dict, pmt::string_to_symbol("conditions"), not_found);
      std::string table = pmt::symbol_to_string(pmt::dict_ref(individual_dict, pmt::string_to_symbol("table"), not_found));
      //query = update(attributes, conditions, table);
    }
    try{
      //worker.exec(query);
      db_connection = true;
    }
    catch (const std::exception &e){
      printf("Error writing to database\n");
      std::cerr << e.what();
      db_connection = false;
    }

  }
  if(db_connection) {
    worker.commit();
  }
}
}
std::string CentralRemConnector::insert(pmt::pmt_t dict, std::string table)
{
  std::string query = "INSERT INTO " + table + " ";
  pmt_t key_list = pmt::dict_keys(dict);
  std::string columns = "(";
  std::string values = "(";
  int size = pmt::length(key_list);
  pmt::pmt_t not_found = pmt::mp(0);
  for (size_t i = 0; i < size; i++)
  {
    columns = columns + pmt::symbol_to_string(pmt::nth(i, key_list));
    pmt_t temp = pmt::dict_ref(dict, pmt::nth(i, key_list), not_found);
    if (pmt::is_number(temp))
    {
      if (pmt::is_integer(temp))
      {
        values = values + "'" + std::to_string(pmt::to_long(temp)) + "'";
      }
      else
      {
        values = values + "'" + std::to_string(pmt::to_double(temp)) + "'";
      }
    }
    else if (pmt::is_f32vector(temp))
    {
      std::vector<float> value_vector = pmt::f32vector_elements(temp);
      values = values + "'{";
      for (int i = 0; i < value_vector.size(); i++)
      {
        values = values + std::to_string(value_vector[i]);
        if (i != value_vector.size() - 1)
        {
          values = values + ",";
        }
        else
        {
          values = values + "}'";
        }
      }
    }
    else if (pmt::is_symbol(temp))
    {
      values = values + "'" + pmt::symbol_to_string(temp) + "'";
    }

    if (i != size - 1)
    {
      columns = columns + ",";
      values = values + ",";
    }
  }
  columns = columns + ")";
  values = values + ")";
  query = query + columns + " VALUES " + values + ";";
  dprintf("%s\n", query.c_str());
  return query;
}

std::string CentralRemConnector::update(pmt::pmt_t attributes, pmt::pmt_t conditions, std::string table)
{
  std::string query = "UPDATE " + table + " SET ";
  pmt_t key_list = pmt::dict_keys(attributes);
  int size = pmt::length(key_list);
  pmt::pmt_t not_found = pmt::mp(0);

  // setting attributes
  for (size_t i = 0; i < size; i++)
  {
    pmt_t temp = pmt::dict_ref(attributes, pmt::nth(i, key_list), not_found);
    if (pmt::is_number(temp))
    {
      if (pmt::is_integer(temp))
      {
        query = query + pmt::symbol_to_string(pmt::nth(i, key_list)) + " = '" + std::to_string(pmt::to_long(temp)) + "'";
      }
      else
      {
        query = query + pmt::symbol_to_string(pmt::nth(i, key_list)) + " = '" + std::to_string(pmt::to_double(temp)) + "'";
      }
    }
    else if (pmt::is_f32vector(temp))
    {
      query = query + pmt::symbol_to_string(pmt::nth(i, key_list)) + " = ";
      std::vector<float> value_vector = pmt::f32vector_elements(temp);
      query = query + "'{";
      for (int i = 0; i < value_vector.size(); i++)
      {
        query = query + std::to_string(value_vector[i]);
        if (i != value_vector.size() - 1)
        {
          query = query + ",";
        }
        else
        {
          query = query + "}'";
        }
      }
    }
    else if (pmt::is_symbol(temp))
    {
      query = query + pmt::symbol_to_string(pmt::nth(i, key_list)) + " = '" + pmt::symbol_to_string(temp) + "'";
    }

    if (i != size - 1)
    {
      query = query + ",";
    }
  }

  // setting conditions
  if (!pmt::equal(conditions, pmt::mp(0)))
  {
    key_list = pmt::dict_keys(conditions);
    size = pmt::length(key_list);
    query = query + " WHERE ";
    for (size_t i = 0; i < size; i++)
    {
      pmt_t temp = pmt::dict_ref(conditions, pmt::nth(i, key_list), not_found);
      if (pmt::is_number(temp))
      {
        if (pmt::is_integer(temp))
        {
          query = query + pmt::symbol_to_string(pmt::nth(i, key_list)) + " = '" + std::to_string(pmt::to_long(temp)) + "'";
        }
        else
        {
          query = query + pmt::symbol_to_string(pmt::nth(i, key_list)) + " = '" + std::to_string(pmt::to_double(temp)) + "'";
        }
      }
      else if (pmt::is_f32vector(temp))
      {
      }
      else if (pmt::is_symbol(temp))
      {
        query = query + pmt::symbol_to_string(pmt::nth(i, key_list)) + " = '" + pmt::symbol_to_string(temp) + "'";
      }

      if (i != size - 1)
      {
        query = query + " AND ";
      }
    }
  }
  query = query + ";";
  dprintf("%s\n", query.c_str());
  return query;
}

// TODO: Efficient search needed
bool CentralRemConnector::nodeKnown(int nodeID)
{
  for (int i = 0; i < known_nodes.size(); i++)
  {
    if (nodeID == known_nodes[i])
    {
      return true;
    }
  }
  known_nodes.push_back(nodeID);
  return false;
}

void CentralRemConnector::pushData(std::vector<float> occ_vector , double center_freq, int nodeID, float bandwidth,pqxx::work *worker)
{
  //  printf("NodeID %d \n",nodeID);
  //  printf("center_freq %d \n",center_freq);
  center_freq = round(center_freq/1e6)*1e6;
  for(int i = 0; i < node_info_vector.size(); i++) {
    if(nodeID == node_info_vector[i].nodeID) {
      //  printf("NodeID %d Found\n",nodeID);
      std::vector<float> decision = node_info_vector[i].decision_maker.getDecision(occ_vector,center_freq);
      if(decision.size() > 0){

        double frequency = node_info_vector[i].decision_maker.get_previous_center_frequency() - bandwidth/2;
        double start_freq = frequency/1e6;
        int channel_bw = round((bandwidth/1e6)/decision.size());
        if((channel_bw%2) > 0){
          channel_bw = (channel_bw + 1)*1e6;
        } else{
          channel_bw = channel_bw * 1e6;
        }

        if((((int)start_freq)%2) > 0){
          start_freq = (start_freq-1)*1e6;
        } else{
          start_freq= start_freq * 1e6;
        }

        std::cout << "Node ID: " << nodeID << "\n";
        std::cout << "Start Freq: " << start_freq << "\n";
        std::cout << "Center Freq: " << node_info_vector[i].decision_maker.get_previous_center_frequency() << "\n";
        std::cout << "bandwidth: " << bandwidth << "\n";
        printf("Channel BW: %d\n", channel_bw);
        std::cout << "Vector: ";
        for(int i=0; i<decision.size(); ++i){
          std::cout << decision[i] << " ";
        }
        std::cout <<  "\n";
        for(int k = 0; k < decision.size(); k++){
          std::string sql;
          double temp_freq = start_freq + (channel_bw*k);
          //    printf("Temp Freq: %f\n", temp_freq);
          //    std::cout << "Start Freq: " << temp_freq << "\n";
          for(int i = 0; i < (channel_bw/(2e6));i++) {
            sql = sql + "UPDATE channelinfo_" + std::to_string(nodeID) +" SET occ = "+ std::to_string(decision[k])
            + " WHERE startfreq = " + std::to_string(temp_freq+(2e6*i)) + ";";
          }
          /*
          std::cout << "\n------------------------------\n";
          std::cout << sql << std::endl;
          std::cout << "\n------------------------------\n";
          */
          parseData(decision[k],temp_freq,channel_bw,nodeID);
          try{
            worker->exec(sql);

          }catch (const std::exception &e){
            printf("Error writing to channel info database\n");
            std::cerr << e.what();
          }

        }
        //std::cout << "Cn"
        updateNodeActivity(nodeID,worker);
      }

    }
  }
}

void CentralRemConnector::parseData(double occ, double lowerFreq, double bandwidth, int nodeID){
  pmt::pmt_t info = pmt::make_dict();
  pmt::pmt_t key = pmt::string_to_symbol("nodeID");
  pmt::pmt_t value = pmt::mp(nodeID);
  info = pmt::dict_add(info,key,value);
  key = pmt::string_to_symbol("lowerFreq");
  value = pmt::from_double(lowerFreq);
  info = pmt::dict_add(info,key,value);
  key = pmt::string_to_symbol("occ");
  value = pmt::from_double(occ);
  info = pmt::dict_add(info,key,value);
  key = pmt::string_to_symbol("bandwidth");
  value = pmt::from_double(bandwidth);
  info = pmt::dict_add(info,key,value);
  informationParser.sendData(info);
}

void CentralRemConnector::updateNodeActivity(int nodeID,pqxx::work *worker){
  time_t now = time(0);
  struct tm  tstruct;
  char       buf[80];
  tstruct = *localtime(&now);
  strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
  std::string current_time(buf);

  std::string sql = "UPDATE nodeinfo SET last_active = '" + current_time + "' WHERE " +
  "nodeid = " + std::to_string(nodeID);

  std::cout << sql << std::endl;

  try{
    worker->exec(sql);

  }catch (const std::exception &e){
    printf("Error writing to channel info database\n");
    std::cerr << e.what();
  }
}
