// YANG Development Kit
// Copyright 2016 Cisco Systems. All rights reserved
//
////////////////////////////////////////////////////////////////
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
// -------------------------------------------------------------------
// This file has been modified by Yan Gorelik, YDK Solutions.
// All modifications in original under CiscoDevNet domain
// introduced since October 2019 are copyrighted.
// All rights reserved under Apache License, Version 2.0.
//////////////////////////////////////////////////////////////////

#include <libyang/libyang.h>

#include "common_utilities.hpp"
#include "entity_data_node_walker.hpp"
#include "errors.hpp"
#include "ietf_parser.hpp"
#include "logger.hpp"
#include "netconf_model_provider.hpp"
#include "netconf_ssh_client.hpp"
#include "netconf_tcp_client.hpp"
#include "types.hpp"
#include "ydk_yang.hpp"

using namespace std;

namespace ydk
{

namespace path
{

static path::SchemaNode* get_schema_for_operation(path::RootSchemaNode& root_schema, string yfilter);

static shared_ptr<path::Rpc> create_rpc_instance(path::RootSchemaNode & root_schema, string rpc_name);
static path::DataNode& create_rpc_input(path::Rpc & netconf_rpc);

static bool is_yang_1_1_supported(vector<string> & caps);
static bool is_candidate_supported(vector<string> capbilities);
static bool is_get_schema_supported(vector<string> capbilities);

static void create_input_target(path::DataNode & input, bool candidate_supported);
static void create_input_source(path::DataNode & input, bool config);
static void create_input_error_option(path::DataNode & input);
static string get_annotated_config_payload(path::RootSchemaNode & root_schema, path::Rpc & rpc, path::Annotation & annotation);
static string get_commit_rpc_payload();
static string get_caps_rpc_payload();
static shared_ptr<path::DataNode> handle_crud_edit_reply(string reply, NetconfClient & client, bool candidate_supported);

static string get_read_rpc_name(bool config);
static bool is_config(path::Rpc & rpc);
static string get_filter_payload(path::Rpc & ydk_rpc);
static string get_netconf_payload(path::DataNode & input, const string& data_tag, const string& data_value);
static void check_rpc_reply_for_error(const string& reply);
static void log_rpc_request(const string& payload);

shared_ptr<path::DataNode> handle_rpc_output(const string & reply, path::RootSchemaNode & root_schema, const string& rpc_path);
shared_ptr<path::DataNode> handle_action_output(const string & reply, path::RootSchemaNode & root_schema, const string& action_node_path);
string get_netconf_output(const string & reply);

const char* CANDIDATE = "urn:ietf:params:netconf:capability:candidate:1.0";
const string PROTOCOL_SSH = "ssh";
const string PROTOCOL_TCP = "tcp";

static bool is_netconf_get_rpc(path::Rpc & rpc);
static shared_ptr<path::DataNode> netconf_output_to_datanode(const string & data, path::RootSchemaNode & root_schema);
static string extract_rpc_output(const string & reply);
static string extract_rpc_data(const string & reply, const string & start_tag, const string & end_tag, bool is_first_tag=false);

NetconfSession::NetconfSession(path::Repository & repo,
                               const string& address,
                               const string& username,
                               const string& password,
                               int port,
                               const string& protocol,
                               bool on_demand,
                               int timeout)
{
    // Correct default settings
    if (port == 0)
        port = 830;

    initialize_client(address, username, password, port, protocol, timeout);
    initialize_repo(repo, on_demand);
    YLOG_INFO("Connected to {} on port {} using {} with timeout of {}", address, port, protocol, timeout);
}

NetconfSession::NetconfSession(const string& address,
                               const string& username,
                               const string& password,
                               int port,
                               const string& protocol,
                               bool on_demand,
                               bool common_cache,
                               int timeout)
{
    // Correct default settings
    if (port == 0)
        port = 830;

    initialize_client(address, username, password, port, protocol, timeout);
    auto caching_option = common_cache ? path::ModelCachingOption::COMMON : path::ModelCachingOption::PER_DEVICE;
    path::Repository repo(caching_option);
    initialize_repo(repo, on_demand);
    YLOG_INFO("Connected to {} on port {} using {} with timeout of {}", address, port, protocol, timeout);
}

// todo: decide whether or not to add TWO more signatures for
//       constructor to match above or just ONE more signature (go style)

NetconfSession::NetconfSession(path::Repository& repo,
                               const string& address,
                               const string& username,
                               const string& private_key_path,
                               const string& public_key_path,
                               int port,
                               bool on_demand,
                               int timeout)
{
    // Correct default settings
    if (port == 0)
        port = 830;

    initialize_client_with_key(address, username, private_key_path, public_key_path, port, timeout);
    initialize_repo(repo, on_demand);
    YLOG_INFO("Connected to {} on port {} using SSH with timeout of {}", address, port, timeout);
}

NetconfSession::NetconfSession(const string& address,
                               const string& username,
                               const string& private_key_path,
                               const string& public_key_path,
                               int port,
                               bool on_demand,
                               bool common_cache,
                               int timeout)
{
    // Correct default settings
    if (port == 0)
        port = 830;

    initialize_client_with_key(address, username, private_key_path, public_key_path, port, timeout);
    auto caching_option = common_cache ? path::ModelCachingOption::COMMON : path::ModelCachingOption::PER_DEVICE;
    path::Repository repo(caching_option);
    initialize_repo(repo, on_demand);
    YLOG_INFO("Connected to {} on port {} using SSH with timeout of {}", address, port, timeout);
}

void NetconfSession::initialize_client_with_key(const string& address,
                                       const string& username,
                                       const string& private_key_path,
                                       const string& public_key_path,
                                       int port,
                                       int timeout)
{
    client = make_shared<NetconfSSHClient>(
        username, private_key_path, public_key_path, address, port, timeout);
}

void NetconfSession::initialize_client(const string& address,
                                       const string& username,
                                       const string& password,
                                       int port,
                                       const string& protocol,
                                       int timeout)
{
    if (protocol.compare(PROTOCOL_SSH) == 0 || protocol.empty())
    {
        client = make_shared<NetconfSSHClient>(username, password, address, port, timeout);
    }
    else if (protocol.compare(PROTOCOL_TCP) == 0)
    {
        client = make_shared<NetconfTCPClient>(username, password, address, port);
    }
    else
    {
        YLOG_ERROR("Protocol {} not supported.", protocol);
        throw(YOperationNotSupportedError{"Protocol is not supported!"});
    }

}

void NetconfSession::initialize_repo(path::Repository & repo, bool on_demand)
{
    IetfCapabilitiesParser capabilities_parser{};
    client->connect();
    server_capabilities = client->get_capabilities();

    if(is_get_schema_supported(server_capabilities))
    {
        model_provider = make_shared<NetconfModelProvider>(*client);
    }
    else
    {
        model_provider = make_shared<StaticModelProvider>(*client);
    }

    repo.add_model_provider(model_provider.get());

    vector<path::Capability> yang_caps;
    vector<string> empty_caps;
    vector<path::Capability> all_caps;

    if(is_yang_1_1_supported(server_capabilities))
    {
        auto caps_1_1 = get_yang_1_1_capabilities();
        server_capabilities = get_union(server_capabilities, caps_1_1);
    }

    all_caps = capabilities_parser.parse(server_capabilities);
    auto lookup_table = capabilities_parser.get_lookup_table(server_capabilities);

    if (on_demand)
        yang_caps = capabilities_parser.parse(empty_caps);
    else
        yang_caps = all_caps;

    root_schema = repo.create_root_schema(lookup_table, yang_caps);
    if(root_schema.get() == nullptr)
    {
        YLOG_ERROR("Root schema cannot be obtained");
        throw(YIllegalStateError{"Root schema cannot be obtained"});
    }
    repo.set_server_capabilities(all_caps);
}

NetconfSession::~NetconfSession()
{
    YLOG_INFO("Disconnected from device");
}

void NetconfSession::check_session_state()
{
    client->perform_session_check("Netconf session is not connected");
}

vector<string> NetconfSession::get_capabilities() const
{
    return server_capabilities;
}

path::RootSchemaNode& NetconfSession::get_root_schema() const
{
    return *root_schema;
}

shared_ptr<path::DataNode> NetconfSession::handle_crud_read(path::Rpc& ydk_rpc) const
{
    //for now we only support crud rpc's
    bool config = is_config(ydk_rpc);
    auto netconf_rpc = create_rpc_instance(*root_schema, get_read_rpc_name(config));
    auto & input = create_rpc_input(*netconf_rpc);
    create_input_source(input, config);
    string filter_value = get_filter_payload(ydk_rpc);

    string netconf_payload = get_netconf_payload(input, "filter", filter_value);

    auto data = get_netconf_output(execute_payload(netconf_payload));
    if (data.empty()) return nullptr;
    return netconf_output_to_datanode(data, *root_schema);
}

shared_ptr<path::DataNode> NetconfSession::handle_crud_edit(path::Rpc& ydk_rpc, path::Annotation annotation) const
{
    //for now we only support crud rpc's
    bool candidate_supported = is_candidate_supported(server_capabilities);

    auto netconf_rpc = create_rpc_instance(*root_schema, "ietf-netconf:edit-config");
    auto & input = create_rpc_input(*netconf_rpc);
    create_input_target(input, candidate_supported);
    create_input_error_option(input);
    string config_payload = get_annotated_config_payload(*root_schema, ydk_rpc, annotation);

    ly_verb(LY_LLSILENT); //turn off libyang logging at the beginning
    string netconf_payload = get_netconf_payload(input, "config", config_payload);
    ly_verb(LY_LLVRB); // enable libyang logging after payload has been created

    return handle_crud_edit_reply(execute_payload(netconf_payload), *client, candidate_supported);
}

shared_ptr<path::DataNode> NetconfSession::handle_netconf_operation(path::Rpc& ydk_rpc) const
{
    path::Codec codec_service{};
    auto netconf_payload = codec_service.encode(ydk_rpc.get_input_node(), EncodingFormat::XML, true);
    string payload{"<rpc xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\">"};
    netconf_payload = payload + netconf_payload + "</rpc>";

    log_rpc_request(netconf_payload);

    string reply = execute_payload(netconf_payload);
    check_rpc_reply_for_error(reply);

    if (is_netconf_get_rpc(ydk_rpc))
    {
        auto data = get_netconf_output(reply);
        if (!data.empty())
            return netconf_output_to_datanode(data, *root_schema);
    }
    else if (ydk_rpc.has_output_node())
    {
        return handle_rpc_output(reply, *root_schema, ydk_rpc.get_input_node().get_path());
    }
    return nullptr;
}

std::string NetconfSession::execute_netconf_operation(path::Rpc& ydk_rpc) const
{
    path::Codec codec_service{};
    auto netconf_payload = codec_service.encode(ydk_rpc.get_input_node(), EncodingFormat::XML, true);
    string payload{"<rpc xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\">"};
    netconf_payload = payload + netconf_payload + "</rpc>";

    log_rpc_request(netconf_payload);

    string reply = execute_payload(netconf_payload);
    check_rpc_reply_for_error(reply);

    if (is_netconf_get_rpc(ydk_rpc))
    {
        return get_netconf_output(reply);
    }
    else {
        return extract_rpc_output(reply);
    }
}

shared_ptr<path::DataNode> NetconfSession::invoke(path::DataNode& datanode) const
{
    if(!datanode.has_action_node())
    {
        YLOG_ERROR("Datanode {} does not contain any action nodes", datanode.get_path());
        throw(YServiceProviderError{"Datanode does not contain any action nodes: " + datanode.get_path()});
    }
    path::Codec codec_service{};
    auto netconf_payload = codec_service.encode(datanode, EncodingFormat::XML, true);

    netconf_payload = "<rpc xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\"><action xmlns=\"urn:ietf:params:xml:ns:yang:1\">\n" + netconf_payload + "</action></rpc>";
    log_rpc_request(netconf_payload);
    string reply = execute_payload(netconf_payload);
    check_rpc_reply_for_error(reply);

    return handle_action_output(reply, *root_schema, datanode.get_action_node_path());
}

shared_ptr<path::DataNode> NetconfSession::invoke(path::Rpc& rpc) const
{
    path::SchemaNode* create_schema = get_schema_for_operation(*root_schema, "ydk:create");
    path::SchemaNode* read_schema = get_schema_for_operation(*root_schema, "ydk:read");
    path::SchemaNode* update_schema = get_schema_for_operation(*root_schema, "ydk:update");
    path::SchemaNode* delete_schema = get_schema_for_operation(*root_schema, "ydk:delete");

    //for now we only support crud rpc's
    path::SchemaNode* rpc_schema = &(rpc.get_schema_node());
    shared_ptr<path::DataNode> datanode = nullptr;

    if(rpc_schema == create_schema || rpc_schema == delete_schema || rpc_schema == update_schema)
    {
        //for each child node in datanode add the nc:operation attribute
        path::Annotation an{IETF_NETCONF_MODULE_NAME, "operation", rpc_schema == delete_schema ? "delete" : "merge"};
        return handle_crud_edit(rpc, an);
    }
    else if(rpc_schema == read_schema)
    {
        return handle_crud_read(rpc);
    }
    else
    {
       return handle_netconf_operation(rpc);
    }

    return datanode;
}

string NetconfSession::execute_payload(const string & payload) const
{
    string reply = client->execute_payload(payload);
    YLOG_INFO("============= Received RPC from device =============\n{}", reply);
    return reply;
}

vector<string> NetconfSession::get_yang_1_1_capabilities() const
{
    IetfCapabilitiesXmlParser parser{};
    string payload = get_caps_rpc_payload();

    YLOG_INFO("============= Requesting YANG 1.1 capabilities =============");
    string reply = execute_payload(payload);
    return parser.parse_yang_1_1(reply);
}

static bool is_yang_1_1_supported(vector<string> & caps)
{
  for(string &c : caps )
    {
        if(c.find("urn:ietf:params:netconf:capability:yang-library:1.0") != string::npos)
        {
            return true;
        }
    }
    return false;
}

static bool is_get_schema_supported(vector<string> capbilities)
{
  for(string &c : capbilities )
    {
        if(c.find("ietf-netconf-monitoring") != string::npos)
        {
            return true;
        }
    }
    return false;
}

static bool is_candidate_supported(vector<string> capabilities)
{
    if(find(capabilities.begin(), capabilities.end(), CANDIDATE) != capabilities.end())
    {
        //candidate is supported
        return true;
    }
    return false;
}

static shared_ptr<path::Rpc> create_rpc_instance(path::RootSchemaNode & root_schema, string rpc_name)
{
    auto rpc = shared_ptr<path::Rpc>(root_schema.create_rpc(rpc_name));
    if(rpc == nullptr){
        YLOG_ERROR("Cannot create payload for RPC: {}", rpc_name);
        throw(YIllegalStateError{"Cannot create payload for RPC: "+ rpc_name});
    }
    return rpc;
}

static path::DataNode& create_rpc_input(path::Rpc & netconf_rpc)
{
    return netconf_rpc.get_input_node();
}

static string get_caps_rpc_payload()
{
    return R"(<rpc xmlns="urn:ietf:params:xml:ns:netconf:base:1.0">
    <get>
      <filter type="subtree">
        <modules-state xmlns="urn:ietf:params:xml:ns:yang:ietf-yang-library">
          <module/>
        </modules-state>
      </filter>
    </get>
    </rpc>)";
}

static string get_commit_rpc_payload()
{
    return "<rpc xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\">"
           "\n  <commit/>"
           "\n</rpc>\n";
}


//////////////////////////////////
static void create_input_target(path::DataNode & input, bool candidate_supported)
{
    if(candidate_supported){
        input.create_datanode("target/candidate", "");
    }
    else {
        input.create_datanode("target/running", "");
    }
}

static void create_input_error_option(path::DataNode & input)
{
    input.create_datanode("error-option", "rollback-on-error");
}

static void create_input_source(path::DataNode & input, bool config)
{
    if(config)
    {
        input.create_datanode("source/running");
    }
}

static string get_annotated_config_payload(path::RootSchemaNode & root_schema,
        path::Rpc & rpc, path::Annotation & annotation)
{
    path::Codec codec_service{};
    auto entity = rpc.get_input_node().find("entity");
    if(entity.empty()){
        YLOG_ERROR("Failed to get entity node");
        throw(YInvalidArgumentError{"Failed to get entity node"});
    }

    path::DataNode* entity_node = entity[0].get();
    string entity_value = entity_node->get_value();

    //deserialize the entity_value
    auto datanode = codec_service.decode(root_schema, entity_value, EncodingFormat::XML);

    if(!datanode){
        YLOG_ERROR("Failed to decode entity node");
        throw(YInvalidArgumentError{"Failed to decode entity node"});
    }

    string config_payload {};
    for(auto const & child : datanode->get_children())
    {
        if((child->annotations()).size()==0)
        {
            child->add_annotation(annotation);
        }
        config_payload += codec_service.encode(*child, EncodingFormat::XML, true);
    }
    return config_payload;
}

static string get_filter_payload(path::Rpc & ydk_rpc)
{
    auto entity = ydk_rpc.get_input_node().find("filter");
    if(entity.empty()){
        YLOG_WARN("Failed to get filter node for RPC; it then will be skipped.");
        return string{};
        //throw(YInvalidArgumentError{"Failed to get entity node"});
    }

    auto datanode = entity[0];
    return datanode->get_value();
}

static string get_netconf_payload(path::DataNode & input, const string &  data_tag, const string &  data_value)
{
    path::Codec codec_service{};
    if (data_tag != "filter" || data_value != "") {
        input.create_datanode(data_tag, data_value);
    }
    string payload{"<rpc xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\">\n"};
    payload+=codec_service.encode(input, EncodingFormat::XML, true);
    payload+="</rpc>";
    log_rpc_request(payload);
    return payload;
}

static string extract_rpc_error(const string & reply)
{
    string msg = "RPC error occurred; check log for details";
    auto error_tag_pos = reply.find("<error-message");
    auto error_tag_close_pos = reply.find("</error-message>");
    if (error_tag_pos != string::npos && error_tag_close_pos != string::npos)
    {
        auto msg_start = reply.find(">", error_tag_pos) + 1;
        msg = reply.substr(msg_start, error_tag_close_pos-msg_start);
    }
    return msg;
}

static shared_ptr<path::DataNode> handle_crud_edit_reply(string reply, NetconfClient & client, bool candidate_supported)
{
    if(reply.find("<ok/>") == string::npos)
    {
        YLOG_ERROR("Did not receive OK reply from the device");
        throw(YServiceProviderError{reply});
    }

    if(candidate_supported)
    {
        //need to send the commit request
        string commit_payload = get_commit_rpc_payload();

        YLOG_INFO("============= Executing commit =============\n{}", commit_payload);
        reply = client.execute_payload(commit_payload);

        YLOG_INFO("============= RPC received from device =============\n{}", reply);
        if(reply.find("<ok/>") == string::npos)
        {
            YLOG_ERROR("RPC error occurred:\n{}", reply);
            auto msg = extract_rpc_error(reply);
            throw(YServiceProviderError(msg));
        }
    }

    //no error no output for edit-config
    return nullptr;
}

static bool is_netconf_get_rpc(path::Rpc & rpc)
{
    return (rpc.get_schema_node().get_path() == "/ietf-netconf:get"
            or rpc.get_schema_node().get_path() == "/ietf-netconf:get-config");
}

std::string get_netconf_output(const string & reply)
{
    if (reply.find("<data/>") != string::npos || reply.find("<nc:data/>") != string::npos)
    {
        YLOG_INFO( "Found empty data tag, meaning requested data are not found on Netconf server");
        return {};
    }

    string rpc_output = extract_rpc_data(reply, "<data", "</data>");
    if (rpc_output.length() == reply.length()) {
        rpc_output = extract_rpc_data(reply, "<nc:data", "</nc:data>");
    }

    if (rpc_output.length() == reply.length())
    {
        YLOG_ERROR( "Cannot find 'data' tag in RPC reply from device\n{}", reply);
        throw(YServiceProviderError{"Cannot find 'data' tag in RPC reply"});
    }

    return rpc_output;
}

static shared_ptr<path::DataNode> netconf_output_to_datanode(const string & data, path::RootSchemaNode & root_schema)
{
    path::Codec codec_service{};
    auto datanode = shared_ptr<path::DataNode>(codec_service.decode(root_schema, data, EncodingFormat::XML));

    if(!datanode){
        YLOG_ERROR( "Codec service failed to decode datanode");
        throw(YError{"Problems deserializing output"});
    }
    return datanode;
}

static string
extract_rpc_data(const string & reply, const string & start_tag, const string & end_tag, bool is_first)
{
    auto data_start = reply.find(start_tag);
    auto data_end = reply.rfind(end_tag);
    if (data_start == string::npos || data_end == string::npos || (is_first && data_start > 0)) {
        return reply;
    }
    if (start_tag.find("<") == 0 && start_tag.find("<!") != 0) {
        auto data_start_end = reply.find(">", data_start);
        data_start = data_start_end + 1;
    }
    else {
        data_start += start_tag.length();
    }
    string data = trim( reply.substr(data_start, data_end - data_start) );
    return data;
}

static string
extract_rpc_output(const string & reply)
{
	string rpc_output = extract_rpc_data(reply, "<rpc-reply ", "</rpc-reply>");
    if (rpc_output.length() == reply.length()) {
        // Try with Netconf namespace prefix
        rpc_output = extract_rpc_data(reply, "<nc:rpc-reply ", "</nc:rpc-reply>");
    }

    string reply_data = rpc_output;
    rpc_output = extract_rpc_data(reply_data, "<data", "</data>", true);
    if (rpc_output.length() == reply_data.length()) {
        rpc_output = extract_rpc_data(reply_data, "<nc:data", "</nc:data>", true);
    }

    rpc_output = extract_rpc_data(rpc_output, "<![CDATA[", "]]>", true);
    return rpc_output;
}

shared_ptr<path::DataNode>
handle_rpc_output(const string & reply, path::RootSchemaNode & root_schema, const string& rpc_path)
{
    string data = extract_rpc_data(reply, "<rpc-reply ", "</rpc-reply>");
    if (data.length() == reply.length()) {
        // Try with Netconf namespace prefix
    	data = extract_rpc_data(reply, "<nc:rpc-reply ", "</nc:rpc-reply>");
    }
    if (data.length() == reply.length()) {
        YLOG_INFO( "Could not locate start and/or end 'rpc-reply' tag in the reply");
        return nullptr;
    }
    if (data.find("<ok/>") != string::npos) {
        return nullptr;
    }

    shared_ptr<path::DataNode> datanode = Codec().decode_rpc_output(
                                                    root_schema,
                                                    data,
                                                    rpc_path,
                                                    EncodingFormat::XML);
    return datanode;
}

shared_ptr<path::DataNode>
handle_action_output(const string & reply, path::RootSchemaNode & root_schema, const string& action_node_path)
{
    if (reply.find("<data/>") != string::npos || reply.find("<nc:data/>") != string::npos) {
        YLOG_INFO( "Found empty data tag");
        return nullptr;
    }

    string data = extract_rpc_data(reply, "<data", "</data>");
    if (data.length() == reply.length()) {
        data = extract_rpc_data(reply, "<nc:data", "</nc:data>");
        if (data.length() == reply.length()) {
            YLOG_INFO( "Could not locate start and/or end 'data' tag in the RPC reply");
            return nullptr;
        }
    }

    shared_ptr<path::DataNode> datanode = Codec().decode_rpc_output(
                                                    root_schema,
                                                    data,
                                                    action_node_path,
                                                    EncodingFormat::XML);
    return datanode;
}

static string get_read_rpc_name(bool config)
{
    if(config)
    {
        return "ietf-netconf:get-config";
    }
    return "ietf-netconf:get";
}

static bool is_config(path::Rpc & rpc)
{
    if(!rpc.get_input_node().find("only-config").empty())
    {
        return true;
    }
    return false;
}

static path::SchemaNode* get_schema_for_operation(path::RootSchemaNode & root_schema, string yfilter)
{
    auto c = root_schema.find(yfilter);
    if(c.empty())
    {
        YLOG_ERROR("CRUD create rpc schema not found!");
        throw(YIllegalStateError{"CRUD create rpc schema not found!"});
    }
    return c[0];
}

static void check_rpc_reply_for_error(const string& reply)
{
    if (reply.find("</rpc-error>") != string::npos)
    {
        YLOG_ERROR("RPC error occurred:\n{}", reply);
        auto msg = extract_rpc_error(reply);
        throw(YServiceProviderError(msg));
    }
}

static void log_rpc_request(const string& payload)
{
    YLOG_INFO("============= Sending RPC to device =============\n{}", payload);
}

} //namespace path

} //namespace ydk
