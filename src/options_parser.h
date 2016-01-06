#ifndef __OPTIONS_PARSER_H_
#define __OPTIONS_PARSER_H_

#include "server.h"

void cmd_options_parse(int argc, char *argv[], struct config *conf_p);
void json_options_parse(struct config *conf_p);

#endif
