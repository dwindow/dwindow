#pragma once

// warning: these functions are not thread safe.

int telnet_start_server();
int telnet_stop_server();
int telnet_restart_server();
int telnet_set_port(int new_port);				// set port also cause a restart
int telnet_set_password();						// set password won't disconnect any authorized connection, to do that, call restart_telnet_server.