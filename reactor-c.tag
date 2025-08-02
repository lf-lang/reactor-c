<?xml version='1.0' encoding='UTF-8' standalone='yes' ?>
<tagfile doxygen_version="1.14.0">
  <compound kind="file">
    <name>rti_common.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/core/federated/RTI/</path>
    <filename>rti__common_8h.html</filename>
    <includes id="low__level__platform_8h" name="low_level_platform.h" local="yes" import="no" module="no" objc="no">low_level_platform.h</includes>
    <includes id="util_8h" name="util.h" local="yes" import="no" module="no" objc="no">util.h</includes>
    <includes id="tag_8h" name="tag.h" local="yes" import="no" module="no" objc="no">tag.h</includes>
    <includes id="tracepoint_8h" name="tracepoint.h" local="yes" import="no" module="no" objc="no">tracepoint.h</includes>
    <class kind="struct">minimum_delay_t</class>
    <class kind="struct">rti_common_t</class>
    <class kind="struct">scheduling_node_t</class>
    <class kind="struct">tag_advance_grant_t</class>
    <member kind="typedef">
      <type>enum execution_mode_t</type>
      <name>execution_mode_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga50856f252373f4c456a34c6f26d385ce</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct minimum_delay_t</type>
      <name>minimum_delay_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga51ee50491dd9db504fa075ae0b490e14</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct rti_common_t</type>
      <name>rti_common_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga2df16421461ba5b27dc451b16865b750</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>enum scheduling_node_state_t</type>
      <name>scheduling_node_state_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga991b71a39df8e306998cbc9d15f9e381</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct scheduling_node_t</type>
      <name>scheduling_node_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gad4b7689a045ef99a1c86753731fb8836</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>execution_mode_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga83ca4d4187a661b1395c9f860d61c97e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FAST</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gga83ca4d4187a661b1395c9f860d61c97eaf84c11ba888e499a8a282a3e6f5de7de</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>REALTIME</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gga83ca4d4187a661b1395c9f860d61c97eadbd89a052eecc45eaa443bcbecc7c5e4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>scheduling_node_state_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga2c9591789f1d6afd603e0330e13f3744</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>NOT_CONNECTED</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gga2c9591789f1d6afd603e0330e13f3744a96c582a5af213ca7fb34f970d83875f5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>GRANTED</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gga2c9591789f1d6afd603e0330e13f3744a440c8b08fdd77c2aa90283c06dbe465a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>PENDING</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gga2c9591789f1d6afd603e0330e13f3744a1869818fd53ff519eb8e429301bdff73</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_logical_tag_complete</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga5f9802f30e0cb2ceedf199ebb35c946b</anchor>
      <arglist>(scheduling_node_t *e, tag_t completed)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>downstream_next_event_tag</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga00b9b047401cd08937d21ae84ee2ef79</anchor>
      <arglist>(scheduling_node_t *node, uint16_t node_sending_new_net_id)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>earliest_future_incoming_message_tag</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gad5f8cfd324d9403aa800a88020276969</anchor>
      <arglist>(scheduling_node_t *e)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>eimt_strict</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gadaad58c39361263d511f49b0952bcaee</anchor>
      <arglist>(scheduling_node_t *e)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>free_scheduling_nodes</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gaa8f3d17093904564d4a1eebd526a2164</anchor>
      <arglist>(scheduling_node_t **scheduling_nodes, uint16_t number_of_scheduling_nodes)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>get_dnet_candidate</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gac81735735888368a08b2b16cf8d440ea</anchor>
      <arglist>(tag_t next_event_tag, tag_t minimum_delay)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>initialize_rti_common</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gadd2a2ad3c8a9f8e6cde5ecbcb83d7e8d</anchor>
      <arglist>(rti_common_t *rti_common)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>initialize_scheduling_node</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga9a27b6186b947cfacb408dc3a0829f6e</anchor>
      <arglist>(scheduling_node_t *e, uint16_t id)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>invalidate_min_delays</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga5f7d50fc74bdc38f889c38109c406468</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_in_cycle</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gab38455d4faf77b4d86dcd77976afe1f1</anchor>
      <arglist>(scheduling_node_t *node)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_in_zero_delay_cycle</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga94101c5bfb54d670a8f47f448e351a34</anchor>
      <arglist>(scheduling_node_t *node)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>notify_advance_grant_if_safe</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gaa057279652cc77f238cc73d0fc0e705e</anchor>
      <arglist>(scheduling_node_t *e)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>notify_downstream_advance_grant_if_safe</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga0903cfa1c0fa064824b445c099cd2aa6</anchor>
      <arglist>(scheduling_node_t *e, bool visited[])</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>notify_downstream_next_event_tag</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gae38c64692f527911c9cc748c03d2246d</anchor>
      <arglist>(scheduling_node_t *e, tag_t tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>notify_provisional_tag_advance_grant</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gab226921e491807e98a406487cfdf6335</anchor>
      <arglist>(scheduling_node_t *e, tag_t tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>notify_tag_advance_grant</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga21a88113d348968980a137c9e4e4148e</anchor>
      <arglist>(scheduling_node_t *e, tag_t tag)</arglist>
    </member>
    <member kind="function">
      <type>tag_advance_grant_t</type>
      <name>tag_advance_grant_if_safe</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gaf849510938f837c74cfe8f843cb7dcb2</anchor>
      <arglist>(scheduling_node_t *e)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update_min_delays</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga93ef5ce52bd47e14977ea3571c1b152b</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update_scheduling_node_next_event_tag_locked</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga21533a3d64de78b3766016c2166460c9</anchor>
      <arglist>(scheduling_node_t *e, tag_t next_event_tag)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>rti_local.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/core/federated/RTI/</path>
    <filename>rti__local_8h.html</filename>
    <includes id="lf__types_8h" name="lf_types.h" local="yes" import="no" module="no" objc="no">lf_types.h</includes>
    <includes id="rti__common_8h" name="rti_common.h" local="yes" import="no" module="no" objc="no">rti_common.h</includes>
    <class kind="struct">enclave_info_t</class>
    <class kind="struct">rti_local_t</class>
    <member kind="typedef">
      <type>struct enclave_info_t</type>
      <name>enclave_info_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga1567a1034e3b7c6528bc12fdc04a4c71</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>free_local_rti</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gaa202c053941549eb84d77776c67b5137</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>initialize_enclave_info</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gaf934cc54979bcf640a7868377daab5e2</anchor>
      <arglist>(enclave_info_t *enclave, int idx, environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>initialize_local_rti</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga167855af2f7fb010609dceda0a59b43d</anchor>
      <arglist>(environment_t *envs, int num_envs)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_get_downstream_of</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga06003604b6defab443bdfe34f3ab17ee</anchor>
      <arglist>(int enclave_id, uint16_t **result)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_get_upstream_delay_of</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gaaba4ae9dd32581f9f34a7a81d9c7791c</anchor>
      <arglist>(int enclave_id, interval_t **result)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_get_upstream_of</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga8f432bb04e691f66f4f81cbbecb741ed</anchor>
      <arglist>(int enclave_id, uint16_t **result)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>rti_logical_tag_complete_locked</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gafa27405a96dae1488b670aa25fc8ad1b</anchor>
      <arglist>(enclave_info_t *enclave, tag_t completed)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>rti_next_event_tag_locked</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga48640596070ecfb23f94532363b5cd6a</anchor>
      <arglist>(enclave_info_t *enclave, tag_t next_event_tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>rti_update_other_net_locked</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gaf921c9092b09500768f94e41b3a7c8f4</anchor>
      <arglist>(enclave_info_t *src, enclave_info_t *target, tag_t net)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>rti_remote.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/core/federated/RTI/</path>
    <filename>rti__remote_8h.html</filename>
    <includes id="rti__common_8h" name="rti_common.h" local="yes" import="no" module="no" objc="no">rti_common.h</includes>
    <includes id="lf__types_8h" name="lf_types.h" local="yes" import="no" module="no" objc="no">lf_types.h</includes>
    <includes id="pqueue__tag_8h" name="pqueue_tag.h" local="yes" import="no" module="no" objc="no">pqueue_tag.h</includes>
    <includes id="socket__common_8h" name="socket_common.h" local="yes" import="no" module="no" objc="no">socket_common.h</includes>
    <class kind="struct">federate_info_t</class>
    <class kind="struct">rti_remote_t</class>
    <member kind="define">
      <type>#define</type>
      <name>MAX_TIME_FOR_REPLY_TO_STOP_REQUEST</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga0066544a32ab71d5601142354230452b</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>enum clock_sync_stat</type>
      <name>clock_sync_stat</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga39e986990bfc20d1512b61ab119ce628</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct federate_info_t</type>
      <name>federate_info_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gacd32a9389f9882becea414555263cde1</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct rti_remote_t</type>
      <name>rti_remote_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga224a440a405fa3f473a27ad65edca186</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>clock_sync_stat</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga9705d612b9ce908ee485e92eb3f2769f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>clock_sync_off</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gga9705d612b9ce908ee485e92eb3f2769fafef85b2461484e7a55ae3f50d3ca996c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>clock_sync_init</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gga9705d612b9ce908ee485e92eb3f2769faf21c0b4c30338f2717ebc9f53fa34558</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>clock_sync_on</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gga9705d612b9ce908ee485e92eb3f2769fa8423b23ffffdfc03fcb3f68cf4007531</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>clock_synchronization_thread</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga272b481a0cc2f86f21c75e8efa19a551</anchor>
      <arglist>(void *noargs)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>federate_info_thread_TCP</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gae0b396a4e41b93505274bbfb55b7a510</anchor>
      <arglist>(void *fed)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_address_ad</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga3f4f3aae4aa73c87569b677f2c0957b7</anchor>
      <arglist>(uint16_t federate_id)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_address_query</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga0424648d3659346e9c7c645cca35d470</anchor>
      <arglist>(uint16_t fed_id)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_latest_tag_confirmed</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga1ffcd0bc844a81aa45cfaa4e1e697ef1</anchor>
      <arglist>(federate_info_t *fed)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_next_event_tag</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gad68a1cbfc6299b091b8f7b0e97f8bb5b</anchor>
      <arglist>(federate_info_t *fed)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_physical_clock_sync_message</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gad69c4cb2041a1a262bce829c49ae9246</anchor>
      <arglist>(federate_info_t *my_fed, socket_type_t socket_type)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_port_absent_message</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga307edbac9eb75981db9dc7c0fcfc73e6</anchor>
      <arglist>(federate_info_t *sending_federate, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_stop_request_message</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga41ca594227fe9bb62f67b21cc2e7b6d6</anchor>
      <arglist>(federate_info_t *fed)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_stop_request_reply</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gac4d4e5a1df9c31133d942a6e50e4c163</anchor>
      <arglist>(federate_info_t *fed)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_timed_message</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga9fca6a5c7e73b40db2f6ef6f50c7a112</anchor>
      <arglist>(federate_info_t *sending_federate, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_timestamp</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga0a2eadb2f35483bc7ce62a5845110330</anchor>
      <arglist>(federate_info_t *my_fed)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>initialize_federate</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gab32a595d53f125832570251023d10c46</anchor>
      <arglist>(federate_info_t *fed, uint16_t id)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>initialize_RTI</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga0aefafd71cb0b057a604a04f1af61174</anchor>
      <arglist>(rti_remote_t *rti)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_connect_to_federates</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga52a9225745a4b229aee86fcc4617b904</anchor>
      <arglist>(int socket_descriptor)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_critical_section_enter</name>
      <anchorfile>rti__remote_8h.html</anchorfile>
      <anchor>ae0fda178667bc6cd94890a13316c285c</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_critical_section_exit</name>
      <anchorfile>rti__remote_8h.html</anchorfile>
      <anchor>aed2c25495b50b46780c6288e4370541e</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>process_args</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga80aaf4eeed3e2902f8fe9de80b45777d</anchor>
      <arglist>(int argc, const char *argv[])</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>process_clock_sync_args</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gad7d4392b21b300612a5239fbb1ffa274</anchor>
      <arglist>(int argc, const char *argv[])</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>respond_to_erroneous_connections</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gac6af5f2343ecf9ed87cdbebd98b94271</anchor>
      <arglist>(void *nothing)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>send_physical_clock</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga5b16bd2ead60426e3ace71eca1cfede5</anchor>
      <arglist>(unsigned char message_type, federate_info_t *fed, socket_type_t socket_type)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>send_reject</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gac1d01420f22f3dc5dbdef49ffebdb443</anchor>
      <arglist>(int *socket_id, unsigned char error_code)</arglist>
    </member>
    <member kind="function">
      <type>int32_t</type>
      <name>start_rti_server</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga6cc1fe69c154d09d88de1f1c06eb4b0d</anchor>
      <arglist>(uint16_t port)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update_federate_next_event_tag_locked</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga7190c8d1000afb0a5e8898011d041917</anchor>
      <arglist>(uint16_t federate_id, tag_t next_event_tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>usage</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga7f44f474f50286c4ba8c0ebac254bb28</anchor>
      <arglist>(int argc, const char *argv[])</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>wait_for_federates</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga005cb43e8e6c7795c8f0db27e2424475</anchor>
      <arglist>(int socket_descriptor)</arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>_lf_federate_reports_error</name>
      <anchorfile>rti__remote_8h.html</anchorfile>
      <anchor>a759ce03cd00ba1ac4870b7728b8fae86</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>doxygen.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/docs/</path>
    <filename>doxygen_8h.html</filename>
  </compound>
  <compound kind="file">
    <name>1_landing_page.md</name>
    <path>/Users/runner/work/reactor-c/reactor-c/docs/markdown/</path>
    <filename>1__landing__page_8md.html</filename>
  </compound>
  <compound kind="file">
    <name>2_introduction.md</name>
    <path>/Users/runner/work/reactor-c/reactor-c/docs/markdown/</path>
    <filename>2__introduction_8md.html</filename>
  </compound>
  <compound kind="file">
    <name>3_contributing.md</name>
    <path>/Users/runner/work/reactor-c/reactor-c/docs/markdown/</path>
    <filename>3__contributing_8md.html</filename>
  </compound>
  <compound kind="file">
    <name>4_license.md</name>
    <path>/Users/runner/work/reactor-c/reactor-c/docs/markdown/</path>
    <filename>4__license_8md.html</filename>
  </compound>
  <compound kind="file">
    <name>reaction_macros.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/api/</path>
    <filename>reaction__macros_8h.html</filename>
    <member kind="define">
      <type>#define</type>
      <name>lf_reactor_full_name</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga78918b87982fba15b59f35a8f926b021</anchor>
      <arglist>(reactor)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_reactor_name</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga42c6f935901d6fc56d6e82be619a8bd3</anchor>
      <arglist>(reactor)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_set</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaef602f51d34bbd214643161e425d909d</anchor>
      <arglist>(out, val)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_set_array</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga8a3a63e70ec63e35d46573293ecec905</anchor>
      <arglist>(out, val, len)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_set_copy_constructor</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaa515ab9df816c6ac8a450def4dc02f40</anchor>
      <arglist>(out, cpy_ctor)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_set_destructor</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaf4b2874af3da2bb85edfb3f0a57028a1</anchor>
      <arglist>(out, dtor)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_set_mode</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga0120cc579143c138482e89186e180ebc</anchor>
      <arglist>(mode)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_set_present</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga5c1e2963a361057f0b249b95f40a8f8d</anchor>
      <arglist>(out)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_set_token</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga1aa76760517d7100306d59b92fd41a26</anchor>
      <arglist>(out, newtoken)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_tag</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga2d81456725407157f9dc521a5e14a679</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_time_logical</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga4fe4453dda4223671dc90fa1ecbcac85</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_time_logical_elapsed</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga7aaaed76bc1ae823bb13d6603807f874</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>schedule.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/api/</path>
    <filename>schedule_8h.html</filename>
    <includes id="lf__types_8h" name="lf_types.h" local="yes" import="no" module="no" objc="no">lf_types.h</includes>
    <includes id="tag_8h" name="tag.h" local="yes" import="no" module="no" objc="no">tag.h</includes>
    <member kind="function">
      <type>bool</type>
      <name>lf_check_deadline</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gab3a04dd0a1581844829b28686b6b3c53</anchor>
      <arglist>(void *self, bool invoke_deadline_handler)</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>lf_schedule</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga6778eef97447cf0ba1f0afa8ba3a8dca</anchor>
      <arglist>(void *action, interval_t offset)</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>lf_schedule_copy</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga747594e2d7264ae8b044a095eb92ba27</anchor>
      <arglist>(void *action, interval_t offset, void *value, size_t length)</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>lf_schedule_int</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga073ea4406a084a24e71b65936ba39e36</anchor>
      <arglist>(void *action, interval_t extra_delay, int value)</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>lf_schedule_token</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gabe3fd30bf6a2689fdb3ec03b4e2f47d1</anchor>
      <arglist>(void *action, interval_t extra_delay, lf_token_t *token)</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>lf_schedule_trigger</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga28927b8a184fe101ad414ed866c49148</anchor>
      <arglist>(environment_t *env, trigger_t *trigger, interval_t delay, lf_token_t *token)</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>lf_schedule_value</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga986bb1be3a9e4f71e5b5dde30d9dc6ad</anchor>
      <arglist>(void *action, interval_t extra_delay, void *value, int length)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>clock.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/</path>
    <filename>clock_8h.html</filename>
    <includes id="low__level__platform_8h" name="low_level_platform.h" local="yes" import="no" module="no" objc="no">low_level_platform.h</includes>
    <member kind="function">
      <type>int</type>
      <name>lf_clock_cond_timedwait</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga1d6ed42d060c926d05db76544172fed8</anchor>
      <arglist>(lf_cond_t *cond, instant_t wakeup_time)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_clock_gettime</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gaad3e8b04cd7bf1132b774940602a72d3</anchor>
      <arglist>(instant_t *now)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_clock_interruptable_sleep_until_locked</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga341e2d260240fd35d29ada9531aa9ead</anchor>
      <arglist>(environment_t *env, instant_t wakeup_time)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>environment.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/</path>
    <filename>environment_8h.html</filename>
    <includes id="lf__types_8h" name="lf_types.h" local="yes" import="no" module="no" objc="no">lf_types.h</includes>
    <includes id="low__level__platform_8h" name="low_level_platform.h" local="yes" import="no" module="no" objc="no">low_level_platform.h</includes>
    <includes id="tracepoint_8h" name="tracepoint.h" local="yes" import="no" module="no" objc="no">tracepoint.h</includes>
    <class kind="struct">environment_t</class>
    <class kind="struct">mode_environment_t</class>
    <member kind="define">
      <type>#define</type>
      <name>GLOBAL_ENVIRONMENT</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gafaff13b938d14da158c3fa1424358353</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct enclave_info_t</type>
      <name>enclave_info_t</name>
      <anchorfile>environment_8h.html</anchorfile>
      <anchor>a1567a1034e3b7c6528bc12fdc04a4c71</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct environment_t</type>
      <name>environment_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaa47b54e9e041dfe1b75fffceb1051466</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct lf_scheduler_t</type>
      <name>lf_scheduler_t</name>
      <anchorfile>environment_8h.html</anchorfile>
      <anchor>a0199f9b027e13cf08095d91fe798c663</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct mode_environment_t</type>
      <name>mode_environment_t</name>
      <anchorfile>environment_8h.html</anchorfile>
      <anchor>a7941464261ddb811cc9b63a85c11d1ee</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct watchdog_t</type>
      <name>watchdog_t</name>
      <anchorfile>environment_8h.html</anchorfile>
      <anchor>aee1cd2bc521f76fa428cc659474d9570</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>_lf_get_environments</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gac109cd752121228507a95495a1eb6d8f</anchor>
      <arglist>(environment_t **envs)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>environment_free</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaf73b90f735c070d534171f3e92730ac8</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>environment_init</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gab49da5954eb69f3d126162da44c25b36</anchor>
      <arglist>(environment_t *env, const char *name, int id, int num_workers, int num_timers, int num_startup_reactions, int num_shutdown_reactions, int num_reset_reactions, int num_is_present_fields, int num_modes, int num_state_resets, int num_watchdogs, const char *trace_file_name)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>environment_init_tags</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaf23dde6465214cc92114e0c49bccdc72</anchor>
      <arglist>(environment_t *env, instant_t start_time, interval_t duration)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>environment_verify</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaeb51302599cae953b2b8942088879e2f</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>clock-sync.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/federated/</path>
    <filename>clock-sync_8h.html</filename>
    <includes id="low__level__platform_8h" name="low_level_platform.h" local="yes" import="no" module="no" objc="no">low_level_platform.h</includes>
    <class kind="struct">lf_stat_ll</class>
    <class kind="struct">socket_stat_t</class>
    <member kind="define">
      <type>#define</type>
      <name>_LF_CLOCK_SYNC_ATTENUATION</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga532da6f271eb75c9ac745571b995c404</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>_LF_CLOCK_SYNC_COLLECT_STATS</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga4b9290ecd850995c857e04746aa45d10</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>_LF_CLOCK_SYNC_EXCHANGES_PER_INTERVAL</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gab0eeaea19d6e5c9217a4eed928c32141</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CLOCK_SYNC_GUARD_BAND</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga0810a64801750ce9b148848c228c86e9</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_CLOCK_SYNC</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gab36196523f89c0d8f30c1965b458beb6</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_CLOCK_SYNC_INIT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga7ed4e5f2a4216fdf6d76eafcab5b49b7</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_CLOCK_SYNC_OFF</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga742c3183fb89d811377514d09e526b8f</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_CLOCK_SYNC_ON</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaa9efe35bfc06d22220c852574c4a5feb</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct lf_stat_ll</type>
      <name>lf_stat_ll</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga9e708b16d53622a88d5a2638affb6934</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct socket_stat_t</type>
      <name>socket_stat_t</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga7e49fed082ec884e26d761e1c4f0d428</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>clock_sync_add_offset</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga6b3edec4d337711a2e914c9f5581ce1c</anchor>
      <arglist>(instant_t *t)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>clock_sync_set_constant_bias</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga4f1aaaa9e0b74867ba6b60eb962dfca6</anchor>
      <arglist>(interval_t offset)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>clock_sync_subtract_offset</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaf5fb44eb0db80b9dfa61399bdab8b85c</anchor>
      <arglist>(instant_t *t)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>create_clock_sync_thread</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gac094b53ced87d3cbd617a66591f4282a</anchor>
      <arglist>(lf_thread_t *thread_id)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>handle_T1_clock_sync_message</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gace14df89540b56b069c6c619e8f37493</anchor>
      <arglist>(unsigned char *buffer, int socket, instant_t t2)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_T4_clock_sync_message</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga1b35d21eda090ea4bf8a79f401dbdad0</anchor>
      <arglist>(unsigned char *buffer, int socket, instant_t r4)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>reset_socket_stat</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae267d03a5f2263604459ca4c1aef2c2c</anchor>
      <arglist>(struct socket_stat_t *socket_stat)</arglist>
    </member>
    <member kind="function">
      <type>uint16_t</type>
      <name>setup_clock_synchronization_with_rti</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga834b31e00e677a23b6a86119b7a2fe59</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>synchronize_initial_physical_clock_with_rti</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga3eba5f95a19f86a70d9d11fd2c736dd1</anchor>
      <arglist>(int *rti_socket_TCP)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>federate.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/federated/</path>
    <filename>federate_8h.html</filename>
    <includes id="tag_8h" name="tag.h" local="yes" import="no" module="no" objc="no">tag.h</includes>
    <includes id="lf__types_8h" name="lf_types.h" local="yes" import="no" module="no" objc="no">lf_types.h</includes>
    <includes id="environment_8h" name="environment.h" local="yes" import="no" module="no" objc="no">environment.h</includes>
    <includes id="low__level__platform_8h" name="low_level_platform.h" local="yes" import="no" module="no" objc="no">low_level_platform.h</includes>
    <includes id="socket__common_8h" name="socket_common.h" local="yes" import="no" module="no" objc="no">socket_common.h</includes>
    <class kind="struct">federate_instance_t</class>
    <class kind="struct">federation_metadata_t</class>
    <class kind="struct">staa_t</class>
    <member kind="define">
      <type>#define</type>
      <name>ADVANCE_MESSAGE_INTERVAL</name>
      <anchorfile>federate_8h.html</anchorfile>
      <anchor>a2312893474cb0415e16af40b1de063ca</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct federate_instance_t</type>
      <name>federate_instance_t</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga675c841ec6a29e45cacc71b61ef8d270</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct federation_metadata_t</type>
      <name>federation_metadata_t</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae335f4cb4e7d5e88ed712be8cf9592ac</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>enum parse_rti_code_t</type>
      <name>parse_rti_code_t</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga98d812b2acffbba5c8b1b72913513d19</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct staa_t</type>
      <name>staa_t</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga0bce8f0d13040846780f5bb02e43e81d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>parse_rti_code_t</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga9acb70e6b48452bd9d146e35bafc535c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>SUCCESS</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gga9acb70e6b48452bd9d146e35bafc535cac7f69f7c9e5aea9b8f54cf02870e2bf8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>INVALID_PORT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gga9acb70e6b48452bd9d146e35bafc535cad65c958d0ccb000b69ef0ef4e3a5bfdf</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>INVALID_HOST</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gga9acb70e6b48452bd9d146e35bafc535caea85d37354b294f21e7ab9c5c142a237</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>INVALID_USER</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gga9acb70e6b48452bd9d146e35bafc535ca1dbf923bd60da7209a684ed484935973</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FAILED_TO_PARSE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gga9acb70e6b48452bd9d146e35bafc535ca3ad4ab464aba04397206e8b89aa1955a</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_connect_to_federate</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga26b7c3ab8c2a50f65e53997a6f26a0dc</anchor>
      <arglist>(uint16_t remote_federate_id)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_connect_to_rti</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga92e8c30255091911a80601bf341cf0a2</anchor>
      <arglist>(const char *hostname, int port_number)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_create_server</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga926a4fb7f9b045acb13fee6c2b7192dd</anchor>
      <arglist>(int specified_port)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_enqueue_port_absent_reactions</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae427b4c0340dbe19d46c93708fb6151a</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>lf_handle_p2p_connections_from_federates</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaf7ba635fb6ffa82e4b05a51d4fc0020f</anchor>
      <arglist>(void *ignored)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_latest_tag_confirmed</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga80af1b6a9d7200df3d85c534edd8cbbc</anchor>
      <arglist>(tag_t tag_to_send)</arglist>
    </member>
    <member kind="function">
      <type>parse_rti_code_t</type>
      <name>lf_parse_rti_addr</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae612f180643d0436d4496738b957af68</anchor>
      <arglist>(const char *rti_addr)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_reset_status_fields_on_input_port_triggers</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae3bc503fcbeaffe48f4500fddba4b21a</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_send_message</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga2f330bfe2fdb03cbf49596bcc012bc58</anchor>
      <arglist>(int message_type, unsigned short port, unsigned short federate, const char *next_destination_str, size_t length, unsigned char *message)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_send_neighbor_structure_to_RTI</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga131226459d7dacc6068c0a6d1d9ebde1</anchor>
      <arglist>(int socket_TCP_RTI)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>lf_send_next_event_tag</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga33d69f68b22b5143c029f463d6efba4f</anchor>
      <arglist>(environment_t *env, tag_t tag, bool wait_for_reply)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_send_port_absent_to_federate</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga527e8cd401ba68b503403706815ed1a0</anchor>
      <arglist>(environment_t *env, interval_t additional_delay, unsigned short port_ID, unsigned short fed_ID)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_send_stop_request_to_rti</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gab4d7e691d4b52f2c0dac90e772d86dd5</anchor>
      <arglist>(tag_t stop_tag)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_send_tagged_message</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga0931fe1bb9eac2a9beebe0c0ed03408e</anchor>
      <arglist>(environment_t *env, interval_t additional_delay, int message_type, unsigned short port, unsigned short federate, const char *next_destination_str, size_t length, unsigned char *message)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_set_federation_id</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga032d945ee3fd6995a5f7bb15b57f2ddf</anchor>
      <arglist>(const char *fid)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_spawn_staa_thread</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga3aff644df1b85540aa6a3d2997f819c5</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_stall_advance_level_federation</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga971322f63f26490a27bdd9006c05b8fe</anchor>
      <arglist>(environment_t *env, size_t level)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_stall_advance_level_federation_locked</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gab0f2188d27dfffa9fbbd417bed9305ea</anchor>
      <arglist>(size_t level)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_synchronize_with_other_federates</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga36681c905141edace5d23ff8d5c8f205</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>lf_update_max_level</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga84e5177d12e705274be1e6652b5d7c01</anchor>
      <arglist>(tag_t tag, bool is_provisional)</arglist>
    </member>
    <member kind="function">
      <type>instant_t</type>
      <name>lf_wait_until_time</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaa0c8d0811c7faae11b9f1a7fcb30f917</anchor>
      <arglist>(tag_t tag)</arglist>
    </member>
    <member kind="variable">
      <type>lf_mutex_t</type>
      <name>lf_outbound_socket_mutex</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga1881fdaaffead81a8d2993121d9cd78f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_cond_t</type>
      <name>lf_port_status_changed</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga4ea10c9ed824595585d91f37dbfd4364</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>net_common.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/federated/network/</path>
    <filename>net__common_8h.html</filename>
    <member kind="define">
      <type>#define</type>
      <name>ADDRESS_QUERY_RETRY_INTERVAL</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga8ce563da4edbe9c4f7c1ccf35ad8694f</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DELAY_START</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga4b8c713b515dba0c86d9205dc0caf4ed</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ENCODE_STOP_GRANTED</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga8086398bfefdc0104767df037e59daa5</anchor>
      <arglist>(buffer, time, microstep)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ENCODE_STOP_REQUEST</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga3268a658c2cb5126be5284a86ad9bd62</anchor>
      <arglist>(buffer, time, microstep)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ENCODE_STOP_REQUEST_REPLY</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae06b33f7fcdc71f52eb0fcf81e07e4d6</anchor>
      <arglist>(buffer, time, microstep)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FED_COM_BUFFER_SIZE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gacc95612e1d2dbbdf34afe76d50e75223</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FEDERATE_ID_IN_USE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae3bd830cd17cf0914b61d0516360abc1</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FEDERATE_ID_OUT_OF_RANGE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga5846fdcf4c92041f543b73e29e78aa21</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FEDERATION_ID_DOES_NOT_MATCH</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga653676d1f302fe08249af3dee78fa294</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>HMAC_DOES_NOT_MATCH</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga51d08a784b4ee6463688a971d99d2944</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_ACK</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gad94567b2d2e277ddc1be0da9a92b09e7</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_ADDRESS_ADVERTISEMENT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae004cb4e5add42afe5483f6706e11d35</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_ADDRESS_QUERY</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga5ac191bca25da16eca3e4f02d21172ef</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_ADDRESS_QUERY_REPLY</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaabe4cac3ef1d0834a99fa2532dfaa6ba</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_CLOCK_SYNC_CODED_PROBE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaa13eface5080ad75bbd53abe919c80b6</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_CLOCK_SYNC_T1</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaea37eff76ade1b2781a7e6298afb3a04</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_CLOCK_SYNC_T3</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga52a76e4cc36217a169f32d5adde590cc</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_CLOCK_SYNC_T4</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae128056ab2af39988103856ee815d930</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_DOWNSTREAM_NEXT_EVENT_TAG</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gafb060091e032562cf32c0eb62340d309</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_FAILED</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaf27674f627be1c469a529a995da5c074</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_FED_IDS</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga8e49ce0b1c3a58c881849ca4d0bae824</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_FED_NONCE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga191b27bec42ab0370248fbc64cc9b860</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_FED_RESPONSE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gacd7e1e07253e568044a204a1f82d36a1</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_LATEST_TAG_CONFIRMED</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gada47c9f6736992a3df380526d87089f9</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_MESSAGE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gac79b5228f132029285408a30a31a174e</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_NEIGHBOR_STRUCTURE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga48ec489cb1543b161c262f4bee6c9598</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_NEIGHBOR_STRUCTURE_HEADER_SIZE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga77a9c1b741d7ca0f4e8d00a5b74ef91e</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_NEXT_EVENT_TAG</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaf662a6a84cd64cddad92e20e26af877e</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_P2P_MESSAGE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga2b9f13f8df66448bf81ac5fe0774c124</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_P2P_SENDING_FED_ID</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gacd33bbab7bf74e5ac8bad3bd27145f8b</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_P2P_TAGGED_MESSAGE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga5c1256c8c62fbbcb1b16ea67d8f529fb</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_PORT_ABSENT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gadb9610b1edbee4c85e194e391a6eeb74</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_PROVISIONAL_TAG_ADVANCE_GRANT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga9a9bb60d4df1ba581a29319850097cc2</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_REJECT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga59a69d0685fdc2a216718f1efa083c4c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_RESIGN</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga9e19e307a4c3a9dbccea4f2539cd67dd</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_RTI_RESPONSE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga20f24b4b20547d44523120689afd9b98</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_STOP_GRANTED</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaad37dd00423e88f213ca7d7d238bce2e</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_STOP_GRANTED_LENGTH</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga76275384e9865f1f1ed32408c03d876a</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_STOP_REQUEST</name>
      <anchorfile>net__common_8h.html</anchorfile>
      <anchor>a8588a57a3ae81bf33c740dfc57103a23</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_STOP_REQUEST_LENGTH</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaa3ed75054ae1aaa64dafa6399f7a23cb</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_STOP_REQUEST_REPLY</name>
      <anchorfile>net__common_8h.html</anchorfile>
      <anchor>adbe10e103635ee7ec4bca3226ccebc56</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_STOP_REQUEST_REPLY_LENGTH</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaa62d9986e928cb5e872caa6a509cae6c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_TAG_ADVANCE_GRANT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga94fe2c510160682b2c0ffc00b35e0ad5</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_TAGGED_MESSAGE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga2922af172f2e95bc73bd0675a4107b3b</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_TIMESTAMP</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga27db349e7460afc1758bf2eec95d7005</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_TIMESTAMP_LENGTH</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga3450aedd1ca1c368ed28ed2e859588ef</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_UDP_PORT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae2c2fdb5fbcc47750409348d37b0cd78</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NONCE_LENGTH</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga6771c37605e49c8faae7898797f254b7</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>RTI_NOT_EXECUTED_WITH_AUTH</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae546b6c6176fe607616181e144364f2b</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SHA256_HMAC_LENGTH</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gabd435507a255ff2571133013bdf93bd2</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>UNEXPECTED_MESSAGE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae2e1a44a10d4219f4645a4e99fee009c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>WRONG_SERVER</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga5a6c87886a0136b58ae5bb1d627c7ae3</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>net_util.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/federated/network/</path>
    <filename>net__util_8h.html</filename>
    <includes id="low__level__platform_8h" name="low_level_platform.h" local="yes" import="no" module="no" objc="no">low_level_platform.h</includes>
    <includes id="tag_8h" name="tag.h" local="yes" import="no" module="no" objc="no">tag.h</includes>
    <includes id="socket__common_8h" name="socket_common.h" local="yes" import="no" module="no" objc="no">socket_common.h</includes>
    <class kind="struct">rti_addr_info_t</class>
    <member kind="define">
      <type>#define</type>
      <name>HOST_BIG_ENDIAN</name>
      <anchorfile>net__util_8h.html</anchorfile>
      <anchor>a18c839c3ef122fe4ecc7b907ea688a97</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>HOST_LITTLE_ENDIAN</name>
      <anchorfile>net__util_8h.html</anchorfile>
      <anchor>ae0b024f7ee4bd875149b1a0a50c20b0a</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct rti_addr_info_t</type>
      <name>rti_addr_info_t</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaf820f39ab52ce0a58d7ba739051b8f24</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>encode_int32</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gafc6b3d0e0e777738422c11fa07b35e0f</anchor>
      <arglist>(int32_t data, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>encode_int64</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae233fa02382ed619a78b1c32e14a8657</anchor>
      <arglist>(int64_t data, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>encode_tag</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga7b5ae6582f28e14a37d50a2d243613c5</anchor>
      <arglist>(unsigned char *buffer, tag_t tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>encode_uint16</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gafad4dadc9bbc06596be44e7ecc4c7281</anchor>
      <arglist>(uint16_t data, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>encode_uint32</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga9b75c9d94d4f3d34d52f46c65cf950d4</anchor>
      <arglist>(uint32_t data, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>extract_header</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga82060dae45e1c3b922005e56829c9814</anchor>
      <arglist>(unsigned char *buffer, uint16_t *port_id, uint16_t *federate_id, size_t *length)</arglist>
    </member>
    <member kind="function">
      <type>int32_t</type>
      <name>extract_int32</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga8f772b5761c6b74b4136db6ee021e6c5</anchor>
      <arglist>(unsigned char *bytes)</arglist>
    </member>
    <member kind="function">
      <type>int64_t</type>
      <name>extract_int64</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga8decc0f4a38aa42fbc6ccfb029e3a061</anchor>
      <arglist>(unsigned char *bytes)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>extract_match_group</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga8ec38908b111a79943446bfbdec188f0</anchor>
      <arglist>(const char *rti_addr, char *dest, regmatch_t group, size_t max_len, size_t min_len, const char *err_msg)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>extract_match_groups</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga50c7f94caa2a61bcba5f89535da07036</anchor>
      <arglist>(const char *rti_addr, char **rti_addr_strs, bool **rti_addr_flags, regmatch_t *group_array, int *gids, size_t *max_lens, size_t *min_lens, const char **err_msgs)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>extract_rti_addr_info</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga0737fc3d45aae606811f57a16ad87208</anchor>
      <arglist>(const char *rti_addr, rti_addr_info_t *rti_addr_info)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>extract_tag</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga9cd95311c2c29ce5bed1c44d5336584d</anchor>
      <arglist>(unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>extract_timed_header</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga8e2cc45fc8571af05bb05f4952d4cde5</anchor>
      <arglist>(unsigned char *buffer, uint16_t *port_id, uint16_t *federate_id, size_t *length, tag_t *tag)</arglist>
    </member>
    <member kind="function">
      <type>uint16_t</type>
      <name>extract_uint16</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga1686d838d49741a6ff2ee65bd766a987</anchor>
      <arglist>(unsigned char *bytes)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>host_is_big_endian</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gad791461950852eb074b90bc75156b413</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>match_regex</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga7a568c79c856e633f5f181dd21700b74</anchor>
      <arglist>(const char *str, char *regex)</arglist>
    </member>
    <member kind="function">
      <type>int32_t</type>
      <name>swap_bytes_if_big_endian_int32</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaeaff8773e0cba7d0f8a6d03b8f0f7766</anchor>
      <arglist>(int32_t src)</arglist>
    </member>
    <member kind="function">
      <type>int64_t</type>
      <name>swap_bytes_if_big_endian_int64</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaa548ffc52c264f564127b80f63170c33</anchor>
      <arglist>(int64_t src)</arglist>
    </member>
    <member kind="function">
      <type>uint16_t</type>
      <name>swap_bytes_if_big_endian_uint16</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga48fa075b3a868790da8fb303a397cd60</anchor>
      <arglist>(uint16_t src)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>validate_host</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga0f6ec1479ffe28cc089fe6b13e675f0e</anchor>
      <arglist>(const char *host)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>validate_port</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga841bec9ddc3fb61c2b615f5d512dc3f0</anchor>
      <arglist>(char *port)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>validate_user</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga1285f4b0283c8e0c020e12e76a4426c2</anchor>
      <arglist>(const char *user)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>socket_common.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/federated/network/</path>
    <filename>socket__common_8h.html</filename>
    <includes id="low__level__platform_8h" name="low_level_platform.h" local="yes" import="no" module="no" objc="no">low_level_platform.h</includes>
    <member kind="define">
      <type>#define</type>
      <name>CONNECT_RETRY_INTERVAL</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gab2106828de539188aed925f592751c12</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CONNECT_TIMEOUT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga252b2cb72531cb00ecd4d4db37a5a473</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DEFAULT_PORT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga16b710f592bf8f7900666392adc444dc</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DELAY_BETWEEN_SOCKET_RETRIES</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga7b7cd916c6c027dc9ebdff449fb6edad</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MAX_NUM_PORT_ADDRESSES</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga5dbc42b5857eb262a06aa04399475d16</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_FAILED</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaf27674f627be1c469a529a995da5c074</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NUMBER_OF_FEDERATES</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaf58c457e08491f7cfd5a0a46940e11ad</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>PORT_BIND_RETRY_INTERVAL</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaf11c9d6cd02e9e78e38a848cf75205cb</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>PORT_BIND_RETRY_LIMIT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga04c08dc0b0733010f3190bf6df123433</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>TCP_TIMEOUT_TIME</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gab1edbb864391382835b9ad71408c5c53</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>UDP_TIMEOUT_TIME</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaac9f4a449d302b4f39e69a14b3a4c8d5</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>enum socket_type_t</type>
      <name>socket_type_t</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga45bb50f52b617bc6a30719cbaafd075d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>socket_type_t</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga54c375e3893ff5969d20df65b90c8335</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TCP</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gga54c375e3893ff5969d20df65b90c8335aa040cd7feeb588104634cdadf35abf1c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>UDP</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gga54c375e3893ff5969d20df65b90c8335adb542475cf9d0636e4225e216cee9ae6</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>accept_socket</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga3f3dfc2ccd62e181467f7a22ab5ebe49</anchor>
      <arglist>(int socket, int rti_socket)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>connect_to_socket</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga5ac5b1b8bf1c832cbdd2f6cdbb769df8</anchor>
      <arglist>(int sock, const char *hostname, int port)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>create_real_time_tcp_socket_errexit</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga10b0373c1cff0213b17cb7308949f0a2</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>create_server</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga94aaee169c4c822e4c9e6a73f59a6952</anchor>
      <arglist>(uint16_t port, int *final_socket, uint16_t *final_port, socket_type_t sock_type, bool increment_port_on_retry)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>init_shutdown_mutex</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gadc2dc02aa0e242eab3574240e90984b4</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>ssize_t</type>
      <name>peek_from_socket</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae4ba6b1361cd7c47e8a0eb70729d9636</anchor>
      <arglist>(int socket, unsigned char *result)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>read_from_socket</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaa8f7af0d4004aa925499fecefa1ac6b8</anchor>
      <arglist>(int socket, size_t num_bytes, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>read_from_socket_close_on_error</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga941fd71700b7646e6edbbb76db4f7bd2</anchor>
      <arglist>(int *socket, size_t num_bytes, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>read_from_socket_fail_on_error</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga46a44d92c24d3caadec0bc9e59a26361</anchor>
      <arglist>(int *socket, size_t num_bytes, unsigned char *buffer, char *format,...)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>shutdown_socket</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga365eba5b8b3f6445eeaffcb4435165c5</anchor>
      <arglist>(int *socket, bool read_before_closing)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>write_to_socket</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae8d4b83faeac37f665666429742813f9</anchor>
      <arglist>(int socket, size_t num_bytes, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>write_to_socket_close_on_error</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaf88884c303b81143ef5ab7af4683a66c</anchor>
      <arglist>(int *socket, size_t num_bytes, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write_to_socket_fail_on_error</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gada8d9360bdf4e9d7f36bbfc7e682f06e</anchor>
      <arglist>(int *socket, size_t num_bytes, unsigned char *buffer, lf_mutex_t *mutex, char *format,...)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>lf_token.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/</path>
    <filename>lf__token_8h.html</filename>
    <class kind="struct">lf_port_base_t</class>
    <class kind="struct">lf_sparse_io_record_t</class>
    <class kind="struct">lf_token_t</class>
    <class kind="struct">token_template_t</class>
    <class kind="struct">token_type_t</class>
    <member kind="typedef">
      <type>struct lf_port_base_t</type>
      <name>lf_port_base_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga91b17c088cd50ce69df73f1470a18799</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct lf_sparse_io_record_t</type>
      <name>lf_sparse_io_record_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaa6696d69bef6bb4bdd52ef9ab9d2c614</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct lf_token_t</type>
      <name>lf_token_t</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga973e404c4c1bd798a54501d0e1d640f9</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct self_base_t</type>
      <name>self_base_t</name>
      <anchorfile>lf__token_8h.html</anchorfile>
      <anchor>a6202eb05c29c30bfd6a8fc203de6422f</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>enum token_freed</type>
      <name>token_freed</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga0cb4f0fedba2f1e1fd3893440ab53647</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct token_template_t</type>
      <name>token_template_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gad0befcbc6fe23c8dd0b6f483d4067e45</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct token_type_t</type>
      <name>token_type_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga58dc2f4a624d2f3030b7e5f3596e58d6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>token_freed</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gabe23a36a87d2f0c076da417eb0114c7e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>NOT_FREED</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ggabe23a36a87d2f0c076da417eb0114c7ea3d7522b54086645e077eb70e78731c5c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>VALUE_FREED</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ggabe23a36a87d2f0c076da417eb0114c7ea417a0268666423fd955fea6f38cde238</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TOKEN_FREED</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ggabe23a36a87d2f0c076da417eb0114c7ea0a6094445d7a54e61aa3a43f6d017e2b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TOKEN_AND_VALUE_FREED</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ggabe23a36a87d2f0c076da417eb0114c7ea02ef194d373714ee3ac62226729e0cb0</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>token_freed</type>
      <name>_lf_done_using</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga88c890be1f8d45461a6985cbfe6faa99</anchor>
      <arglist>(lf_token_t *token)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_free_all_tokens</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gafd97c46ee623b1ae34a70088ee9b5020</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>token_freed</type>
      <name>_lf_free_token</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga2798a92c59a1d46b602298cdbd187ab1</anchor>
      <arglist>(lf_token_t *token)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_free_token_copies</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gab1efa737bf70317f885c1dc772c4f23b</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>lf_token_t *</type>
      <name>_lf_get_token</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga8f2f9c98968a10bf4d37077fd363ac48</anchor>
      <arglist>(token_template_t *tmplt)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_initialize_template</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga5d1a2996844350bc1e29de47e3b56644</anchor>
      <arglist>(token_template_t *tmplt, size_t element_size)</arglist>
    </member>
    <member kind="function">
      <type>lf_token_t *</type>
      <name>_lf_initialize_token</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga6e2c7940f2e59f5ff57807df6b41f5fe</anchor>
      <arglist>(token_template_t *tmplt, size_t length)</arglist>
    </member>
    <member kind="function">
      <type>lf_token_t *</type>
      <name>_lf_initialize_token_with_value</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gac171b72d59f37653f012d30cad72a2d2</anchor>
      <arglist>(token_template_t *tmplt, void *value, size_t length)</arglist>
    </member>
    <member kind="function">
      <type>lf_token_t *</type>
      <name>_lf_new_token</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga4616dad8eeb4cbe04a4f9697d3de9b16</anchor>
      <arglist>(token_type_t *type, void *value, size_t length)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_replace_template_token</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gabeff98dcfb6b5715aac8e1438c5a6e77</anchor>
      <arglist>(token_template_t *tmplt, lf_token_t *newtoken)</arglist>
    </member>
    <member kind="function">
      <type>lf_token_t *</type>
      <name>lf_new_token</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaa47b51a11727eec252ff7e786794bd88</anchor>
      <arglist>(void *port_or_action, void *val, size_t len)</arglist>
    </member>
    <member kind="function">
      <type>lf_token_t *</type>
      <name>lf_writable_copy</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaaf8e6f18b021d0b8ece7e1b64280432f</anchor>
      <arglist>(lf_port_base_t *port)</arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>_lf_count_token_allocations</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaf4657205de7da8f0bf7b346985a983fc</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>lf_types.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/</path>
    <filename>lf__types_8h.html</filename>
    <includes id="modes_8h" name="modes.h" local="yes" import="no" module="no" objc="no">modal_models/modes.h</includes>
    <includes id="pqueue_8h" name="pqueue.h" local="yes" import="no" module="no" objc="no">utils/pqueue.h</includes>
    <includes id="pqueue__tag_8h" name="pqueue_tag.h" local="yes" import="no" module="no" objc="no">utils/pqueue_tag.h</includes>
    <includes id="lf__token_8h" name="lf_token.h" local="yes" import="no" module="no" objc="no">lf_token.h</includes>
    <includes id="tag_8h" name="tag.h" local="yes" import="no" module="no" objc="no">tag.h</includes>
    <includes id="vector_8h" name="vector.h" local="yes" import="no" module="no" objc="no">vector.h</includes>
    <class kind="struct">allocation_record_t</class>
    <class kind="struct">event_t</class>
    <class kind="struct">lf_action_base_t</class>
    <class kind="struct">lf_action_internal_t</class>
    <class kind="struct">lf_port_internal_t</class>
    <class kind="struct">lf_tag_advancement_barrier_t</class>
    <class kind="struct">reaction_t</class>
    <class kind="struct">self_base_t</class>
    <class kind="struct">trigger_t</class>
    <member kind="define">
      <type>#define</type>
      <name>SCHED_ADAPTIVE</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga7516169f705d99222725e6970f0ec703</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SCHED_GEDF_NP</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga0238f536f81a61c0d568b36eac9b9a00</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SCHED_NP</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gacc410134875d15b02634fb0aa8163a00</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct allocation_record_t</type>
      <name>allocation_record_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga416845ec4469b3186de047c32402f5e8</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct event_t</type>
      <name>event_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga59f5f6b9c6023baebf9c49c328b639a1</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>pqueue_pri_t</type>
      <name>index_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga4448d06be794d3f5412d0edb412dc00e</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct lf_tag_advancement_barrier_t</type>
      <name>lf_tag_advancement_barrier_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga18d624d162daca00e24d1d528ec3c18f</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>reaction_function_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga149e5fee1c1841bcc96c72f200601d90</anchor>
      <arglist>)(void *)</arglist>
    </member>
    <member kind="typedef">
      <type>struct reaction_t</type>
      <name>reaction_t</name>
      <anchorfile>lf__types_8h.html</anchorfile>
      <anchor>a1f0f71b6c8e0c54c65cbfaa154b40694</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct self_base_t</type>
      <name>self_base_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga6202eb05c29c30bfd6a8fc203de6422f</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>char *</type>
      <name>string</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga4505c08c065b48840a30eedd9845cce2</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>int</type>
      <name>trigger_handle_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga3608c2ed78ba97535f8d82a489846305</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct trigger_t</type>
      <name>trigger_t</name>
      <anchorfile>lf__types_8h.html</anchorfile>
      <anchor>ae98c19a4a03c495fdd3044c206e99afc</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>unsigned short int</type>
      <name>ushort</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga3fa7784c89589b49764048e9909d0e07</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>lf_spacing_policy_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga0183c0b43037a172a1cd9aa6ed6b3822</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>defer</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga0183c0b43037a172a1cd9aa6ed6b3822a893b1cf0de04eaf44a009fecabd16b90</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>drop</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga0183c0b43037a172a1cd9aa6ed6b3822a1e34755950041e469ca91ff2b7d1c019</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>replace</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga0183c0b43037a172a1cd9aa6ed6b3822a8a4df390c6f816287b90cb2b33ab4323</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>update</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga0183c0b43037a172a1cd9aa6ed6b3822a15edc24cdf7dea17a43c6c50580eba2b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>port_status_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga759ba374f75ea0025b9af1bb35f14d7e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>absent</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga759ba374f75ea0025b9af1bb35f14d7ea1a8fae68a24a59c5629c241401fabb08</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>present</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga759ba374f75ea0025b9af1bb35f14d7eaaeb73d7cb56b19bff3d9f80426ed3267</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>unknown</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga759ba374f75ea0025b9af1bb35f14d7ea5b9f6d065e6e98483b3d3ed01f4f6cbe</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>reaction_status_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga6e496c05213aa4bcbc0055ceee7808fa</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>inactive</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga6e496c05213aa4bcbc0055ceee7808faa76c1253bb97844abbdf89af6dfc3c7d6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>queued</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga6e496c05213aa4bcbc0055ceee7808faa8ff224790af0c8a18f259da89dfb2225</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>running</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga6e496c05213aa4bcbc0055ceee7808faab514bba77fe136c3a3b6f56b818f7b0c</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>mixed_radix.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/</path>
    <filename>mixed__radix_8h.html</filename>
    <class kind="struct">mixed_radix_int_t</class>
    <member kind="typedef">
      <type>struct mixed_radix_int_t</type>
      <name>mixed_radix_int_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga7d95374fb5368705263c2f1ac2579183</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>mixed_radix_incr</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaab4b6c3ec9d416bc0965f81ff9194736</anchor>
      <arglist>(mixed_radix_int_t *mixed)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>mixed_radix_parent</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga9405e2b2a5f79663f57c7933a2cec2b8</anchor>
      <arglist>(mixed_radix_int_t *mixed, int n)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>mixed_radix_to_int</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga0402727c71049a3b200c1f9fbfdfcb41</anchor>
      <arglist>(mixed_radix_int_t *mixed)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>modes.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/modal_models/</path>
    <filename>modes_8h.html</filename>
    <includes id="lf__types_8h" name="lf_types.h" local="yes" import="no" module="no" objc="no">lf_types.h</includes>
    <includes id="tag_8h" name="tag.h" local="yes" import="no" module="no" objc="no">tag.h</includes>
    <class kind="struct">mode_state_variable_reset_data_t</class>
    <class kind="struct">reactor_mode_state_t</class>
    <class kind="struct">reactor_mode_t</class>
    <member kind="define">
      <type>#define</type>
      <name>_LF_SET_MODE_WITH_TYPE</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>gaa83c858bb273af532a903ba430b6b87e</anchor>
      <arglist>(mode, change_type)</arglist>
    </member>
    <member kind="typedef">
      <type>struct mode_state_variable_reset_data_t</type>
      <name>mode_state_variable_reset_data_t</name>
      <anchorfile>modes_8h.html</anchorfile>
      <anchor>aa0d16fb5be46a95c06fe8b41ba0163f1</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct reactor_mode_state_t</type>
      <name>reactor_mode_state_t</name>
      <anchorfile>modes_8h.html</anchorfile>
      <anchor>a322742003099a35eebd3c66ce26fa3fa</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct reactor_mode_t</type>
      <name>reactor_mode_t</name>
      <anchorfile>modes_8h.html</anchorfile>
      <anchor>a96f0d86e5736a519dcb4c5862fd5b8d4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>lf_mode_change_type_t</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>gadd32beb39577775204a6f1ed1f947df9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>no_transition</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>ggadd32beb39577775204a6f1ed1f947df9acbde8a6b8987eeb436dbcc6127b6f65d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>reset_transition</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>ggadd32beb39577775204a6f1ed1f947df9a6af83e1bf871157796f45f54867ec8ff</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>history_transition</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>ggadd32beb39577775204a6f1ed1f947df9a8ed3a9f84d1b9a4b7b8ddb92567887b5</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_add_suspended_event</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>ga3d965a89cfdb1a7539f352227c951165</anchor>
      <arglist>(event_t *event)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_handle_mode_changes</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>ga8818f3e66cd3aaf22372631a8806f87b</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_handle_mode_shutdown_reactions</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>ga8f4063faae1603cd785180388799574d</anchor>
      <arglist>(environment_t *env, reaction_t **shutdown_reactions, int shutdown_reactions_size)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_handle_mode_startup_reset_reactions</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>gad52c1087a0fdb07c57978d7e4cbc8400</anchor>
      <arglist>(environment_t *env, reaction_t **startup_reactions, int startup_reactions_size, reaction_t **reset_reactions, int reset_reactions_size, reactor_mode_state_t *states[], int states_size)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_handle_mode_triggered_reactions</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>ga4d51921004a50a572fd95a884b19f812</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_initialize_mode_states</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>gae0e14964227e6b844179c1bc71a9aabb</anchor>
      <arglist>(environment_t *env, reactor_mode_state_t *states[], int states_size)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_initialize_modes</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>gaa330772299f55fb273cb0350d08d91ed</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>_lf_mode_is_active</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>ga4280d9bbbef5095c7bf4ebdff2b0df90</anchor>
      <arglist>(reactor_mode_t *mode)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_process_mode_changes</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>ga8833697b31d0e552d6e9e75b9f7458b5</anchor>
      <arglist>(environment_t *env, reactor_mode_state_t *states[], int states_size, mode_state_variable_reset_data_t reset_data[], int reset_data_size, trigger_t *timer_triggers[], int timer_triggers_size)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_terminate_modal_reactors</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>gaef08f864ed14b6382a9746b80c1819dd</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>port.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/</path>
    <filename>port_8h.html</filename>
    <includes id="lf__token_8h" name="lf_token.h" local="yes" import="no" module="no" objc="no">lf_token.h</includes>
    <class kind="struct">lf_multiport_iterator_t</class>
    <member kind="define">
      <type>#define</type>
      <name>lf_multiport_iterator</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga5759266c62b989a6b305584cb72f8840</anchor>
      <arglist>(in)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_SPARSE_CAPACITY_DIVIDER</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga3e66ec583172bbad678982af8c57001b</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_SPARSE_WIDTH_THRESHOLD</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaafcc3f0b909a44166db182035ca759c6</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct lf_multiport_iterator_t</type>
      <name>lf_multiport_iterator_t</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaf8a81a7373f3d5f77fd865e437964ef3</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>lf_multiport_iterator_t</type>
      <name>_lf_multiport_iterator_impl</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga23e88870b9a699d1a067ff5b397e0887</anchor>
      <arglist>(lf_port_base_t **port, int width)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_multiport_next</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gac7c743d3c64a839642e54781b8e9127f</anchor>
      <arglist>(lf_multiport_iterator_t *iterator)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>reactor.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/</path>
    <filename>reactor_8h.html</filename>
    <includes id="lf__types_8h" name="lf_types.h" local="yes" import="no" module="no" objc="no">lf_types.h</includes>
    <includes id="modes_8h" name="modes.h" local="yes" import="no" module="no" objc="no">modes.h</includes>
    <includes id="port_8h" name="port.h" local="yes" import="no" module="no" objc="no">port.h</includes>
    <includes id="tag_8h" name="tag.h" local="yes" import="no" module="no" objc="no">tag.h</includes>
    <includes id="clock_8h" name="clock.h" local="yes" import="no" module="no" objc="no">clock.h</includes>
    <includes id="tracepoint_8h" name="tracepoint.h" local="yes" import="no" module="no" objc="no">tracepoint.h</includes>
    <includes id="util_8h" name="util.h" local="yes" import="no" module="no" objc="no">util.h</includes>
    <member kind="define">
      <type>#define</type>
      <name>SUPPRESS_UNUSED_WARNING</name>
      <anchorfile>reactor_8h.html</anchorfile>
      <anchor>a4625be3023ef492f25fd5193ef754774</anchor>
      <arglist>(x)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>lf_allocate</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga1a5fdb69bc112879d4014bb0790e843c</anchor>
      <arglist>(size_t count, size_t size, struct allocation_record_t **head)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_free_all_reactors</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gadf76c4fc43b07691236fa6a483762481</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_free_reactor</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga2f0b7c8d624c2da93012538bd93568ad</anchor>
      <arglist>(self_base_t *self)</arglist>
    </member>
    <member kind="function">
      <type>interval_t</type>
      <name>lf_get_sta</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae03d197bf8d64f82be4a68c95a940195</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>interval_t</type>
      <name>lf_get_stp_offset</name>
      <anchorfile>reactor_8h.html</anchorfile>
      <anchor>a7bca70211dd5f68beea197df4e4a17b8</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>lf_is_tag_after_stop_tag</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaf4bdd144e6cc65dfa3b996d4bd82f83a</anchor>
      <arglist>(environment_t *env, tag_t tag)</arglist>
    </member>
    <member kind="function">
      <type>self_base_t *</type>
      <name>lf_new_reactor</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga129c6df527165b2378d1dc4852411c35</anchor>
      <arglist>(size_t size)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_print_snapshot</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaae4a4a9ce970c18bff7785cf7863777c</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>lf_reactor_full_name</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gab5bf112c4237da16d08736ff3e3e36b8</anchor>
      <arglist>(self_base_t *self)</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>lf_reactor_name</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gae76171d83c29dadd3adb3f9294a92138</anchor>
      <arglist>(self_base_t *self)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_request_stop</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gab49affc958f705d9e33c5e3463848bda</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_set_present</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gadaa6f5f1a265e7a37aeb3b6a0d101732</anchor>
      <arglist>(lf_port_base_t *port)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_set_sta</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaccbc0981eff8f1f1726075d2ee4ba0ef</anchor>
      <arglist>(interval_t offset)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_set_stop_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga07b2e94bcac5d7bcfd47d4eaf35a4977</anchor>
      <arglist>(environment_t *env, tag_t tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_set_stp_offset</name>
      <anchorfile>reactor_8h.html</anchorfile>
      <anchor>a7354a8f02313a836f25c9e76168991ab</anchor>
      <arglist>(interval_t offset)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>reactor_common.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/</path>
    <filename>reactor__common_8h.html</filename>
    <includes id="lf__types_8h" name="lf_types.h" local="yes" import="no" module="no" objc="no">lf_types.h</includes>
    <includes id="environment_8h" name="environment.h" local="yes" import="no" module="no" objc="no">environment.h</includes>
    <includes id="tag_8h" name="tag.h" local="yes" import="no" module="no" objc="no">tag.h</includes>
    <includes id="pqueue_8h" name="pqueue.h" local="yes" import="no" module="no" objc="no">pqueue.h</includes>
    <includes id="vector_8h" name="vector.h" local="yes" import="no" module="no" objc="no">vector.h</includes>
    <includes id="util_8h" name="util.h" local="yes" import="no" module="no" objc="no">util.h</includes>
    <includes id="modes_8h" name="modes.h" local="yes" import="no" module="no" objc="no">modes.h</includes>
    <includes id="port_8h" name="port.h" local="yes" import="no" module="no" objc="no">port.h</includes>
    <member kind="define">
      <type>#define</type>
      <name>MIN_SLEEP_DURATION</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga3b755b6f58cb9ea64ae2f1ba9a382c86</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_advance_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga4659db62370c837baa55484134b3bfb7</anchor>
      <arglist>(environment_t *env, tag_t next_tag)</arglist>
    </member>
    <member kind="function">
      <type>event_t *</type>
      <name>_lf_create_dummy_events</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga6396bfcdac50ddb71b6b29fa33a3cc5d</anchor>
      <arglist>(environment_t *env, tag_t tag)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>_lf_initialize_timer</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga61a2c70695093f8a38b1e922fb36547f</anchor>
      <arglist>(environment_t *env, trigger_t *timer)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>_lf_initialize_timers</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga37bdd5c8fe3428b85eff05f0629da411</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_initialize_trigger_objects</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga52ffa06ff177dc19d33713beb2ff344e</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>_lf_insert_reactions_for_trigger</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaeac3f6a2d15f30e3adecc9f431162bef</anchor>
      <arglist>(environment_t *env, trigger_t *trigger, lf_token_t *token)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_invoke_reaction</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga7fe988f0eee005defaa2ad2c9f1f2fd8</anchor>
      <arglist>(environment_t *env, reaction_t *reaction, int worker)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_pop_events</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga41e1c14ed7c1ab5ab19b8b98d84006b6</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>_lf_schedule_at_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga9d2634d70492498740984f320dffe8f0</anchor>
      <arglist>(environment_t *env, trigger_t *trigger, tag_t tag, lf_token_t *token)</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>_lf_schedule_copy</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaf31c25686db5996e9f3493745e63856a</anchor>
      <arglist>(environment_t *env, void *action, interval_t offset, void *value, size_t length)</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>_lf_schedule_token</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga6d8b49ac9cf089b35a5d2df6a9209255</anchor>
      <arglist>(environment_t *env, void *action, interval_t extra_delay, lf_token_t *token)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_start_time_step</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gab5d69d8631d56d64fb90547a8d6b10cd</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_trigger_reaction</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaf31f8aca1b004a6e5e0e695063de1b47</anchor>
      <arglist>(environment_t *env, reaction_t *reaction, int worker_number)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_trigger_shutdown_reactions</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga282a342efac4fc3e198fb9656f0a9adc</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_trigger_startup_reactions</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gafd3a0abded3adbc25ab7dbc261e7b16c</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>initialize_global</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga42d0bf55641d6ff4390081175de65496</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>index_t</type>
      <name>lf_combine_deadline_and_level</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga6876ddf559d9ecf14ae78f76e6ff2045</anchor>
      <arglist>(interval_t deadline, int level)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_create_environments</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaf2a6b2663dca116472afc45b50040a3d</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_free</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaa028b6b458854278bb2a2de486e40268</anchor>
      <arglist>(struct allocation_record_t **head)</arglist>
    </member>
    <member kind="function">
      <type>event_t *</type>
      <name>lf_get_new_event</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaeb163964110b0029fc4c460b2478ea4d</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_recycle_event</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga0dbdf3a0cb8b0075acfc45437f4c7e27</anchor>
      <arglist>(environment_t *env, event_t *e)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_replace_token</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga3c0888056123fd1ef7c2fdd7a8081ddf</anchor>
      <arglist>(event_t *event, lf_token_t *token)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_set_default_command_line_options</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga2dce2075be67995107b9d8f2d5e20551</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_terminate_execution</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gad149603053631bf4d6236426ddae2bde</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>process_args</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga80aaf4eeed3e2902f8fe9de80b45777d</anchor>
      <arglist>(int argc, const char *argv[])</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>schedule_output_reactions</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaeaaa76aeb7d93efc4e0f0c484548af70</anchor>
      <arglist>(environment_t *env, reaction_t *reaction, int worker)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>termination</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaa329f59a16f5617b5195f2c05872c9e9</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>_lf_normal_termination</name>
      <anchorfile>reactor__common_8h.html</anchorfile>
      <anchor>a30eba499960e8780d3746d3ad433c371</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>unsigned int</type>
      <name>_lf_number_of_workers</name>
      <anchorfile>reactor__common_8h.html</anchorfile>
      <anchor>a01fd0d1404992f7d875791a764a26925</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct allocation_record_t *</type>
      <name>_lf_reactors_to_free</name>
      <anchorfile>reactor__common_8h.html</anchorfile>
      <anchor>af4b5bfd131da48f0fb95eab14e6e61c8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>default_argc</name>
      <anchorfile>reactor__common_8h.html</anchorfile>
      <anchor>aab46895a3a4d1341a19b689a0ea902d1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char **</type>
      <name>default_argv</name>
      <anchorfile>reactor__common_8h.html</anchorfile>
      <anchor>ad0fd32b8326e2d630ee8f4a431ea2867</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>instant_t</type>
      <name>duration</name>
      <anchorfile>reactor__common_8h.html</anchorfile>
      <anchor>a8079fdfadf07cba8a660c9c25cb9dc77</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>fast</name>
      <anchorfile>reactor__common_8h.html</anchorfile>
      <anchor>a9c91e5c84c3910df17c909ccdea074db</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>keepalive_specified</name>
      <anchorfile>reactor__common_8h.html</anchorfile>
      <anchor>a3e69bf5802b3968ed00af688cc7a8006</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>interval_t</type>
      <name>lf_fed_STA_offset</name>
      <anchorfile>reactor__common_8h.html</anchorfile>
      <anchor>a21ebb201d06c3d2e564e7a4867ff3a7d</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>reactor_threaded.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/threaded/</path>
    <filename>reactor__threaded_8h.html</filename>
    <includes id="lf__types_8h" name="lf_types.h" local="yes" import="no" module="no" objc="no">lf_types.h</includes>
    <member kind="function">
      <type>void</type>
      <name>_lf_decrement_tag_barrier_locked</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga5a0540790dfc6d954fb443da3336ce27</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_increment_tag_barrier</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaa8e97abcbd89bb371d396da44ff4becb</anchor>
      <arglist>(environment_t *env, tag_t future_tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_increment_tag_barrier_locked</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga01d3c6cadb7930c096ffe1f794173f5c</anchor>
      <arglist>(environment_t *env, tag_t future_tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_next_locked</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gab9e357a21e338cd3719cdec409b9f7a6</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>_lf_wait_on_tag_barrier</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga3d1cd5263a79c14e62a5fb34530a0a93</anchor>
      <arglist>(environment_t *env, tag_t proposed_tag)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>get_next_event_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga9b50c51a9046dfb8814b2f609020d0a4</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>send_next_event_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga6f9a4a14de3aa9e560935a57093eb122</anchor>
      <arglist>(environment_t *env, tag_t tag, bool wait_for_reply)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>wait_until</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga70c4ab92f00f9bcc31e4d696db1c0526</anchor>
      <arglist>(instant_t wait_until_time, lf_cond_t *condition)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>scheduler.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/threaded/</path>
    <filename>scheduler_8h.html</filename>
    <includes id="lf__types_8h" name="lf_types.h" local="yes" import="no" module="no" objc="no">lf_types.h</includes>
    <includes id="scheduler__instance_8h" name="scheduler_instance.h" local="yes" import="no" module="no" objc="no">scheduler_instance.h</includes>
    <member kind="function">
      <type>void</type>
      <name>lf_sched_done_with_reaction</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga485e4339d95d23ae5bcbb06c244e7145</anchor>
      <arglist>(size_t worker_number, reaction_t *done_reaction)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_sched_free</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga2139bc60dc5be91d750d5e877af07843</anchor>
      <arglist>(lf_scheduler_t *scheduler)</arglist>
    </member>
    <member kind="function">
      <type>reaction_t *</type>
      <name>lf_sched_get_ready_reaction</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga95107b668caa59d5bca9fff1af21e7fb</anchor>
      <arglist>(lf_scheduler_t *scheduler, int worker_number)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_sched_init</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga0ebe8d7670a73a6572c7152d31e1fb62</anchor>
      <arglist>(environment_t *env, size_t number_of_workers, sched_params_t *parameters)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_scheduler_trigger_reaction</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gae47f6c04336244e4739c05f5c38e730e</anchor>
      <arglist>(lf_scheduler_t *scheduler, reaction_t *reaction, int worker_number)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>scheduler_instance.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/threaded/</path>
    <filename>scheduler__instance_8h.html</filename>
    <class kind="struct">lf_scheduler_t</class>
    <class kind="struct">sched_params_t</class>
    <member kind="define">
      <type>#define</type>
      <name>DEFAULT_MAX_REACTION_LEVEL</name>
      <anchorfile>scheduler__instance_8h.html</anchorfile>
      <anchor>a7c3f7c6b1c85f7572b42fa34fa853973</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NUMBER_OF_WORKERS</name>
      <anchorfile>scheduler__instance_8h.html</anchorfile>
      <anchor>a43fcdba4ed8864d210afd3f031b4e346</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct custom_scheduler_data_t</type>
      <name>custom_scheduler_data_t</name>
      <anchorfile>scheduler__instance_8h.html</anchorfile>
      <anchor>a3b87d9f4ab1631bfa6dbea7f81bfdd38</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct lf_scheduler_t</type>
      <name>lf_scheduler_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga0199f9b027e13cf08095d91fe798c663</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>init_sched_instance</name>
      <anchorfile>scheduler__instance_8h.html</anchorfile>
      <anchor>a763b254fdbe81abb51ce307567969306</anchor>
      <arglist>(struct environment_t *env, lf_scheduler_t **instance, size_t number_of_workers, sched_params_t *params)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>scheduler_sync_tag_advance.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/threaded/</path>
    <filename>scheduler__sync__tag__advance_8h.html</filename>
    <includes id="tag_8h" name="tag.h" local="yes" import="no" module="no" objc="no">tag.h</includes>
    <includes id="scheduler__instance_8h" name="scheduler_instance.h" local="yes" import="no" module="no" objc="no">scheduler_instance.h</includes>
    <member kind="function">
      <type>bool</type>
      <name>_lf_sched_advance_tag_locked</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaf2c6b4fac0a87c3cc914c713714e1fca</anchor>
      <arglist>(lf_scheduler_t *sched)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>logical_tag_complete</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gad96dd94446ff66184dcf0f8f65cdb4f0</anchor>
      <arglist>(tag_t tag_to_send)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>should_stop_locked</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga5ee7c21a8b90bb09784f221c1de4d9c9</anchor>
      <arglist>(lf_scheduler_t *sched)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>watchdog.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/threaded/</path>
    <filename>watchdog_8h.html</filename>
    <includes id="lf__types_8h" name="lf_types.h" local="yes" import="no" module="no" objc="no">lf_types.h</includes>
    <includes id="environment_8h" name="environment.h" local="yes" import="no" module="no" objc="no">environment.h</includes>
    <includes id="platform_8h" name="platform.h" local="yes" import="no" module="no" objc="no">platform.h</includes>
    <class kind="struct">watchdog_t</class>
    <member kind="typedef">
      <type>void(*</type>
      <name>watchdog_function_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga4caef7fcd0476a936700512d28a23aa8</anchor>
      <arglist>)(void *)</arglist>
    </member>
    <member kind="typedef">
      <type>struct watchdog_t</type>
      <name>watchdog_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaee1cd2bc521f76fa428cc659474d9570</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_initialize_watchdogs</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaa6a016400f119168b48505e51baaaa55</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_watchdog_terminate_all</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gab3957d31bade9b6ebcbc27aae6be3f14</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_watchdog_start</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga82bf2c7bd91fdf03b357914cf875dbb9</anchor>
      <arglist>(watchdog_t *watchdog, interval_t additional_timeout)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_watchdog_stop</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaa27ad22f94bbdaa33b99fe6cd81f1bdc</anchor>
      <arglist>(watchdog_t *watchdog)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>tracepoint.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/</path>
    <filename>tracepoint_8h.html</filename>
    <includes id="lf__types_8h" name="lf_types.h" local="yes" import="no" module="no" objc="no">lf_types.h</includes>
    <includes id="net__common_8h" name="net_common.h" local="yes" import="no" module="no" objc="no">net_common.h</includes>
    <includes id="trace__types_8h" name="trace_types.h" local="yes" import="no" module="no" objc="no">trace_types.h</includes>
    <includes id="trace_8h" name="trace.h" local="yes" import="no" module="no" objc="no">trace.h</includes>
    <class kind="struct">trace_record_t</class>
    <member kind="define">
      <type>#define</type>
      <name>tracepoint_reaction_deadline_missed</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaaf1dece34c4fcc135c2bd4feaf44e095</anchor>
      <arglist>(env, reaction, worker)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>tracepoint_reaction_ends</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaebbdf64d4b017a879c69fcda11e74efe</anchor>
      <arglist>(env, reaction, worker)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>tracepoint_reaction_starts</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga8c90883c30c8d773cde2df65f9f95e59</anchor>
      <arglist>(env, reaction, worker)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>tracepoint_scheduler_advancing_time_ends</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga77931b6e1d5a6c7f462902e78db801ba</anchor>
      <arglist>(env)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>tracepoint_scheduler_advancing_time_starts</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga65f95d31a900f5cde73f152c36f116bf</anchor>
      <arglist>(env)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>tracepoint_worker_wait_ends</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga4ea3c33342ac48ed332eb540a14ea53f</anchor>
      <arglist>(env, worker)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>tracepoint_worker_wait_starts</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga5d07ee55d71cbd545ad4bb577c2dc6b9</anchor>
      <arglist>(env, worker)</arglist>
    </member>
    <member kind="typedef">
      <type>INTERNAL struct trace_record_t</type>
      <name>trace_record_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga7804d825257e1eb4296de7da8fec62f6</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>_lf_register_trace_event</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga292c73e4f09daa50330b53079df620a9</anchor>
      <arglist>(void *pointer1, void *pointer2, _lf_trace_object_t type, char *description)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>call_tracepoint</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gad0ae74c1c8d1935b3fb92e546988503c</anchor>
      <arglist>(int event_type, void *reactor, tag_t tag, int worker, int src_id, int dst_id, instant_t *physical_time, trigger_t *trigger, interval_t extra_delay)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_tracing_check_version</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga5a16cb75bd134d91bbb002b5d1ddc45c</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>register_user_trace_event</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gae6078a659280c57ae1c4dfe939f319e7</anchor>
      <arglist>(void *self, char *description)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tracepoint_federate_from_federate</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga43868b9ea442f34eedbfe7052247f0a3</anchor>
      <arglist>(trace_event_t event_type, int fed_id, int partner_id, tag_t *tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tracepoint_federate_from_rti</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gab1bd5a0184ea0773425733fe1d9faa1c</anchor>
      <arglist>(trace_event_t event_type, int fed_id, tag_t *tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tracepoint_federate_to_federate</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga1307585fa1ca4dc4506f0398842115ee</anchor>
      <arglist>(trace_event_t event_type, int fed_id, int partner_id, tag_t *tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tracepoint_federate_to_rti</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaa074cf1f2690197f9edfd7a115381d6a</anchor>
      <arglist>(trace_event_t event_type, int fed_id, tag_t *tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tracepoint_rti_from_federate</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga13560f0cae54155219f4d8d9c487b0ef</anchor>
      <arglist>(trace_event_t event_type, int fed_id, tag_t *tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tracepoint_rti_to_federate</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga0da8aecc05366988f8d929118be90bbe</anchor>
      <arglist>(trace_event_t event_type, int fed_id, tag_t *tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tracepoint_schedule</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga896f27619ab0582d5a70d8f613567671</anchor>
      <arglist>(environment_t *env, trigger_t *trigger, interval_t extra_delay)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tracepoint_user_event</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gacd97e049ba442bdd90f020c0b7c1a4fa</anchor>
      <arglist>(void *self, char *description)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tracepoint_user_value</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga064170081cf3de9c123b58bdc51b1d4f</anchor>
      <arglist>(void *self, char *description, long long value)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>hashset.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/utils/hashset/</path>
    <filename>hashset_8h.html</filename>
    <class kind="struct">hashset_st</class>
    <member kind="typedef">
      <type>struct hashset_st *</type>
      <name>hashset_t</name>
      <anchorfile>hashset_8h.html</anchorfile>
      <anchor>ad69fd91d4c662d832841b95188dd47f4</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>hashset_add</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga31e95da7ee1f76c30b9fec773d9b380c</anchor>
      <arglist>(hashset_t set, void *item)</arglist>
    </member>
    <member kind="function">
      <type>hashset_t</type>
      <name>hashset_create</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga2c94a06d31c15e8da48b04b3de1a9113</anchor>
      <arglist>(unsigned short nbits)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>hashset_destroy</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga6e280b06a1572145d3211e36e47eea6e</anchor>
      <arglist>(hashset_t set)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>hashset_is_member</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga55de036dbc978294c262a7751f4caa81</anchor>
      <arglist>(hashset_t set, void *item)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>hashset_num_items</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga8d4800d73d1a58ad953501d9035de5ec</anchor>
      <arglist>(hashset_t set)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>hashset_remove</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga5b0ad513d6e64cd754213b0103a094e0</anchor>
      <arglist>(hashset_t set, void *item)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>hashset_itr.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/utils/hashset/</path>
    <filename>hashset__itr_8h.html</filename>
    <includes id="hashset_8h" name="hashset.h" local="yes" import="no" module="no" objc="no">hashset.h</includes>
    <class kind="struct">hashset_itr_st</class>
    <member kind="typedef">
      <type>struct hashset_itr_st *</type>
      <name>hashset_itr_t</name>
      <anchorfile>hashset__itr_8h.html</anchorfile>
      <anchor>aabacd1abe5de3d3d03fb2b649a1f2ba5</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>hashset_itr_t</type>
      <name>hashset_iterator</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga5350d2cd6a1be22846c5541816fb857f</anchor>
      <arglist>(hashset_t set)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>hashset_iterator_has_next</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga50a2126c05927f7d11b0859af5a38f02</anchor>
      <arglist>(hashset_itr_t itr)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>hashset_iterator_next</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga1f8e715379e1c95db280451114056bd8</anchor>
      <arglist>(hashset_itr_t itr)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>hashset_iterator_value</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga08d3680504ca63e10abbf03b115acb1b</anchor>
      <arglist>(hashset_itr_t itr)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>hashmap.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/utils/impl/</path>
    <filename>hashmap_8h.html</filename>
    <class kind="struct">hashmap_entry_t</class>
    <class kind="struct">hashmap_t</class>
    <member kind="define">
      <type>#define</type>
      <name>HASH_OF</name>
      <anchorfile>hashmap_8h.html</anchorfile>
      <anchor>a8747d841d1bffb751bf1223e2020e49d</anchor>
      <arglist>(key)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>HASHMAP</name>
      <anchorfile>hashmap_8h.html</anchorfile>
      <anchor>acbbbf0a5df7e698a480154ab66280715</anchor>
      <arglist>(token)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>K</name>
      <anchorfile>hashmap_8h.html</anchorfile>
      <anchor>a97d832ae23af4f215e801e37e4f94254</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>V</name>
      <anchorfile>hashmap_8h.html</anchorfile>
      <anchor>af40a326b23c68a27cebe60f16634a2cb</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct hashmap_entry_t</type>
      <name>hashmap_entry_t</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga1044f93547018a97e5d3b50cac8a1516</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct hashmap_t</type>
      <name>hashmap_t</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga19e5de2971328f1de610c3a3f42d295f</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>hashmap_free</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gafa5597a9582cc604eb0da7aac536ce4c</anchor>
      <arglist>(hashmap_t *hashmap)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>hashmap_get</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga510dfb7539122a20f4f0e299a2d33c88</anchor>
      <arglist>(hashmap_t *hashmap, void *key)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static hashmap_entry_t *</type>
      <name>hashmap_get_actual_address</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga67344ac16c34de2b935e8ff5f343cf6c</anchor>
      <arglist>(hashmap_t *hashmap, void *key)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static hashmap_entry_t *</type>
      <name>hashmap_get_ideal_address</name>
      <anchorfile>hashmap_8h.html</anchorfile>
      <anchor>ae7240e51afc60cf0ae475cf03f7f1491</anchor>
      <arglist>(hashmap_t *hashmap, void *key)</arglist>
    </member>
    <member kind="function">
      <type>hashmap_t *</type>
      <name>hashmap_new</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gac0b6f823784483f653698db3371a7036</anchor>
      <arglist>(size_t capacity, void *nothing)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>hashmap_put</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga47e0b1518c46a6cf0ea799c49490581e</anchor>
      <arglist>(hashmap_t *hashmap, void *key, void *value)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>pointer_hashmap.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/utils/impl/</path>
    <filename>pointer__hashmap_8h.html</filename>
    <includes id="hashmap_8h" name="hashmap.h" local="yes" import="no" module="no" objc="no">hashmap.h</includes>
    <member kind="define">
      <type>#define</type>
      <name>HASH_OF</name>
      <anchorfile>pointer__hashmap_8h.html</anchorfile>
      <anchor>a8747d841d1bffb751bf1223e2020e49d</anchor>
      <arglist>(key)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>HASHMAP</name>
      <anchorfile>pointer__hashmap_8h.html</anchorfile>
      <anchor>acbbbf0a5df7e698a480154ab66280715</anchor>
      <arglist>(token)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>K</name>
      <anchorfile>pointer__hashmap_8h.html</anchorfile>
      <anchor>a97d832ae23af4f215e801e37e4f94254</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>V</name>
      <anchorfile>pointer__hashmap_8h.html</anchorfile>
      <anchor>af40a326b23c68a27cebe60f16634a2cb</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>lf_semaphore.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/utils/</path>
    <filename>lf__semaphore_8h.html</filename>
    <includes id="low__level__platform_8h" name="low_level_platform.h" local="yes" import="no" module="no" objc="no">low_level_platform.h</includes>
    <class kind="struct">lf_semaphore_t</class>
    <member kind="define">
      <type>#define</type>
      <name>NUMBER_OF_WORKERS</name>
      <anchorfile>lf__semaphore_8h.html</anchorfile>
      <anchor>a43fcdba4ed8864d210afd3f031b4e346</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_semaphore_acquire</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga7bafb933f1e301b37b5d5164229f386d</anchor>
      <arglist>(lf_semaphore_t *semaphore)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_semaphore_destroy</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga1207a7db6221cb49ccf260c31e57a5ac</anchor>
      <arglist>(lf_semaphore_t *semaphore)</arglist>
    </member>
    <member kind="function">
      <type>lf_semaphore_t *</type>
      <name>lf_semaphore_new</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga4f13b40eede6275ac98d4ea1e2802e00</anchor>
      <arglist>(size_t count)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_semaphore_release</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gae924daa1634a8e574b5b8966d54158dd</anchor>
      <arglist>(lf_semaphore_t *semaphore, size_t i)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_semaphore_wait</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga2e816883471b300567e207c16471502e</anchor>
      <arglist>(lf_semaphore_t *semaphore)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>pqueue.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/utils/</path>
    <filename>pqueue_8h.html</filename>
    <includes id="pqueue__base_8h" name="pqueue_base.h" local="yes" import="no" module="no" objc="no">pqueue_base.h</includes>
    <member kind="function">
      <type>pqueue_pri_t</type>
      <name>get_reaction_index</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gacd6d67f4e05f2780b23aef72f92468f5</anchor>
      <arglist>(void *reaction)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>get_reaction_position</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga051ae6a8bc2b547818e06cb0c72b14a2</anchor>
      <arglist>(void *reaction)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>in_no_particular_order</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga987e40d356a70d4799d6fe56920d3b8f</anchor>
      <arglist>(pqueue_pri_t thiz, pqueue_pri_t that)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>in_reverse_order</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga19d650d18c331602f44c642bce2456e8</anchor>
      <arglist>(pqueue_pri_t thiz, pqueue_pri_t that)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>print_reaction</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga26e33f5180dc5951b3d26094959913b7</anchor>
      <arglist>(void *reaction)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>reaction_matches</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga648da83816bb67aedeeaa8c10a99ec7a</anchor>
      <arglist>(void *a, void *b)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_reaction_position</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga463f89e588c57a76b4cd6a0e633a94b4</anchor>
      <arglist>(void *reaction, size_t pos)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>pqueue_base.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/utils/</path>
    <filename>pqueue__base_8h.html</filename>
    <class kind="struct">pqueue_t</class>
    <member kind="typedef">
      <type>int(*</type>
      <name>pqueue_cmp_pri_f</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga1c3f02694b2a0ec19584c395a88bb6f9</anchor>
      <arglist>)(pqueue_pri_t next, pqueue_pri_t curr)</arglist>
    </member>
    <member kind="typedef">
      <type>int(*</type>
      <name>pqueue_eq_elem_f</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga664f0abcd86c8089468869aa3dc6e535</anchor>
      <arglist>)(void *next, void *curr)</arglist>
    </member>
    <member kind="typedef">
      <type>size_t(*</type>
      <name>pqueue_get_pos_f</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga31ca7927983005bd7866021819ad7037</anchor>
      <arglist>)(void *a)</arglist>
    </member>
    <member kind="typedef">
      <type>pqueue_pri_t(*</type>
      <name>pqueue_get_pri_f</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaa84f0100faf971295df5aed226c390a6</anchor>
      <arglist>)(void *a)</arglist>
    </member>
    <member kind="typedef">
      <type>unsigned long long</type>
      <name>pqueue_pri_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gad8239ddc32134716f57e54bb972f6bf0</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>pqueue_print_entry_f</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga884902da135214a6167f1536ad4ed4bc</anchor>
      <arglist>)(void *a)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>pqueue_set_pos_f</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gafdc8f52cbc45181ef375df22917bc4f9</anchor>
      <arglist>)(void *a, size_t pos)</arglist>
    </member>
    <member kind="typedef">
      <type>struct pqueue_t</type>
      <name>pqueue_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga05e211b59fd9be5939218e11d1132167</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>pqueue_dump</name>
      <anchorfile>pqueue__base_8h.html</anchorfile>
      <anchor>add0fae8523cca210f1574586e82cf7dd</anchor>
      <arglist>(pqueue_t *q, pqueue_print_entry_f print)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>pqueue_empty_into</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaa0f5e3d63138880461b1f04dc2d4f48a</anchor>
      <arglist>(pqueue_t **dest, pqueue_t **src)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>pqueue_find_equal_same_priority</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga9bb9cb0e5f41746db17b7581f5fe0559</anchor>
      <arglist>(pqueue_t *q, void *e)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>pqueue_find_same_priority</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga23a8f91001427f237232082b8d25e81a</anchor>
      <arglist>(pqueue_t *q, void *e)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>pqueue_free</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gacceacc4429dd9cd31d5af09f3f473cb0</anchor>
      <arglist>(pqueue_t *q)</arglist>
    </member>
    <member kind="function">
      <type>pqueue_t *</type>
      <name>pqueue_init</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga5a4c8b51b16189ab4a687f562733b1a5</anchor>
      <arglist>(size_t n, pqueue_cmp_pri_f cmppri, pqueue_get_pri_f getpri, pqueue_get_pos_f getpos, pqueue_set_pos_f setpos, pqueue_eq_elem_f eqelem, pqueue_print_entry_f prt)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>pqueue_insert</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaddd8cdfbc8c47b8cdd7eb4c4560de7aa</anchor>
      <arglist>(pqueue_t *q, void *d)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>pqueue_is_valid</name>
      <anchorfile>pqueue__base_8h.html</anchorfile>
      <anchor>aecb92aa04f03ad7866508bc27778d7e9</anchor>
      <arglist>(pqueue_t *q)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>pqueue_peek</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga9ec5c03203b587dbb92f8d2a977aa7e4</anchor>
      <arglist>(pqueue_t *q)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>pqueue_pop</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga66bce8cd2c2afa804405005798498823</anchor>
      <arglist>(pqueue_t *q)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>pqueue_print</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga1bc71ac57e101d48d91c75ecbf8fc278</anchor>
      <arglist>(pqueue_t *q, pqueue_print_entry_f print)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>pqueue_remove</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga921be3b49e4021888c595188438fdf7a</anchor>
      <arglist>(pqueue_t *q, void *e)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>pqueue_size</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gae0cf88c8360a5f08ada81feaaeb40505</anchor>
      <arglist>(pqueue_t *q)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>pqueue_tag.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/utils/</path>
    <filename>pqueue__tag_8h.html</filename>
    <includes id="pqueue__base_8h" name="pqueue_base.h" local="yes" import="no" module="no" objc="no">pqueue_base.h</includes>
    <includes id="tag_8h" name="tag.h" local="yes" import="no" module="no" objc="no">tag.h</includes>
    <class kind="struct">pqueue_tag_element_t</class>
    <member kind="typedef">
      <type>pqueue_t</type>
      <name>pqueue_tag_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gac52d056c47d9595f94d37e95484b3acd</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>pqueue_tag_compare</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gac6870c37fb36dcb65dbfcceff317cab4</anchor>
      <arglist>(pqueue_pri_t priority1, pqueue_pri_t priority2)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>pqueue_tag_dump</name>
      <anchorfile>pqueue__tag_8h.html</anchorfile>
      <anchor>a4b5d0cba34127064e3a14d7a499bbdf3</anchor>
      <arglist>(pqueue_tag_t *q)</arglist>
    </member>
    <member kind="function">
      <type>pqueue_tag_element_t *</type>
      <name>pqueue_tag_find_equal_same_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gafe428033cb2f6915828e75efb90edc44</anchor>
      <arglist>(pqueue_tag_t *q, pqueue_tag_element_t *e)</arglist>
    </member>
    <member kind="function">
      <type>pqueue_tag_element_t *</type>
      <name>pqueue_tag_find_with_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga91d0568eb488ec1255fc1146163934fa</anchor>
      <arglist>(pqueue_tag_t *q, tag_t t)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>pqueue_tag_free</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga7eee6edbbb90d5a0bb072a728dd3c7f2</anchor>
      <arglist>(pqueue_tag_t *q)</arglist>
    </member>
    <member kind="function">
      <type>pqueue_tag_t *</type>
      <name>pqueue_tag_init</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga30b038fea77adb97ee6e0ab13af55ede</anchor>
      <arglist>(size_t initial_size)</arglist>
    </member>
    <member kind="function">
      <type>pqueue_tag_t *</type>
      <name>pqueue_tag_init_customize</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga7194e0ba9a3cd659f5e94f5a46c3d1f1</anchor>
      <arglist>(size_t initial_size, pqueue_cmp_pri_f cmppri, pqueue_eq_elem_f eqelem, pqueue_print_entry_f prt)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>pqueue_tag_insert</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga7db3de28c457287e689dedc3a6dc20da</anchor>
      <arglist>(pqueue_tag_t *q, pqueue_tag_element_t *d)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>pqueue_tag_insert_if_no_match</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga067e8fdd88be6f660e79744350a74128</anchor>
      <arglist>(pqueue_tag_t *q, tag_t t)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>pqueue_tag_insert_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga89084b69d8049630eebb8df759c666d0</anchor>
      <arglist>(pqueue_tag_t *q, tag_t t)</arglist>
    </member>
    <member kind="function">
      <type>pqueue_tag_element_t *</type>
      <name>pqueue_tag_peek</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga16cbdbb45d26bd5373e258de819cfdd3</anchor>
      <arglist>(pqueue_tag_t *q)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>pqueue_tag_peek_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga9bcecb00b894ad00b07f84940fe7af95</anchor>
      <arglist>(pqueue_tag_t *q)</arglist>
    </member>
    <member kind="function">
      <type>pqueue_tag_element_t *</type>
      <name>pqueue_tag_pop</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaf6709c3c3756e65205762a4cf33848be</anchor>
      <arglist>(pqueue_tag_t *q)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>pqueue_tag_pop_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaa2144411e9b6d74af078d51078526fe3</anchor>
      <arglist>(pqueue_tag_t *q)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>pqueue_tag_remove</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gac5612f277391c7129183d2826021c3e3</anchor>
      <arglist>(pqueue_tag_t *q, pqueue_tag_element_t *e)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>pqueue_tag_remove_up_to</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga135a225e2361b815a415607bc1f71e3b</anchor>
      <arglist>(pqueue_tag_t *q, tag_t t)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>pqueue_tag_size</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga0679a4db1f4d970d9f1048e253b79562</anchor>
      <arglist>(pqueue_tag_t *q)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>util.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/utils/</path>
    <filename>util_8h.html</filename>
    <includes id="logging__macros_8h" name="logging_macros.h" local="yes" import="no" module="no" objc="no">logging_macros.h</includes>
    <member kind="define">
      <type>#define</type>
      <name>CONCATENATE_THREE_STRINGS</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga8d8698026252ae104cc2405d8bb13f0e</anchor>
      <arglist>(__string1, __string2, __string3)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_COND_BROADCAST</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga8ffb41cff660cdf632693c5bf5a17f52</anchor>
      <arglist>(cond)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_COND_INIT</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga47d73a5ec6fa7ebc7838312cb93c2bb8</anchor>
      <arglist>(cond, mutex)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_COND_SIGNAL</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gabd8bd827c1d0d4b9f108da9098e10e51</anchor>
      <arglist>(cond)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_COND_WAIT</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga8b24bfd4605a8726dbfc2cee30c27e08</anchor>
      <arglist>(cond)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_CRITICAL_SECTION_ENTER</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga912847660fb8b04317fc270125d6b1f3</anchor>
      <arglist>(env)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_CRITICAL_SECTION_EXIT</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga687e881099481d8efe446ad8a17d72e5</anchor>
      <arglist>(env)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_LEVEL</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gad5a08658dc3e13eab4cddafd94734794</anchor>
      <arglist>(index)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_MAX</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaac9240f79bd758e00ed7bbf75dafc4fa</anchor>
      <arglist>(X, Y)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_MIN</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga4359466e7fdb68dcf8116c469946cd92</anchor>
      <arglist>(X, Y)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_MUTEX_INIT</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gadab0b8f13f8462ec0eddc7257ddb5394</anchor>
      <arglist>(mutex)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_MUTEX_LOCK</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gab907d4c8d53c26fdbcbaa8d02e6a8810</anchor>
      <arglist>(mutex)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_MUTEX_UNLOCK</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga2bb9c8d2b589a6eb4f72f6750a1133fc</anchor>
      <arglist>(mutex)</arglist>
    </member>
    <member kind="function">
      <type>void void void void</type>
      <name>error</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga8db9f1cd3ea7eb70e6958e732b26e61d</anchor>
      <arglist>(const char *msg)</arglist>
    </member>
    <member kind="function">
      <type>uint16_t</type>
      <name>lf_fed_id</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga6e9c2ed60ca5adec5ba3f43d4410dc75</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_vprint</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga1b64e4e645fbebb1a3b132280b2c5b35</anchor>
      <arglist>(const char *format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void void void</type>
      <name>lf_vprint_debug</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga334870b12bd4bc49b9da219e31225477</anchor>
      <arglist>(const char *format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_vprint_error</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga3d1b4e46f0394bbf2e74c4eabfd8923f</anchor>
      <arglist>(const char *format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void void void</type>
      <name>lf_vprint_error_and_exit</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga698eb7c2ecf514b4afa1ab7ab598eea2</anchor>
      <arglist>(const char *format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void void</type>
      <name>lf_vprint_log</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga8432a03751d354b69ffe2f5b8c664654</anchor>
      <arglist>(const char *format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void void</type>
      <name>lf_vprint_warning</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga47f8c72c1407daae89508da09273d655</anchor>
      <arglist>(const char *format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="variable">
      <type>uint16_t</type>
      <name>_lf_my_fed_id</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga3c7bddddb86913975950acdcf8bfef2a</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>vector.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/include/core/utils/</path>
    <filename>vector_8h.html</filename>
    <class kind="struct">vector_t</class>
    <member kind="typedef">
      <type>struct vector_t</type>
      <name>vector_t</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gac6a5b15223a2905669f2ee7377fd3dbd</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void **</type>
      <name>vector_at</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga201cb1fd5299e01b6fdfb499d3008952</anchor>
      <arglist>(vector_t *v, size_t idx)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>vector_free</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gab6ea681ea89fa128392d61ec7a516e31</anchor>
      <arglist>(vector_t *v)</arglist>
    </member>
    <member kind="function">
      <type>vector_t</type>
      <name>vector_new</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga640489760dfb72c2001de6ec560fb75f</anchor>
      <arglist>(size_t initial_capacity)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>vector_pop</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga85cdea38a35554168aa2277d83f5a957</anchor>
      <arglist>(vector_t *v)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>vector_push</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga85a9501c4a715501dc0adeb04bd84dcb</anchor>
      <arglist>(vector_t *v, void *element)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>vector_pushall</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gab5ad2a2c71548435b6072b31ac21a9c2</anchor>
      <arglist>(vector_t *v, void **array, size_t size)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>vector_size</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gaaa8f4318bf03a7886169e85c151b6903</anchor>
      <arglist>(vector_t *v)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>vector_vote</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga41e7b0b4a43deefd94df37fd128de0bb</anchor>
      <arglist>(vector_t *v)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>logging.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/logging/api/</path>
    <filename>logging_8h.html</filename>
    <member kind="define">
      <type>#define</type>
      <name>LOG_LEVEL_ALL</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga18226173309d6c2ae828080dad0859cf</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LOG_LEVEL_DEBUG</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga130224df8c6bf22a688e3cb74a45689a</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LOG_LEVEL_ERROR</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga742fc70e331d7e568bd893c514756a29</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LOG_LEVEL_INFO</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga2e25fe130cf710da4ad800747fdd51f3</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LOG_LEVEL_LOG</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga8b58cabecd61bfd1b706be9cb992e0bd</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LOG_LEVEL_WARNING</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaf539a66abed2a7a15e3443d70a3cf1e1</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>void</type>
      <name>print_message_function_t</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gadc4e09bf2433b9e06b632880ce81b897</anchor>
      <arglist>(const char *, va_list)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_print</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga9d83e586b29a3316dd7dc505e30e6858</anchor>
      <arglist>(const char *format,...) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void void void</type>
      <name>lf_print_debug</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga22ebee89c962ac34cc1fa7b9762b77d2</anchor>
      <arglist>(const char *format,...) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void void void void</type>
      <name>lf_print_error</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gae0a6bd6b164c5cc3e9928f5375dd1a97</anchor>
      <arglist>(const char *format,...) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void void void void void void</type>
      <name>lf_print_error_and_exit</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gabf2630e80adfec45b8b4a2782a0767a7</anchor>
      <arglist>(const char *format,...) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void void void void void void void</type>
      <name>lf_print_error_system_failure</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gacbc9741875a95bb30e1a8d68cfb7cf06</anchor>
      <arglist>(const char *format,...)</arglist>
    </member>
    <member kind="function">
      <type>void void</type>
      <name>lf_print_log</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gab202dc9383567eaa8d9ae7240c939c19</anchor>
      <arglist>(const char *format,...) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void void void void void</type>
      <name>lf_print_warning</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga613bc8d331e3fd9a3f78eb1600092e1d</anchor>
      <arglist>(const char *format,...) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_register_print_function</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga6ad22ca79136cd15dc0d560aec067f76</anchor>
      <arglist>(print_message_function_t *function, int log_level)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>logging_macros.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/logging/api/</path>
    <filename>logging__macros_8h.html</filename>
    <includes id="logging_8h" name="logging.h" local="yes" import="no" module="no" objc="no">logging.h</includes>
    <member kind="define">
      <type>#define</type>
      <name>LF_ASSERT</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga26ab6a0fd21cdcff11a5557406536bf1</anchor>
      <arglist>(condition, format,...)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_ASSERT_NON_NULL</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga832d0deaa853b7777e1f54283e7bcc20</anchor>
      <arglist>(pointer)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_ASSERTN</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga1c464cee8cabb65eebf454fc016d47b1</anchor>
      <arglist>(condition, format,...)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_PRINT_DEBUG</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gab5a65df50027549a8245f6b3eaff97e4</anchor>
      <arglist>(format,...)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_PRINT_LOG</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga2a7110df48e8f74b05fd4a8f7581b1da</anchor>
      <arglist>(format,...)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_TEST</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga9af561423d4c16fd397d48ab37edfcff</anchor>
      <arglist>(condition, format,...)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LOG_LEVEL</name>
      <anchorfile>logging__macros_8h.html</anchorfile>
      <anchor>a0b87e0d3bf5853bcbb0b66a7c48fdc05</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const bool</type>
      <name>_lf_log_level_is_debug</name>
      <anchorfile>logging__macros_8h.html</anchorfile>
      <anchor>aa5842514c55365877642c5df9fd3dd70</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const bool</type>
      <name>_lf_log_level_is_log</name>
      <anchorfile>logging__macros_8h.html</anchorfile>
      <anchor>abaede48cb5679264413d3cc86ed251bf</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>low_level_platform.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/low_level_platform/api/</path>
    <filename>low__level__platform_8h.html</filename>
    <includes id="tag_8h" name="tag.h" local="yes" import="no" module="no" objc="no">tag.h</includes>
    <class kind="struct">lf_scheduling_policy_t</class>
    <member kind="define">
      <type>#define</type>
      <name>DEPRECATED</name>
      <anchorfile>low__level__platform_8h.html</anchorfile>
      <anchor>af67a6ff1a54b41b2a6e4bd36bc47b118</anchor>
      <arglist>(X)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_SCHED_MAX_PRIORITY</name>
      <anchorfile>low__level__platform_8h.html</anchorfile>
      <anchor>a71c1286f39f3cca4453fcefc5f4a8b4b</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_SCHED_MIN_PRIORITY</name>
      <anchorfile>low__level__platform_8h.html</anchorfile>
      <anchor>aa9693e6d23d87f8b7832493cbc911344</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_TIMEOUT</name>
      <anchorfile>low__level__platform_8h.html</anchorfile>
      <anchor>a2cb0837301dbf928d9e6a4753e0fb52e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>lf_scheduling_policy_type_t</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gadc74ec49eb5cc6eceda1447090d61ab6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>LF_SCHED_FAIR</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ggadc74ec49eb5cc6eceda1447090d61ab6aad0c30324e2299f1ab579a9db51ae994</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>LF_SCHED_TIMESLICE</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ggadc74ec49eb5cc6eceda1447090d61ab6aa94c07b6d2e7cf9564d407bdb0d5eb3f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>LF_SCHED_PRIORITY</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ggadc74ec49eb5cc6eceda1447090d61ab6a31d917200c3ceb3735770c9acef3eb5e</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>_lf_clock_gettime</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga843c12cfbc698883e96a0bfe23882a9e</anchor>
      <arglist>(instant_t *t)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>_lf_cond_timedwait</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga0ed454c4a1b6eee6debe92efa433fb37</anchor>
      <arglist>(lf_cond_t *cond, instant_t wakeup_time)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_initialize_clock</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gaf5c7b68f137e69c4ad3734a42bcd0448</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>_lf_interruptable_sleep_until_locked</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga0b22a974cdd69f184735503b72597698</anchor>
      <arglist>(environment_t *env, instant_t wakeup_time)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>initialize_lf_thread_id</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga32bb18b9d9ab98e40bb5bcb57dec682d</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_available_cores</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gae138038b96821ccfbb2d4f2ec4c364c1</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_cond_broadcast</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga9ed434626733537f71c9b85e981109c7</anchor>
      <arglist>(lf_cond_t *cond)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_cond_init</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gab32dc7869dd4cf48bda70663f2591ae1</anchor>
      <arglist>(lf_cond_t *cond, lf_mutex_t *mutex)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_cond_signal</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga7267ad6679b93c9a9f321cdf864e0092</anchor>
      <arglist>(lf_cond_t *cond)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_cond_wait</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gaba6871b2088bcd86f973e0299cdd4ff8</anchor>
      <arglist>(lf_cond_t *cond)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_critical_section_enter</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gae0fda178667bc6cd94890a13316c285c</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_critical_section_exit</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gaed2c25495b50b46780c6288e4370541e</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_mutex_init</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gaa6f228487e6af38e496882f406aafaf6</anchor>
      <arglist>(lf_mutex_t *mutex)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_mutex_lock</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gac4c0721974b31d98f491be1febeb2c9a</anchor>
      <arglist>(lf_mutex_t *mutex)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_mutex_unlock</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga01c4d5070c8402d4713a3fcab5a46a9f</anchor>
      <arglist>(lf_mutex_t *mutex)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_nanosleep</name>
      <anchorfile>low__level__platform_8h.html</anchorfile>
      <anchor>a50ad9d5da851a0aeaf45ad759c02bbd1</anchor>
      <arglist>(interval_t sleep_duration)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_notify_of_event</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga3ab5c60e67d36e2b26eccb884ed9f668</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_sleep</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga9a43894d4caf7e2fc1e75b9b49d7285d</anchor>
      <arglist>(interval_t sleep_duration)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_thread_create</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga48fa558d833200b986c03bc8c12bbe77</anchor>
      <arglist>(lf_thread_t *thread, void *(*lf_thread)(void *), void *arguments)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_thread_id</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gafe7e0ce54cfd40a754554105ad214e9b</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_thread_join</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga652d8db6bafa59434d297216607266ed</anchor>
      <arglist>(lf_thread_t thread, void **thread_return)</arglist>
    </member>
    <member kind="function">
      <type>lf_thread_t</type>
      <name>lf_thread_self</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gaa295ab0ba8fa58516cb45f57b62d09e1</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_thread_set_cpu</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gaddc34b525849f46d172fbafa894dc474</anchor>
      <arglist>(lf_thread_t thread, size_t cpu_number)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_thread_set_priority</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga173ebe6288d9a800df16cbd400247578</anchor>
      <arglist>(lf_thread_t thread, int priority)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_thread_set_scheduling_policy</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gadc1acd7c6e9bd4f7bd333ad7d3e7b5d1</anchor>
      <arglist>(lf_thread_t thread, lf_scheduling_policy_t *policy)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>platform.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/platform/api/</path>
    <filename>platform_8h.html</filename>
    <member kind="typedef">
      <type>void *</type>
      <name>lf_platform_mutex_ptr_t</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga8cb87ba531decc7a525fe20e8586e300</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_platform_mutex_free</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga086b854690734be2e49391d6ca65e8ec</anchor>
      <arglist>(lf_platform_mutex_ptr_t mutex)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_platform_mutex_lock</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gaa548193b346ebc70b6e6e3a5e87d5e2d</anchor>
      <arglist>(lf_platform_mutex_ptr_t mutex)</arglist>
    </member>
    <member kind="function">
      <type>lf_platform_mutex_ptr_t</type>
      <name>lf_platform_mutex_new</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga262864ffd60ec9491d6e6b278c58910c</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_platform_mutex_unlock</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga0911d4e67c0a632343e7404115c88ca9</anchor>
      <arglist>(lf_platform_mutex_ptr_t mutex)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>tag.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/tag/api/</path>
    <filename>tag_8h.html</filename>
    <class kind="struct">tag_t</class>
    <member kind="define">
      <type>#define</type>
      <name>BILLION</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga31f99d9c502b52b5f36dc7e2028c2e80</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CHECK_TIMEOUT</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaabb3bc387f7a50f8cc57319f82c17c31</anchor>
      <arglist>(start, duration)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DAY</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga8393c538440a761b4439623e536a7c91</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DAYS</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga987a486d1ad1ff15346b2395847280ab</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FOREVER</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga75c828ed6c02fcd44084e67a032e422c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FOREVER_MICROSTEP</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga43635840119d1af39afff67eb2585248</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FOREVER_TAG</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga183109e082a793ae85cec00d72e70d4b</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FOREVER_TAG_INITIALIZER</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga33ccb2b8fb9f20ab29ea298b9051443a</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>HOUR</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga3cf238d5a1a842131a5adf916d571d12</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>HOURS</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga2249e1bedc7aeaad64ed3fefdd6e7951</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_TIME_BUFFER_LENGTH</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gacc2b8ac5ac3020137e71dfcdbaedf335</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MINUTE</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga72e3dbcf6a85e8c0a04691cfb14d3876</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MINUTES</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga694878332b974b464c7d58c7114ee6e9</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSEC</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga673a8ebab8ec621a1cf731871dd170c8</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSECS</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>gad3edea62ad14babaf45f6d3020cb7674</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NEVER</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga9e5c19968af97bd64f46650ec8732318</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NEVER_MICROSTEP</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga934f060560ba6d2d7c1ad336439e76e8</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NEVER_TAG</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>gafc2f39e1818a6a8b68ca46e7c9dc2704</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NEVER_TAG_INITIALIZER</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga0d876ab54e766798e559c4f47340b359</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NSEC</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga8c561c4985495c722ac86b55b3d9bbb3</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NSECS</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>gaea80ae7708f69d05cd44126f08a08cf3</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SEC</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga30e47c4dd44d168c2982d3ec1d4e0825</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SECOND</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>gaf7dae730408c43d8c820ab73e580865a</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SECONDS</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga2874e6caf9a4e0452441e9edeca046dc</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SECS</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga8d1ccba996ebc6fc86c6c3c52051329d</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>USEC</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>gaa2aeaab0c2033d1db412c8021bff93fc</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>USECS</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>gaa0c5b5c56bdb5016516284c87eac86a9</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>WEEK</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>gac38fd1109df8cd9f53ab99761b3efa04</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>WEEKS</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga6377f87d75908e0d79a4005db7af3b35</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ZERO_TAG</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>gacb18b22c88c7505b59347c3a9c52f53d</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>int64_t</type>
      <name>instant_t</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga827080fd3c574bad5a32db9f7c367587</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>int64_t</type>
      <name>interval_t</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaf5b4e62d03782997d813be6145316f4b</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>uint32_t</type>
      <name>microstep_t</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gad88f1caa8b9c216404eb196cb1850213</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>lf_comma_separated_time</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gae956f1688a3893b44cf4bced3c13ef9a</anchor>
      <arglist>(char *buffer, instant_t time)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>lf_delay_strict</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaf61e1a6183ff7d40b1b998c08447130e</anchor>
      <arglist>(tag_t tag, interval_t interval)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>lf_delay_tag</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaf7feb89416e60e458d6a86cadadbc41d</anchor>
      <arglist>(tag_t tag, interval_t interval)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>lf_readable_time</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga43509483b3e9886acd002c7d7b6482c8</anchor>
      <arglist>(char *buffer, instant_t time)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>lf_tag</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gacbe5117469b98e0e6df45b5421f58026</anchor>
      <arglist>(void *env)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>lf_tag_add</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaa1186f7e330ecbfd20a4dd90b97439e6</anchor>
      <arglist>(tag_t a, tag_t b)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_tag_compare</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga596d8734432616c9c7847283fde63cfa</anchor>
      <arglist>(tag_t tag1, tag_t tag2)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>lf_tag_latest_earlier</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga1e92870e0258c83da4c541e4ec48169b</anchor>
      <arglist>(tag_t tag)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>lf_tag_max</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga4c39037bf099ff2c31a71fe96ac59a61</anchor>
      <arglist>(tag_t tag1, tag_t tag2)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>lf_tag_min</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga80abd9981c0375a0abbe47591204f18b</anchor>
      <arglist>(tag_t tag1, tag_t tag2)</arglist>
    </member>
    <member kind="function">
      <type>instant_t</type>
      <name>lf_time_add</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga1789de56286aa33c086b194b544c2d91</anchor>
      <arglist>(instant_t a, interval_t b)</arglist>
    </member>
    <member kind="function">
      <type>instant_t</type>
      <name>lf_time_logical</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga751c9fce12510f5bb98d862f57077396</anchor>
      <arglist>(void *env)</arglist>
    </member>
    <member kind="function">
      <type>interval_t</type>
      <name>lf_time_logical_elapsed</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga6faad0d905f7135352f511bc235425e1</anchor>
      <arglist>(void *env)</arglist>
    </member>
    <member kind="function">
      <type>instant_t</type>
      <name>lf_time_physical</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga7538766a655ba2e60ddde55f2e020e58</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>instant_t</type>
      <name>lf_time_physical_elapsed</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga98468f1c5132e3aa18d77f85d65bb6ec</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>instant_t</type>
      <name>lf_time_start</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga8da2172c41ab13ff4748994a62ae34b5</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>instant_t</type>
      <name>lf_time_subtract</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga9107f7f62874fa972f7a31f9c277b8fc</anchor>
      <arglist>(instant_t a, interval_t b)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>trace.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/trace/api/</path>
    <filename>trace_8h.html</filename>
    <class kind="struct">object_description_t</class>
    <class kind="struct">trace_record_nodeps_t</class>
    <member kind="typedef">
      <type>struct object_description_t</type>
      <name>object_description_t</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga027205fde7a6d8a5777059d6dc397050</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>_lf_trace_object_t</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gaeec3d6d67240b942f12f5d8770698ae3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>trace_reactor</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggaeec3d6d67240b942f12f5d8770698ae3ac970d1f28c60cf2b9de8353b284197b0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>trace_trigger</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggaeec3d6d67240b942f12f5d8770698ae3a5c05f73b365f900def1359524b9ce5a0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>trace_user</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggaeec3d6d67240b942f12f5d8770698ae3a8fa30d4c503d93cecf4234a4d648ed79</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_tracing_global_init</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga1098d9744b99a07d115e48d873de000d</anchor>
      <arglist>(char *process_name, char *process_names, int process_id, int max_num_local_threads)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_tracing_global_shutdown</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga8a5d5ec80d2716ea7848193647cdadcd</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_tracing_register_trace_event</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga15969599d7817596e12dcceb8c145551</anchor>
      <arglist>(object_description_t description)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_tracing_set_start_time</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga4270e37a116b1ebac46ad126b2fc277d</anchor>
      <arglist>(int64_t start_time)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_tracing_tracepoint</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gaa6e591a66e342a77860e9d3050680446</anchor>
      <arglist>(int worker, trace_record_nodeps_t *tr)</arglist>
    </member>
    <member kind="function">
      <type>const version_t *</type>
      <name>lf_version_tracing</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga8ab5f9fb3d5e1d867dec2d2949e69d7c</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>trace_types.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/trace/api/types/</path>
    <filename>trace__types_8h.html</filename>
    <member kind="enumeration">
      <type></type>
      <name>trace_event_t</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gab02e9e69539d60297cedb38c2193a453</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>reaction_starts</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a7ca2fc1a301d8e66944fab471646728a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>reaction_ends</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a79889b5c51ccdf63962a8ec230ff3f6b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>reaction_deadline_missed</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a8dd4df4dfbe3f6fd12f454467d61cda0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>schedule_called</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a666fca0e7c44992277a6f47331b6ff49</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>user_event</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453aeba45bd40c043d7a65ac7c5d31b9e187</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>user_value</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a1d509d2fbc0fe97dcc61aea8ba7b68c1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>worker_wait_starts</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a9c6849445ded286ba9f914d3b1decd2a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>worker_wait_ends</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453afa179671ba1508f1c16b10ffef3a17c6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>scheduler_advancing_time_starts</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a79e198f97731e9ea1f982ccb8db8f5d0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>scheduler_advancing_time_ends</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a0422fad861a51f83258d104e6b34fdad</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>federated</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a9b673ff06b88b52089cafea62715f7c0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_ACK</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a359e8de71d77ef5495ec551054828a44</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_FAILED</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453ad86a1396b6883943ef6f103f97897ba4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_TIMESTAMP</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a58ad18e86ddd619e77a41305948c9343</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_NET</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a9e1be1b36d5d41388c1bc6496be1c5c6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_LTC</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a813a5c39f4659f6f4dc04cdf63a167df</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_STOP_REQ</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a06e51ca4ea2dfc237aad447f5e92f4df</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_STOP_REQ_REP</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a87da8a1b1ef9018c7f6959432d6cbc3d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_STOP_GRN</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a55634e907daa67db49886a1439ac9d16</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_FED_ID</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a68fe57f3b98b1046a20b96926d44d33f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_PTAG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453af8e3e59ab52fdb846ef24dd8bff6fa7a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_TAG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a1d2d24b8faba6e105f3effdda0f30b45</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_REJECT</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a6debe1c504821e1b55ae91ba185ca768</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_RESIGN</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453aaf479fd10fa46f06b11550611d3c610f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_PORT_ABS</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a1f1176e68caab2d18193117dec523864</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_CLOSE_RQ</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453afb7e97a3dfbc62462125e25024424ed0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_TAGGED_MSG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a813b0d7924cfa072765efc09614c2d81</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_P2P_TAGGED_MSG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a17e891ffbe84e2aadbdb405b47a5c09c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_MSG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a22883b9d184199e94df0fa804fe23a89</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_P2P_MSG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453aa7a4fca7a8ef7dfde896701070fcfe6b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_ADR_AD</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a787c95dd61408de4fcc85bdbb3bc6946</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_ADR_QR</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a4ebac2f241a667876f0a7ab20a8efdec</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_DNET</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453afd2cdb4240669a883e88c58b28b470cd</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_ACK</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a908926b0adde60c94d40c90fb4aa0221</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_FAILED</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a9fc739f4f25f7832d01711069eaf5004</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_TIMESTAMP</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453ac8429797245197de5cb9f32c9dbe8539</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_NET</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453ab3b8cb930ea4e1ebdb0d49fe287d9b8f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_LTC</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453ab3ae4783b8e9be6cf326df41c3c08108</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_STOP_REQ</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453abf340bb6735bbba6a645ad8e74958410</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_STOP_REQ_REP</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a1967b1b61bcc8cd7e7653fa8bab46e97</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_STOP_GRN</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453aeb7f596158f60ffaded5b6f76a4758a1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_FED_ID</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453acfd5cbf76eacc508d0e2e3ccad128b99</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_PTAG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a3546557eebc5ad402b6783e25be9a468</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_TAG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453aa350fa2156010c04148e947826624831</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_REJECT</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a675c6b9020b4996877a16de7efd91cf2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_RESIGN</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a2e2c7ec922abc6561e48520f2929af22</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_PORT_ABS</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453af9c0ed69192d1a4cc7f1f0fda87c85fa</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_CLOSE_RQ</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453aaeaec5a7cb114e56455f16069bf8f589</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_TAGGED_MSG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a17c1e79d4df8091f7279b5c042c2236a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_P2P_TAGGED_MSG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a720c7ea924889f0db7ed7df00eb9e65d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_MSG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a530cf0a58e28bc3a386e75565810b7f7</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_P2P_MSG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453ae06a042ab87b41578d9adf1c95322475</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_ADR_AD</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a92c4ad633d4739f7ee1cd40fdaf8cc5e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_ADR_QR</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453af6e18bcb48b442e2be5a898d60c3bdf2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_DNET</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453affe028b8b1b469f6c85d636e780b6c4d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_UNIDENTIFIED</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a48fffa45ebf3f03caa2dc044af72349f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>NUM_EVENT_TYPES</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453ac009e126725584df074102abf50cc134</anchor>
      <arglist></arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>_suppress_unused_variable_warning_for_static_variable</name>
      <anchorfile>trace__types_8h.html</anchorfile>
      <anchor>a2d11ee780e03c468a1f60c490eef6db4</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const char *</type>
      <name>trace_event_names</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gad8876683159ec2203bd41295d8af3017</anchor>
      <arglist>[]</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>audio_loop.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/util/</path>
    <filename>audio__loop_8h.html</filename>
    <includes id="wave__file__reader_8h" name="wave_file_reader.h" local="yes" import="no" module="no" objc="no">wave_file_reader.h</includes>
    <includes id="tag_8h" name="tag.h" local="yes" import="no" module="no" objc="no">tag.h</includes>
    <member kind="define">
      <type>#define</type>
      <name>AUDIO_BUFFER_SIZE</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga1d48237cb63c5ae67aab6d00cc64afb6</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>BUFFER_DURATION_NS</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gaaee12bd1b49481f758a5a3cf1876268c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MAX_AMPLITUDE</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga141b139da77580918b0f9821ab3dbb99</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NUM_CHANNELS</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gae5597ce31d23d11493e6e674fe257d73</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NUM_NOTES</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga5b0b677cb9527865430a9b3d7a71cb03</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SAMPLE_RATE</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga4b76a0c2859cfd819a343a780070ee2b</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>add_to_sound</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gae0d2efe97b2aac751245705e32d2c927</anchor>
      <arglist>(int index_offset, double value)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_play_audio_waveform</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga20d5fc7b37764eb8c3a4e917e93ad7d7</anchor>
      <arglist>(lf_waveform_t *waveform, float emphasis, instant_t start_time)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_start_audio_loop</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga7e03b277fd2f2b3ae6aa029e5256da3e</anchor>
      <arglist>(instant_t start_time)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_stop_audio_loop</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga65266402bd1ede8be91b6a0a5a34f767</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>deque.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/util/</path>
    <filename>deque_8h.html</filename>
    <class kind="struct">deque_t</class>
    <member kind="typedef">
      <type>struct deque_t</type>
      <name>deque_t</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gafc1887f5ebd7da3ce7dfc73beb195598</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>deque_initialize</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga0e4768355c448b3b0489d851271515bc</anchor>
      <arglist>(deque_t *d)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>deque_is_empty</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gac22527818b92ea67851c2fd4a2790cb7</anchor>
      <arglist>(deque_t *d)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>deque_peek_back</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga3530b26ded106a84ef8571fab0e73d9e</anchor>
      <arglist>(deque_t *d)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>deque_peek_front</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga4b9e8a9de89f124d0cdc1199debb70be</anchor>
      <arglist>(deque_t *d)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>deque_pop_back</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga841ec80a70a2e63f08aab990e528e1f9</anchor>
      <arglist>(deque_t *d)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>deque_pop_front</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga4acd616a7c971b1c137010c64801d746</anchor>
      <arglist>(deque_t *d)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>deque_push_back</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gac1ffd77da7496612086ea430f7d29563</anchor>
      <arglist>(deque_t *d, void *value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>deque_push_front</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga19a0d218efcb14cc2c3d7b009a5d0f16</anchor>
      <arglist>(deque_t *d, void *value)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>deque_size</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gad7e6969a2e1a7e7529575558103d512d</anchor>
      <arglist>(deque_t *d)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>generics.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/util/</path>
    <filename>generics_8h.html</filename>
    <member kind="define">
      <type>#define</type>
      <name>lf_decay</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga9505134796fc957eb9fedf172cab3527</anchor>
      <arglist>(p)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_get_pointer</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gafe05484f1ece6fa30ba7b3bcf33f03b9</anchor>
      <arglist>(p)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_is_pointer</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga9edb07d852ce1b4e090b36f8683c8017</anchor>
      <arglist>(p)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_is_pointer_or_array</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga271f353ddab44fb05a4b0f8627904fbe</anchor>
      <arglist>(p)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_is_same</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga785378b93bab907d10fee21425a292e5</anchor>
      <arglist>(typename, b)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_is_same_type</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga886f2df25fcc1b16e512d6143e910b6f</anchor>
      <arglist>(a, b)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_is_type_equal</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga7d7827494787f07637043eb90e285d44</anchor>
      <arglist>(typename_a, typename_b)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_to_left_type</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga99938402e4e9202fe51e67f6e916f0f5</anchor>
      <arglist>(left, right)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>var</name>
      <anchorfile>generics_8h.html</anchorfile>
      <anchor>a1b0936415a643c88ce543099c10e0d7f</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>sensor_simulator.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/util/</path>
    <filename>sensor__simulator_8h.html</filename>
    <member kind="function">
      <type>void</type>
      <name>end_sensor_simulator</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga562a694d5b2ed8c935aa08d78c488114</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>register_sensor_key</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga196614841046dea5b94b2a60108fff40</anchor>
      <arglist>(char key, void *action)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>show_tick</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga3c9ec0901afe8264c0e75d40c940bd5a</anchor>
      <arglist>(const char *character)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>start_sensor_simulator</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga491b52110965df05da5f18cb820ae99f</anchor>
      <arglist>(const char *message_lines[], int number_of_lines, int tick_window_width, char *log_file, int log_level)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>influxdb.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/util/tracing/</path>
    <filename>influxdb_8h.html</filename>
    <class kind="struct">influx_client_t</class>
    <class kind="struct">influx_v2_client_t</class>
    <member kind="define">
      <type>#define</type>
      <name>_</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>afd91aaf985037d61628552664203a397</anchor>
      <arglist>(c)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>_APPEND</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>a15470ee8d1c5841eaf78f58f73246792</anchor>
      <arglist>(fmter...)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>_GET_NEXT_CHAR</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>a49f65e2120cd077ee205fefbcd5bdc50</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>_GET_NUMBER</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>aab5c9cefe9b019d4aaa5712203c74e0e</anchor>
      <arglist>(n)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>_LOOP_NEXT</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>ad82790662264e54a9d1f0c4815b9245e</anchor>
      <arglist>(statement)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>_UNTIL</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>a6220fbb96443025e3a7ff76dbb816a79</anchor>
      <arglist>(c)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>IF_TYPE_ARG_END</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>a442325f8443dadd92b1a1a82526de4c5</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>IF_TYPE_FIELD_BOOLEAN</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>a721453f210fbe85af4bf6c802631f2da</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>IF_TYPE_FIELD_FLOAT</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>adabd4b2ff9a9ae87bb6af970dc1184f2</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>IF_TYPE_FIELD_INTEGER</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>ab29e8d0ea7feb08d13792e58a9c6f0fe</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>IF_TYPE_FIELD_STRING</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>a60701b77b03519eb778e17f716c192c7</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>IF_TYPE_MEAS</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>ae4983c0f27aa6bd2d0b61c013fb5ce0b</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>IF_TYPE_TAG</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>a34cbf7e811c7fc4a0f8061b4e8995604</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>IF_TYPE_TIMESTAMP</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>ad1c00bdea16ac6c622549b935589840b</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>INFLUX_END</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>a38f08188614a07aa8c0cbe4f99c020b7</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>INFLUX_F_BOL</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>a00d3a4bed017647716879225efc134d4</anchor>
      <arglist>(k, v)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>INFLUX_F_FLT</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>a93a1e5f244d7ad01a9c93b6ef0cfa23e</anchor>
      <arglist>(k, v, p)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>INFLUX_F_INT</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>a196ecabce30a4db7ca6f3c5ae4799adf</anchor>
      <arglist>(k, v)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>INFLUX_F_STR</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>ac5c273b5c370722ed365f2aa7759ca79</anchor>
      <arglist>(k, v)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>INFLUX_MEAS</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>a3ad762f5dcd5acda26a5750e91b6f965</anchor>
      <arglist>(m)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>INFLUX_TAG</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>ad456eef5a970734402bf147d5daa162c</anchor>
      <arglist>(k, v)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>INFLUX_TS</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>a437443d0f707af6a79b24573c3ec58a6</anchor>
      <arglist>(ts)</arglist>
    </member>
    <member kind="typedef">
      <type>struct influx_client_t</type>
      <name>influx_client_t</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga3e866e57b719354cd1cd139976f81a37</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct influx_v2_client_t</type>
      <name>influx_v2_client_t</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga9a5a1c6ca2615dac7b1e875b398832a0</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>_begin_line</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>af41d4fee72efff67b442ebf9bc17f3b4</anchor>
      <arglist>(char **buf)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>_escaped_append</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>a9b6d90384014bed7164be5cafb575e20</anchor>
      <arglist>(char **dest, size_t *len, size_t *used, const char *src, const char *escape_seq)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>_format_line</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>a9c74319e55463b723e504f01bf5343de</anchor>
      <arglist>(char **buf, va_list ap)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>_format_line2</name>
      <anchorfile>influxdb_8h.html</anchorfile>
      <anchor>a2fafb6dfb7a12398ed4e8b7f63a89491</anchor>
      <arglist>(char **buf, va_list ap, size_t *, size_t)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>format_line</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga133af84dbff3d84a97044d8a89cf295d</anchor>
      <arglist>(char **buf, int *len, size_t used,...)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>post_curl</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gacca2186294f2553607cc2b7658cba23e</anchor>
      <arglist>(influx_v2_client_t *c,...)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>post_http</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gad45a883e3caea0fa7c2a8d542314c1b0</anchor>
      <arglist>(influx_client_t *c,...)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>post_http_send_line</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga932e10327e14e5e8cc5b128fddebe82a</anchor>
      <arglist>(influx_client_t *c, char *buf, int len)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>send_udp</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga67f24f962260dcabe6fb82bd44dcb5d7</anchor>
      <arglist>(influx_client_t *c,...)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>send_udp_line</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gabed7324f77b104dd90f6662fffed5c03</anchor>
      <arglist>(influx_client_t *c, char *line, int len)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>trace_util.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/util/tracing/</path>
    <filename>trace__util_8h.html</filename>
    <includes id="reactor_8h" name="reactor.h" local="yes" import="no" module="no" objc="no">reactor.h</includes>
    <includes id="trace_8h" name="trace.h" local="yes" import="no" module="no" objc="no">trace.h</includes>
    <member kind="define">
      <type>#define</type>
      <name>_LF_TRACE_FAILURE</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga7e540fdcdbf452d4198f88859699ce18</anchor>
      <arglist>(trace_file)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>BUFFER_SIZE</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga6b20d41d6252e9871430c242cb1a56e7</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_TRACE</name>
      <anchorfile>trace__util_8h.html</anchorfile>
      <anchor>a3b6aa696de734ff8fa179a9349e14fe7</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>char *</type>
      <name>get_object_description</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga14da003f17f3fb32043a735158a234c5</anchor>
      <arglist>(void *reactor, int *index)</arglist>
    </member>
    <member kind="function">
      <type>char *</type>
      <name>get_trigger_name</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gaa3ab4dc2202a6bc21f29e14f232301c4</anchor>
      <arglist>(void *trigger, int *index)</arglist>
    </member>
    <member kind="function">
      <type>FILE *</type>
      <name>open_file</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga139bc891db3edc74b0d9f49d1cec20b9</anchor>
      <arglist>(const char *path, const char *mode)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>print_table</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gac8588a40ed5942245242363f13a698f5</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>read_header</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga5cdbb52a1460ec9538f305d12fb4dd2a</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>read_trace</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga496d96019ddbb0d38380191c3042d6b7</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>char *</type>
      <name>root_name</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gaa229eee90d0eb571f63beb972c7c8d82</anchor>
      <arglist>(const char *path)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>usage</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga2ef30c42cbc289d899a8be5d2d8f77d0</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable">
      <type>object_description_t *</type>
      <name>object_table</name>
      <anchorfile>trace__util_8h.html</anchorfile>
      <anchor>a3c9ac13d4710a48bf74879b8bc899f5c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>object_table_size</name>
      <anchorfile>trace__util_8h.html</anchorfile>
      <anchor>a503e215edc60d50846872923f378c6a8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FILE *</type>
      <name>output_file</name>
      <anchorfile>trace__util_8h.html</anchorfile>
      <anchor>a2960eb597f48f3b59bbb850b47e895ff</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>instant_t</type>
      <name>start_time</name>
      <anchorfile>trace__util_8h.html</anchorfile>
      <anchor>a20b56e8fb05c017508f1c4fa2e311a87</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>FILE *</type>
      <name>summary_file</name>
      <anchorfile>trace__util_8h.html</anchorfile>
      <anchor>ac84b6d7af9cb309303d5658997f0d7a2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>top_level</name>
      <anchorfile>trace__util_8h.html</anchorfile>
      <anchor>ab1c55a27299014bb233dac20381bd7d4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>trace_record_t</type>
      <name>trace</name>
      <anchorfile>trace__util_8h.html</anchorfile>
      <anchor>ab8e2538ed85d0094a64f532aa261c407</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>const char *</type>
      <name>trace_event_names</name>
      <anchorfile>trace__util_8h.html</anchorfile>
      <anchor>ad8876683159ec2203bd41295d8af3017</anchor>
      <arglist>[]</arglist>
    </member>
    <member kind="variable">
      <type>FILE *</type>
      <name>trace_file</name>
      <anchorfile>trace__util_8h.html</anchorfile>
      <anchor>a3fc7a0b0ac649ffc4c4d3d36df52f5b0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>README.md</name>
    <path>/Users/runner/work/reactor-c/reactor-c/core/federated/RTI/</path>
    <filename>core_2federated_2RTI_2README_8md.html</filename>
  </compound>
  <compound kind="file">
    <name>README.md</name>
    <path>/Users/runner/work/reactor-c/reactor-c/util/</path>
    <filename>util_2README_8md.html</filename>
  </compound>
  <compound kind="file">
    <name>README.md</name>
    <path>/Users/runner/work/reactor-c/reactor-c/util/tracing/</path>
    <filename>util_2tracing_2README_8md.html</filename>
  </compound>
  <compound kind="file">
    <name>README.md</name>
    <path>/Users/runner/work/reactor-c/reactor-c/util/tracing/visualization/</path>
    <filename>util_2tracing_2visualization_2README_8md.html</filename>
  </compound>
  <compound kind="file">
    <name>type_converter.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/util/</path>
    <filename>type__converter_8h.html</filename>
    <member kind="define">
      <type>#define</type>
      <name>DO_CONVERT</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga9cbcaad341a5b5a807196f116619b9c0</anchor>
      <arglist>(fromType, toType, value)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>PASTE</name>
      <anchorfile>type__converter_8h.html</anchorfile>
      <anchor>a9ac9dd35b93432c0b194466d2aa84788</anchor>
      <arglist>(x, y)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>RESOLVE</name>
      <anchorfile>type__converter_8h.html</anchorfile>
      <anchor>abfdc65c203d58dcfbd3c10da587afd77</anchor>
      <arglist>(i, o, in)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>wave_file_reader.h</name>
    <path>/Users/runner/work/reactor-c/reactor-c/util/</path>
    <filename>wave__file__reader_8h.html</filename>
    <class kind="struct">lf_waveform_t</class>
    <member kind="typedef">
      <type>struct lf_waveform_t</type>
      <name>lf_waveform_t</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga464bd5c31a1a562536e2a54213135604</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>lf_waveform_t *</type>
      <name>read_wave_file</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga149d009d65a3c20d73bcf2ae7c9d6814</anchor>
      <arglist>(const char *path)</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>allocation_record_t</name>
    <filename>structallocation__record__t.html</filename>
    <member kind="variable">
      <type>void *</type>
      <name>allocated</name>
      <anchorfile>structallocation__record__t.html</anchorfile>
      <anchor>af527b359cdf811edd20a217fb9279ad2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct allocation_record_t *</type>
      <name>next</name>
      <anchorfile>structallocation__record__t.html</anchorfile>
      <anchor>ad4d48c8b6826fa989e498cd7e629fd6a</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>deque_t</name>
    <filename>structdeque__t.html</filename>
    <member kind="variable">
      <type>struct deque_node_t *</type>
      <name>back</name>
      <anchorfile>structdeque__t.html</anchorfile>
      <anchor>a8e743ddb2e890f97d0513d896b8176d4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct deque_node_t *</type>
      <name>front</name>
      <anchorfile>structdeque__t.html</anchorfile>
      <anchor>af38a8d05684909a3674ebcb3beeff730</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>size</name>
      <anchorfile>structdeque__t.html</anchorfile>
      <anchor>ac8a00537c1990f31e76114c498c86b29</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>enclave_info_t</name>
    <filename>structenclave__info__t.html</filename>
    <member kind="variable">
      <type>scheduling_node_t</type>
      <name>base</name>
      <anchorfile>structenclave__info__t.html</anchorfile>
      <anchor>ad8bf825ccce43f9a0ac1b92e720cd8f1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>environment_t *</type>
      <name>env</name>
      <anchorfile>structenclave__info__t.html</anchorfile>
      <anchor>a870d2cfd561987d5e72cd5d32a0518da</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_cond_t</type>
      <name>next_event_condition</name>
      <anchorfile>structenclave__info__t.html</anchorfile>
      <anchor>aa9db6017d3a2df60078dc26e1176e236</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>environment_t</name>
    <filename>structenvironment__t.html</filename>
    <member kind="variable">
      <type>trigger_handle_t</type>
      <name>_lf_handle</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>aa7d1ec2280c563dd2dfda38c4ebfb47a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t **</type>
      <name>_lf_intended_tag_fields</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a9a20bd6721513d325f75070ded6ec60e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>_lf_intended_tag_fields_size</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>ac17908a0cfa4fdeeb95ae3f26a73017a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_tag_advancement_barrier_t</type>
      <name>barrier</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a5573749a4205b81f375cecc34c9891cc</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t</type>
      <name>current_tag</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>ac86519935540bf879f60baba5424bc0c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>interval_t</type>
      <name>duration</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>ac03164e9ec8a8779c00c8986dcaa874f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>enclave_info_t *</type>
      <name>enclave_info</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a7efb23f2e28e89e501b4021f423adc3c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pqueue_tag_t *</type>
      <name>event_q</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>adf63684f53f2eb88c73eceb38480f714</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_cond_t</type>
      <name>event_q_changed</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a2e3007a4c73bbe1242f5b4d901c729e8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>execution_started</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a98c12d624ec660bb83bd614481643a16</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_cond_t</type>
      <name>global_tag_barrier_requestors_reached_zero</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>aa4647e332e903c405fd86fff8144c776</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>id</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a369cd434d5ddd5d8061537643bd93d75</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>initialized</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a3daf0ba3d9ea05bed6d51f995a53cdf1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool **</type>
      <name>is_present_fields</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>aa7942ffbadd5cdeab9f84674a5c51b97</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool **</type>
      <name>is_present_fields_abbreviated</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>acc9c260bcc5e4782b1c73f2844588c6f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>is_present_fields_abbreviated_size</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a0787a24fb8262fbe34b47046efe899a5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>is_present_fields_size</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a2635fdfad1b72c79e00c28c21b9c2eaf</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>mode_environment_t *</type>
      <name>modes</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a4348d239de993aee36a42e0ae71871da</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_mutex_t</type>
      <name>mutex</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a18c1176afef786aed0a01a92561d4b40</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>name</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a58106ba029a6892ca82028c163cf2e96</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>need_to_send_LTC</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>aeef35dea438a853fc258631ea4a0e90c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>num_workers</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a3f4ab6b55dcc31b6c5cd9e082d65547d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pqueue_tag_t *</type>
      <name>recycle_q</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a8f815a1dde09400b2c7e58f7b4200f66</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>reaction_t **</type>
      <name>reset_reactions</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a898e3b0c489cec6848c730cf1f7e2266</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>reset_reactions_size</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a4e29a0f3ce76dc8c51f5d0cac1de2651</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_scheduler_t *</type>
      <name>scheduler</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>adb630c2889f3cc081ac09219862cbb8d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>reaction_t **</type>
      <name>shutdown_reactions</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a9cfe475319f15c7e233248eab121add5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>shutdown_reactions_size</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a9f2dfc6abece8574368c7c204dca0707</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>vector_t</type>
      <name>sparse_io_record_sizes</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>ad471293c8b58079a698cbb110981ba09</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t</type>
      <name>start_tag</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>aef9e3dec590f00ca3bbd0e9d7a50bcac</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>reaction_t **</type>
      <name>startup_reactions</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a6661236ece5a440dfe22d23bb08625f2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>startup_reactions_size</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a7d5df5e3bcfbb8a9d2146ca0cb7fd940</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t</type>
      <name>stop_tag</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a76de8d17202ec539b9e3f03755a0e593</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_thread_t *</type>
      <name>thread_ids</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>adf02bc4c7926af749b64b24a1d3be7a3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>trigger_t **</type>
      <name>timer_triggers</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a5ec0f96f80f71a929bf3a150d70b5a67</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>timer_triggers_size</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a2e134fb4e7bafe271bed676dc4444d3e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>watchdog_t **</type>
      <name>watchdogs</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>abf04f0583f502d4ba1a8cac7c7967dd7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>watchdogs_size</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a2f005119de69e43295b3d1566f0dc860</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>worker_thread_count</name>
      <anchorfile>structenvironment__t.html</anchorfile>
      <anchor>a64c98375fa4b34629073863a9bcd6845</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>event_t</name>
    <filename>structevent__t.html</filename>
    <member kind="variable">
      <type>pqueue_tag_element_t</type>
      <name>base</name>
      <anchorfile>structevent__t.html</anchorfile>
      <anchor>a544714ee91cfba1b7cb96f54d711637a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t</type>
      <name>intended_tag</name>
      <anchorfile>structevent__t.html</anchorfile>
      <anchor>a1132282fa06d0f80c1dc586fbbdce59f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_token_t *</type>
      <name>token</name>
      <anchorfile>structevent__t.html</anchorfile>
      <anchor>a85bacc8397754414c8942221d79e4edb</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>trigger_t *</type>
      <name>trigger</name>
      <anchorfile>structevent__t.html</anchorfile>
      <anchor>a0687a08899f1c750aedcc2f44e47f308</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>federate_info_t</name>
    <filename>structfederate__info__t.html</filename>
    <member kind="variable">
      <type>bool</type>
      <name>clock_synchronization_enabled</name>
      <anchorfile>structfederate__info__t.html</anchorfile>
      <anchor>a792b3d59fe8438b370dcea4a9beff748</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>scheduling_node_t</type>
      <name>enclave</name>
      <anchorfile>structfederate__info__t.html</anchorfile>
      <anchor>acd6450143749bf7421c64b28d68a1e3e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pqueue_tag_t *</type>
      <name>in_transit_message_tags</name>
      <anchorfile>structfederate__info__t.html</anchorfile>
      <anchor>aa643bb07575eca7bf9c5665377442369</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>requested_stop</name>
      <anchorfile>structfederate__info__t.html</anchorfile>
      <anchor>af1264d573e3ada0d716c64cfa4f26b36</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char</type>
      <name>server_hostname</name>
      <anchorfile>structfederate__info__t.html</anchorfile>
      <anchor>ac1a52d02c1f668f6421e31d4dd144d82</anchor>
      <arglist>[INET_ADDRSTRLEN]</arglist>
    </member>
    <member kind="variable">
      <type>struct in_addr</type>
      <name>server_ip_addr</name>
      <anchorfile>structfederate__info__t.html</anchorfile>
      <anchor>aa9400fd6abe91530dcf2efb682aaeb15</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int32_t</type>
      <name>server_port</name>
      <anchorfile>structfederate__info__t.html</anchorfile>
      <anchor>adefac36562f7dfcc97f6e1138754f6ca</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>socket</name>
      <anchorfile>structfederate__info__t.html</anchorfile>
      <anchor>a4ecfa6d90654e5b3d46207453d3562e5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_thread_t</type>
      <name>thread_id</name>
      <anchorfile>structfederate__info__t.html</anchorfile>
      <anchor>aedcf9a6f3003d2a4f9c78f5601c463d8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct sockaddr_in</type>
      <name>UDP_addr</name>
      <anchorfile>structfederate__info__t.html</anchorfile>
      <anchor>a49e0905a7ff0b8c85d4e4c61ef4c25c6</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>federate_instance_t</name>
    <filename>structfederate__instance__t.html</filename>
    <member kind="variable">
      <type>bool</type>
      <name>has_downstream</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>ac130c67ec68fe84d6753a54e6e73d59a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>has_upstream</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>a603662cf0155920b8a76659e7dacb9ef</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_thread_t</type>
      <name>inbound_p2p_handling_thread_id</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>ab441abf042a2ff79099bdf2860aeb058</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_thread_t *</type>
      <name>inbound_socket_listeners</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>ac751250db764f954659deb15f1427044</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>is_last_TAG_provisional</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>a05a9586ad89b82d0bf5f969a194ef69c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t</type>
      <name>last_DNET</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>a736230ead584f1e37ae91c5a9c7eef1f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t</type>
      <name>last_sent_LTC</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>ae95f54a01b0367048faa6460ff86e987</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t</type>
      <name>last_sent_NET</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>a42618a45ae75b42e1caec4b373471d96</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t</type>
      <name>last_skipped_NET</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>aea5dc61da0294f35ff2d233111441822</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t</type>
      <name>last_TAG</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>aa66a2f08ea9173743106c7ff18f1846a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>instant_t</type>
      <name>min_delay_from_physical_action_to_federate_output</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>a91a34b0d7f0f604c2f21cf062dbe0314</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>number_of_inbound_p2p_connections</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>a6b728a970ccb1ef778dbb9779582d2f2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>number_of_outbound_p2p_connections</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>a2edee0c930dc6f10437a5a58306a097c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>received_any_DNET</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>aacdbed93ccac5ebb5c141d30b20baef4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>received_stop_request_from_rti</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>ae9ecde30fae3a2487757a7bbb33e918e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_thread_t</type>
      <name>RTI_socket_listener</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>a5b6b4b5912a7bb7df1c8987f38e12004</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>server_port</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>a309c9672d657f20cd1d3661edc7d5179</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>server_socket</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>a20c5d19d5166ec82a09efe072c2f1b49</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>socket_TCP_RTI</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>aacdc19a638ccdc9fae494ce641f4cb04</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>sockets_for_inbound_p2p_connections</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>ac22a97892260d332da6f05194c280f9c</anchor>
      <arglist>[1]</arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>sockets_for_outbound_p2p_connections</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>aa4fcfa96a7c2bfd33cf45ddc43ad3437</anchor>
      <arglist>[1]</arglist>
    </member>
    <member kind="variable">
      <type>lf_thread_t</type>
      <name>staaSetter</name>
      <anchorfile>structfederate__instance__t.html</anchorfile>
      <anchor>a80101d5534e5444c0be0e08d4e65aaae</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>federation_metadata_t</name>
    <filename>structfederation__metadata__t.html</filename>
    <member kind="variable">
      <type>const char *</type>
      <name>federation_id</name>
      <anchorfile>structfederation__metadata__t.html</anchorfile>
      <anchor>a84774d9c9e1d4002221f7a925b40f674</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>rti_host</name>
      <anchorfile>structfederation__metadata__t.html</anchorfile>
      <anchor>a8e66865ef3fa328713e63c4a1666c69a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>rti_port</name>
      <anchorfile>structfederation__metadata__t.html</anchorfile>
      <anchor>af75d81e35fab4ed398df21bd48a50b52</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>rti_user</name>
      <anchorfile>structfederation__metadata__t.html</anchorfile>
      <anchor>acb5d86838516db68d87a30babcce36dd</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>hashmap_entry_t</name>
    <filename>structhashmap__entry__t.html</filename>
    <member kind="variable">
      <type>void *</type>
      <name>key</name>
      <anchorfile>structhashmap__entry__t.html</anchorfile>
      <anchor>afb79966be8a0434092be5bf099cf5930</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void *</type>
      <name>value</name>
      <anchorfile>structhashmap__entry__t.html</anchorfile>
      <anchor>a937166c6b4bb5b8fb059db8f0dd8f6df</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>hashmap_t</name>
    <filename>structhashmap__t.html</filename>
    <member kind="variable">
      <type>size_t</type>
      <name>capacity</name>
      <anchorfile>structhashmap__t.html</anchorfile>
      <anchor>aedd84d44e0a0a7b7ec0fe09be24953da</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>hashmap_entry_t *</type>
      <name>entries</name>
      <anchorfile>structhashmap__t.html</anchorfile>
      <anchor>ad223c7921d900ee107f67c4dbb0b8efe</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void *</type>
      <name>nothing</name>
      <anchorfile>structhashmap__t.html</anchorfile>
      <anchor>ae1cedb0c74a86c7b8049e83fb78a9a7b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>num_entries</name>
      <anchorfile>structhashmap__t.html</anchorfile>
      <anchor>ac4e8a0202e68f1a231c511189dcb012c</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>hashset_itr_st</name>
    <filename>structhashset__itr__st.html</filename>
    <member kind="variable">
      <type>int</type>
      <name>index</name>
      <anchorfile>structhashset__itr__st.html</anchorfile>
      <anchor>aab8a03ec6692632867e8bb92ceb6e2bb</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>hashset_t</type>
      <name>set</name>
      <anchorfile>structhashset__itr__st.html</anchorfile>
      <anchor>adb173545ffbde1e6835af6cb47bf2280</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>hashset_st</name>
    <filename>structhashset__st.html</filename>
    <member kind="variable">
      <type>size_t</type>
      <name>capacity</name>
      <anchorfile>structhashset__st.html</anchorfile>
      <anchor>a0ac6f26c5a0356bc3946a2ef2776443d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void **</type>
      <name>items</name>
      <anchorfile>structhashset__st.html</anchorfile>
      <anchor>a00ecfe733efa06a8ca1d7af8956c0e88</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>mask</name>
      <anchorfile>structhashset__st.html</anchorfile>
      <anchor>adf5780dd9e62e8c1bceb11207383cd0d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>n_deleted_items</name>
      <anchorfile>structhashset__st.html</anchorfile>
      <anchor>a507237ecc37b9c45f79df318534f1109</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>nbits</name>
      <anchorfile>structhashset__st.html</anchorfile>
      <anchor>afaeab28ca2127ce6b5c2e454b3e7b9fd</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>nitems</name>
      <anchorfile>structhashset__st.html</anchorfile>
      <anchor>a83faab3dd9ef40a1f919e74bdd195796</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>influx_client_t</name>
    <filename>structinflux__client__t.html</filename>
    <member kind="variable">
      <type>char *</type>
      <name>db</name>
      <anchorfile>structinflux__client__t.html</anchorfile>
      <anchor>ae125569231c3da876b34119b29c42c44</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>host</name>
      <anchorfile>structinflux__client__t.html</anchorfile>
      <anchor>ad99dc6e85e35dad23647e7106594a76b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>port</name>
      <anchorfile>structinflux__client__t.html</anchorfile>
      <anchor>ab0b9365a5f17e384f7890c901662d579</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>pwd</name>
      <anchorfile>structinflux__client__t.html</anchorfile>
      <anchor>a56abebe7080960a20b0f757f50c65bba</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>token</name>
      <anchorfile>structinflux__client__t.html</anchorfile>
      <anchor>a1fd612486c786139eb7abe8648c37899</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>usr</name>
      <anchorfile>structinflux__client__t.html</anchorfile>
      <anchor>ae561f821063f0808c914f93ae6ad1452</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>influx_v2_client_t</name>
    <filename>structinflux__v2__client__t.html</filename>
    <member kind="variable">
      <type>char *</type>
      <name>bucket</name>
      <anchorfile>structinflux__v2__client__t.html</anchorfile>
      <anchor>a9fbaec5d1869f77ce34aad78aee254e0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>host</name>
      <anchorfile>structinflux__v2__client__t.html</anchorfile>
      <anchor>ac9e22dc58eb72cfddbe256492d101a65</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>org</name>
      <anchorfile>structinflux__v2__client__t.html</anchorfile>
      <anchor>aea4185f692b0ec03826caa878ab7b7c5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>port</name>
      <anchorfile>structinflux__v2__client__t.html</anchorfile>
      <anchor>a0ca915fdcfe148214a4f9f14df619ba6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>precision</name>
      <anchorfile>structinflux__v2__client__t.html</anchorfile>
      <anchor>a92721dbe215bdd3ed362dc642bc4f23e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>pwd</name>
      <anchorfile>structinflux__v2__client__t.html</anchorfile>
      <anchor>a216106e8825a18c2b3436748d0fd2a7f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>token</name>
      <anchorfile>structinflux__v2__client__t.html</anchorfile>
      <anchor>a3e27910940db7a6082db043e5207d4ad</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>usr</name>
      <anchorfile>structinflux__v2__client__t.html</anchorfile>
      <anchor>a6296ea8ebcc673915d41519ec69d4677</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>lf_action_base_t</name>
    <filename>structlf__action__base__t.html</filename>
    <member kind="variable">
      <type>bool</type>
      <name>has_value</name>
      <anchorfile>structlf__action__base__t.html</anchorfile>
      <anchor>aaa41924b6862c8128e8ea02b1c964c52</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>is_present</name>
      <anchorfile>structlf__action__base__t.html</anchorfile>
      <anchor>a2ffef0622ed175d43758f8a50046ed50</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>self_base_t *</type>
      <name>parent</name>
      <anchorfile>structlf__action__base__t.html</anchorfile>
      <anchor>abf564683446cb923e96bef03be159fac</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>source_id</name>
      <anchorfile>structlf__action__base__t.html</anchorfile>
      <anchor>af4e189ab3af9a6a85fc693f9099e0120</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>token_template_t</type>
      <name>tmplt</name>
      <anchorfile>structlf__action__base__t.html</anchorfile>
      <anchor>a09b0a1d42be1f4df2b1c460f966937d3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>trigger_t *</type>
      <name>trigger</name>
      <anchorfile>structlf__action__base__t.html</anchorfile>
      <anchor>aacf5d85cce69c1ed02a1717a3fcbb46e</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>lf_action_internal_t</name>
    <filename>structlf__action__internal__t.html</filename>
    <member kind="variable">
      <type>trigger_t *</type>
      <name>trigger</name>
      <anchorfile>structlf__action__internal__t.html</anchorfile>
      <anchor>a0f59c9d0f5520d8f5a733bfa9ee41930</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>lf_multiport_iterator_t</name>
    <filename>structlf__multiport__iterator__t.html</filename>
    <member kind="variable">
      <type>int</type>
      <name>idx</name>
      <anchorfile>structlf__multiport__iterator__t.html</anchorfile>
      <anchor>a6707e4d4535a1630d74d5fcda16d163d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>next</name>
      <anchorfile>structlf__multiport__iterator__t.html</anchorfile>
      <anchor>afecdacfcd14db12bb12e5c0b0589dba3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_port_base_t **</type>
      <name>port</name>
      <anchorfile>structlf__multiport__iterator__t.html</anchorfile>
      <anchor>a30b60ca0b6d101367ccb31a0725ee74b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>width</name>
      <anchorfile>structlf__multiport__iterator__t.html</anchorfile>
      <anchor>a9a506388d73fdb6c0fd45c182f6a41ac</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>lf_port_base_t</name>
    <filename>structlf__port__base__t.html</filename>
    <member kind="variable">
      <type>int</type>
      <name>destination_channel</name>
      <anchorfile>structlf__port__base__t.html</anchorfile>
      <anchor>ab7734e3cdd562ad5f6ad94f97e095822</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>is_present</name>
      <anchorfile>structlf__port__base__t.html</anchorfile>
      <anchor>aa944290a27aff5cbc8334ad026076290</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>num_destinations</name>
      <anchorfile>structlf__port__base__t.html</anchorfile>
      <anchor>abb1d021a0d2668988d927e9754b699dc</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>self_base_t *</type>
      <name>source_reactor</name>
      <anchorfile>structlf__port__base__t.html</anchorfile>
      <anchor>a84814122f901ea1ed0ac6f380bfbc9ee</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_sparse_io_record_t *</type>
      <name>sparse_record</name>
      <anchorfile>structlf__port__base__t.html</anchorfile>
      <anchor>a200acd25d821c527b5290a4137dc261b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>token_template_t</type>
      <name>tmplt</name>
      <anchorfile>structlf__port__base__t.html</anchorfile>
      <anchor>adeb3a2cb66dffc49426a16b8057d1736</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>lf_port_internal_t</name>
    <filename>structlf__port__internal__t.html</filename>
    <member kind="variable">
      <type>int</type>
      <name>destination_channel</name>
      <anchorfile>structlf__port__internal__t.html</anchorfile>
      <anchor>a7c70e1156dfb93174d5af2175ba58191</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>num_destinations</name>
      <anchorfile>structlf__port__internal__t.html</anchorfile>
      <anchor>a7948ecbc67381ccbe57c033272d6544f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>self_base_t *</type>
      <name>source_reactor</name>
      <anchorfile>structlf__port__internal__t.html</anchorfile>
      <anchor>a58c08982921f3d4ef3c0525975f06e40</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_sparse_io_record_t *</type>
      <name>sparse_record</name>
      <anchorfile>structlf__port__internal__t.html</anchorfile>
      <anchor>aaa9ff391953ce66424abf4828777c89b</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>lf_scheduler_t</name>
    <filename>structlf__scheduler__t.html</filename>
    <member kind="variable">
      <type>custom_scheduler_data_t *</type>
      <name>custom_data</name>
      <anchorfile>structlf__scheduler__t.html</anchorfile>
      <anchor>aef6ea4bec6ed373ca0007f6fb5175bd7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct environment_t *</type>
      <name>env</name>
      <anchorfile>structlf__scheduler__t.html</anchorfile>
      <anchor>a5dc0eaf8345d8d5aff85d96954c517a5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>volatile int *</type>
      <name>indexes</name>
      <anchorfile>structlf__scheduler__t.html</anchorfile>
      <anchor>ab358c0fd232603ced1f0c3ad6d337d62</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>max_reaction_level</name>
      <anchorfile>structlf__scheduler__t.html</anchorfile>
      <anchor>aa09d9a831f83e2d1f45f68b84926c010</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>volatile size_t</type>
      <name>number_of_idle_workers</name>
      <anchorfile>structlf__scheduler__t.html</anchorfile>
      <anchor>a1a647a73662fce0f55524dd596542d4b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>number_of_workers</name>
      <anchorfile>structlf__scheduler__t.html</anchorfile>
      <anchor>a572c014dd84577ccddc5478b4c1194fd</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>volatile bool</type>
      <name>should_stop</name>
      <anchorfile>structlf__scheduler__t.html</anchorfile>
      <anchor>adc8bd8e40ac5b717d6d0a1d0518f5765</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void *</type>
      <name>transfer_reactions</name>
      <anchorfile>structlf__scheduler__t.html</anchorfile>
      <anchor>af028b40d82dccf4dc969be37dcd31f4a</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>lf_scheduling_policy_t</name>
    <filename>structlf__scheduling__policy__t.html</filename>
    <member kind="variable">
      <type>lf_scheduling_policy_type_t</type>
      <name>policy</name>
      <anchorfile>structlf__scheduling__policy__t.html</anchorfile>
      <anchor>a48e1693aafa698bb53266497e340e6a5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>priority</name>
      <anchorfile>structlf__scheduling__policy__t.html</anchorfile>
      <anchor>a841601dc7442af1fc4b89fb658915907</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>interval_t</type>
      <name>time_slice</name>
      <anchorfile>structlf__scheduling__policy__t.html</anchorfile>
      <anchor>a15dbf1952ebdd232040fa334af433d0e</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>lf_semaphore_t</name>
    <filename>structlf__semaphore__t.html</filename>
    <member kind="variable">
      <type>lf_cond_t</type>
      <name>cond</name>
      <anchorfile>structlf__semaphore__t.html</anchorfile>
      <anchor>a37a3bc5a68d8a9245dbc59e6b9010f3b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>count</name>
      <anchorfile>structlf__semaphore__t.html</anchorfile>
      <anchor>aefda92a42c49b9003089fb3d17bf62b3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_mutex_t</type>
      <name>mutex</name>
      <anchorfile>structlf__semaphore__t.html</anchorfile>
      <anchor>a6811cbc219c0e9ed43284e8c49d1c724</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>lf_sparse_io_record_t</name>
    <filename>structlf__sparse__io__record__t.html</filename>
    <member kind="variable">
      <type>size_t</type>
      <name>capacity</name>
      <anchorfile>structlf__sparse__io__record__t.html</anchorfile>
      <anchor>a0cc33a35b50da10e8a871bd3b7afcd02</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t *</type>
      <name>present_channels</name>
      <anchorfile>structlf__sparse__io__record__t.html</anchorfile>
      <anchor>aeb8b225c2e6bd83a70d7fdc7b2153472</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>size</name>
      <anchorfile>structlf__sparse__io__record__t.html</anchorfile>
      <anchor>aa4dbb7002633cc49c634ab3ba9822ef3</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>lf_stat_ll</name>
    <filename>structlf__stat__ll.html</filename>
    <member kind="variable">
      <type>int64_t</type>
      <name>average</name>
      <anchorfile>structlf__stat__ll.html</anchorfile>
      <anchor>aa1018daf35cf8d68814ee876382eaecc</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int64_t</type>
      <name>max</name>
      <anchorfile>structlf__stat__ll.html</anchorfile>
      <anchor>a8841b6fc40c60f3f1b73ada60ce556b7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int64_t</type>
      <name>standard_deviation</name>
      <anchorfile>structlf__stat__ll.html</anchorfile>
      <anchor>a012949dce8d9257172f510eebdf848d4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int64_t</type>
      <name>variance</name>
      <anchorfile>structlf__stat__ll.html</anchorfile>
      <anchor>a58abf354e780a1ec59cae3644d2fcbc1</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>lf_tag_advancement_barrier_t</name>
    <filename>structlf__tag__advancement__barrier__t.html</filename>
    <member kind="variable">
      <type>tag_t</type>
      <name>horizon</name>
      <anchorfile>structlf__tag__advancement__barrier__t.html</anchorfile>
      <anchor>a835fb4c508100af2868c932963825f28</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>requestors</name>
      <anchorfile>structlf__tag__advancement__barrier__t.html</anchorfile>
      <anchor>a070d091e5f93dcd11250c088fe5c2f0b</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>lf_token_t</name>
    <filename>structlf__token__t.html</filename>
    <member kind="variable">
      <type>size_t</type>
      <name>length</name>
      <anchorfile>structlf__token__t.html</anchorfile>
      <anchor>a953c95d30422caa9f4bfcfbc7d12b0b3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct lf_token_t *</type>
      <name>next</name>
      <anchorfile>structlf__token__t.html</anchorfile>
      <anchor>a318e7910743e63444e08cca50281fd2c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>ref_count</name>
      <anchorfile>structlf__token__t.html</anchorfile>
      <anchor>a920428c414bcc04de620a6cdfd325023</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>token_type_t *</type>
      <name>type</name>
      <anchorfile>structlf__token__t.html</anchorfile>
      <anchor>a9c197ef8846eab66823cef19cbb35848</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void *</type>
      <name>value</name>
      <anchorfile>structlf__token__t.html</anchorfile>
      <anchor>a96f790f8f8dcc01474dbc255e05ceaa8</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>lf_waveform_t</name>
    <filename>structlf__waveform__t.html</filename>
    <member kind="variable">
      <type>uint32_t</type>
      <name>length</name>
      <anchorfile>structlf__waveform__t.html</anchorfile>
      <anchor>a04db217ea68b2e445ff25f2f53f855cd</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t</type>
      <name>num_channels</name>
      <anchorfile>structlf__waveform__t.html</anchorfile>
      <anchor>ac1821431ece937edd64e32209a793018</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int16_t *</type>
      <name>waveform</name>
      <anchorfile>structlf__waveform__t.html</anchorfile>
      <anchor>add06d5d0fc6b2337a8aa902736c28558</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>minimum_delay_t</name>
    <filename>structminimum__delay__t.html</filename>
    <member kind="variable">
      <type>int</type>
      <name>id</name>
      <anchorfile>structminimum__delay__t.html</anchorfile>
      <anchor>a73c4d4da28ebc2a2a67d30432f5be8a2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t</type>
      <name>min_delay</name>
      <anchorfile>structminimum__delay__t.html</anchorfile>
      <anchor>a7138c787d5c62fdf5e9cacc52ea2e048</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>mixed_radix_int_t</name>
    <filename>structmixed__radix__int__t.html</filename>
    <member kind="variable">
      <type>int *</type>
      <name>digits</name>
      <anchorfile>structmixed__radix__int__t.html</anchorfile>
      <anchor>a0e7296e98823c245fa1e3d7b6b590e4d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int *</type>
      <name>permutation</name>
      <anchorfile>structmixed__radix__int__t.html</anchorfile>
      <anchor>ae267c25cb2c754b10d976709b6f2a740</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int *</type>
      <name>radixes</name>
      <anchorfile>structmixed__radix__int__t.html</anchorfile>
      <anchor>a867a3d65a976b2d4faab1b0c4129592b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>size</name>
      <anchorfile>structmixed__radix__int__t.html</anchorfile>
      <anchor>a746c75cd4e29669f4e349dda3f30d1bd</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>mode_environment_t</name>
    <filename>structmode__environment__t.html</filename>
    <member kind="variable">
      <type>reactor_mode_state_t **</type>
      <name>modal_reactor_states</name>
      <anchorfile>structmode__environment__t.html</anchorfile>
      <anchor>a8864ab1c43d05c20c27b0a0e926407ca</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>modal_reactor_states_size</name>
      <anchorfile>structmode__environment__t.html</anchorfile>
      <anchor>a889cfa50e8a96efdca54fd14d4a83b10</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>mode_state_variable_reset_data_t *</type>
      <name>state_resets</name>
      <anchorfile>structmode__environment__t.html</anchorfile>
      <anchor>ac6b6f511e6330c3f2e2b7ac99043eda7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>state_resets_size</name>
      <anchorfile>structmode__environment__t.html</anchorfile>
      <anchor>a800cf216bd18aca6b5ebd6b8b67c9bbd</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint8_t</type>
      <name>triggered_reactions_request</name>
      <anchorfile>structmode__environment__t.html</anchorfile>
      <anchor>a6f997442d82d11508e8a2396ad27069e</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>mode_state_variable_reset_data_t</name>
    <filename>structmode__state__variable__reset__data__t.html</filename>
    <member kind="variable">
      <type>reactor_mode_t *</type>
      <name>mode</name>
      <anchorfile>structmode__state__variable__reset__data__t.html</anchorfile>
      <anchor>a2e7a10ff1c15709a1870ff712023d773</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>size</name>
      <anchorfile>structmode__state__variable__reset__data__t.html</anchorfile>
      <anchor>a19f5068c6cefaeabc5f8bc037cb8a82b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void *</type>
      <name>source</name>
      <anchorfile>structmode__state__variable__reset__data__t.html</anchorfile>
      <anchor>a544d394876401faf073d4ac4e342d9f1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void *</type>
      <name>target</name>
      <anchorfile>structmode__state__variable__reset__data__t.html</anchorfile>
      <anchor>aae34be2cd3f5b2b1082c3ded9c4f16da</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>object_description_t</name>
    <filename>structobject__description__t.html</filename>
    <member kind="variable">
      <type>char *</type>
      <name>description</name>
      <anchorfile>structobject__description__t.html</anchorfile>
      <anchor>a8f7829cb65aea820db731eaf70c079fc</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void *</type>
      <name>pointer</name>
      <anchorfile>structobject__description__t.html</anchorfile>
      <anchor>a1537fbbe447da2353ff1052b5659403c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void *</type>
      <name>trigger</name>
      <anchorfile>structobject__description__t.html</anchorfile>
      <anchor>a377d60ccfc378013acc940c03a5c724b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>_lf_trace_object_t</type>
      <name>type</name>
      <anchorfile>structobject__description__t.html</anchorfile>
      <anchor>ad8e5d674d5037ef1500a6608cc9d9209</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>pqueue_t</name>
    <filename>structpqueue__t.html</filename>
    <member kind="variable">
      <type>size_t</type>
      <name>avail</name>
      <anchorfile>structpqueue__t.html</anchorfile>
      <anchor>a2edc2359d1420efc7031677ecd19c1ec</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pqueue_cmp_pri_f</type>
      <name>cmppri</name>
      <anchorfile>structpqueue__t.html</anchorfile>
      <anchor>af44c1953e47510529147cfbea33489ef</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void **</type>
      <name>d</name>
      <anchorfile>structpqueue__t.html</anchorfile>
      <anchor>a92b9ee0486259382e9087cd1f05f2792</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pqueue_eq_elem_f</type>
      <name>eqelem</name>
      <anchorfile>structpqueue__t.html</anchorfile>
      <anchor>a27ad9197156c9be3cbc2845478223f32</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pqueue_get_pos_f</type>
      <name>getpos</name>
      <anchorfile>structpqueue__t.html</anchorfile>
      <anchor>a20a38eccf17fb90e2c483503326c3fba</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pqueue_get_pri_f</type>
      <name>getpri</name>
      <anchorfile>structpqueue__t.html</anchorfile>
      <anchor>ae1c098eb8c14bdfbce6797ede3f0c387</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pqueue_print_entry_f</type>
      <name>prt</name>
      <anchorfile>structpqueue__t.html</anchorfile>
      <anchor>aeb48e61916a065bb67c7dc3cc963ffeb</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pqueue_set_pos_f</type>
      <name>setpos</name>
      <anchorfile>structpqueue__t.html</anchorfile>
      <anchor>a0960bed72e0c851bccf1591c39360c73</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>size</name>
      <anchorfile>structpqueue__t.html</anchorfile>
      <anchor>a37a81db394494e542cdaec80ce4f2f42</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>step</name>
      <anchorfile>structpqueue__t.html</anchorfile>
      <anchor>ab405fbb415065192ce9a1a39b5f45978</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>pqueue_tag_element_t</name>
    <filename>structpqueue__tag__element__t.html</filename>
    <member kind="variable">
      <type>int</type>
      <name>is_dynamic</name>
      <anchorfile>structpqueue__tag__element__t.html</anchorfile>
      <anchor>ab559a9ee2aa4bb0b97a802cc4e0c8fa5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>pos</name>
      <anchorfile>structpqueue__tag__element__t.html</anchorfile>
      <anchor>a7b2eee8519e95e266c6bfe58b9c0fa1a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t</type>
      <name>tag</name>
      <anchorfile>structpqueue__tag__element__t.html</anchorfile>
      <anchor>a3c8c28164b948edc70f9b0d403043fcb</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>reaction_t</name>
    <filename>structreaction__t.html</filename>
    <member kind="variable">
      <type>interval_t</type>
      <name>deadline</name>
      <anchorfile>structreaction__t.html</anchorfile>
      <anchor>a9dbda39c51da25a63258a83793f97cd8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>reaction_function_t</type>
      <name>deadline_violation_handler</name>
      <anchorfile>structreaction__t.html</anchorfile>
      <anchor>a487aaa96abda17f7184c07ccd1e870b2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>reaction_function_t</type>
      <name>function</name>
      <anchorfile>structreaction__t.html</anchorfile>
      <anchor>afd6087e8a7d3cc46b430d4187128896c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>index_t</type>
      <name>index</name>
      <anchorfile>structreaction__t.html</anchorfile>
      <anchor>a6dbc6da9eb7523555e9f548438069c7e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>is_an_input_reaction</name>
      <anchorfile>structreaction__t.html</anchorfile>
      <anchor>aa9cc59b3b6029cb8b0e3d1f5fde94fce</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>is_STP_violated</name>
      <anchorfile>structreaction__t.html</anchorfile>
      <anchor>a2bc026e89d82a877a3b7224541c25d42</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>reaction_t *</type>
      <name>last_enabling_reaction</name>
      <anchorfile>structreaction__t.html</anchorfile>
      <anchor>acbf0778e73f7eeedbb89345d1beb09b6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>reactor_mode_t *</type>
      <name>mode</name>
      <anchorfile>structreaction__t.html</anchorfile>
      <anchor>af1f2e2ef6137841735a4815d8ebdc060</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *</type>
      <name>name</name>
      <anchorfile>structreaction__t.html</anchorfile>
      <anchor>a5af95a97a0b83dbf7954236378f1bfa2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>num_outputs</name>
      <anchorfile>structreaction__t.html</anchorfile>
      <anchor>ac0bd71c667c13a7d2f96cb856d297cc0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>number</name>
      <anchorfile>structreaction__t.html</anchorfile>
      <anchor>aab519d27f6f52545358a4f5da15f3a18</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool **</type>
      <name>output_produced</name>
      <anchorfile>structreaction__t.html</anchorfile>
      <anchor>a1cf8e3d4461fd794f7fbecb2214cac7c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>pos</name>
      <anchorfile>structreaction__t.html</anchorfile>
      <anchor>a9f568fa1c755a3ade1962c705c55f66b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void *</type>
      <name>self</name>
      <anchorfile>structreaction__t.html</anchorfile>
      <anchor>a534c11eeddbe4c0272fc2d77abb82168</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>reaction_status_t</type>
      <name>status</name>
      <anchorfile>structreaction__t.html</anchorfile>
      <anchor>a2c42ec5e47d9ee2c7b44882d96117002</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>reaction_function_t</type>
      <name>STP_handler</name>
      <anchorfile>structreaction__t.html</anchorfile>
      <anchor>a7c0b5a8f57c78b1434e92cedf0379228</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int *</type>
      <name>triggered_sizes</name>
      <anchorfile>structreaction__t.html</anchorfile>
      <anchor>a9b8474ec66b8b956699cc4a1e2370508</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>trigger_t ***</type>
      <name>triggers</name>
      <anchorfile>structreaction__t.html</anchorfile>
      <anchor>a75ea18d69a144fdb181857cc413ae62c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>worker_affinity</name>
      <anchorfile>structreaction__t.html</anchorfile>
      <anchor>a861cbf9521eb367cedf9e2d0ecee0fca</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>reactor_mode_state_t</name>
    <filename>structreactor__mode__state__t.html</filename>
    <member kind="variable">
      <type>reactor_mode_t *</type>
      <name>current_mode</name>
      <anchorfile>structreactor__mode__state__t.html</anchorfile>
      <anchor>ae48f584679825831a437772831895774</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>reactor_mode_t *</type>
      <name>initial_mode</name>
      <anchorfile>structreactor__mode__state__t.html</anchorfile>
      <anchor>a57ed04659b5eb224c6df34cf0029c274</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_mode_change_type_t</type>
      <name>mode_change</name>
      <anchorfile>structreactor__mode__state__t.html</anchorfile>
      <anchor>a8d8a130b7227671d11496e1b6fcfdace</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>reactor_mode_t *</type>
      <name>next_mode</name>
      <anchorfile>structreactor__mode__state__t.html</anchorfile>
      <anchor>ac8365c61460d0e4bd957499755765a3a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>reactor_mode_t *</type>
      <name>parent_mode</name>
      <anchorfile>structreactor__mode__state__t.html</anchorfile>
      <anchor>aeefe02c62696747dd96089d2cfd42cf2</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>reactor_mode_t</name>
    <filename>structreactor__mode__t.html</filename>
    <member kind="variable">
      <type>instant_t</type>
      <name>deactivation_time</name>
      <anchorfile>structreactor__mode__t.html</anchorfile>
      <anchor>a6d46c06bb2282db0cc44f3e12e86e4bf</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint8_t</type>
      <name>flags</name>
      <anchorfile>structreactor__mode__t.html</anchorfile>
      <anchor>a1cfbbf04e57197d289000420ab3742a4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>name</name>
      <anchorfile>structreactor__mode__t.html</anchorfile>
      <anchor>a26ee520b8d6383bc025cf9fbdde7ef5e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>reactor_mode_state_t *</type>
      <name>state</name>
      <anchorfile>structreactor__mode__t.html</anchorfile>
      <anchor>a140e493ed78d6fb504fe988b6577e7e6</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>rti_addr_info_t</name>
    <filename>structrti__addr__info__t.html</filename>
    <member kind="variable">
      <type>bool</type>
      <name>has_host</name>
      <anchorfile>structrti__addr__info__t.html</anchorfile>
      <anchor>a73312cb4f806de3a9b23eb811861ac24</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>has_port</name>
      <anchorfile>structrti__addr__info__t.html</anchorfile>
      <anchor>a814382a63d0c415566268997e7e2c238</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>has_user</name>
      <anchorfile>structrti__addr__info__t.html</anchorfile>
      <anchor>a66ce998dc61ba1e1a939c18cb79ae9af</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char</type>
      <name>rti_host_str</name>
      <anchorfile>structrti__addr__info__t.html</anchorfile>
      <anchor>ab67e59a55eda384237ee0c01b161808c</anchor>
      <arglist>[256]</arglist>
    </member>
    <member kind="variable">
      <type>char</type>
      <name>rti_port_str</name>
      <anchorfile>structrti__addr__info__t.html</anchorfile>
      <anchor>a182bcc2f25e999e33b4065115287a6a0</anchor>
      <arglist>[6]</arglist>
    </member>
    <member kind="variable">
      <type>char</type>
      <name>rti_user_str</name>
      <anchorfile>structrti__addr__info__t.html</anchorfile>
      <anchor>a3604fd6c5f95ce9afa7afcb188093ac8</anchor>
      <arglist>[256]</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>rti_common_t</name>
    <filename>structrti__common__t.html</filename>
    <member kind="variable">
      <type>bool</type>
      <name>dnet_disabled</name>
      <anchorfile>structrti__common__t.html</anchorfile>
      <anchor>a21547718724f363f1b684898535a6874</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t</type>
      <name>max_stop_tag</name>
      <anchorfile>structrti__common__t.html</anchorfile>
      <anchor>a415277bcbc13aabe238614b9a0d6d4de</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t *</type>
      <name>min_delays</name>
      <anchorfile>structrti__common__t.html</anchorfile>
      <anchor>affa919b013ef94b2ff61d1b82a2663fe</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_mutex_t *</type>
      <name>mutex</name>
      <anchorfile>structrti__common__t.html</anchorfile>
      <anchor>a1a6c918ac1891377b8bd69c06a44e290</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>num_scheduling_nodes_handling_stop</name>
      <anchorfile>structrti__common__t.html</anchorfile>
      <anchor>afa7adc70f4136fb39ef29a18114deaca</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t</type>
      <name>number_of_scheduling_nodes</name>
      <anchorfile>structrti__common__t.html</anchorfile>
      <anchor>aca4395883c414d136a8bdafc1fbc871c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>scheduling_node_t **</type>
      <name>scheduling_nodes</name>
      <anchorfile>structrti__common__t.html</anchorfile>
      <anchor>abc9727d3d0ae56a137f773a09ffd323d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>tracing_enabled</name>
      <anchorfile>structrti__common__t.html</anchorfile>
      <anchor>aaa7989c9852afa9d1fc453184c288433</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>rti_local_t</name>
    <filename>structrti__local__t.html</filename>
    <member kind="variable">
      <type>rti_common_t</type>
      <name>base</name>
      <anchorfile>structrti__local__t.html</anchorfile>
      <anchor>a26e44960e6bc517df43a9fd1e7dccdb0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>rti_remote_t</name>
    <filename>structrti__remote__t.html</filename>
    <member kind="variable">
      <type>volatile bool</type>
      <name>all_federates_exited</name>
      <anchorfile>structrti__remote__t.html</anchorfile>
      <anchor>a6d72eef29e4ac1543971baed62c5df25</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>authentication_enabled</name>
      <anchorfile>structrti__remote__t.html</anchorfile>
      <anchor>a75eec5d1f98a80feb8b84e2e4c5e6a2e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>rti_common_t</type>
      <name>base</name>
      <anchorfile>structrti__remote__t.html</anchorfile>
      <anchor>a3fac0d1830b8baebe13e3fc1366868c7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int32_t</type>
      <name>clock_sync_exchanges_per_interval</name>
      <anchorfile>structrti__remote__t.html</anchorfile>
      <anchor>abe870cbec048a83fe1a114ba08e7a576</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>clock_sync_stat</type>
      <name>clock_sync_global_status</name>
      <anchorfile>structrti__remote__t.html</anchorfile>
      <anchor>a461fd60c5d1130bec748b4774f4a878a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>clock_sync_period_ns</name>
      <anchorfile>structrti__remote__t.html</anchorfile>
      <anchor>a59948493be7f9c37c4d9e87fe794cc4a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_thread_t</type>
      <name>clock_thread</name>
      <anchorfile>structrti__remote__t.html</anchorfile>
      <anchor>ae82c97407ee17f16cd9e95cc4fade654</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *</type>
      <name>federation_id</name>
      <anchorfile>structrti__remote__t.html</anchorfile>
      <anchor>aa6ebdbc41ba330fc7d23f478bf62d7ea</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t</type>
      <name>final_port_TCP</name>
      <anchorfile>structrti__remote__t.html</anchorfile>
      <anchor>af398be0ec31a58806d978a0670b59fac</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t</type>
      <name>final_port_UDP</name>
      <anchorfile>structrti__remote__t.html</anchorfile>
      <anchor>a313ec8858bdf1eb12c8a24c17290afb2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int64_t</type>
      <name>max_start_time</name>
      <anchorfile>structrti__remote__t.html</anchorfile>
      <anchor>af8774b865d79565eff6296da5459ac83</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>num_feds_proposed_start</name>
      <anchorfile>structrti__remote__t.html</anchorfile>
      <anchor>a1c695fdf2ff3d64f7599887ce525d64d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>socket_descriptor_TCP</name>
      <anchorfile>structrti__remote__t.html</anchorfile>
      <anchor>a092fe7859e3ead8ad646a848ef6282e7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>socket_descriptor_UDP</name>
      <anchorfile>structrti__remote__t.html</anchorfile>
      <anchor>acdd54a055b69ed96feef48423e7e0496</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>stop_in_progress</name>
      <anchorfile>structrti__remote__t.html</anchorfile>
      <anchor>a498c079c9b24587dccb394f17a3fdc2c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t</type>
      <name>user_specified_port</name>
      <anchorfile>structrti__remote__t.html</anchorfile>
      <anchor>a2f275409a0e4a1a537c3028e674bada3</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>sched_params_t</name>
    <filename>structsched__params__t.html</filename>
    <member kind="variable">
      <type>size_t *</type>
      <name>num_reactions_per_level</name>
      <anchorfile>structsched__params__t.html</anchorfile>
      <anchor>a826bb0752d0c9e915bf5abdefe9ee62d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>num_reactions_per_level_size</name>
      <anchorfile>structsched__params__t.html</anchorfile>
      <anchor>a15d5c083329a86fde98d5990c21e43a4</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>scheduling_node_t</name>
    <filename>structscheduling__node__t.html</filename>
    <member kind="variable">
      <type>tag_t</type>
      <name>completed</name>
      <anchorfile>structscheduling__node__t.html</anchorfile>
      <anchor>a83efaa5495f70aeb5c6a9dd5e868c692</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>flags</name>
      <anchorfile>structscheduling__node__t.html</anchorfile>
      <anchor>a648268bb3cc4c6f98c214d550b9ebc9e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t</type>
      <name>id</name>
      <anchorfile>structscheduling__node__t.html</anchorfile>
      <anchor>a9e2003990fc390c31ea6db9462adb9c1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t *</type>
      <name>immediate_downstreams</name>
      <anchorfile>structscheduling__node__t.html</anchorfile>
      <anchor>ae236232316abbcbdb2360317e534984f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>interval_t *</type>
      <name>immediate_upstream_delays</name>
      <anchorfile>structscheduling__node__t.html</anchorfile>
      <anchor>af35c3654a0b65be2b2ac0dc4b694e1cc</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t *</type>
      <name>immediate_upstreams</name>
      <anchorfile>structscheduling__node__t.html</anchorfile>
      <anchor>a79c9c4d1de81e3d6f5ab40bb7197f623</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t</type>
      <name>last_DNET</name>
      <anchorfile>structscheduling__node__t.html</anchorfile>
      <anchor>af10365aa9ba44ea956e6b1740280a32b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t</type>
      <name>last_granted</name>
      <anchorfile>structscheduling__node__t.html</anchorfile>
      <anchor>aa3bfdec341507272562c0e707497734e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t</type>
      <name>last_provisionally_granted</name>
      <anchorfile>structscheduling__node__t.html</anchorfile>
      <anchor>a8333ae8c4f4f712ab20b50af6fc11397</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>execution_mode_t</type>
      <name>mode</name>
      <anchorfile>structscheduling__node__t.html</anchorfile>
      <anchor>a7fb62536a8824971861eb3c83c15fb1b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t</type>
      <name>next_event</name>
      <anchorfile>structscheduling__node__t.html</anchorfile>
      <anchor>ac61795068b7988acd45cf179039fe41c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t</type>
      <name>num_immediate_downstreams</name>
      <anchorfile>structscheduling__node__t.html</anchorfile>
      <anchor>ae03d5b3e440b6cbc5bcb244d0f82eb2a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t</type>
      <name>num_immediate_upstreams</name>
      <anchorfile>structscheduling__node__t.html</anchorfile>
      <anchor>a7d9ee494bf8d5fe27788192b1e2ed30c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>scheduling_node_state_t</type>
      <name>state</name>
      <anchorfile>structscheduling__node__t.html</anchorfile>
      <anchor>a6397a26be230f2c07d88dd291e8f5f37</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>self_base_t</name>
    <filename>structself__base__t.html</filename>
    <member kind="variable">
      <type>reactor_mode_state_t</type>
      <name>_lf__mode_state</name>
      <anchorfile>structself__base__t.html</anchorfile>
      <anchor>a61e55f22a4a5e037d9d25f9cb516ff8b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct allocation_record_t *</type>
      <name>allocations</name>
      <anchorfile>structself__base__t.html</anchorfile>
      <anchor>a851c71d2d9ce4ca9c1811283bcf5f2f9</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>environment_t *</type>
      <name>environment</name>
      <anchorfile>structself__base__t.html</anchorfile>
      <anchor>a382226593b7045aeb0a30ce90122f64e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct reaction_t *</type>
      <name>executing_reaction</name>
      <anchorfile>structself__base__t.html</anchorfile>
      <anchor>a2282568a7a814225eec8a2148e087f09</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>full_name</name>
      <anchorfile>structself__base__t.html</anchorfile>
      <anchor>aa37770fb258cdb7d8406d8416bade873</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>name</name>
      <anchorfile>structself__base__t.html</anchorfile>
      <anchor>a2e2f5caeb7ed80f890ab6defba500dff</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>self_base_t *</type>
      <name>parent</name>
      <anchorfile>structself__base__t.html</anchorfile>
      <anchor>aa450d272dcc263b883d8727d20f3c1dc</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void *</type>
      <name>reactor_mutex</name>
      <anchorfile>structself__base__t.html</anchorfile>
      <anchor>ad19c2e3ac3920c1244d81076ecbad2c9</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>socket_stat_t</name>
    <filename>structsocket__stat__t.html</filename>
    <member kind="variable">
      <type>interval_t</type>
      <name>clock_synchronization_error_bound</name>
      <anchorfile>structsocket__stat__t.html</anchorfile>
      <anchor>aa86b4d56287ba851680428e98fd6cdf2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>interval_t</type>
      <name>history</name>
      <anchorfile>structsocket__stat__t.html</anchorfile>
      <anchor>ab34f093b789df801fe4717cca5a3e745</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>interval_t</type>
      <name>local_delay</name>
      <anchorfile>structsocket__stat__t.html</anchorfile>
      <anchor>a645afb48648fcee15ce3625456eb5c21</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>instant_t</type>
      <name>local_physical_clock_snapshot_T2</name>
      <anchorfile>structsocket__stat__t.html</anchorfile>
      <anchor>af084180438ba84b4ec8e8fd873222605</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>interval_t</type>
      <name>network_stat_round_trip_delay_max</name>
      <anchorfile>structsocket__stat__t.html</anchorfile>
      <anchor>a2d53d66644d18d6e948aafd2913b638d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>network_stat_sample_index</name>
      <anchorfile>structsocket__stat__t.html</anchorfile>
      <anchor>a5782a45124508e912999a1e5a723e07d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>interval_t</type>
      <name>network_stat_samples</name>
      <anchorfile>structsocket__stat__t.html</anchorfile>
      <anchor>a6beb517569df57774d106dab76c55f6c</anchor>
      <arglist>[10]</arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>received_T4_messages_in_current_sync_window</name>
      <anchorfile>structsocket__stat__t.html</anchorfile>
      <anchor>ac7e50f6cbdc0f9709868bca0ec83b177</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>instant_t</type>
      <name>remote_physical_clock_snapshot_T1</name>
      <anchorfile>structsocket__stat__t.html</anchorfile>
      <anchor>a337626dbaef95e87f6e611219cc445ef</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>staa_t</name>
    <filename>structstaa__t.html</filename>
    <member kind="variable">
      <type>lf_action_base_t **</type>
      <name>actions</name>
      <anchorfile>structstaa__t.html</anchorfile>
      <anchor>aff9c51bd0ac57c405643145f3a79b35a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>num_actions</name>
      <anchorfile>structstaa__t.html</anchorfile>
      <anchor>a87ab8a5a57771449baf84b2961c8a2eb</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>STAA</name>
      <anchorfile>structstaa__t.html</anchorfile>
      <anchor>a3c2010ee8b29f79a42a20ccd1ddd3041</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>tag_advance_grant_t</name>
    <filename>structtag__advance__grant__t.html</filename>
    <member kind="variable">
      <type>bool</type>
      <name>is_provisional</name>
      <anchorfile>structtag__advance__grant__t.html</anchorfile>
      <anchor>ad4947e0481e99ff55c03e36acbe75fab</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t</type>
      <name>tag</name>
      <anchorfile>structtag__advance__grant__t.html</anchorfile>
      <anchor>a613648e13671f471afa6034295bd3be6</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>tag_t</name>
    <filename>structtag__t.html</filename>
    <member kind="variable">
      <type>microstep_t</type>
      <name>microstep</name>
      <anchorfile>structtag__t.html</anchorfile>
      <anchor>a2ecfcdcf77463769a58980f47d0a813c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>instant_t</type>
      <name>time</name>
      <anchorfile>structtag__t.html</anchorfile>
      <anchor>ad6fc78329d72e52bc7e449036f64937d</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>token_template_t</name>
    <filename>structtoken__template__t.html</filename>
    <member kind="variable">
      <type>size_t</type>
      <name>length</name>
      <anchorfile>structtoken__template__t.html</anchorfile>
      <anchor>a19c2f4ac754a3fdf0419052f00d341da</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_token_t *</type>
      <name>token</name>
      <anchorfile>structtoken__template__t.html</anchorfile>
      <anchor>a076f845b64482f5cf083da333bfc60de</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>token_type_t</type>
      <name>type</name>
      <anchorfile>structtoken__template__t.html</anchorfile>
      <anchor>a7a7e76104c33f7d019670420033d5d12</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>token_type_t</name>
    <filename>structtoken__type__t.html</filename>
    <member kind="variable">
      <type>void *(*</type>
      <name>copy_constructor</name>
      <anchorfile>structtoken__type__t.html</anchorfile>
      <anchor>a4e5da2b59631b75ace560f9812a11824</anchor>
      <arglist>)(void *value)</arglist>
    </member>
    <member kind="variable">
      <type>void(*</type>
      <name>destructor</name>
      <anchorfile>structtoken__type__t.html</anchorfile>
      <anchor>a304b9805bf712b9e1745432f8bd9796c</anchor>
      <arglist>)(void *value)</arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>element_size</name>
      <anchorfile>structtoken__type__t.html</anchorfile>
      <anchor>a68cbb1b9a12c72df5dd5e7f356f50eba</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>trace_record_nodeps_t</name>
    <filename>structtrace__record__nodeps__t.html</filename>
    <member kind="variable">
      <type>int</type>
      <name>dst_id</name>
      <anchorfile>structtrace__record__nodeps__t.html</anchorfile>
      <anchor>a594dcef3f6fe657e8c1c45dbac60e821</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>event_type</name>
      <anchorfile>structtrace__record__nodeps__t.html</anchorfile>
      <anchor>a53a74b9169b6feea84279727415e947d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int64_t</type>
      <name>extra_delay</name>
      <anchorfile>structtrace__record__nodeps__t.html</anchorfile>
      <anchor>a4c492f9001246c50fbcdebcb07f60fbd</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int64_t</type>
      <name>logical_time</name>
      <anchorfile>structtrace__record__nodeps__t.html</anchorfile>
      <anchor>abda0a12d07332a37b3249ebec1127077</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int64_t</type>
      <name>microstep</name>
      <anchorfile>structtrace__record__nodeps__t.html</anchorfile>
      <anchor>ad333788b7357fdc665418c9eb424d594</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int64_t</type>
      <name>physical_time</name>
      <anchorfile>structtrace__record__nodeps__t.html</anchorfile>
      <anchor>a14a0682d99d9f5e00f6ae90a1cd80417</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void *</type>
      <name>pointer</name>
      <anchorfile>structtrace__record__nodeps__t.html</anchorfile>
      <anchor>a5b76c09109982c1c8f146c8a6d348bd7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>src_id</name>
      <anchorfile>structtrace__record__nodeps__t.html</anchorfile>
      <anchor>a13a0922284065e2870515c920ad13bbc</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void *</type>
      <name>trigger</name>
      <anchorfile>structtrace__record__nodeps__t.html</anchorfile>
      <anchor>abee05773cd25de7c1025390d397253a9</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>trace_record_t</name>
    <filename>structtrace__record__t.html</filename>
    <member kind="variable">
      <type>int</type>
      <name>dst_id</name>
      <anchorfile>structtrace__record__t.html</anchorfile>
      <anchor>a5a8dc061d005024c8e16b97c5e1aeb2d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>trace_event_t</type>
      <name>event_type</name>
      <anchorfile>structtrace__record__t.html</anchorfile>
      <anchor>ae9e3d3538bf7109339a117b85cf28738</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>interval_t</type>
      <name>extra_delay</name>
      <anchorfile>structtrace__record__t.html</anchorfile>
      <anchor>aecd3b41b7fef7c78bdd0da4e19ce7048</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>instant_t</type>
      <name>logical_time</name>
      <anchorfile>structtrace__record__t.html</anchorfile>
      <anchor>a7707b8a45e0e5de32ded6b0f235d3417</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>microstep_t</type>
      <name>microstep</name>
      <anchorfile>structtrace__record__t.html</anchorfile>
      <anchor>adb452f2dd8896678e759933c63fb718e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>instant_t</type>
      <name>physical_time</name>
      <anchorfile>structtrace__record__t.html</anchorfile>
      <anchor>afb3aaa3d422442ca97f25df32d403fd1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void *</type>
      <name>pointer</name>
      <anchorfile>structtrace__record__t.html</anchorfile>
      <anchor>a866ea8e5ccf2bb992723bc74d6e5ee9a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>src_id</name>
      <anchorfile>structtrace__record__t.html</anchorfile>
      <anchor>a886535c90ca00f6587c509baa01ebae0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>trigger_t *</type>
      <name>trigger</name>
      <anchorfile>structtrace__record__t.html</anchorfile>
      <anchor>adc712673588c62b3a2c5e9970ba953fd</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>trigger_t</name>
    <filename>structtrigger__t.html</filename>
    <member kind="variable">
      <type>tag_t</type>
      <name>intended_tag</name>
      <anchorfile>structtrigger__t.html</anchorfile>
      <anchor>a05debab7774fc477462beb9dd0474bf5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>is_physical</name>
      <anchorfile>structtrigger__t.html</anchorfile>
      <anchor>aec6676110ebe0f82293e2f916312e4a0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>is_timer</name>
      <anchorfile>structtrigger__t.html</anchorfile>
      <anchor>a6beec6375e3259c9ed1aa006074bd315</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t</type>
      <name>last_known_status_tag</name>
      <anchorfile>structtrigger__t.html</anchorfile>
      <anchor>a4da5aaedea429e79f07fbc6137358d62</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>tag_t</type>
      <name>last_tag</name>
      <anchorfile>structtrigger__t.html</anchorfile>
      <anchor>a60626b318fd14e908f12e065af616880</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>reactor_mode_t *</type>
      <name>mode</name>
      <anchorfile>structtrigger__t.html</anchorfile>
      <anchor>ab6427e404b91c483332fc1a1722e8cc7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>number_of_reactions</name>
      <anchorfile>structtrigger__t.html</anchorfile>
      <anchor>a53bebeaf8708d9dbe859df99972fb667</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>interval_t</type>
      <name>offset</name>
      <anchorfile>structtrigger__t.html</anchorfile>
      <anchor>afb30417114ed72187e37528523ca65c6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>interval_t</type>
      <name>period</name>
      <anchorfile>structtrigger__t.html</anchorfile>
      <anchor>a0bffe925390579e456b79410a4bff2f5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>instant_t</type>
      <name>physical_time_of_arrival</name>
      <anchorfile>structtrigger__t.html</anchorfile>
      <anchor>af05242e899dc112b01e2bfa80625e88f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_spacing_policy_t</type>
      <name>policy</name>
      <anchorfile>structtrigger__t.html</anchorfile>
      <anchor>ab6c4b1ca7821838840fa9167a7f14571</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>reaction_t **</type>
      <name>reactions</name>
      <anchorfile>structtrigger__t.html</anchorfile>
      <anchor>a3674777164e73e63388cdfe6c5d876a3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>port_status_t</type>
      <name>status</name>
      <anchorfile>structtrigger__t.html</anchorfile>
      <anchor>aebe251fea18aa0bf9435acf5d5ab6012</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>token_template_t</type>
      <name>tmplt</name>
      <anchorfile>structtrigger__t.html</anchorfile>
      <anchor>a83a162f60d2b51505348c20650943f38</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>vector_t</name>
    <filename>structvector__t.html</filename>
    <member kind="variable">
      <type>void **</type>
      <name>end</name>
      <anchorfile>structvector__t.html</anchorfile>
      <anchor>addfa7386ac98017635ffd05f4c11b5a4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void **</type>
      <name>next</name>
      <anchorfile>structvector__t.html</anchorfile>
      <anchor>a5fa7e662c81935085f83c4d248c53eea</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void **</type>
      <name>start</name>
      <anchorfile>structvector__t.html</anchorfile>
      <anchor>acf8d5fbfd9ac917ccbd54a7ea0169eaf</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>votes</name>
      <anchorfile>structvector__t.html</anchorfile>
      <anchor>a2dd65528257b283111eadc0500b93464</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>votes_required</name>
      <anchorfile>structvector__t.html</anchorfile>
      <anchor>a4b68f14d631f8f4a8d16bd1aa36b3011</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>watchdog_t</name>
    <filename>structwatchdog__t.html</filename>
    <member kind="variable">
      <type>bool</type>
      <name>active</name>
      <anchorfile>structwatchdog__t.html</anchorfile>
      <anchor>a340c92684e32d85df6e2081f3abd9155</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct self_base_t *</type>
      <name>base</name>
      <anchorfile>structwatchdog__t.html</anchorfile>
      <anchor>a1077fbd7508d445a526ab060f637bfcd</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_cond_t</type>
      <name>cond</name>
      <anchorfile>structwatchdog__t.html</anchorfile>
      <anchor>a23d65cf0c4a3e97ec1168d6805a4aa00</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>instant_t</type>
      <name>expiration</name>
      <anchorfile>structwatchdog__t.html</anchorfile>
      <anchor>a7cbcda4103cfddef8a9a99aa1f2d4f3e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>interval_t</type>
      <name>min_expiration</name>
      <anchorfile>structwatchdog__t.html</anchorfile>
      <anchor>a9e2ad93a70be8954567bed43664b8a35</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>terminate</name>
      <anchorfile>structwatchdog__t.html</anchorfile>
      <anchor>af503486d68aae7eb02ee348a5254b0fd</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_thread_t</type>
      <name>thread_id</name>
      <anchorfile>structwatchdog__t.html</anchorfile>
      <anchor>a7b5873e876b6afdfebcca38cd2e6b56c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>trigger_t *</type>
      <name>trigger</name>
      <anchorfile>structwatchdog__t.html</anchorfile>
      <anchor>aa1f3c34d01395ed2671f93995e2947a1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>watchdog_function_t</type>
      <name>watchdog_function</name>
      <anchorfile>structwatchdog__t.html</anchorfile>
      <anchor>a1f8645aac940f26a359516be2339ac7d</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>API</name>
    <title>API for Reactions</title>
    <filename>group__API.html</filename>
    <file>logging.h</file>
    <file>port.h</file>
    <file>reaction_macros.h</file>
    <file>reactor.h</file>
    <file>schedule.h</file>
    <file>tag.h</file>
    <class kind="struct">lf_multiport_iterator_t</class>
    <class kind="struct">lf_token_t</class>
    <class kind="struct">tag_t</class>
    <member kind="define">
      <type>#define</type>
      <name>CHECK_TIMEOUT</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaabb3bc387f7a50f8cc57319f82c17c31</anchor>
      <arglist>(start, duration)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_ASSERT</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga26ab6a0fd21cdcff11a5557406536bf1</anchor>
      <arglist>(condition, format,...)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_ASSERT_NON_NULL</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga832d0deaa853b7777e1f54283e7bcc20</anchor>
      <arglist>(pointer)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_ASSERTN</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga1c464cee8cabb65eebf454fc016d47b1</anchor>
      <arglist>(condition, format,...)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_multiport_iterator</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga5759266c62b989a6b305584cb72f8840</anchor>
      <arglist>(in)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_PRINT_DEBUG</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gab5a65df50027549a8245f6b3eaff97e4</anchor>
      <arglist>(format,...)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_PRINT_LOG</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga2a7110df48e8f74b05fd4a8f7581b1da</anchor>
      <arglist>(format,...)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_reactor_full_name</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga78918b87982fba15b59f35a8f926b021</anchor>
      <arglist>(reactor)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_reactor_name</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga42c6f935901d6fc56d6e82be619a8bd3</anchor>
      <arglist>(reactor)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_set</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaef602f51d34bbd214643161e425d909d</anchor>
      <arglist>(out, val)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_set_array</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga8a3a63e70ec63e35d46573293ecec905</anchor>
      <arglist>(out, val, len)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_set_copy_constructor</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaa515ab9df816c6ac8a450def4dc02f40</anchor>
      <arglist>(out, cpy_ctor)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_set_destructor</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaf4b2874af3da2bb85edfb3f0a57028a1</anchor>
      <arglist>(out, dtor)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_set_mode</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga0120cc579143c138482e89186e180ebc</anchor>
      <arglist>(mode)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_set_present</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga5c1e2963a361057f0b249b95f40a8f8d</anchor>
      <arglist>(out)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_set_token</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga1aa76760517d7100306d59b92fd41a26</anchor>
      <arglist>(out, newtoken)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_SPARSE_CAPACITY_DIVIDER</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga3e66ec583172bbad678982af8c57001b</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_SPARSE_WIDTH_THRESHOLD</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaafcc3f0b909a44166db182035ca759c6</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_tag</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga2d81456725407157f9dc521a5e14a679</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_TEST</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga9af561423d4c16fd397d48ab37edfcff</anchor>
      <arglist>(condition, format,...)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_TIME_BUFFER_LENGTH</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gacc2b8ac5ac3020137e71dfcdbaedf335</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_time_logical</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga4fe4453dda4223671dc90fa1ecbcac85</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_time_logical_elapsed</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga7aaaed76bc1ae823bb13d6603807f874</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LOG_LEVEL_ALL</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga18226173309d6c2ae828080dad0859cf</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LOG_LEVEL_DEBUG</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga130224df8c6bf22a688e3cb74a45689a</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LOG_LEVEL_ERROR</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga742fc70e331d7e568bd893c514756a29</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LOG_LEVEL_INFO</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga2e25fe130cf710da4ad800747fdd51f3</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LOG_LEVEL_LOG</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga8b58cabecd61bfd1b706be9cb992e0bd</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LOG_LEVEL_WARNING</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaf539a66abed2a7a15e3443d70a3cf1e1</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>int64_t</type>
      <name>instant_t</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga827080fd3c574bad5a32db9f7c367587</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>int64_t</type>
      <name>interval_t</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaf5b4e62d03782997d813be6145316f4b</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct lf_multiport_iterator_t</type>
      <name>lf_multiport_iterator_t</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaf8a81a7373f3d5f77fd865e437964ef3</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct lf_token_t</type>
      <name>lf_token_t</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga973e404c4c1bd798a54501d0e1d640f9</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>uint32_t</type>
      <name>microstep_t</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gad88f1caa8b9c216404eb196cb1850213</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>void</type>
      <name>print_message_function_t</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gadc4e09bf2433b9e06b632880ce81b897</anchor>
      <arglist>(const char *, va_list)</arglist>
    </member>
    <member kind="typedef">
      <type>char *</type>
      <name>string</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga4505c08c065b48840a30eedd9845cce2</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>lf_check_deadline</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gab3a04dd0a1581844829b28686b6b3c53</anchor>
      <arglist>(void *self, bool invoke_deadline_handler)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>lf_comma_separated_time</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gae956f1688a3893b44cf4bced3c13ef9a</anchor>
      <arglist>(char *buffer, instant_t time)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>lf_delay_tag</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaf7feb89416e60e458d6a86cadadbc41d</anchor>
      <arglist>(tag_t tag, interval_t interval)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>lf_is_tag_after_stop_tag</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaf4bdd144e6cc65dfa3b996d4bd82f83a</anchor>
      <arglist>(environment_t *env, tag_t tag)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_multiport_next</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gac7c743d3c64a839642e54781b8e9127f</anchor>
      <arglist>(lf_multiport_iterator_t *iterator)</arglist>
    </member>
    <member kind="function">
      <type>lf_token_t *</type>
      <name>lf_new_token</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaa47b51a11727eec252ff7e786794bd88</anchor>
      <arglist>(void *port_or_action, void *val, size_t len)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_print</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga9d83e586b29a3316dd7dc505e30e6858</anchor>
      <arglist>(const char *format,...) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void void void</type>
      <name>lf_print_debug</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga22ebee89c962ac34cc1fa7b9762b77d2</anchor>
      <arglist>(const char *format,...) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void void void void</type>
      <name>lf_print_error</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gae0a6bd6b164c5cc3e9928f5375dd1a97</anchor>
      <arglist>(const char *format,...) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void void void void void void</type>
      <name>lf_print_error_and_exit</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gabf2630e80adfec45b8b4a2782a0767a7</anchor>
      <arglist>(const char *format,...) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void void void void void void void</type>
      <name>lf_print_error_system_failure</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gacbc9741875a95bb30e1a8d68cfb7cf06</anchor>
      <arglist>(const char *format,...)</arglist>
    </member>
    <member kind="function">
      <type>void void</type>
      <name>lf_print_log</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gab202dc9383567eaa8d9ae7240c939c19</anchor>
      <arglist>(const char *format,...) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void void void void void</type>
      <name>lf_print_warning</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga613bc8d331e3fd9a3f78eb1600092e1d</anchor>
      <arglist>(const char *format,...) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>lf_reactor_full_name</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gab5bf112c4237da16d08736ff3e3e36b8</anchor>
      <arglist>(self_base_t *self)</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>lf_reactor_name</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gae76171d83c29dadd3adb3f9294a92138</anchor>
      <arglist>(self_base_t *self)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>lf_readable_time</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga43509483b3e9886acd002c7d7b6482c8</anchor>
      <arglist>(char *buffer, instant_t time)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_register_print_function</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga6ad22ca79136cd15dc0d560aec067f76</anchor>
      <arglist>(print_message_function_t *function, int log_level)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_request_stop</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gab49affc958f705d9e33c5e3463848bda</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>lf_schedule</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga6778eef97447cf0ba1f0afa8ba3a8dca</anchor>
      <arglist>(void *action, interval_t offset)</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>lf_schedule_copy</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga747594e2d7264ae8b044a095eb92ba27</anchor>
      <arglist>(void *action, interval_t offset, void *value, size_t length)</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>lf_schedule_int</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga073ea4406a084a24e71b65936ba39e36</anchor>
      <arglist>(void *action, interval_t extra_delay, int value)</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>lf_schedule_token</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gabe3fd30bf6a2689fdb3ec03b4e2f47d1</anchor>
      <arglist>(void *action, interval_t extra_delay, lf_token_t *token)</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>lf_schedule_value</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga986bb1be3a9e4f71e5b5dde30d9dc6ad</anchor>
      <arglist>(void *action, interval_t extra_delay, void *value, int length)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_set_present</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gadaa6f5f1a265e7a37aeb3b6a0d101732</anchor>
      <arglist>(lf_port_base_t *port)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>lf_tag</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gacbe5117469b98e0e6df45b5421f58026</anchor>
      <arglist>(void *env)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>lf_tag_add</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaa1186f7e330ecbfd20a4dd90b97439e6</anchor>
      <arglist>(tag_t a, tag_t b)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_tag_compare</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga596d8734432616c9c7847283fde63cfa</anchor>
      <arglist>(tag_t tag1, tag_t tag2)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>lf_tag_max</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga4c39037bf099ff2c31a71fe96ac59a61</anchor>
      <arglist>(tag_t tag1, tag_t tag2)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>lf_tag_min</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga80abd9981c0375a0abbe47591204f18b</anchor>
      <arglist>(tag_t tag1, tag_t tag2)</arglist>
    </member>
    <member kind="function">
      <type>instant_t</type>
      <name>lf_time_add</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga1789de56286aa33c086b194b544c2d91</anchor>
      <arglist>(instant_t a, interval_t b)</arglist>
    </member>
    <member kind="function">
      <type>instant_t</type>
      <name>lf_time_physical</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga7538766a655ba2e60ddde55f2e020e58</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>instant_t</type>
      <name>lf_time_physical_elapsed</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga98468f1c5132e3aa18d77f85d65bb6ec</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>instant_t</type>
      <name>lf_time_start</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga8da2172c41ab13ff4748994a62ae34b5</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>instant_t</type>
      <name>lf_time_subtract</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga9107f7f62874fa972f7a31f9c277b8fc</anchor>
      <arglist>(instant_t a, interval_t b)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_watchdog_start</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga82bf2c7bd91fdf03b357914cf875dbb9</anchor>
      <arglist>(watchdog_t *watchdog, interval_t additional_timeout)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_watchdog_stop</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaa27ad22f94bbdaa33b99fe6cd81f1bdc</anchor>
      <arglist>(watchdog_t *watchdog)</arglist>
    </member>
    <member kind="function">
      <type>lf_token_t *</type>
      <name>lf_writable_copy</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gaaf8e6f18b021d0b8ece7e1b64280432f</anchor>
      <arglist>(lf_port_base_t *port)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>register_user_trace_event</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gae6078a659280c57ae1c4dfe939f319e7</anchor>
      <arglist>(void *self, char *description)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tracepoint_user_event</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>gacd97e049ba442bdd90f020c0b7c1a4fa</anchor>
      <arglist>(void *self, char *description)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tracepoint_user_value</name>
      <anchorfile>group__API.html</anchorfile>
      <anchor>ga064170081cf3de9c123b58bdc51b1d4f</anchor>
      <arglist>(void *self, char *description, long long value)</arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>Constants</name>
    <title>Constants</title>
    <filename>group__Constants.html</filename>
    <member kind="define">
      <type>#define</type>
      <name>BILLION</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga31f99d9c502b52b5f36dc7e2028c2e80</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DAY</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga8393c538440a761b4439623e536a7c91</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DAYS</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga987a486d1ad1ff15346b2395847280ab</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FOREVER</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga75c828ed6c02fcd44084e67a032e422c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FOREVER_MICROSTEP</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga43635840119d1af39afff67eb2585248</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FOREVER_TAG</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga183109e082a793ae85cec00d72e70d4b</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FOREVER_TAG_INITIALIZER</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga33ccb2b8fb9f20ab29ea298b9051443a</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>HOUR</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga3cf238d5a1a842131a5adf916d571d12</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>HOURS</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga2249e1bedc7aeaad64ed3fefdd6e7951</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MINUTE</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga72e3dbcf6a85e8c0a04691cfb14d3876</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MINUTES</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga694878332b974b464c7d58c7114ee6e9</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSEC</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga673a8ebab8ec621a1cf731871dd170c8</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSECS</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>gad3edea62ad14babaf45f6d3020cb7674</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NEVER</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga9e5c19968af97bd64f46650ec8732318</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NEVER_MICROSTEP</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga934f060560ba6d2d7c1ad336439e76e8</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NEVER_TAG</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>gafc2f39e1818a6a8b68ca46e7c9dc2704</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NEVER_TAG_INITIALIZER</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga0d876ab54e766798e559c4f47340b359</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NSEC</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga8c561c4985495c722ac86b55b3d9bbb3</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NSECS</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>gaea80ae7708f69d05cd44126f08a08cf3</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SEC</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga30e47c4dd44d168c2982d3ec1d4e0825</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SECOND</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>gaf7dae730408c43d8c820ab73e580865a</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SECONDS</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga2874e6caf9a4e0452441e9edeca046dc</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SECS</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga8d1ccba996ebc6fc86c6c3c52051329d</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>USEC</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>gaa2aeaab0c2033d1db412c8021bff93fc</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>USECS</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>gaa0c5b5c56bdb5016516284c87eac86a9</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>WEEK</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>gac38fd1109df8cd9f53ab99761b3efa04</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>WEEKS</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>ga6377f87d75908e0d79a4005db7af3b35</anchor>
      <arglist>(t)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ZERO_TAG</name>
      <anchorfile>group__Constants.html</anchorfile>
      <anchor>gacb18b22c88c7505b59347c3a9c52f53d</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>Utilities</name>
    <title>Utilities</title>
    <filename>group__Utilities.html</filename>
    <file>audio_loop.h</file>
    <file>deque.h</file>
    <file>generics.h</file>
    <file>hashmap.h</file>
    <file>hashset.h</file>
    <file>hashset_itr.h</file>
    <file>pointer_hashmap.h</file>
    <file>sensor_simulator.h</file>
    <file>type_converter.h</file>
    <file>wave_file_reader.h</file>
    <class kind="struct">deque_t</class>
    <class kind="struct">hashmap_entry_t</class>
    <class kind="struct">hashmap_t</class>
    <class kind="struct">hashset_itr_st</class>
    <class kind="struct">hashset_st</class>
    <class kind="struct">lf_waveform_t</class>
    <class kind="struct">vector_t</class>
    <member kind="define">
      <type>#define</type>
      <name>AUDIO_BUFFER_SIZE</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga1d48237cb63c5ae67aab6d00cc64afb6</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>BUFFER_DURATION_NS</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gaaee12bd1b49481f758a5a3cf1876268c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DO_CONVERT</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga9cbcaad341a5b5a807196f116619b9c0</anchor>
      <arglist>(fromType, toType, value)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_decay</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga9505134796fc957eb9fedf172cab3527</anchor>
      <arglist>(p)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_get_pointer</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gafe05484f1ece6fa30ba7b3bcf33f03b9</anchor>
      <arglist>(p)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_is_pointer</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga9edb07d852ce1b4e090b36f8683c8017</anchor>
      <arglist>(p)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_is_pointer_or_array</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga271f353ddab44fb05a4b0f8627904fbe</anchor>
      <arglist>(p)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_is_same</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga785378b93bab907d10fee21425a292e5</anchor>
      <arglist>(typename, b)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_is_same_type</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga886f2df25fcc1b16e512d6143e910b6f</anchor>
      <arglist>(a, b)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_is_type_equal</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga7d7827494787f07637043eb90e285d44</anchor>
      <arglist>(typename_a, typename_b)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>lf_to_left_type</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga99938402e4e9202fe51e67f6e916f0f5</anchor>
      <arglist>(left, right)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MAX_AMPLITUDE</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga141b139da77580918b0f9821ab3dbb99</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NUM_CHANNELS</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gae5597ce31d23d11493e6e674fe257d73</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NUM_NOTES</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga5b0b677cb9527865430a9b3d7a71cb03</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SAMPLE_RATE</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga4b76a0c2859cfd819a343a780070ee2b</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct deque_t</type>
      <name>deque_t</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gafc1887f5ebd7da3ce7dfc73beb195598</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct hashmap_entry_t</type>
      <name>hashmap_entry_t</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga1044f93547018a97e5d3b50cac8a1516</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct hashmap_t</type>
      <name>hashmap_t</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga19e5de2971328f1de610c3a3f42d295f</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct lf_waveform_t</type>
      <name>lf_waveform_t</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga464bd5c31a1a562536e2a54213135604</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct vector_t</type>
      <name>vector_t</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gac6a5b15223a2905669f2ee7377fd3dbd</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>add_to_sound</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gae0d2efe97b2aac751245705e32d2c927</anchor>
      <arglist>(int index_offset, double value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>deque_initialize</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga0e4768355c448b3b0489d851271515bc</anchor>
      <arglist>(deque_t *d)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>deque_is_empty</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gac22527818b92ea67851c2fd4a2790cb7</anchor>
      <arglist>(deque_t *d)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>deque_peek_back</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga3530b26ded106a84ef8571fab0e73d9e</anchor>
      <arglist>(deque_t *d)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>deque_peek_front</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga4b9e8a9de89f124d0cdc1199debb70be</anchor>
      <arglist>(deque_t *d)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>deque_pop_back</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga841ec80a70a2e63f08aab990e528e1f9</anchor>
      <arglist>(deque_t *d)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>deque_pop_front</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga4acd616a7c971b1c137010c64801d746</anchor>
      <arglist>(deque_t *d)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>deque_push_back</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gac1ffd77da7496612086ea430f7d29563</anchor>
      <arglist>(deque_t *d, void *value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>deque_push_front</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga19a0d218efcb14cc2c3d7b009a5d0f16</anchor>
      <arglist>(deque_t *d, void *value)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>deque_size</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gad7e6969a2e1a7e7529575558103d512d</anchor>
      <arglist>(deque_t *d)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>end_sensor_simulator</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga562a694d5b2ed8c935aa08d78c488114</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>hashmap_free</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gafa5597a9582cc604eb0da7aac536ce4c</anchor>
      <arglist>(hashmap_t *hashmap)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>hashmap_get</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga510dfb7539122a20f4f0e299a2d33c88</anchor>
      <arglist>(hashmap_t *hashmap, void *key)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static hashmap_entry_t *</type>
      <name>hashmap_get_actual_address</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga67344ac16c34de2b935e8ff5f343cf6c</anchor>
      <arglist>(hashmap_t *hashmap, void *key)</arglist>
    </member>
    <member kind="function">
      <type>hashmap_t *</type>
      <name>hashmap_new</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gac0b6f823784483f653698db3371a7036</anchor>
      <arglist>(size_t capacity, void *nothing)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>hashmap_put</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga47e0b1518c46a6cf0ea799c49490581e</anchor>
      <arglist>(hashmap_t *hashmap, void *key, void *value)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>hashset_add</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga31e95da7ee1f76c30b9fec773d9b380c</anchor>
      <arglist>(hashset_t set, void *item)</arglist>
    </member>
    <member kind="function">
      <type>hashset_t</type>
      <name>hashset_create</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga2c94a06d31c15e8da48b04b3de1a9113</anchor>
      <arglist>(unsigned short nbits)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>hashset_destroy</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga6e280b06a1572145d3211e36e47eea6e</anchor>
      <arglist>(hashset_t set)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>hashset_is_member</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga55de036dbc978294c262a7751f4caa81</anchor>
      <arglist>(hashset_t set, void *item)</arglist>
    </member>
    <member kind="function">
      <type>hashset_itr_t</type>
      <name>hashset_iterator</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga5350d2cd6a1be22846c5541816fb857f</anchor>
      <arglist>(hashset_t set)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>hashset_iterator_has_next</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga50a2126c05927f7d11b0859af5a38f02</anchor>
      <arglist>(hashset_itr_t itr)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>hashset_iterator_next</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga1f8e715379e1c95db280451114056bd8</anchor>
      <arglist>(hashset_itr_t itr)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>hashset_iterator_value</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga08d3680504ca63e10abbf03b115acb1b</anchor>
      <arglist>(hashset_itr_t itr)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>hashset_num_items</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga8d4800d73d1a58ad953501d9035de5ec</anchor>
      <arglist>(hashset_t set)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>hashset_remove</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga5b0ad513d6e64cd754213b0103a094e0</anchor>
      <arglist>(hashset_t set, void *item)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_play_audio_waveform</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga20d5fc7b37764eb8c3a4e917e93ad7d7</anchor>
      <arglist>(lf_waveform_t *waveform, float emphasis, instant_t start_time)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_start_audio_loop</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga7e03b277fd2f2b3ae6aa029e5256da3e</anchor>
      <arglist>(instant_t start_time)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_stop_audio_loop</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga65266402bd1ede8be91b6a0a5a34f767</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>lf_waveform_t *</type>
      <name>read_wave_file</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga149d009d65a3c20d73bcf2ae7c9d6814</anchor>
      <arglist>(const char *path)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>register_sensor_key</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga196614841046dea5b94b2a60108fff40</anchor>
      <arglist>(char key, void *action)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>show_tick</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga3c9ec0901afe8264c0e75d40c940bd5a</anchor>
      <arglist>(const char *character)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>start_sensor_simulator</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga491b52110965df05da5f18cb820ae99f</anchor>
      <arglist>(const char *message_lines[], int number_of_lines, int tick_window_width, char *log_file, int log_level)</arglist>
    </member>
    <member kind="function">
      <type>void **</type>
      <name>vector_at</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga201cb1fd5299e01b6fdfb499d3008952</anchor>
      <arglist>(vector_t *v, size_t idx)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>vector_free</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gab6ea681ea89fa128392d61ec7a516e31</anchor>
      <arglist>(vector_t *v)</arglist>
    </member>
    <member kind="function">
      <type>vector_t</type>
      <name>vector_new</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga640489760dfb72c2001de6ec560fb75f</anchor>
      <arglist>(size_t initial_capacity)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>vector_pop</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga85cdea38a35554168aa2277d83f5a957</anchor>
      <arglist>(vector_t *v)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>vector_push</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga85a9501c4a715501dc0adeb04bd84dcb</anchor>
      <arglist>(vector_t *v, void *element)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>vector_pushall</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gab5ad2a2c71548435b6072b31ac21a9c2</anchor>
      <arglist>(vector_t *v, void **array, size_t size)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>vector_size</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>gaaa8f4318bf03a7886169e85c151b6903</anchor>
      <arglist>(vector_t *v)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>vector_vote</name>
      <anchorfile>group__Utilities.html</anchorfile>
      <anchor>ga41e7b0b4a43deefd94df37fd128de0bb</anchor>
      <arglist>(vector_t *v)</arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>Platform</name>
    <title>Platform API</title>
    <filename>group__Platform.html</filename>
    <file>clock.h</file>
    <file>low_level_platform.h</file>
    <file>platform.h</file>
    <class kind="struct">lf_scheduling_policy_t</class>
    <member kind="typedef">
      <type>void *</type>
      <name>lf_platform_mutex_ptr_t</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga8cb87ba531decc7a525fe20e8586e300</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>lf_scheduling_policy_type_t</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gadc74ec49eb5cc6eceda1447090d61ab6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>LF_SCHED_FAIR</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ggadc74ec49eb5cc6eceda1447090d61ab6aad0c30324e2299f1ab579a9db51ae994</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>LF_SCHED_TIMESLICE</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ggadc74ec49eb5cc6eceda1447090d61ab6aa94c07b6d2e7cf9564d407bdb0d5eb3f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>LF_SCHED_PRIORITY</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ggadc74ec49eb5cc6eceda1447090d61ab6a31d917200c3ceb3735770c9acef3eb5e</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>_lf_clock_gettime</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga843c12cfbc698883e96a0bfe23882a9e</anchor>
      <arglist>(instant_t *t)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>_lf_cond_timedwait</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga0ed454c4a1b6eee6debe92efa433fb37</anchor>
      <arglist>(lf_cond_t *cond, instant_t wakeup_time)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_initialize_clock</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gaf5c7b68f137e69c4ad3734a42bcd0448</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>_lf_interruptable_sleep_until_locked</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga0b22a974cdd69f184735503b72597698</anchor>
      <arglist>(environment_t *env, instant_t wakeup_time)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>initialize_lf_thread_id</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga32bb18b9d9ab98e40bb5bcb57dec682d</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_available_cores</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gae138038b96821ccfbb2d4f2ec4c364c1</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_clock_cond_timedwait</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga1d6ed42d060c926d05db76544172fed8</anchor>
      <arglist>(lf_cond_t *cond, instant_t wakeup_time)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_clock_gettime</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gaad3e8b04cd7bf1132b774940602a72d3</anchor>
      <arglist>(instant_t *now)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_clock_interruptable_sleep_until_locked</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga341e2d260240fd35d29ada9531aa9ead</anchor>
      <arglist>(environment_t *env, instant_t wakeup_time)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_cond_broadcast</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga9ed434626733537f71c9b85e981109c7</anchor>
      <arglist>(lf_cond_t *cond)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_cond_init</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gab32dc7869dd4cf48bda70663f2591ae1</anchor>
      <arglist>(lf_cond_t *cond, lf_mutex_t *mutex)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_cond_signal</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga7267ad6679b93c9a9f321cdf864e0092</anchor>
      <arglist>(lf_cond_t *cond)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_cond_wait</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gaba6871b2088bcd86f973e0299cdd4ff8</anchor>
      <arglist>(lf_cond_t *cond)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_critical_section_enter</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gae0fda178667bc6cd94890a13316c285c</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_critical_section_exit</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gaed2c25495b50b46780c6288e4370541e</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_mutex_init</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gaa6f228487e6af38e496882f406aafaf6</anchor>
      <arglist>(lf_mutex_t *mutex)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_mutex_lock</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gac4c0721974b31d98f491be1febeb2c9a</anchor>
      <arglist>(lf_mutex_t *mutex)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_mutex_unlock</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga01c4d5070c8402d4713a3fcab5a46a9f</anchor>
      <arglist>(lf_mutex_t *mutex)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_notify_of_event</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga3ab5c60e67d36e2b26eccb884ed9f668</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_platform_mutex_free</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga086b854690734be2e49391d6ca65e8ec</anchor>
      <arglist>(lf_platform_mutex_ptr_t mutex)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_platform_mutex_lock</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gaa548193b346ebc70b6e6e3a5e87d5e2d</anchor>
      <arglist>(lf_platform_mutex_ptr_t mutex)</arglist>
    </member>
    <member kind="function">
      <type>lf_platform_mutex_ptr_t</type>
      <name>lf_platform_mutex_new</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga262864ffd60ec9491d6e6b278c58910c</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_platform_mutex_unlock</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga0911d4e67c0a632343e7404115c88ca9</anchor>
      <arglist>(lf_platform_mutex_ptr_t mutex)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_sleep</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga9a43894d4caf7e2fc1e75b9b49d7285d</anchor>
      <arglist>(interval_t sleep_duration)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_thread_create</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga48fa558d833200b986c03bc8c12bbe77</anchor>
      <arglist>(lf_thread_t *thread, void *(*lf_thread)(void *), void *arguments)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_thread_id</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gafe7e0ce54cfd40a754554105ad214e9b</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_thread_join</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga652d8db6bafa59434d297216607266ed</anchor>
      <arglist>(lf_thread_t thread, void **thread_return)</arglist>
    </member>
    <member kind="function">
      <type>lf_thread_t</type>
      <name>lf_thread_self</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gaa295ab0ba8fa58516cb45f57b62d09e1</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_thread_set_cpu</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gaddc34b525849f46d172fbafa894dc474</anchor>
      <arglist>(lf_thread_t thread, size_t cpu_number)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_thread_set_priority</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>ga173ebe6288d9a800df16cbd400247578</anchor>
      <arglist>(lf_thread_t thread, int priority)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_thread_set_scheduling_policy</name>
      <anchorfile>group__Platform.html</anchorfile>
      <anchor>gadc1acd7c6e9bd4f7bd333ad7d3e7b5d1</anchor>
      <arglist>(lf_thread_t thread, lf_scheduling_policy_t *policy)</arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>Internal</name>
    <title>Internal</title>
    <filename>group__Internal.html</filename>
    <file>environment.h</file>
    <file>lf_semaphore.h</file>
    <file>lf_token.h</file>
    <file>lf_types.h</file>
    <file>logging_macros.h</file>
    <file>mixed_radix.h</file>
    <file>pqueue.h</file>
    <file>pqueue_base.h</file>
    <file>pqueue_tag.h</file>
    <file>reactor_common.h</file>
    <file>reactor_threaded.h</file>
    <file>scheduler.h</file>
    <file>scheduler_instance.h</file>
    <file>scheduler_sync_tag_advance.h</file>
    <file>tracepoint.h</file>
    <file>util.h</file>
    <file>watchdog.h</file>
    <class kind="struct">allocation_record_t</class>
    <class kind="struct">environment_t</class>
    <class kind="struct">event_t</class>
    <class kind="struct">lf_action_base_t</class>
    <class kind="struct">lf_port_base_t</class>
    <class kind="struct">lf_scheduler_t</class>
    <class kind="struct">lf_semaphore_t</class>
    <class kind="struct">lf_sparse_io_record_t</class>
    <class kind="struct">lf_tag_advancement_barrier_t</class>
    <class kind="struct">mixed_radix_int_t</class>
    <class kind="struct">pqueue_t</class>
    <class kind="struct">reaction_t</class>
    <class kind="struct">sched_params_t</class>
    <class kind="struct">self_base_t</class>
    <class kind="struct">token_template_t</class>
    <class kind="struct">token_type_t</class>
    <class kind="struct">trace_record_t</class>
    <class kind="struct">trigger_t</class>
    <class kind="struct">watchdog_t</class>
    <member kind="define">
      <type>#define</type>
      <name>CONCATENATE_THREE_STRINGS</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga8d8698026252ae104cc2405d8bb13f0e</anchor>
      <arglist>(__string1, __string2, __string3)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>GLOBAL_ENVIRONMENT</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gafaff13b938d14da158c3fa1424358353</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_COND_BROADCAST</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga8ffb41cff660cdf632693c5bf5a17f52</anchor>
      <arglist>(cond)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_COND_INIT</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga47d73a5ec6fa7ebc7838312cb93c2bb8</anchor>
      <arglist>(cond, mutex)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_COND_SIGNAL</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gabd8bd827c1d0d4b9f108da9098e10e51</anchor>
      <arglist>(cond)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_COND_WAIT</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga8b24bfd4605a8726dbfc2cee30c27e08</anchor>
      <arglist>(cond)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_CRITICAL_SECTION_ENTER</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga912847660fb8b04317fc270125d6b1f3</anchor>
      <arglist>(env)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_CRITICAL_SECTION_EXIT</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga687e881099481d8efe446ad8a17d72e5</anchor>
      <arglist>(env)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_LEVEL</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gad5a08658dc3e13eab4cddafd94734794</anchor>
      <arglist>(index)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_MAX</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaac9240f79bd758e00ed7bbf75dafc4fa</anchor>
      <arglist>(X, Y)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_MIN</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga4359466e7fdb68dcf8116c469946cd92</anchor>
      <arglist>(X, Y)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_MUTEX_INIT</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gadab0b8f13f8462ec0eddc7257ddb5394</anchor>
      <arglist>(mutex)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_MUTEX_LOCK</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gab907d4c8d53c26fdbcbaa8d02e6a8810</anchor>
      <arglist>(mutex)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_MUTEX_UNLOCK</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga2bb9c8d2b589a6eb4f72f6750a1133fc</anchor>
      <arglist>(mutex)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MIN_SLEEP_DURATION</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga3b755b6f58cb9ea64ae2f1ba9a382c86</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SCHED_ADAPTIVE</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga7516169f705d99222725e6970f0ec703</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SCHED_GEDF_NP</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga0238f536f81a61c0d568b36eac9b9a00</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SCHED_NP</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gacc410134875d15b02634fb0aa8163a00</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>tracepoint_reaction_deadline_missed</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaaf1dece34c4fcc135c2bd4feaf44e095</anchor>
      <arglist>(env, reaction, worker)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>tracepoint_reaction_ends</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaebbdf64d4b017a879c69fcda11e74efe</anchor>
      <arglist>(env, reaction, worker)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>tracepoint_reaction_starts</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga8c90883c30c8d773cde2df65f9f95e59</anchor>
      <arglist>(env, reaction, worker)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>tracepoint_scheduler_advancing_time_ends</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga77931b6e1d5a6c7f462902e78db801ba</anchor>
      <arglist>(env)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>tracepoint_scheduler_advancing_time_starts</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga65f95d31a900f5cde73f152c36f116bf</anchor>
      <arglist>(env)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>tracepoint_worker_wait_ends</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga4ea3c33342ac48ed332eb540a14ea53f</anchor>
      <arglist>(env, worker)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>tracepoint_worker_wait_starts</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga5d07ee55d71cbd545ad4bb577c2dc6b9</anchor>
      <arglist>(env, worker)</arglist>
    </member>
    <member kind="typedef">
      <type>struct allocation_record_t</type>
      <name>allocation_record_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga416845ec4469b3186de047c32402f5e8</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct environment_t</type>
      <name>environment_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaa47b54e9e041dfe1b75fffceb1051466</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct event_t</type>
      <name>event_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga59f5f6b9c6023baebf9c49c328b639a1</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>pqueue_pri_t</type>
      <name>index_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga4448d06be794d3f5412d0edb412dc00e</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct lf_port_base_t</type>
      <name>lf_port_base_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga91b17c088cd50ce69df73f1470a18799</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct lf_scheduler_t</type>
      <name>lf_scheduler_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga0199f9b027e13cf08095d91fe798c663</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct lf_sparse_io_record_t</type>
      <name>lf_sparse_io_record_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaa6696d69bef6bb4bdd52ef9ab9d2c614</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct lf_tag_advancement_barrier_t</type>
      <name>lf_tag_advancement_barrier_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga18d624d162daca00e24d1d528ec3c18f</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct mixed_radix_int_t</type>
      <name>mixed_radix_int_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga7d95374fb5368705263c2f1ac2579183</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>int(*</type>
      <name>pqueue_cmp_pri_f</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga1c3f02694b2a0ec19584c395a88bb6f9</anchor>
      <arglist>)(pqueue_pri_t next, pqueue_pri_t curr)</arglist>
    </member>
    <member kind="typedef">
      <type>int(*</type>
      <name>pqueue_eq_elem_f</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga664f0abcd86c8089468869aa3dc6e535</anchor>
      <arglist>)(void *next, void *curr)</arglist>
    </member>
    <member kind="typedef">
      <type>size_t(*</type>
      <name>pqueue_get_pos_f</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga31ca7927983005bd7866021819ad7037</anchor>
      <arglist>)(void *a)</arglist>
    </member>
    <member kind="typedef">
      <type>pqueue_pri_t(*</type>
      <name>pqueue_get_pri_f</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaa84f0100faf971295df5aed226c390a6</anchor>
      <arglist>)(void *a)</arglist>
    </member>
    <member kind="typedef">
      <type>unsigned long long</type>
      <name>pqueue_pri_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gad8239ddc32134716f57e54bb972f6bf0</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>pqueue_print_entry_f</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga884902da135214a6167f1536ad4ed4bc</anchor>
      <arglist>)(void *a)</arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>pqueue_set_pos_f</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gafdc8f52cbc45181ef375df22917bc4f9</anchor>
      <arglist>)(void *a, size_t pos)</arglist>
    </member>
    <member kind="typedef">
      <type>struct pqueue_t</type>
      <name>pqueue_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga05e211b59fd9be5939218e11d1132167</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>pqueue_t</type>
      <name>pqueue_tag_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gac52d056c47d9595f94d37e95484b3acd</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>reaction_function_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga149e5fee1c1841bcc96c72f200601d90</anchor>
      <arglist>)(void *)</arglist>
    </member>
    <member kind="typedef">
      <type>struct self_base_t</type>
      <name>self_base_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga6202eb05c29c30bfd6a8fc203de6422f</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>enum token_freed</type>
      <name>token_freed</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga0cb4f0fedba2f1e1fd3893440ab53647</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct token_template_t</type>
      <name>token_template_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gad0befcbc6fe23c8dd0b6f483d4067e45</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct token_type_t</type>
      <name>token_type_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga58dc2f4a624d2f3030b7e5f3596e58d6</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>INTERNAL struct trace_record_t</type>
      <name>trace_record_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga7804d825257e1eb4296de7da8fec62f6</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>int</type>
      <name>trigger_handle_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga3608c2ed78ba97535f8d82a489846305</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>unsigned short int</type>
      <name>ushort</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga3fa7784c89589b49764048e9909d0e07</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>watchdog_function_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga4caef7fcd0476a936700512d28a23aa8</anchor>
      <arglist>)(void *)</arglist>
    </member>
    <member kind="typedef">
      <type>struct watchdog_t</type>
      <name>watchdog_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaee1cd2bc521f76fa428cc659474d9570</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>lf_spacing_policy_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga0183c0b43037a172a1cd9aa6ed6b3822</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>defer</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga0183c0b43037a172a1cd9aa6ed6b3822a893b1cf0de04eaf44a009fecabd16b90</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>drop</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga0183c0b43037a172a1cd9aa6ed6b3822a1e34755950041e469ca91ff2b7d1c019</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>replace</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga0183c0b43037a172a1cd9aa6ed6b3822a8a4df390c6f816287b90cb2b33ab4323</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>update</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga0183c0b43037a172a1cd9aa6ed6b3822a15edc24cdf7dea17a43c6c50580eba2b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>port_status_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga759ba374f75ea0025b9af1bb35f14d7e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>absent</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga759ba374f75ea0025b9af1bb35f14d7ea1a8fae68a24a59c5629c241401fabb08</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>present</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga759ba374f75ea0025b9af1bb35f14d7eaaeb73d7cb56b19bff3d9f80426ed3267</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>unknown</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga759ba374f75ea0025b9af1bb35f14d7ea5b9f6d065e6e98483b3d3ed01f4f6cbe</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>reaction_status_t</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga6e496c05213aa4bcbc0055ceee7808fa</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>inactive</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga6e496c05213aa4bcbc0055ceee7808faa76c1253bb97844abbdf89af6dfc3c7d6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>queued</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga6e496c05213aa4bcbc0055ceee7808faa8ff224790af0c8a18f259da89dfb2225</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>running</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gga6e496c05213aa4bcbc0055ceee7808faab514bba77fe136c3a3b6f56b818f7b0c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>token_freed</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gabe23a36a87d2f0c076da417eb0114c7e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>NOT_FREED</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ggabe23a36a87d2f0c076da417eb0114c7ea3d7522b54086645e077eb70e78731c5c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>VALUE_FREED</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ggabe23a36a87d2f0c076da417eb0114c7ea417a0268666423fd955fea6f38cde238</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TOKEN_FREED</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ggabe23a36a87d2f0c076da417eb0114c7ea0a6094445d7a54e61aa3a43f6d017e2b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TOKEN_AND_VALUE_FREED</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ggabe23a36a87d2f0c076da417eb0114c7ea02ef194d373714ee3ac62226729e0cb0</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_advance_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga4659db62370c837baa55484134b3bfb7</anchor>
      <arglist>(environment_t *env, tag_t next_tag)</arglist>
    </member>
    <member kind="function">
      <type>event_t *</type>
      <name>_lf_create_dummy_events</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga6396bfcdac50ddb71b6b29fa33a3cc5d</anchor>
      <arglist>(environment_t *env, tag_t tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_decrement_tag_barrier_locked</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga5a0540790dfc6d954fb443da3336ce27</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>token_freed</type>
      <name>_lf_done_using</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga88c890be1f8d45461a6985cbfe6faa99</anchor>
      <arglist>(lf_token_t *token)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_free_all_tokens</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gafd97c46ee623b1ae34a70088ee9b5020</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>token_freed</type>
      <name>_lf_free_token</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga2798a92c59a1d46b602298cdbd187ab1</anchor>
      <arglist>(lf_token_t *token)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_free_token_copies</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gab1efa737bf70317f885c1dc772c4f23b</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>_lf_get_environments</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gac109cd752121228507a95495a1eb6d8f</anchor>
      <arglist>(environment_t **envs)</arglist>
    </member>
    <member kind="function">
      <type>lf_token_t *</type>
      <name>_lf_get_token</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga8f2f9c98968a10bf4d37077fd363ac48</anchor>
      <arglist>(token_template_t *tmplt)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_increment_tag_barrier</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaa8e97abcbd89bb371d396da44ff4becb</anchor>
      <arglist>(environment_t *env, tag_t future_tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_increment_tag_barrier_locked</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga01d3c6cadb7930c096ffe1f794173f5c</anchor>
      <arglist>(environment_t *env, tag_t future_tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_initialize_template</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga5d1a2996844350bc1e29de47e3b56644</anchor>
      <arglist>(token_template_t *tmplt, size_t element_size)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>_lf_initialize_timer</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga61a2c70695093f8a38b1e922fb36547f</anchor>
      <arglist>(environment_t *env, trigger_t *timer)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>_lf_initialize_timers</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga37bdd5c8fe3428b85eff05f0629da411</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>lf_token_t *</type>
      <name>_lf_initialize_token</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga6e2c7940f2e59f5ff57807df6b41f5fe</anchor>
      <arglist>(token_template_t *tmplt, size_t length)</arglist>
    </member>
    <member kind="function">
      <type>lf_token_t *</type>
      <name>_lf_initialize_token_with_value</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gac171b72d59f37653f012d30cad72a2d2</anchor>
      <arglist>(token_template_t *tmplt, void *value, size_t length)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_initialize_trigger_objects</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga52ffa06ff177dc19d33713beb2ff344e</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_initialize_watchdogs</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaa6a016400f119168b48505e51baaaa55</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>_lf_insert_reactions_for_trigger</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaeac3f6a2d15f30e3adecc9f431162bef</anchor>
      <arglist>(environment_t *env, trigger_t *trigger, lf_token_t *token)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_invoke_reaction</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga7fe988f0eee005defaa2ad2c9f1f2fd8</anchor>
      <arglist>(environment_t *env, reaction_t *reaction, int worker)</arglist>
    </member>
    <member kind="function">
      <type>lf_multiport_iterator_t</type>
      <name>_lf_multiport_iterator_impl</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga23e88870b9a699d1a067ff5b397e0887</anchor>
      <arglist>(lf_port_base_t **port, int width)</arglist>
    </member>
    <member kind="function">
      <type>lf_token_t *</type>
      <name>_lf_new_token</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga4616dad8eeb4cbe04a4f9697d3de9b16</anchor>
      <arglist>(token_type_t *type, void *value, size_t length)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_next_locked</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gab9e357a21e338cd3719cdec409b9f7a6</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_pop_events</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga41e1c14ed7c1ab5ab19b8b98d84006b6</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>_lf_register_trace_event</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga292c73e4f09daa50330b53079df620a9</anchor>
      <arglist>(void *pointer1, void *pointer2, _lf_trace_object_t type, char *description)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_replace_template_token</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gabeff98dcfb6b5715aac8e1438c5a6e77</anchor>
      <arglist>(token_template_t *tmplt, lf_token_t *newtoken)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>_lf_sched_advance_tag_locked</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaf2c6b4fac0a87c3cc914c713714e1fca</anchor>
      <arglist>(lf_scheduler_t *sched)</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>_lf_schedule_at_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga9d2634d70492498740984f320dffe8f0</anchor>
      <arglist>(environment_t *env, trigger_t *trigger, tag_t tag, lf_token_t *token)</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>_lf_schedule_copy</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaf31c25686db5996e9f3493745e63856a</anchor>
      <arglist>(environment_t *env, void *action, interval_t offset, void *value, size_t length)</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>_lf_schedule_token</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga6d8b49ac9cf089b35a5d2df6a9209255</anchor>
      <arglist>(environment_t *env, void *action, interval_t extra_delay, lf_token_t *token)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_start_time_step</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gab5d69d8631d56d64fb90547a8d6b10cd</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_trigger_reaction</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaf31f8aca1b004a6e5e0e695063de1b47</anchor>
      <arglist>(environment_t *env, reaction_t *reaction, int worker_number)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_trigger_shutdown_reactions</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga282a342efac4fc3e198fb9656f0a9adc</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_trigger_startup_reactions</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gafd3a0abded3adbc25ab7dbc261e7b16c</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>_lf_wait_on_tag_barrier</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga3d1cd5263a79c14e62a5fb34530a0a93</anchor>
      <arglist>(environment_t *env, tag_t proposed_tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_watchdog_terminate_all</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gab3957d31bade9b6ebcbc27aae6be3f14</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>call_tracepoint</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gad0ae74c1c8d1935b3fb92e546988503c</anchor>
      <arglist>(int event_type, void *reactor, tag_t tag, int worker, int src_id, int dst_id, instant_t *physical_time, trigger_t *trigger, interval_t extra_delay)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>environment_free</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaf73b90f735c070d534171f3e92730ac8</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>environment_init</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gab49da5954eb69f3d126162da44c25b36</anchor>
      <arglist>(environment_t *env, const char *name, int id, int num_workers, int num_timers, int num_startup_reactions, int num_shutdown_reactions, int num_reset_reactions, int num_is_present_fields, int num_modes, int num_state_resets, int num_watchdogs, const char *trace_file_name)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>environment_init_tags</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaf23dde6465214cc92114e0c49bccdc72</anchor>
      <arglist>(environment_t *env, instant_t start_time, interval_t duration)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>environment_verify</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaeb51302599cae953b2b8942088879e2f</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void void void void</type>
      <name>error</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga8db9f1cd3ea7eb70e6958e732b26e61d</anchor>
      <arglist>(const char *msg)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>get_next_event_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga9b50c51a9046dfb8814b2f609020d0a4</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>pqueue_pri_t</type>
      <name>get_reaction_index</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gacd6d67f4e05f2780b23aef72f92468f5</anchor>
      <arglist>(void *reaction)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>get_reaction_position</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga051ae6a8bc2b547818e06cb0c72b14a2</anchor>
      <arglist>(void *reaction)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>in_no_particular_order</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga987e40d356a70d4799d6fe56920d3b8f</anchor>
      <arglist>(pqueue_pri_t thiz, pqueue_pri_t that)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>in_reverse_order</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga19d650d18c331602f44c642bce2456e8</anchor>
      <arglist>(pqueue_pri_t thiz, pqueue_pri_t that)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>initialize_global</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga42d0bf55641d6ff4390081175de65496</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>lf_allocate</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga1a5fdb69bc112879d4014bb0790e843c</anchor>
      <arglist>(size_t count, size_t size, struct allocation_record_t **head)</arglist>
    </member>
    <member kind="function">
      <type>index_t</type>
      <name>lf_combine_deadline_and_level</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga6876ddf559d9ecf14ae78f76e6ff2045</anchor>
      <arglist>(interval_t deadline, int level)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_create_environments</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaf2a6b2663dca116472afc45b50040a3d</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>lf_delay_strict</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaf61e1a6183ff7d40b1b998c08447130e</anchor>
      <arglist>(tag_t tag, interval_t interval)</arglist>
    </member>
    <member kind="function">
      <type>uint16_t</type>
      <name>lf_fed_id</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga6e9c2ed60ca5adec5ba3f43d4410dc75</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_free</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaa028b6b458854278bb2a2de486e40268</anchor>
      <arglist>(struct allocation_record_t **head)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_free_all_reactors</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gadf76c4fc43b07691236fa6a483762481</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_free_reactor</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga2f0b7c8d624c2da93012538bd93568ad</anchor>
      <arglist>(self_base_t *self)</arglist>
    </member>
    <member kind="function">
      <type>event_t *</type>
      <name>lf_get_new_event</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaeb163964110b0029fc4c460b2478ea4d</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>self_base_t *</type>
      <name>lf_new_reactor</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga129c6df527165b2378d1dc4852411c35</anchor>
      <arglist>(size_t size)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_print_snapshot</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaae4a4a9ce970c18bff7785cf7863777c</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_recycle_event</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga0dbdf3a0cb8b0075acfc45437f4c7e27</anchor>
      <arglist>(environment_t *env, event_t *e)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_replace_token</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga3c0888056123fd1ef7c2fdd7a8081ddf</anchor>
      <arglist>(event_t *event, lf_token_t *token)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_sched_done_with_reaction</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga485e4339d95d23ae5bcbb06c244e7145</anchor>
      <arglist>(size_t worker_number, reaction_t *done_reaction)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_sched_free</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga2139bc60dc5be91d750d5e877af07843</anchor>
      <arglist>(lf_scheduler_t *scheduler)</arglist>
    </member>
    <member kind="function">
      <type>reaction_t *</type>
      <name>lf_sched_get_ready_reaction</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga95107b668caa59d5bca9fff1af21e7fb</anchor>
      <arglist>(lf_scheduler_t *scheduler, int worker_number)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_sched_init</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga0ebe8d7670a73a6572c7152d31e1fb62</anchor>
      <arglist>(environment_t *env, size_t number_of_workers, sched_params_t *parameters)</arglist>
    </member>
    <member kind="function">
      <type>trigger_handle_t</type>
      <name>lf_schedule_trigger</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga28927b8a184fe101ad414ed866c49148</anchor>
      <arglist>(environment_t *env, trigger_t *trigger, interval_t delay, lf_token_t *token)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_scheduler_trigger_reaction</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gae47f6c04336244e4739c05f5c38e730e</anchor>
      <arglist>(lf_scheduler_t *scheduler, reaction_t *reaction, int worker_number)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_semaphore_acquire</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga7bafb933f1e301b37b5d5164229f386d</anchor>
      <arglist>(lf_semaphore_t *semaphore)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_semaphore_destroy</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga1207a7db6221cb49ccf260c31e57a5ac</anchor>
      <arglist>(lf_semaphore_t *semaphore)</arglist>
    </member>
    <member kind="function">
      <type>lf_semaphore_t *</type>
      <name>lf_semaphore_new</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga4f13b40eede6275ac98d4ea1e2802e00</anchor>
      <arglist>(size_t count)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_semaphore_release</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gae924daa1634a8e574b5b8966d54158dd</anchor>
      <arglist>(lf_semaphore_t *semaphore, size_t i)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_semaphore_wait</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga2e816883471b300567e207c16471502e</anchor>
      <arglist>(lf_semaphore_t *semaphore)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_set_default_command_line_options</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga2dce2075be67995107b9d8f2d5e20551</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_set_stop_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga07b2e94bcac5d7bcfd47d4eaf35a4977</anchor>
      <arglist>(environment_t *env, tag_t tag)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>lf_tag_latest_earlier</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga1e92870e0258c83da4c541e4ec48169b</anchor>
      <arglist>(tag_t tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_terminate_execution</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gad149603053631bf4d6236426ddae2bde</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>instant_t</type>
      <name>lf_time_logical</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga751c9fce12510f5bb98d862f57077396</anchor>
      <arglist>(void *env)</arglist>
    </member>
    <member kind="function">
      <type>interval_t</type>
      <name>lf_time_logical_elapsed</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga6faad0d905f7135352f511bc235425e1</anchor>
      <arglist>(void *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_tracing_check_version</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga5a16cb75bd134d91bbb002b5d1ddc45c</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_vprint</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga1b64e4e645fbebb1a3b132280b2c5b35</anchor>
      <arglist>(const char *format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void void void</type>
      <name>lf_vprint_debug</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga334870b12bd4bc49b9da219e31225477</anchor>
      <arglist>(const char *format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_vprint_error</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga3d1b4e46f0394bbf2e74c4eabfd8923f</anchor>
      <arglist>(const char *format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void void void</type>
      <name>lf_vprint_error_and_exit</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga698eb7c2ecf514b4afa1ab7ab598eea2</anchor>
      <arglist>(const char *format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void void</type>
      <name>lf_vprint_log</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga8432a03751d354b69ffe2f5b8c664654</anchor>
      <arglist>(const char *format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void void</type>
      <name>lf_vprint_warning</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga47f8c72c1407daae89508da09273d655</anchor>
      <arglist>(const char *format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>logical_tag_complete</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gad96dd94446ff66184dcf0f8f65cdb4f0</anchor>
      <arglist>(tag_t tag_to_send)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>mixed_radix_incr</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaab4b6c3ec9d416bc0965f81ff9194736</anchor>
      <arglist>(mixed_radix_int_t *mixed)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>mixed_radix_parent</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga9405e2b2a5f79663f57c7933a2cec2b8</anchor>
      <arglist>(mixed_radix_int_t *mixed, int n)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>mixed_radix_to_int</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga0402727c71049a3b200c1f9fbfdfcb41</anchor>
      <arglist>(mixed_radix_int_t *mixed)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>pqueue_empty_into</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaa0f5e3d63138880461b1f04dc2d4f48a</anchor>
      <arglist>(pqueue_t **dest, pqueue_t **src)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>pqueue_find_equal_same_priority</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga9bb9cb0e5f41746db17b7581f5fe0559</anchor>
      <arglist>(pqueue_t *q, void *e)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>pqueue_find_same_priority</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga23a8f91001427f237232082b8d25e81a</anchor>
      <arglist>(pqueue_t *q, void *e)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>pqueue_free</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gacceacc4429dd9cd31d5af09f3f473cb0</anchor>
      <arglist>(pqueue_t *q)</arglist>
    </member>
    <member kind="function">
      <type>pqueue_t *</type>
      <name>pqueue_init</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga5a4c8b51b16189ab4a687f562733b1a5</anchor>
      <arglist>(size_t n, pqueue_cmp_pri_f cmppri, pqueue_get_pri_f getpri, pqueue_get_pos_f getpos, pqueue_set_pos_f setpos, pqueue_eq_elem_f eqelem, pqueue_print_entry_f prt)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>pqueue_insert</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaddd8cdfbc8c47b8cdd7eb4c4560de7aa</anchor>
      <arglist>(pqueue_t *q, void *d)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>pqueue_peek</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga9ec5c03203b587dbb92f8d2a977aa7e4</anchor>
      <arglist>(pqueue_t *q)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>pqueue_pop</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga66bce8cd2c2afa804405005798498823</anchor>
      <arglist>(pqueue_t *q)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>pqueue_print</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga1bc71ac57e101d48d91c75ecbf8fc278</anchor>
      <arglist>(pqueue_t *q, pqueue_print_entry_f print)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>pqueue_remove</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga921be3b49e4021888c595188438fdf7a</anchor>
      <arglist>(pqueue_t *q, void *e)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>pqueue_size</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gae0cf88c8360a5f08ada81feaaeb40505</anchor>
      <arglist>(pqueue_t *q)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>pqueue_tag_compare</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gac6870c37fb36dcb65dbfcceff317cab4</anchor>
      <arglist>(pqueue_pri_t priority1, pqueue_pri_t priority2)</arglist>
    </member>
    <member kind="function">
      <type>pqueue_tag_element_t *</type>
      <name>pqueue_tag_find_equal_same_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gafe428033cb2f6915828e75efb90edc44</anchor>
      <arglist>(pqueue_tag_t *q, pqueue_tag_element_t *e)</arglist>
    </member>
    <member kind="function">
      <type>pqueue_tag_element_t *</type>
      <name>pqueue_tag_find_with_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga91d0568eb488ec1255fc1146163934fa</anchor>
      <arglist>(pqueue_tag_t *q, tag_t t)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>pqueue_tag_free</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga7eee6edbbb90d5a0bb072a728dd3c7f2</anchor>
      <arglist>(pqueue_tag_t *q)</arglist>
    </member>
    <member kind="function">
      <type>pqueue_tag_t *</type>
      <name>pqueue_tag_init</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga30b038fea77adb97ee6e0ab13af55ede</anchor>
      <arglist>(size_t initial_size)</arglist>
    </member>
    <member kind="function">
      <type>pqueue_tag_t *</type>
      <name>pqueue_tag_init_customize</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga7194e0ba9a3cd659f5e94f5a46c3d1f1</anchor>
      <arglist>(size_t initial_size, pqueue_cmp_pri_f cmppri, pqueue_eq_elem_f eqelem, pqueue_print_entry_f prt)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>pqueue_tag_insert</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga7db3de28c457287e689dedc3a6dc20da</anchor>
      <arglist>(pqueue_tag_t *q, pqueue_tag_element_t *d)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>pqueue_tag_insert_if_no_match</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga067e8fdd88be6f660e79744350a74128</anchor>
      <arglist>(pqueue_tag_t *q, tag_t t)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>pqueue_tag_insert_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga89084b69d8049630eebb8df759c666d0</anchor>
      <arglist>(pqueue_tag_t *q, tag_t t)</arglist>
    </member>
    <member kind="function">
      <type>pqueue_tag_element_t *</type>
      <name>pqueue_tag_peek</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga16cbdbb45d26bd5373e258de819cfdd3</anchor>
      <arglist>(pqueue_tag_t *q)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>pqueue_tag_peek_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga9bcecb00b894ad00b07f84940fe7af95</anchor>
      <arglist>(pqueue_tag_t *q)</arglist>
    </member>
    <member kind="function">
      <type>pqueue_tag_element_t *</type>
      <name>pqueue_tag_pop</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaf6709c3c3756e65205762a4cf33848be</anchor>
      <arglist>(pqueue_tag_t *q)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>pqueue_tag_pop_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaa2144411e9b6d74af078d51078526fe3</anchor>
      <arglist>(pqueue_tag_t *q)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>pqueue_tag_remove</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gac5612f277391c7129183d2826021c3e3</anchor>
      <arglist>(pqueue_tag_t *q, pqueue_tag_element_t *e)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>pqueue_tag_remove_up_to</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga135a225e2361b815a415607bc1f71e3b</anchor>
      <arglist>(pqueue_tag_t *q, tag_t t)</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>pqueue_tag_size</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga0679a4db1f4d970d9f1048e253b79562</anchor>
      <arglist>(pqueue_tag_t *q)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>print_reaction</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga26e33f5180dc5951b3d26094959913b7</anchor>
      <arglist>(void *reaction)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>process_args</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga80aaf4eeed3e2902f8fe9de80b45777d</anchor>
      <arglist>(int argc, const char *argv[])</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>reaction_matches</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga648da83816bb67aedeeaa8c10a99ec7a</anchor>
      <arglist>(void *a, void *b)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>schedule_output_reactions</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaeaaa76aeb7d93efc4e0f0c484548af70</anchor>
      <arglist>(environment_t *env, reaction_t *reaction, int worker)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>send_next_event_tag</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga6f9a4a14de3aa9e560935a57093eb122</anchor>
      <arglist>(environment_t *env, tag_t tag, bool wait_for_reply)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_reaction_position</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga463f89e588c57a76b4cd6a0e633a94b4</anchor>
      <arglist>(void *reaction, size_t pos)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>should_stop_locked</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga5ee7c21a8b90bb09784f221c1de4d9c9</anchor>
      <arglist>(lf_scheduler_t *sched)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>termination</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaa329f59a16f5617b5195f2c05872c9e9</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tracepoint_schedule</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga896f27619ab0582d5a70d8f613567671</anchor>
      <arglist>(environment_t *env, trigger_t *trigger, interval_t extra_delay)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>wait_until</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga70c4ab92f00f9bcc31e4d696db1c0526</anchor>
      <arglist>(instant_t wait_until_time, lf_cond_t *condition)</arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>_lf_count_token_allocations</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>gaf4657205de7da8f0bf7b346985a983fc</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t</type>
      <name>_lf_my_fed_id</name>
      <anchorfile>group__Internal.html</anchorfile>
      <anchor>ga3c7bddddb86913975950acdcf8bfef2a</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>Modal</name>
    <title>Modal</title>
    <filename>group__Modal.html</filename>
    <file>modes.h</file>
    <class kind="struct">mode_environment_t</class>
    <class kind="struct">mode_state_variable_reset_data_t</class>
    <class kind="struct">reactor_mode_state_t</class>
    <class kind="struct">reactor_mode_t</class>
    <member kind="define">
      <type>#define</type>
      <name>_LF_SET_MODE_WITH_TYPE</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>gaa83c858bb273af532a903ba430b6b87e</anchor>
      <arglist>(mode, change_type)</arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>lf_mode_change_type_t</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>gadd32beb39577775204a6f1ed1f947df9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>no_transition</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>ggadd32beb39577775204a6f1ed1f947df9acbde8a6b8987eeb436dbcc6127b6f65d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>reset_transition</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>ggadd32beb39577775204a6f1ed1f947df9a6af83e1bf871157796f45f54867ec8ff</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>history_transition</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>ggadd32beb39577775204a6f1ed1f947df9a8ed3a9f84d1b9a4b7b8ddb92567887b5</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_add_suspended_event</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>ga3d965a89cfdb1a7539f352227c951165</anchor>
      <arglist>(event_t *event)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_handle_mode_changes</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>ga8818f3e66cd3aaf22372631a8806f87b</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_handle_mode_shutdown_reactions</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>ga8f4063faae1603cd785180388799574d</anchor>
      <arglist>(environment_t *env, reaction_t **shutdown_reactions, int shutdown_reactions_size)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_handle_mode_startup_reset_reactions</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>gad52c1087a0fdb07c57978d7e4cbc8400</anchor>
      <arglist>(environment_t *env, reaction_t **startup_reactions, int startup_reactions_size, reaction_t **reset_reactions, int reset_reactions_size, reactor_mode_state_t *states[], int states_size)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_handle_mode_triggered_reactions</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>ga4d51921004a50a572fd95a884b19f812</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_initialize_mode_states</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>gae0e14964227e6b844179c1bc71a9aabb</anchor>
      <arglist>(environment_t *env, reactor_mode_state_t *states[], int states_size)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_initialize_modes</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>gaa330772299f55fb273cb0350d08d91ed</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>_lf_mode_is_active</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>ga4280d9bbbef5095c7bf4ebdff2b0df90</anchor>
      <arglist>(reactor_mode_t *mode)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_process_mode_changes</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>ga8833697b31d0e552d6e9e75b9f7458b5</anchor>
      <arglist>(environment_t *env, reactor_mode_state_t *states[], int states_size, mode_state_variable_reset_data_t reset_data[], int reset_data_size, trigger_t *timer_triggers[], int timer_triggers_size)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_lf_terminate_modal_reactors</name>
      <anchorfile>group__Modal.html</anchorfile>
      <anchor>gaef08f864ed14b6382a9746b80c1819dd</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>Federated</name>
    <title>Federated</title>
    <filename>group__Federated.html</filename>
    <file>clock-sync.h</file>
    <file>federate.h</file>
    <file>net_common.h</file>
    <file>net_util.h</file>
    <file>socket_common.h</file>
    <class kind="struct">federate_instance_t</class>
    <class kind="struct">federation_metadata_t</class>
    <class kind="struct">lf_stat_ll</class>
    <class kind="struct">rti_addr_info_t</class>
    <class kind="struct">socket_stat_t</class>
    <class kind="struct">staa_t</class>
    <member kind="define">
      <type>#define</type>
      <name>_LF_CLOCK_SYNC_ATTENUATION</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga532da6f271eb75c9ac745571b995c404</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>_LF_CLOCK_SYNC_COLLECT_STATS</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga4b9290ecd850995c857e04746aa45d10</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>_LF_CLOCK_SYNC_EXCHANGES_PER_INTERVAL</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gab0eeaea19d6e5c9217a4eed928c32141</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ADDRESS_QUERY_RETRY_INTERVAL</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga8ce563da4edbe9c4f7c1ccf35ad8694f</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CLOCK_SYNC_GUARD_BAND</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga0810a64801750ce9b148848c228c86e9</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CONNECT_RETRY_INTERVAL</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gab2106828de539188aed925f592751c12</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CONNECT_TIMEOUT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga252b2cb72531cb00ecd4d4db37a5a473</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DEFAULT_PORT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga16b710f592bf8f7900666392adc444dc</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DELAY_BETWEEN_SOCKET_RETRIES</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga7b7cd916c6c027dc9ebdff449fb6edad</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DELAY_START</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga4b8c713b515dba0c86d9205dc0caf4ed</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ENCODE_STOP_GRANTED</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga8086398bfefdc0104767df037e59daa5</anchor>
      <arglist>(buffer, time, microstep)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ENCODE_STOP_REQUEST</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga3268a658c2cb5126be5284a86ad9bd62</anchor>
      <arglist>(buffer, time, microstep)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ENCODE_STOP_REQUEST_REPLY</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae06b33f7fcdc71f52eb0fcf81e07e4d6</anchor>
      <arglist>(buffer, time, microstep)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FED_COM_BUFFER_SIZE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gacc95612e1d2dbbdf34afe76d50e75223</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FEDERATE_ID_IN_USE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae3bd830cd17cf0914b61d0516360abc1</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FEDERATE_ID_OUT_OF_RANGE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga5846fdcf4c92041f543b73e29e78aa21</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FEDERATION_ID_DOES_NOT_MATCH</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga653676d1f302fe08249af3dee78fa294</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>HMAC_DOES_NOT_MATCH</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga51d08a784b4ee6463688a971d99d2944</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_CLOCK_SYNC</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gab36196523f89c0d8f30c1965b458beb6</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_CLOCK_SYNC_INIT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga7ed4e5f2a4216fdf6d76eafcab5b49b7</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_CLOCK_SYNC_OFF</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga742c3183fb89d811377514d09e526b8f</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LF_CLOCK_SYNC_ON</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaa9efe35bfc06d22220c852574c4a5feb</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MAX_NUM_PORT_ADDRESSES</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga5dbc42b5857eb262a06aa04399475d16</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_ACK</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gad94567b2d2e277ddc1be0da9a92b09e7</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_ADDRESS_ADVERTISEMENT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae004cb4e5add42afe5483f6706e11d35</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_ADDRESS_QUERY</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga5ac191bca25da16eca3e4f02d21172ef</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_ADDRESS_QUERY_REPLY</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaabe4cac3ef1d0834a99fa2532dfaa6ba</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_CLOCK_SYNC_CODED_PROBE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaa13eface5080ad75bbd53abe919c80b6</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_CLOCK_SYNC_T1</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaea37eff76ade1b2781a7e6298afb3a04</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_CLOCK_SYNC_T3</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga52a76e4cc36217a169f32d5adde590cc</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_CLOCK_SYNC_T4</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae128056ab2af39988103856ee815d930</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_DOWNSTREAM_NEXT_EVENT_TAG</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gafb060091e032562cf32c0eb62340d309</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_FAILED</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaf27674f627be1c469a529a995da5c074</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_FAILED</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaf27674f627be1c469a529a995da5c074</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_FED_IDS</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga8e49ce0b1c3a58c881849ca4d0bae824</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_FED_NONCE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga191b27bec42ab0370248fbc64cc9b860</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_FED_RESPONSE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gacd7e1e07253e568044a204a1f82d36a1</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_LATEST_TAG_CONFIRMED</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gada47c9f6736992a3df380526d87089f9</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_MESSAGE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gac79b5228f132029285408a30a31a174e</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_NEIGHBOR_STRUCTURE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga48ec489cb1543b161c262f4bee6c9598</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_NEIGHBOR_STRUCTURE_HEADER_SIZE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga77a9c1b741d7ca0f4e8d00a5b74ef91e</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_NEXT_EVENT_TAG</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaf662a6a84cd64cddad92e20e26af877e</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_P2P_MESSAGE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga2b9f13f8df66448bf81ac5fe0774c124</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_P2P_SENDING_FED_ID</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gacd33bbab7bf74e5ac8bad3bd27145f8b</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_P2P_TAGGED_MESSAGE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga5c1256c8c62fbbcb1b16ea67d8f529fb</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_PORT_ABSENT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gadb9610b1edbee4c85e194e391a6eeb74</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_PROVISIONAL_TAG_ADVANCE_GRANT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga9a9bb60d4df1ba581a29319850097cc2</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_REJECT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga59a69d0685fdc2a216718f1efa083c4c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_RESIGN</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga9e19e307a4c3a9dbccea4f2539cd67dd</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_RTI_RESPONSE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga20f24b4b20547d44523120689afd9b98</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_STOP_GRANTED</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaad37dd00423e88f213ca7d7d238bce2e</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_STOP_GRANTED_LENGTH</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga76275384e9865f1f1ed32408c03d876a</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_STOP_REQUEST_LENGTH</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaa3ed75054ae1aaa64dafa6399f7a23cb</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_STOP_REQUEST_REPLY_LENGTH</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaa62d9986e928cb5e872caa6a509cae6c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_TAG_ADVANCE_GRANT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga94fe2c510160682b2c0ffc00b35e0ad5</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_TAGGED_MESSAGE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga2922af172f2e95bc73bd0675a4107b3b</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_TIMESTAMP</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga27db349e7460afc1758bf2eec95d7005</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_TIMESTAMP_LENGTH</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga3450aedd1ca1c368ed28ed2e859588ef</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MSG_TYPE_UDP_PORT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae2c2fdb5fbcc47750409348d37b0cd78</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NONCE_LENGTH</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga6771c37605e49c8faae7898797f254b7</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NUMBER_OF_FEDERATES</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaf58c457e08491f7cfd5a0a46940e11ad</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>PORT_BIND_RETRY_INTERVAL</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaf11c9d6cd02e9e78e38a848cf75205cb</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>PORT_BIND_RETRY_LIMIT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga04c08dc0b0733010f3190bf6df123433</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>RTI_NOT_EXECUTED_WITH_AUTH</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae546b6c6176fe607616181e144364f2b</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SHA256_HMAC_LENGTH</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gabd435507a255ff2571133013bdf93bd2</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>TCP_TIMEOUT_TIME</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gab1edbb864391382835b9ad71408c5c53</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>UDP_TIMEOUT_TIME</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaac9f4a449d302b4f39e69a14b3a4c8d5</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>UNEXPECTED_MESSAGE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae2e1a44a10d4219f4645a4e99fee009c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>WRONG_SERVER</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga5a6c87886a0136b58ae5bb1d627c7ae3</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct federate_instance_t</type>
      <name>federate_instance_t</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga675c841ec6a29e45cacc71b61ef8d270</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct federation_metadata_t</type>
      <name>federation_metadata_t</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae335f4cb4e7d5e88ed712be8cf9592ac</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct lf_stat_ll</type>
      <name>lf_stat_ll</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga9e708b16d53622a88d5a2638affb6934</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>enum parse_rti_code_t</type>
      <name>parse_rti_code_t</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga98d812b2acffbba5c8b1b72913513d19</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct rti_addr_info_t</type>
      <name>rti_addr_info_t</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaf820f39ab52ce0a58d7ba739051b8f24</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct socket_stat_t</type>
      <name>socket_stat_t</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga7e49fed082ec884e26d761e1c4f0d428</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>enum socket_type_t</type>
      <name>socket_type_t</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga45bb50f52b617bc6a30719cbaafd075d</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct staa_t</type>
      <name>staa_t</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga0bce8f0d13040846780f5bb02e43e81d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>parse_rti_code_t</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga9acb70e6b48452bd9d146e35bafc535c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>SUCCESS</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gga9acb70e6b48452bd9d146e35bafc535cac7f69f7c9e5aea9b8f54cf02870e2bf8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>INVALID_PORT</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gga9acb70e6b48452bd9d146e35bafc535cad65c958d0ccb000b69ef0ef4e3a5bfdf</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>INVALID_HOST</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gga9acb70e6b48452bd9d146e35bafc535caea85d37354b294f21e7ab9c5c142a237</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>INVALID_USER</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gga9acb70e6b48452bd9d146e35bafc535ca1dbf923bd60da7209a684ed484935973</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FAILED_TO_PARSE</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gga9acb70e6b48452bd9d146e35bafc535ca3ad4ab464aba04397206e8b89aa1955a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>socket_type_t</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga54c375e3893ff5969d20df65b90c8335</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TCP</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gga54c375e3893ff5969d20df65b90c8335aa040cd7feeb588104634cdadf35abf1c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>UDP</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gga54c375e3893ff5969d20df65b90c8335adb542475cf9d0636e4225e216cee9ae6</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>accept_socket</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga3f3dfc2ccd62e181467f7a22ab5ebe49</anchor>
      <arglist>(int socket, int rti_socket)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>clock_sync_add_offset</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga6b3edec4d337711a2e914c9f5581ce1c</anchor>
      <arglist>(instant_t *t)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>clock_sync_set_constant_bias</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga4f1aaaa9e0b74867ba6b60eb962dfca6</anchor>
      <arglist>(interval_t offset)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>clock_sync_subtract_offset</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaf5fb44eb0db80b9dfa61399bdab8b85c</anchor>
      <arglist>(instant_t *t)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>connect_to_socket</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga5ac5b1b8bf1c832cbdd2f6cdbb769df8</anchor>
      <arglist>(int sock, const char *hostname, int port)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>create_clock_sync_thread</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gac094b53ced87d3cbd617a66591f4282a</anchor>
      <arglist>(lf_thread_t *thread_id)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>create_real_time_tcp_socket_errexit</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga10b0373c1cff0213b17cb7308949f0a2</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>create_server</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga94aaee169c4c822e4c9e6a73f59a6952</anchor>
      <arglist>(uint16_t port, int *final_socket, uint16_t *final_port, socket_type_t sock_type, bool increment_port_on_retry)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>encode_int32</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gafc6b3d0e0e777738422c11fa07b35e0f</anchor>
      <arglist>(int32_t data, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>encode_int64</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae233fa02382ed619a78b1c32e14a8657</anchor>
      <arglist>(int64_t data, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>encode_tag</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga7b5ae6582f28e14a37d50a2d243613c5</anchor>
      <arglist>(unsigned char *buffer, tag_t tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>encode_uint16</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gafad4dadc9bbc06596be44e7ecc4c7281</anchor>
      <arglist>(uint16_t data, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>encode_uint32</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga9b75c9d94d4f3d34d52f46c65cf950d4</anchor>
      <arglist>(uint32_t data, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>extract_header</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga82060dae45e1c3b922005e56829c9814</anchor>
      <arglist>(unsigned char *buffer, uint16_t *port_id, uint16_t *federate_id, size_t *length)</arglist>
    </member>
    <member kind="function">
      <type>int32_t</type>
      <name>extract_int32</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga8f772b5761c6b74b4136db6ee021e6c5</anchor>
      <arglist>(unsigned char *bytes)</arglist>
    </member>
    <member kind="function">
      <type>int64_t</type>
      <name>extract_int64</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga8decc0f4a38aa42fbc6ccfb029e3a061</anchor>
      <arglist>(unsigned char *bytes)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>extract_match_group</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga8ec38908b111a79943446bfbdec188f0</anchor>
      <arglist>(const char *rti_addr, char *dest, regmatch_t group, size_t max_len, size_t min_len, const char *err_msg)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>extract_match_groups</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga50c7f94caa2a61bcba5f89535da07036</anchor>
      <arglist>(const char *rti_addr, char **rti_addr_strs, bool **rti_addr_flags, regmatch_t *group_array, int *gids, size_t *max_lens, size_t *min_lens, const char **err_msgs)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>extract_rti_addr_info</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga0737fc3d45aae606811f57a16ad87208</anchor>
      <arglist>(const char *rti_addr, rti_addr_info_t *rti_addr_info)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>extract_tag</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga9cd95311c2c29ce5bed1c44d5336584d</anchor>
      <arglist>(unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>extract_timed_header</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga8e2cc45fc8571af05bb05f4952d4cde5</anchor>
      <arglist>(unsigned char *buffer, uint16_t *port_id, uint16_t *federate_id, size_t *length, tag_t *tag)</arglist>
    </member>
    <member kind="function">
      <type>uint16_t</type>
      <name>extract_uint16</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga1686d838d49741a6ff2ee65bd766a987</anchor>
      <arglist>(unsigned char *bytes)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>handle_T1_clock_sync_message</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gace14df89540b56b069c6c619e8f37493</anchor>
      <arglist>(unsigned char *buffer, int socket, instant_t t2)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_T4_clock_sync_message</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga1b35d21eda090ea4bf8a79f401dbdad0</anchor>
      <arglist>(unsigned char *buffer, int socket, instant_t r4)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>host_is_big_endian</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gad791461950852eb074b90bc75156b413</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>init_shutdown_mutex</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gadc2dc02aa0e242eab3574240e90984b4</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_connect_to_federate</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga26b7c3ab8c2a50f65e53997a6f26a0dc</anchor>
      <arglist>(uint16_t remote_federate_id)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_connect_to_rti</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga92e8c30255091911a80601bf341cf0a2</anchor>
      <arglist>(const char *hostname, int port_number)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_create_server</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga926a4fb7f9b045acb13fee6c2b7192dd</anchor>
      <arglist>(int specified_port)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_enqueue_port_absent_reactions</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae427b4c0340dbe19d46c93708fb6151a</anchor>
      <arglist>(environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>interval_t</type>
      <name>lf_get_sta</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae03d197bf8d64f82be4a68c95a940195</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>lf_handle_p2p_connections_from_federates</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaf7ba635fb6ffa82e4b05a51d4fc0020f</anchor>
      <arglist>(void *ignored)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_latest_tag_confirmed</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga80af1b6a9d7200df3d85c534edd8cbbc</anchor>
      <arglist>(tag_t tag_to_send)</arglist>
    </member>
    <member kind="function">
      <type>parse_rti_code_t</type>
      <name>lf_parse_rti_addr</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae612f180643d0436d4496738b957af68</anchor>
      <arglist>(const char *rti_addr)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_reset_status_fields_on_input_port_triggers</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae3bc503fcbeaffe48f4500fddba4b21a</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_send_message</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga2f330bfe2fdb03cbf49596bcc012bc58</anchor>
      <arglist>(int message_type, unsigned short port, unsigned short federate, const char *next_destination_str, size_t length, unsigned char *message)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_send_neighbor_structure_to_RTI</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga131226459d7dacc6068c0a6d1d9ebde1</anchor>
      <arglist>(int socket_TCP_RTI)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>lf_send_next_event_tag</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga33d69f68b22b5143c029f463d6efba4f</anchor>
      <arglist>(environment_t *env, tag_t tag, bool wait_for_reply)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_send_port_absent_to_federate</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga527e8cd401ba68b503403706815ed1a0</anchor>
      <arglist>(environment_t *env, interval_t additional_delay, unsigned short port_ID, unsigned short fed_ID)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_send_stop_request_to_rti</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gab4d7e691d4b52f2c0dac90e772d86dd5</anchor>
      <arglist>(tag_t stop_tag)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_send_tagged_message</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga0931fe1bb9eac2a9beebe0c0ed03408e</anchor>
      <arglist>(environment_t *env, interval_t additional_delay, int message_type, unsigned short port, unsigned short federate, const char *next_destination_str, size_t length, unsigned char *message)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_set_federation_id</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga032d945ee3fd6995a5f7bb15b57f2ddf</anchor>
      <arglist>(const char *fid)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_set_sta</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaccbc0981eff8f1f1726075d2ee4ba0ef</anchor>
      <arglist>(interval_t offset)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_spawn_staa_thread</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga3aff644df1b85540aa6a3d2997f819c5</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_stall_advance_level_federation</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga971322f63f26490a27bdd9006c05b8fe</anchor>
      <arglist>(environment_t *env, size_t level)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_stall_advance_level_federation_locked</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gab0f2188d27dfffa9fbbd417bed9305ea</anchor>
      <arglist>(size_t level)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_synchronize_with_other_federates</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga36681c905141edace5d23ff8d5c8f205</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>lf_update_max_level</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga84e5177d12e705274be1e6652b5d7c01</anchor>
      <arglist>(tag_t tag, bool is_provisional)</arglist>
    </member>
    <member kind="function">
      <type>instant_t</type>
      <name>lf_wait_until_time</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaa0c8d0811c7faae11b9f1a7fcb30f917</anchor>
      <arglist>(tag_t tag)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>match_regex</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga7a568c79c856e633f5f181dd21700b74</anchor>
      <arglist>(const char *str, char *regex)</arglist>
    </member>
    <member kind="function">
      <type>ssize_t</type>
      <name>peek_from_socket</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae4ba6b1361cd7c47e8a0eb70729d9636</anchor>
      <arglist>(int socket, unsigned char *result)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>read_from_socket</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaa8f7af0d4004aa925499fecefa1ac6b8</anchor>
      <arglist>(int socket, size_t num_bytes, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>read_from_socket_close_on_error</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga941fd71700b7646e6edbbb76db4f7bd2</anchor>
      <arglist>(int *socket, size_t num_bytes, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>read_from_socket_fail_on_error</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga46a44d92c24d3caadec0bc9e59a26361</anchor>
      <arglist>(int *socket, size_t num_bytes, unsigned char *buffer, char *format,...)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>reset_socket_stat</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae267d03a5f2263604459ca4c1aef2c2c</anchor>
      <arglist>(struct socket_stat_t *socket_stat)</arglist>
    </member>
    <member kind="function">
      <type>uint16_t</type>
      <name>setup_clock_synchronization_with_rti</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga834b31e00e677a23b6a86119b7a2fe59</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>shutdown_socket</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga365eba5b8b3f6445eeaffcb4435165c5</anchor>
      <arglist>(int *socket, bool read_before_closing)</arglist>
    </member>
    <member kind="function">
      <type>int32_t</type>
      <name>swap_bytes_if_big_endian_int32</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaeaff8773e0cba7d0f8a6d03b8f0f7766</anchor>
      <arglist>(int32_t src)</arglist>
    </member>
    <member kind="function">
      <type>int64_t</type>
      <name>swap_bytes_if_big_endian_int64</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaa548ffc52c264f564127b80f63170c33</anchor>
      <arglist>(int64_t src)</arglist>
    </member>
    <member kind="function">
      <type>uint16_t</type>
      <name>swap_bytes_if_big_endian_uint16</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga48fa075b3a868790da8fb303a397cd60</anchor>
      <arglist>(uint16_t src)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>synchronize_initial_physical_clock_with_rti</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga3eba5f95a19f86a70d9d11fd2c736dd1</anchor>
      <arglist>(int *rti_socket_TCP)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tracepoint_federate_from_federate</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga43868b9ea442f34eedbfe7052247f0a3</anchor>
      <arglist>(trace_event_t event_type, int fed_id, int partner_id, tag_t *tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tracepoint_federate_from_rti</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gab1bd5a0184ea0773425733fe1d9faa1c</anchor>
      <arglist>(trace_event_t event_type, int fed_id, tag_t *tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tracepoint_federate_to_federate</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga1307585fa1ca4dc4506f0398842115ee</anchor>
      <arglist>(trace_event_t event_type, int fed_id, int partner_id, tag_t *tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tracepoint_federate_to_rti</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaa074cf1f2690197f9edfd7a115381d6a</anchor>
      <arglist>(trace_event_t event_type, int fed_id, tag_t *tag)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>validate_host</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga0f6ec1479ffe28cc089fe6b13e675f0e</anchor>
      <arglist>(const char *host)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>validate_port</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga841bec9ddc3fb61c2b615f5d512dc3f0</anchor>
      <arglist>(char *port)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>validate_user</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga1285f4b0283c8e0c020e12e76a4426c2</anchor>
      <arglist>(const char *user)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>write_to_socket</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gae8d4b83faeac37f665666429742813f9</anchor>
      <arglist>(int socket, size_t num_bytes, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>write_to_socket_close_on_error</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gaf88884c303b81143ef5ab7af4683a66c</anchor>
      <arglist>(int *socket, size_t num_bytes, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write_to_socket_fail_on_error</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>gada8d9360bdf4e9d7f36bbfc7e682f06e</anchor>
      <arglist>(int *socket, size_t num_bytes, unsigned char *buffer, lf_mutex_t *mutex, char *format,...)</arglist>
    </member>
    <member kind="variable">
      <type>lf_mutex_t</type>
      <name>lf_outbound_socket_mutex</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga1881fdaaffead81a8d2993121d9cd78f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>lf_cond_t</type>
      <name>lf_port_status_changed</name>
      <anchorfile>group__Federated.html</anchorfile>
      <anchor>ga4ea10c9ed824595585d91f37dbfd4364</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>Tracing</name>
    <title>Tracing</title>
    <filename>group__Tracing.html</filename>
    <file>influxdb.h</file>
    <file>trace.h</file>
    <file>trace_types.h</file>
    <file>trace_util.h</file>
    <class kind="struct">influx_client_t</class>
    <class kind="struct">influx_v2_client_t</class>
    <class kind="struct">trace_record_nodeps_t</class>
    <member kind="define">
      <type>#define</type>
      <name>_LF_TRACE_FAILURE</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga7e540fdcdbf452d4198f88859699ce18</anchor>
      <arglist>(trace_file)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>BUFFER_SIZE</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga6b20d41d6252e9871430c242cb1a56e7</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct influx_client_t</type>
      <name>influx_client_t</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga3e866e57b719354cd1cd139976f81a37</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct influx_v2_client_t</type>
      <name>influx_v2_client_t</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga9a5a1c6ca2615dac7b1e875b398832a0</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct object_description_t</type>
      <name>object_description_t</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga027205fde7a6d8a5777059d6dc397050</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>_lf_trace_object_t</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gaeec3d6d67240b942f12f5d8770698ae3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>trace_reactor</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggaeec3d6d67240b942f12f5d8770698ae3ac970d1f28c60cf2b9de8353b284197b0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>trace_trigger</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggaeec3d6d67240b942f12f5d8770698ae3a5c05f73b365f900def1359524b9ce5a0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>trace_user</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggaeec3d6d67240b942f12f5d8770698ae3a8fa30d4c503d93cecf4234a4d648ed79</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>trace_event_t</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gab02e9e69539d60297cedb38c2193a453</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>reaction_starts</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a7ca2fc1a301d8e66944fab471646728a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>reaction_ends</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a79889b5c51ccdf63962a8ec230ff3f6b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>reaction_deadline_missed</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a8dd4df4dfbe3f6fd12f454467d61cda0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>schedule_called</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a666fca0e7c44992277a6f47331b6ff49</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>user_event</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453aeba45bd40c043d7a65ac7c5d31b9e187</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>user_value</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a1d509d2fbc0fe97dcc61aea8ba7b68c1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>worker_wait_starts</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a9c6849445ded286ba9f914d3b1decd2a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>worker_wait_ends</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453afa179671ba1508f1c16b10ffef3a17c6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>scheduler_advancing_time_starts</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a79e198f97731e9ea1f982ccb8db8f5d0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>scheduler_advancing_time_ends</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a0422fad861a51f83258d104e6b34fdad</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>federated</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a9b673ff06b88b52089cafea62715f7c0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_ACK</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a359e8de71d77ef5495ec551054828a44</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_FAILED</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453ad86a1396b6883943ef6f103f97897ba4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_TIMESTAMP</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a58ad18e86ddd619e77a41305948c9343</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_NET</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a9e1be1b36d5d41388c1bc6496be1c5c6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_LTC</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a813a5c39f4659f6f4dc04cdf63a167df</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_STOP_REQ</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a06e51ca4ea2dfc237aad447f5e92f4df</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_STOP_REQ_REP</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a87da8a1b1ef9018c7f6959432d6cbc3d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_STOP_GRN</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a55634e907daa67db49886a1439ac9d16</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_FED_ID</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a68fe57f3b98b1046a20b96926d44d33f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_PTAG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453af8e3e59ab52fdb846ef24dd8bff6fa7a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_TAG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a1d2d24b8faba6e105f3effdda0f30b45</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_REJECT</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a6debe1c504821e1b55ae91ba185ca768</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_RESIGN</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453aaf479fd10fa46f06b11550611d3c610f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_PORT_ABS</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a1f1176e68caab2d18193117dec523864</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_CLOSE_RQ</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453afb7e97a3dfbc62462125e25024424ed0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_TAGGED_MSG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a813b0d7924cfa072765efc09614c2d81</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_P2P_TAGGED_MSG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a17e891ffbe84e2aadbdb405b47a5c09c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_MSG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a22883b9d184199e94df0fa804fe23a89</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_P2P_MSG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453aa7a4fca7a8ef7dfde896701070fcfe6b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_ADR_AD</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a787c95dd61408de4fcc85bdbb3bc6946</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_ADR_QR</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a4ebac2f241a667876f0a7ab20a8efdec</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>send_DNET</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453afd2cdb4240669a883e88c58b28b470cd</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_ACK</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a908926b0adde60c94d40c90fb4aa0221</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_FAILED</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a9fc739f4f25f7832d01711069eaf5004</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_TIMESTAMP</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453ac8429797245197de5cb9f32c9dbe8539</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_NET</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453ab3b8cb930ea4e1ebdb0d49fe287d9b8f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_LTC</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453ab3ae4783b8e9be6cf326df41c3c08108</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_STOP_REQ</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453abf340bb6735bbba6a645ad8e74958410</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_STOP_REQ_REP</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a1967b1b61bcc8cd7e7653fa8bab46e97</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_STOP_GRN</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453aeb7f596158f60ffaded5b6f76a4758a1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_FED_ID</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453acfd5cbf76eacc508d0e2e3ccad128b99</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_PTAG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a3546557eebc5ad402b6783e25be9a468</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_TAG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453aa350fa2156010c04148e947826624831</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_REJECT</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a675c6b9020b4996877a16de7efd91cf2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_RESIGN</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a2e2c7ec922abc6561e48520f2929af22</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_PORT_ABS</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453af9c0ed69192d1a4cc7f1f0fda87c85fa</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_CLOSE_RQ</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453aaeaec5a7cb114e56455f16069bf8f589</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_TAGGED_MSG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a17c1e79d4df8091f7279b5c042c2236a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_P2P_TAGGED_MSG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a720c7ea924889f0db7ed7df00eb9e65d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_MSG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a530cf0a58e28bc3a386e75565810b7f7</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_P2P_MSG</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453ae06a042ab87b41578d9adf1c95322475</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_ADR_AD</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a92c4ad633d4739f7ee1cd40fdaf8cc5e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_ADR_QR</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453af6e18bcb48b442e2be5a898d60c3bdf2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_DNET</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453affe028b8b1b469f6c85d636e780b6c4d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>receive_UNIDENTIFIED</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453a48fffa45ebf3f03caa2dc044af72349f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>NUM_EVENT_TYPES</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ggab02e9e69539d60297cedb38c2193a453ac009e126725584df074102abf50cc134</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>format_line</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga133af84dbff3d84a97044d8a89cf295d</anchor>
      <arglist>(char **buf, int *len, size_t used,...)</arglist>
    </member>
    <member kind="function">
      <type>char *</type>
      <name>get_object_description</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga14da003f17f3fb32043a735158a234c5</anchor>
      <arglist>(void *reactor, int *index)</arglist>
    </member>
    <member kind="function">
      <type>char *</type>
      <name>get_trigger_name</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gaa3ab4dc2202a6bc21f29e14f232301c4</anchor>
      <arglist>(void *trigger, int *index)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_tracing_global_init</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga1098d9744b99a07d115e48d873de000d</anchor>
      <arglist>(char *process_name, char *process_names, int process_id, int max_num_local_threads)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_tracing_global_shutdown</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga8a5d5ec80d2716ea7848193647cdadcd</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_tracing_register_trace_event</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga15969599d7817596e12dcceb8c145551</anchor>
      <arglist>(object_description_t description)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_tracing_set_start_time</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga4270e37a116b1ebac46ad126b2fc277d</anchor>
      <arglist>(int64_t start_time)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_tracing_tracepoint</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gaa6e591a66e342a77860e9d3050680446</anchor>
      <arglist>(int worker, trace_record_nodeps_t *tr)</arglist>
    </member>
    <member kind="function">
      <type>const version_t *</type>
      <name>lf_version_tracing</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga8ab5f9fb3d5e1d867dec2d2949e69d7c</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>FILE *</type>
      <name>open_file</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga139bc891db3edc74b0d9f49d1cec20b9</anchor>
      <arglist>(const char *path, const char *mode)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>post_curl</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gacca2186294f2553607cc2b7658cba23e</anchor>
      <arglist>(influx_v2_client_t *c,...)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>post_http</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gad45a883e3caea0fa7c2a8d542314c1b0</anchor>
      <arglist>(influx_client_t *c,...)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>post_http_send_line</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga932e10327e14e5e8cc5b128fddebe82a</anchor>
      <arglist>(influx_client_t *c, char *buf, int len)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>print_table</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gac8588a40ed5942245242363f13a698f5</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>read_header</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga5cdbb52a1460ec9538f305d12fb4dd2a</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>read_trace</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga496d96019ddbb0d38380191c3042d6b7</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>char *</type>
      <name>root_name</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gaa229eee90d0eb571f63beb972c7c8d82</anchor>
      <arglist>(const char *path)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>send_udp</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga67f24f962260dcabe6fb82bd44dcb5d7</anchor>
      <arglist>(influx_client_t *c,...)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>send_udp_line</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gabed7324f77b104dd90f6662fffed5c03</anchor>
      <arglist>(influx_client_t *c, char *line, int len)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>usage</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>ga2ef30c42cbc289d899a8be5d2d8f77d0</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const char *</type>
      <name>trace_event_names</name>
      <anchorfile>group__Tracing.html</anchorfile>
      <anchor>gad8876683159ec2203bd41295d8af3017</anchor>
      <arglist>[]</arglist>
    </member>
  </compound>
  <compound kind="group">
    <name>RTI</name>
    <title>RTI</title>
    <filename>group__RTI.html</filename>
    <file>README.md</file>
    <file>rti_common.h</file>
    <file>rti_local.h</file>
    <file>rti_remote.h</file>
    <class kind="struct">enclave_info_t</class>
    <class kind="struct">federate_info_t</class>
    <class kind="struct">minimum_delay_t</class>
    <class kind="struct">rti_common_t</class>
    <class kind="struct">rti_local_t</class>
    <class kind="struct">rti_remote_t</class>
    <class kind="struct">scheduling_node_t</class>
    <member kind="define">
      <type>#define</type>
      <name>MAX_TIME_FOR_REPLY_TO_STOP_REQUEST</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga0066544a32ab71d5601142354230452b</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>enum clock_sync_stat</type>
      <name>clock_sync_stat</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga39e986990bfc20d1512b61ab119ce628</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct enclave_info_t</type>
      <name>enclave_info_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga1567a1034e3b7c6528bc12fdc04a4c71</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>enum execution_mode_t</type>
      <name>execution_mode_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga50856f252373f4c456a34c6f26d385ce</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct federate_info_t</type>
      <name>federate_info_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gacd32a9389f9882becea414555263cde1</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct minimum_delay_t</type>
      <name>minimum_delay_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga51ee50491dd9db504fa075ae0b490e14</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct rti_common_t</type>
      <name>rti_common_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga2df16421461ba5b27dc451b16865b750</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct rti_remote_t</type>
      <name>rti_remote_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga224a440a405fa3f473a27ad65edca186</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>enum scheduling_node_state_t</type>
      <name>scheduling_node_state_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga991b71a39df8e306998cbc9d15f9e381</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct scheduling_node_t</type>
      <name>scheduling_node_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gad4b7689a045ef99a1c86753731fb8836</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>clock_sync_stat</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga9705d612b9ce908ee485e92eb3f2769f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>clock_sync_off</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gga9705d612b9ce908ee485e92eb3f2769fafef85b2461484e7a55ae3f50d3ca996c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>clock_sync_init</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gga9705d612b9ce908ee485e92eb3f2769faf21c0b4c30338f2717ebc9f53fa34558</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>clock_sync_on</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gga9705d612b9ce908ee485e92eb3f2769fa8423b23ffffdfc03fcb3f68cf4007531</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>execution_mode_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga83ca4d4187a661b1395c9f860d61c97e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FAST</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gga83ca4d4187a661b1395c9f860d61c97eaf84c11ba888e499a8a282a3e6f5de7de</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>REALTIME</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gga83ca4d4187a661b1395c9f860d61c97eadbd89a052eecc45eaa443bcbecc7c5e4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>scheduling_node_state_t</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga2c9591789f1d6afd603e0330e13f3744</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>NOT_CONNECTED</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gga2c9591789f1d6afd603e0330e13f3744a96c582a5af213ca7fb34f970d83875f5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>GRANTED</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gga2c9591789f1d6afd603e0330e13f3744a440c8b08fdd77c2aa90283c06dbe465a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>PENDING</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gga2c9591789f1d6afd603e0330e13f3744a1869818fd53ff519eb8e429301bdff73</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>_logical_tag_complete</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga5f9802f30e0cb2ceedf199ebb35c946b</anchor>
      <arglist>(scheduling_node_t *e, tag_t completed)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>clock_synchronization_thread</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga272b481a0cc2f86f21c75e8efa19a551</anchor>
      <arglist>(void *noargs)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>downstream_next_event_tag</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga00b9b047401cd08937d21ae84ee2ef79</anchor>
      <arglist>(scheduling_node_t *node, uint16_t node_sending_new_net_id)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>earliest_future_incoming_message_tag</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gad5f8cfd324d9403aa800a88020276969</anchor>
      <arglist>(scheduling_node_t *e)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>eimt_strict</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gadaad58c39361263d511f49b0952bcaee</anchor>
      <arglist>(scheduling_node_t *e)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>federate_info_thread_TCP</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gae0b396a4e41b93505274bbfb55b7a510</anchor>
      <arglist>(void *fed)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>free_local_rti</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gaa202c053941549eb84d77776c67b5137</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>free_scheduling_nodes</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gaa8f3d17093904564d4a1eebd526a2164</anchor>
      <arglist>(scheduling_node_t **scheduling_nodes, uint16_t number_of_scheduling_nodes)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>get_dnet_candidate</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gac81735735888368a08b2b16cf8d440ea</anchor>
      <arglist>(tag_t next_event_tag, tag_t minimum_delay)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_address_ad</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga3f4f3aae4aa73c87569b677f2c0957b7</anchor>
      <arglist>(uint16_t federate_id)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_address_query</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga0424648d3659346e9c7c645cca35d470</anchor>
      <arglist>(uint16_t fed_id)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_latest_tag_confirmed</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga1ffcd0bc844a81aa45cfaa4e1e697ef1</anchor>
      <arglist>(federate_info_t *fed)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_next_event_tag</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gad68a1cbfc6299b091b8f7b0e97f8bb5b</anchor>
      <arglist>(federate_info_t *fed)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_physical_clock_sync_message</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gad69c4cb2041a1a262bce829c49ae9246</anchor>
      <arglist>(federate_info_t *my_fed, socket_type_t socket_type)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_port_absent_message</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga307edbac9eb75981db9dc7c0fcfc73e6</anchor>
      <arglist>(federate_info_t *sending_federate, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_stop_request_message</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga41ca594227fe9bb62f67b21cc2e7b6d6</anchor>
      <arglist>(federate_info_t *fed)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_stop_request_reply</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gac4d4e5a1df9c31133d942a6e50e4c163</anchor>
      <arglist>(federate_info_t *fed)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_timed_message</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga9fca6a5c7e73b40db2f6ef6f50c7a112</anchor>
      <arglist>(federate_info_t *sending_federate, unsigned char *buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>handle_timestamp</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga0a2eadb2f35483bc7ce62a5845110330</anchor>
      <arglist>(federate_info_t *my_fed)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>initialize_enclave_info</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gaf934cc54979bcf640a7868377daab5e2</anchor>
      <arglist>(enclave_info_t *enclave, int idx, environment_t *env)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>initialize_federate</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gab32a595d53f125832570251023d10c46</anchor>
      <arglist>(federate_info_t *fed, uint16_t id)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>initialize_local_rti</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga167855af2f7fb010609dceda0a59b43d</anchor>
      <arglist>(environment_t *envs, int num_envs)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>initialize_RTI</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga0aefafd71cb0b057a604a04f1af61174</anchor>
      <arglist>(rti_remote_t *rti)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>initialize_rti_common</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gadd2a2ad3c8a9f8e6cde5ecbcb83d7e8d</anchor>
      <arglist>(rti_common_t *rti_common)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>initialize_scheduling_node</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga9a27b6186b947cfacb408dc3a0829f6e</anchor>
      <arglist>(scheduling_node_t *e, uint16_t id)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>invalidate_min_delays</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga5f7d50fc74bdc38f889c38109c406468</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_in_cycle</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gab38455d4faf77b4d86dcd77976afe1f1</anchor>
      <arglist>(scheduling_node_t *node)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_in_zero_delay_cycle</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga94101c5bfb54d670a8f47f448e351a34</anchor>
      <arglist>(scheduling_node_t *node)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>lf_connect_to_federates</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga52a9225745a4b229aee86fcc4617b904</anchor>
      <arglist>(int socket_descriptor)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_get_downstream_of</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga06003604b6defab443bdfe34f3ab17ee</anchor>
      <arglist>(int enclave_id, uint16_t **result)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_get_upstream_delay_of</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gaaba4ae9dd32581f9f34a7a81d9c7791c</anchor>
      <arglist>(int enclave_id, interval_t **result)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>lf_get_upstream_of</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga8f432bb04e691f66f4f81cbbecb741ed</anchor>
      <arglist>(int enclave_id, uint16_t **result)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>notify_advance_grant_if_safe</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gaa057279652cc77f238cc73d0fc0e705e</anchor>
      <arglist>(scheduling_node_t *e)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>notify_downstream_advance_grant_if_safe</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga0903cfa1c0fa064824b445c099cd2aa6</anchor>
      <arglist>(scheduling_node_t *e, bool visited[])</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>notify_downstream_next_event_tag</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gae38c64692f527911c9cc748c03d2246d</anchor>
      <arglist>(scheduling_node_t *e, tag_t tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>notify_provisional_tag_advance_grant</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gab226921e491807e98a406487cfdf6335</anchor>
      <arglist>(scheduling_node_t *e, tag_t tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>notify_tag_advance_grant</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga21a88113d348968980a137c9e4e4148e</anchor>
      <arglist>(scheduling_node_t *e, tag_t tag)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>process_args</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga80aaf4eeed3e2902f8fe9de80b45777d</anchor>
      <arglist>(int argc, const char *argv[])</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>process_clock_sync_args</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gad7d4392b21b300612a5239fbb1ffa274</anchor>
      <arglist>(int argc, const char *argv[])</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>respond_to_erroneous_connections</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gac6af5f2343ecf9ed87cdbebd98b94271</anchor>
      <arglist>(void *nothing)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>rti_logical_tag_complete_locked</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gafa27405a96dae1488b670aa25fc8ad1b</anchor>
      <arglist>(enclave_info_t *enclave, tag_t completed)</arglist>
    </member>
    <member kind="function">
      <type>tag_t</type>
      <name>rti_next_event_tag_locked</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga48640596070ecfb23f94532363b5cd6a</anchor>
      <arglist>(enclave_info_t *enclave, tag_t next_event_tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>rti_update_other_net_locked</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gaf921c9092b09500768f94e41b3a7c8f4</anchor>
      <arglist>(enclave_info_t *src, enclave_info_t *target, tag_t net)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>send_physical_clock</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga5b16bd2ead60426e3ace71eca1cfede5</anchor>
      <arglist>(unsigned char message_type, federate_info_t *fed, socket_type_t socket_type)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>send_reject</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gac1d01420f22f3dc5dbdef49ffebdb443</anchor>
      <arglist>(int *socket_id, unsigned char error_code)</arglist>
    </member>
    <member kind="function">
      <type>int32_t</type>
      <name>start_rti_server</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga6cc1fe69c154d09d88de1f1c06eb4b0d</anchor>
      <arglist>(uint16_t port)</arglist>
    </member>
    <member kind="function">
      <type>tag_advance_grant_t</type>
      <name>tag_advance_grant_if_safe</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>gaf849510938f837c74cfe8f843cb7dcb2</anchor>
      <arglist>(scheduling_node_t *e)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tracepoint_rti_from_federate</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga13560f0cae54155219f4d8d9c487b0ef</anchor>
      <arglist>(trace_event_t event_type, int fed_id, tag_t *tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tracepoint_rti_to_federate</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga0da8aecc05366988f8d929118be90bbe</anchor>
      <arglist>(trace_event_t event_type, int fed_id, tag_t *tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update_federate_next_event_tag_locked</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga7190c8d1000afb0a5e8898011d041917</anchor>
      <arglist>(uint16_t federate_id, tag_t next_event_tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update_min_delays</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga93ef5ce52bd47e14977ea3571c1b152b</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update_scheduling_node_next_event_tag_locked</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga21533a3d64de78b3766016c2166460c9</anchor>
      <arglist>(scheduling_node_t *e, tag_t next_event_tag)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>usage</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga7f44f474f50286c4ba8c0ebac254bb28</anchor>
      <arglist>(int argc, const char *argv[])</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>wait_for_federates</name>
      <anchorfile>group__RTI.html</anchorfile>
      <anchor>ga005cb43e8e6c7795c8f0db27e2424475</anchor>
      <arglist>(int socket_descriptor)</arglist>
    </member>
  </compound>
  <compound kind="page">
    <name>intro</name>
    <title>Introduction</title>
    <filename>intro.html</filename>
  </compound>
  <compound kind="page">
    <name>contributing</name>
    <title>Contributing</title>
    <filename>contributing.html</filename>
    <docanchor file="contributing.html">docs</docanchor>
    <docanchor file="contributing.html">groups</docanchor>
    <docanchor file="contributing.html">tests</docanchor>
    <docanchor file="contributing.html">code-style-and-formatting</docanchor>
  </compound>
  <compound kind="page">
    <name>license</name>
    <title>License</title>
    <filename>license.html</filename>
  </compound>
  <compound kind="page">
    <name>index</name>
    <title>reactor-c</title>
    <filename>index.html</filename>
  </compound>
</tagfile>
