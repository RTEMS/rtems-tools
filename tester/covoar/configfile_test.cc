#include "ConfigFile.h"
#include <stdio.h>

Configuration::Options_t Options[] = {
  { "projectName", NULL },
  { "verbose",     NULL },
  { NULL,          NULL }
};

int main(
  int   argc,
  char *argv[]
)
{
  Configuration::FileReader *config;

  config = new Configuration::FileReader(Options);
  
  config->processFile( "configFile.txt" );
  config->printOptions();

}

